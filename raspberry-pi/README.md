# GPIO Server for Raspberry Pi

A minimal TCP server for low-latency GPIO control from Open Ephys.

This replaces `pigpiod` with a custom implementation that uses direct GPIO hardware access for minimal latency (~1-2ms instead of 10-20ms).

## Features

- Direct `/dev/gpiomem` access for fastest GPIO control
- Compatible with pigpiod protocol (WRITE and TRIG commands)
- Single-threaded design for predictable latency
- No response waiting on TRIG commands for fire-and-forget operation

## Building

On your Raspberry Pi:

```bash
cd raspberry-pi
make
```

## Running

### Stop pigpiod (if running)
```bash
sudo killall pigpiod
```

### Start the GPIO server
```bash
sudo ./gpio_server
```

The server will listen on port 8888 (same as pigpiod).

### Run at startup (optional)

Create a systemd service:

```bash
sudo nano /etc/systemd/system/gpio-server.service
```

Add:

```ini
[Unit]
Description=GPIO Server for Open Ephys
After=network.target

[Service]
ExecStart=/usr/local/bin/gpio_server
Restart=always
User=root

[Install]
WantedBy=multi-user.target
```

Then:

```bash
sudo make install
sudo systemctl enable gpio-server
sudo systemctl start gpio-server
```

## Supported Commands

- **WRITE** (cmd=4): Set GPIO level
  - p1 = GPIO number
  - p2 = level (0=LOW, 1=HIGH)

- **TRIG** (cmd=37): Generate pulse
  - p1 = GPIO number
  - p2 = pulse duration (microseconds)
  - extension = level (0=LOW pulse, 1=HIGH pulse)

- **PIGPV** (cmd=26): Get version (returns 79)

## Performance

Expected latency from Open Ephys event to GPIO toggle: **1-2ms**

This is significantly faster than pigpiod (~10-20ms) because:
1. Direct GPIO memory access (no daemon overhead)
2. Real-time scheduling priority (SCHED_FIFO, priority 99)
3. Memory locking to prevent paging delays
4. Tight event loop (no scheduling delays)
5. Fire-and-forget protocol (no response waiting)

### Reducing Latency Jitter

If you see variable latency (sometimes 1-2ms, sometimes 10-15ms), apply these optimizations:

**1. Disable CPU frequency scaling (keep CPU at max speed):**
```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

**2. Isolate a CPU core for gpio_server (advanced):**

Edit `/boot/cmdline.txt` and add `isolcpus=3` to the end of the line (replace 3 with your desired core):
```
... isolcpus=3
```

Reboot, then run:
```bash
sudo taskset -c 3 ./gpio_server
```

**3. Disable IRQ balance (prevent interrupt migration):**
```bash
sudo systemctl stop irqbalance
sudo systemctl disable irqbalance
```

**4. Check for competing processes:**
```bash
top
# Look for processes using significant CPU
```

## Troubleshooting

**Permission denied on /dev/gpiomem:**
```bash
sudo ./gpio_server
```

**Port already in use:**
Make sure pigpiod is stopped:
```bash
sudo killall pigpiod
```

**Wrong Pi version:**
The code tries BCM2835 base address. For Pi 4, you may need to adjust `BCM2711_PERI_BASE` usage (current code works with /dev/gpiomem which is version-agnostic).
