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
    desiredWidth = 250;

    // Hostname/IP input (text)
    addTextBoxParameterEditor ("hostname", 10, 29);

    // Port number
    addTextBoxParameterEditor ("port", 10, 54);

    // Connect button
    connectButton = std::make_unique<UtilityButton> ("CONNECT", Font ("Small Text", 13, Font::plain));
    connectButton->setBounds (10, 79, 80, 20);
    connectButton->addListener (this);
    addAndMakeVisible (connectButton.get());

    // Connection status label
    statusLabel = std::make_unique<Label> ("Status", "Disconnected");
    statusLabel->setBounds (95, 79, 145, 20);
    statusLabel->setFont (Font ("Small Text", 12, Font::plain));
    statusLabel->setColour (Label::textColourId, Colours::white);
    addAndMakeVisible (statusLabel.get());

    // GPIO pin
    addComboBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "gpio_pin", 10, 109);

    // Pulse duration
    addTextBoxParameterEditor ("pulse_duration", 10, 134);

    // Input line
    addComboBoxParameterEditor (Parameter::STREAM_SCOPE, "input_line", 10, 159);

    // Gate line
    addComboBoxParameterEditor (Parameter::STREAM_SCOPE, "gate_line", 10, 184);

    // Start timer to update connection status
    startTimer (500); // Update every 500ms

    // Update initial status
    updateConnectionStatus();
}

void PigpiodOutputEditor::buttonClicked (Button* button)
{
    if (button == connectButton.get())
    {
        PigpiodOutput* processor = (PigpiodOutput*) getProcessor();

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
        connectButton->setLabel ("DISCONNECT");
        statusLabel->setText (processor->getConnectionStatus(), dontSendNotification);
        statusLabel->setColour (Label::textColourId, Colours::lightgreen);
    }
    else
    {
        connectButton->setLabel ("CONNECT");
        statusLabel->setText (processor->getConnectionStatus(), dontSendNotification);
        statusLabel->setColour (Label::textColourId, Colours::grey);
    }
}
