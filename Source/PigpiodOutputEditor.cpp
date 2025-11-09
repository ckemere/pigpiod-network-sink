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

#include "PigpiodOutputEditor.h"
#include <stdio.h>

PigpiodOutputEditor::PigpiodOutputEditor (GenericProcessor* parentNode)
    : GenericEditor (parentNode)
{
    desiredWidth = 340;

    // Column 1: Connection settings
    // Hostname/IP input (text)
    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "hostname", 10, 29);

    // Port number
    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "port", 10, 54);

    // Connect button
    connectButton = std::make_unique<UtilityButton> ("CONNECT");
    connectButton->setBounds (10, 79, 80, 20);
    connectButton->addListener (this);
    addAndMakeVisible (connectButton.get());

    // Test button (hidden until connected)
    testButton = std::make_unique<UtilityButton> ("TEST");
    testButton->setBounds (10, 104, 80, 20);
    testButton->addListener (this);
    addChildComponent (testButton.get()); // addChildComponent keeps it hidden initially

    // Connection status label
    statusLabel = std::make_unique<Label> ("Status", "");
    statusLabel->setBounds (95, 79, 70, 20);
    statusLabel->setColour (Label::textColourId, Colours::grey);
    addAndMakeVisible (statusLabel.get());

    // Column 2: Pin configuration
    // GPIO pin
    addComboBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "gpio_pin", 175, 29);

    // Pulse duration
    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "pulse_duration", 175, 54);

    // Input line
    addComboBoxParameterEditor (Parameter::STREAM_SCOPE, "input_line", 175, 79);

    // Gate line
    addComboBoxParameterEditor (Parameter::STREAM_SCOPE, "gate_line", 175, 104);

    // Start timer to update connection status
    startTimer (500); // Update every 500ms

    // Update initial status
    updateConnectionStatus();
}

void PigpiodOutputEditor::buttonClicked (Button* button)
{
    PigpiodOutput* processor = (PigpiodOutput*) getProcessor();

    if (button == connectButton.get())
    {
        if (processor->isConnectedToPigpiod())
        {
            // Disconnect
            processor->disconnectFromPigpiod();
        }
        else
        {
            // Connect
            processor->connectToPigpiod();
        }

        updateConnectionStatus();
    }
    else if (button == testButton.get())
    {
        // Trigger a test pulse
        int gpio = (int) processor->getParameter ("gpio_pin")->getValue();
        int pulseDurationUs = (int) processor->getParameter ("pulse_duration")->getValue();

        LOGC ("Test button clicked: GPIO=", gpio, " duration=", pulseDurationUs, "us");

        PigpiodClient& pigpiod = processor->getPigpiodClient();

        if (!pigpiod.isConnected())
        {
            CoreServices::sendStatusMessage ("Test pulse failed: Not connected to pigpiod");
            LOGC ("Test pulse failed: Not connected");
            return;
        }

        int result = pigpiod.trig (gpio, pulseDurationUs);

        LOGC ("Test pulse result: ", result);

        if (result < 0)
        {
            CoreServices::sendStatusMessage ("Test pulse failed (code " + String(result) + "): " + pigpiod.getLastError());
        }
        else
        {
            CoreServices::sendStatusMessage ("Test pulse sent on GPIO " + String(gpio) + " (" + String(pulseDurationUs) + "us)");
        }
    }
}

void PigpiodOutputEditor::timerCallback()
{
    updateConnectionStatus();
}

void PigpiodOutputEditor::updateConnectionStatus()
{
    PigpiodOutput* processor = (PigpiodOutput*) getProcessor();

    if (processor->isConnectedToPigpiod())
    {
        connectButton->setLabel ("CONNECTED");
        testButton->setVisible (true);
        statusLabel->setText ("", dontSendNotification);
    }
    else
    {
        connectButton->setLabel ("CONNECT");
        testButton->setVisible (false);
        statusLabel->setText (processor->getConnectionStatus(), dontSendNotification);
        statusLabel->setColour (Label::textColourId, Colours::grey);
    }
}
