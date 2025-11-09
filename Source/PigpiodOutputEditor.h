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

#ifndef __PIGPIODOUTPUTEDITOR_H_28EB4CC9__
#define __PIGPIODOUTPUTEDITOR_H_28EB4CC9__

#include "PigpiodOutput.h"
#include <EditorHeaders.h>

/**

  User interface for the PigpiodOutput processor.

  @see PigpiodOutput

*/

class PigpiodOutputEditor : public GenericEditor, public Timer, public Button::Listener
{
public:
    /** Constructor*/
    PigpiodOutputEditor (GenericProcessor* parentNode);

    /** Destructor*/
    ~PigpiodOutputEditor() {}

    /** Called when the connect button is clicked */
    void buttonClicked (Button* button) override;

private:

    /** Timer callback to update connection status */
    void timerCallback() override;

    /** Update the connection status label */
    void updateConnectionStatus();

    std::unique_ptr<UtilityButton> connectButton;
    std::unique_ptr<UtilityButton> testButton;
    std::unique_ptr<Label> statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PigpiodOutputEditor);
};

#endif // __PIGPIODOUTPUTEDITOR_H_28EB4CC9__
