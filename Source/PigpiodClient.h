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

#pragma once

#include <juce_core/juce_core.h>

// pigpiod socket interface command codes
#define PI_CMD_PIGPV 26  // Get pigpio version
#define PI_CMD_WRITE 4   // Write GPIO level

// GPIO levels
#define PI_LOW 0
#define PI_HIGH 1

// Error codes
#define PI_NOT_CONNECTED -1
#define PI_SOCKET_ERROR -2
#define PI_BAD_GPIO -3

/**
 * Client for communicating with pigpiod daemon over TCP socket.
 *
 * Implements the pigpiod binary socket protocol for remote GPIO control.
 */
class PigpiodClient
{
public:
    PigpiodClient();
    ~PigpiodClient();

    /** Connect to pigpiod daemon
     *
     * @param hostname IP address or hostname
     * @param port Port number (default 8888)
     * @return true if connection successful
     */
    bool connect (const String& hostname, int port = 8888);

    /** Disconnect from pigpiod daemon */
    void disconnect();

    /** Check if connected to pigpiod */
    bool isConnected() const { return socket != nullptr && socket->isConnected(); }

    /** Get pigpiod version
     *
     * @return version number, or negative error code
     */
    int getVersion();

    /** Write GPIO pin level
     *
     * @param gpio GPIO number (BCM numbering)
     * @param level 0 for LOW, 1 for HIGH
     * @return 0 on success, negative error code on failure
     */
    int write (int gpio, int level);

    /** Get last error message */
    String getLastError() const { return lastError; }

private:
    /** Send command to pigpiod and receive response
     *
     * @param cmd Command code
     * @param p1 Parameter 1
     * @param p2 Parameter 2
     * @param p3 Parameter 3
     * @return Response value, or negative error code
     */
    int sendCommand (uint32_t cmd, uint32_t p1 = 0, uint32_t p2 = 0, uint32_t p3 = 0);

    std::unique_ptr<StreamingSocket> socket;
    String lastError;
    String hostname;
    int port;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PigpiodClient);
};
