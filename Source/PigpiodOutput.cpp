/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2024 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "PigpiodOutput.h"
#include "PigpiodOutputEditor.h"

#include <stdio.h>

PigpiodOutput::PigpiodOutput()
    : GenericProcessor ("Pigpiod Sink")
    , connected (false)
    , gateIsOpen (true)
    , hostname ("localhost")
    , pigpiodPort (8888)
    , connectionStatus ("Disconnected")
{
}

PigpiodOutput::~PigpiodOutput()
{
    disconnectFromPigpiod();
}

void PigpiodOutput::registerParameters()
{
    addStringParameter (Parameter::PROCESSOR_SCOPE, "hostname", "Hostname/IP",
                       "The hostname or IP address of the Raspberry Pi running pigpiod",
                       "localhost", true);

    addIntParameter (Parameter::PROCESSOR_SCOPE, "port", "Port",
                    "The port number for pigpiod", 8888, 1, 65535);

    addIntParameter (Parameter::PROCESSOR_SCOPE, "gpio_pin", "GPIO Pin",
                    "The Raspberry Pi GPIO pin to use (BCM numbering)", 17, 2, 27);

    addIntParameter (Parameter::PROCESSOR_SCOPE, "pulse_duration", "Pulse duration (us)",
                    "Duration of the output pulse in microseconds", 50, 10, 100);

    addIntParameter (Parameter::STREAM_SCOPE, "input_line", "Input line",
                    "The TTL line for triggering output", 1, 1, 16);

    addIntParameter (Parameter::STREAM_SCOPE, "gate_line", "Gate line",
                    "The TTL line for gating the output", 0, 0, 16);
}

AudioProcessorEditor* PigpiodOutput::createEditor()
{
    editor = std::make_unique<PigpiodOutputEditor> (this);
    return editor.get();
}

bool PigpiodOutput::connectToPigpiod()
{
    hostname = getParameter ("hostname")->getValue().toString();
    pigpiodPort = (int) getParameter ("port")->getValue();

    LOGC ("Connecting to pigpiod at ", hostname, ":", pigpiodPort);

    if (pigpiod.connect (hostname, pigpiodPort))
    {
        int version = pigpiod.getVersion();
        connected = true;
        connectionStatus = "Connected (version " + String (version) + ")";
        LOGC ("Connected to pigpiod version ", version);

        // Initialize GPIO pin (required for TRIG command to work)
        // First set mode to OUTPUT, then set level to LOW
        int gpio = (int) getParameter ("gpio_pin")->getValue();

        // Set GPIO to OUTPUT mode
        int modeResult = pigpiod.setMode (gpio, PI_OUTPUT);
        if (modeResult < 0)
        {
            LOGC ("Warning: Failed to set GPIO ", gpio, " to OUTPUT mode: ", modeResult);
        }
        else
        {
            LOGC ("Set GPIO ", gpio, " to OUTPUT mode");
        }

        // Set GPIO to LOW level
        int writeResult = pigpiod.write (gpio, PI_LOW);
        if (writeResult < 0)
        {
            LOGC ("Warning: Failed to set GPIO ", gpio, " to LOW: ", writeResult);
        }
        else
        {
            LOGC ("Set GPIO ", gpio, " to LOW");
        }

        CoreServices::sendStatusMessage ("Connected to pigpiod at " + hostname + ":" + String (pigpiodPort));
        CoreServices::updateSignalChain (this);
        return true;
    }
    else
    {
        connected = false;
        connectionStatus = "Error: " + pigpiod.getLastError();
        LOGC ("Failed to connect: ", pigpiod.getLastError());
        CoreServices::sendStatusMessage ("Failed to connect to pigpiod: " + pigpiod.getLastError());
        CoreServices::updateSignalChain (this);
        return false;
    }
}

void PigpiodOutput::disconnectFromPigpiod()
{
    if (connected)
    {
        // Set GPIO low before disconnecting
        int gpio = (int) getParameter ("gpio_pin")->getValue();
        pigpiod.write (gpio, PI_LOW);

        pigpiod.disconnect();
        connected = false;
        connectionStatus = "Disconnected";
        LOGC ("Disconnected from pigpiod");
        CoreServices::sendStatusMessage ("Disconnected from pigpiod");
        CoreServices::updateSignalChain (this);
    }
}

bool PigpiodOutput::isConnectedToPigpiod() const
{
    return connected && pigpiod.isConnected();
}

String PigpiodOutput::getConnectionStatus() const
{
    return connectionStatus;
}

void PigpiodOutput::updateSettings()
{
    isEnabled = connected;
}

bool PigpiodOutput::stopAcquisition()
{
    // Set GPIO low
    if (connected)
    {
        int gpio = (int) getParameter ("gpio_pin")->getValue();
        pigpiod.write (gpio, PI_LOW);
    }

    return true;
}

void PigpiodOutput::parameterValueChanged (Parameter* param)
{
    if (param->getName().equalsIgnoreCase ("gate_line"))
    {
        if (int (param->getValue()) == 0)
            gateIsOpen = true;
        else
            gateIsOpen = false;
    }
    else if (param->getName().equalsIgnoreCase ("gpio_pin"))
    {
        // Initialize new GPIO pin if connected (required for TRIG to work)
        if (connected)
        {
            int gpio = (int) param->getValue();

            // Set GPIO to OUTPUT mode
            int modeResult = pigpiod.setMode (gpio, PI_OUTPUT);
            if (modeResult < 0)
            {
                LOGC ("Warning: Failed to set GPIO ", gpio, " to OUTPUT mode: ", modeResult);
            }

            // Set GPIO to LOW level
            int writeResult = pigpiod.write (gpio, PI_LOW);
            if (writeResult < 0)
            {
                LOGC ("Warning: Failed to set GPIO ", gpio, " to LOW: ", writeResult);
            }
            else
            {
                LOGC ("Changed GPIO pin to ", gpio, " and initialized to LOW");
            }
        }
    }
}

void PigpiodOutput::process (AudioBuffer<float>& buffer)
{
    checkForEvents();
}

void PigpiodOutput::handleTTLEvent (TTLEventPtr event)
{
    if (!connected)
        return;

    const int eventBit = event->getLine() + 1;
    DataStream* stream = getDataStream (event->getStreamId());

    // Handle gate line
    if (eventBit == int ((*stream)["gate_line"]))
    {
        if (event->getState())
            gateIsOpen = true;
        else
            gateIsOpen = false;
    }

    // Handle input line (trigger)
    if (gateIsOpen)
    {
        if (eventBit == int ((*stream)["input_line"]))
        {
            if (event->getState()) // Rising edge
            {
                int gpio = (int) getParameter ("gpio_pin")->getValue();
                int pulseDurationUs = (int) getParameter ("pulse_duration")->getValue();

                // Trigger pulse using pigpiod TRIG command
                int result = pigpiod.trig (gpio, pulseDurationUs);

                if (result < 0)
                {
                    LOGC ("Failed to trigger GPIO pulse: ", result);
                }
            }
        }
    }
}
