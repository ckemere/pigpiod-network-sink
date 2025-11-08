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

#ifndef __PIGPIODOUTPUT_H_F7BDA585__
#define __PIGPIODOUTPUT_H_F7BDA585__

#include <ProcessorHeaders.h>
#include "PigpiodClient.h"

/**

    Provides a network interface to a Raspberry Pi running pigpiod.

    Sends GPIO trigger pulses via the pigpiod daemon.

    @see GenericProcessor
 */
class PigpiodOutput : public GenericProcessor
{
public:
    /** Constructor */
    PigpiodOutput();

    /** Destructor */
    ~PigpiodOutput();

    /** Registers the parameters for a given processor */
    void registerParameters() override;

    /** Called whenever a parameter's value is changed */
    void parameterValueChanged (Parameter* param) override;

    /** Searches for events and triggers the pigpiod output when appropriate. */
    void process (AudioBuffer<float>& buffer) override;

    /** Convenient interface for responding to incoming events. */
    void handleTTLEvent (TTLEventPtr event) override;

    /** Called when settings need to be updated. */
    void updateSettings() override;

    /** Called immediately after the end of data acquisition. */
    bool stopAcquisition() override;

    /** Creates the PigpiodOutputEditor. */
    AudioProcessorEditor* createEditor() override;

    /** Connect to pigpiod daemon */
    bool connectToPigpiod();

    /** Disconnect from pigpiod daemon */
    void disconnectFromPigpiod();

    /** Get connection status */
    bool isConnectedToPigpiod() const;

    /** Get connection status message */
    String getConnectionStatus() const;

    /** Get reference to pigpiod client (for test button) */
    PigpiodClient& getPigpiodClient() { return pigpiod; }

private:
    /** pigpiod client */
    PigpiodClient pigpiod;

    /** Connection state */
    bool connected;
    String connectionStatus;

    /** Gate state */
    bool gateIsOpen;

    /** Hostname/IP for pigpiod */
    String hostname;

    /** Port for pigpiod */
    int pigpiodPort;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PigpiodOutput);
};

#endif // __PIGPIODOUTPUT_H_F7BDA585__
