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

#include "PigpiodClient.h"

PigpiodClient::PigpiodClient()
    : port (8888)
{
}

PigpiodClient::~PigpiodClient()
{
    disconnect();
}

bool PigpiodClient::isConnected() const
{
    return socket != nullptr && socket->isConnected();
}

bool PigpiodClient::connect (const juce::String& hostname, int port)
{
    disconnect(); // Close any existing connection

    this->hostname = hostname;
    this->port = port;

    socket = std::make_unique<juce::StreamingSocket>();

    if (socket->connect (hostname, port, 3000)) // 3 second timeout
    {
        lastError = "";

        // Verify connection by getting version
        int version = getVersion();
        if (version > 0)
        {
            return true;
        }
        else
        {
            lastError = "Failed to get pigpiod version. Is pigpiod running?";
            disconnect();
            return false;
        }
    }
    else
    {
        lastError = "Failed to connect to " + hostname + ":" + juce::String (port);
        socket = nullptr;
        return false;
    }
}

void PigpiodClient::disconnect()
{
    if (socket != nullptr)
    {
        socket->close();
        socket = nullptr;
    }
    lastError = "";
}

int PigpiodClient::getVersion()
{
    return sendCommand (PI_CMD_PIGPV);
}

int PigpiodClient::setMode (int gpio, int mode)
{
    if (gpio < 0 || gpio > 53)
    {
        lastError = "Invalid GPIO number: " + juce::String (gpio);
        return PI_BAD_GPIO;
    }

    DBG ("PigpiodClient::setMode - Setting GPIO " + juce::String(gpio) + " to mode " + juce::String(mode));
    return sendCommand (PI_CMD_MODES, gpio, mode);
}

int PigpiodClient::write (int gpio, int level)
{
    if (gpio < 0 || gpio > 53)
    {
        lastError = "Invalid GPIO number: " + juce::String (gpio);
        return PI_BAD_GPIO;
    }

    return sendCommand (PI_CMD_WRITE, gpio, level);
}

int PigpiodClient::trig (int gpio, int pulseLength)
{
    if (gpio < 0 || gpio > 53)
    {
        lastError = "Invalid GPIO number: " + juce::String (gpio);
        return PI_BAD_GPIO;
    }

    if (pulseLength < 1 || pulseLength > 100)
    {
        lastError = "Invalid pulse length: " + juce::String (pulseLength) + " (must be 1-100 microseconds)";
        return PI_BAD_GPIO;
    }

    DBG ("PigpiodClient::trig - Sending TRIG command: gpio=" + juce::String(gpio) + " pulseLength=" + juce::String(pulseLength) + "us");
    int result = sendCommand (PI_CMD_TRIG, gpio, pulseLength);
    DBG ("PigpiodClient::trig - Result: " + juce::String(result));

    return result;
}

int PigpiodClient::sendCommand (uint32_t cmd, uint32_t p1, uint32_t p2, uint32_t p3)
{
    if (!isConnected())
    {
        lastError = "Not connected to pigpiod";
        return PI_NOT_CONNECTED;
    }

    // Prepare command (16 bytes: 4x uint32_t in little-endian)
    uint8_t cmdBuf[16];
    memcpy (cmdBuf + 0, &cmd, 4);
    memcpy (cmdBuf + 4, &p1, 4);
    memcpy (cmdBuf + 8, &p2, 4);
    memcpy (cmdBuf + 12, &p3, 4);

    // Send command
    int sent = socket->write (cmdBuf, 16);
    if (sent != 16)
    {
        lastError = "Failed to send command";
        return PI_SOCKET_ERROR;
    }

    // Receive response (16 bytes: status + data)
    uint8_t resBuf[16];
    int totalReceived = 0;
    int attempts = 0;
    const int maxAttempts = 100; // Prevent infinite loop

    // Keep reading until we get all 16 bytes
    while (totalReceived < 16 && attempts < maxAttempts)
    {
        int received = socket->read (resBuf + totalReceived, 16 - totalReceived, true); // blocking read

        if (received > 0)
        {
            totalReceived += received;
        }
        else if (received < 0)
        {
            lastError = "Socket read error";
            return PI_SOCKET_ERROR;
        }

        attempts++;
    }

    if (totalReceived != 16)
    {
        lastError = "Incomplete response from pigpiod";
        return PI_SOCKET_ERROR;
    }

    // Extract status (first 4 bytes)
    int32_t status;
    memcpy (&status, resBuf, 4);

    return status;
}
