# Pigpio Digital Output Sink

![ephys-socket-screenshot](https://open-ephys.github.io/gui-docs/_images/ephyssocket-01.png)

A simple sink which generates digital pulses from events on a Raspberry Pi using the Pigpio `pigpiod` daemon.

## Installation

This plugin can be added via the Open Ephys GUI Plugin Installer. To access the Plugin Installer, press **ctrl-P** or **⌘P** from inside the GUI. Once the installer is loaded, browse to the "PIGPIOD SINK" plugin and click "Install."

## Usage

### Raspberry Pi Setup

1. Install pigpiod on your Raspberry Pi:
   ```bash
   sudo apt-get install pigpio
   ```

2. Start the pigpiod daemon (allow remote connections):
   ```bash
   sudo pigpiod
   ```

3. To enable pigpiod on boot:
   ```bash
   sudo systemctl enable pigpiod
   sudo systemctl start pigpiod
   ```

### Performance Optimization (Optional)

For minimal latency (~1-2ms), you can optionally use the included custom GPIO server instead of pigpiod, and apply real-time optimizations on the Raspberry Pi.

#### Using the Custom GPIO Server

A minimal GPIO server is included in the `raspberry-pi/` directory that provides lower latency than pigpiod:

```bash
cd raspberry-pi
make
sudo ./gpio_server
```

See `raspberry-pi/README.md` for complete instructions.

#### Core Isolation (Advanced)

To achieve the most consistent low latency, you can isolate a CPU core for the GPIO server:

1. Edit `/boot/cmdline.txt` and add `isolcpus=3` to the end of the line:
   ```
   ... isolcpus=3
   ```

2. Reboot the Raspberry Pi

3. Run the GPIO server (or pigpiod) on the isolated core:
   ```bash
   sudo taskset -c 3 ./gpio_server
   # or for pigpiod:
   sudo taskset -c 3 pigpiod
   ```

#### Additional Optimizations

**Disable CPU frequency scaling:**
```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

**Disable IRQ balance:**
```bash
sudo systemctl stop irqbalance
sudo systemctl disable irqbalance
```

These optimizations prevent the Linux scheduler from preempting the GPIO server and ensure consistent sub-2ms latency.

### Plugin Configuration

1. Add the "Pigpiod Sink" plugin to your signal chain
2. Enter the Raspberry Pi's IP address (or "localhost" if running locally)
3. Set the port (default: 8888)
4. Click "CONNECT" to establish connection with pigpiod
5. Configure the GPIO pin (BCM numbering, pins 2-27 available)
6. Set the pulse duration in microseconds (10 - 100 µs, default: 50 µs)
7. Select the TTL input line that will trigger the GPIO pulse
8. Optionally select a gate line to enable/disable triggering

When a rising edge is detected on the input line (and the gate is open), the plugin will send a pulse to the configured GPIO pin on the Raspberry Pi.

## Building from source

First, follow the instructions on [this page](https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-the-GUI.html) to build the Open Ephys GUI.

**Important:** This plugin is intended for use with the latest version of the GUI (0.6.0 and higher). The GUI should be compiled from the [`main`](https://github.com/open-ephys/plugin-gui/tree/main) branch, rather than the former `master` branch.

Then, clone this repository into a directory at the same level as the `plugin-GUI`, e.g.:
 
```
Code
├── plugin-GUI
│   ├── Build
│   ├── Source
│   └── ...
├── pigpiod-network-sink
│   ├── Build
│   ├── Source
│   └── ...
```

### Windows

**Requirements:** [Visual Studio](https://visualstudio.microsoft.com/) and [CMake](https://cmake.org/install/)

From the `Build` directory, enter:

```bash
cmake -G "Visual Studio 17 2022" -A x64 ..
```

Next, launch Visual Studio and open the `OE_PLUGIN_ephys-socket.sln` file that was just created. Select the appropriate configuration (Debug/Release) and build the solution.

Selecting the `INSTALL` project and manually building it will copy the `.dll` and any other required files into the GUI's `plugins` directory. The next time you launch the GUI from Visual Studio, the Pigpiod Sink plugin should be available.


### Linux

**Requirements:** [CMake](https://cmake.org/install/)

From the `Build` directory, enter:

```bash
cmake -G "Unix Makefiles" ..
cd Debug
make -j
make install
```

This will build the plugin and copy the `.so` file into the GUI's `plugins` directory. The next time you launch the GUI compiled version of the GUI, the Pigpiod Sink plugin should be available.


### macOS

**Requirements:** [Xcode](https://developer.apple.com/xcode/) and [CMake](https://cmake.org/install/)

From the `Build` directory, enter:

```bash
cmake -G "Xcode" ..
```

Next, launch Xcode and open the `network-pulse-sink.xcodeproj` file that now lives in the “Build” directory.

Running the `ALL_BUILD` scheme will compile the plugin; running the `INSTALL` scheme will install the `.bundle` file to `/Users/<username>/Library/Application Support/open-ephys/plugins-api`. The Pigpiod Sink plugin should be available the next time you launch the GUI from Xcode.



## Attribution

Based on the Arduino Sink and Ephys-Socket plugins. 
