/*
 * Minimal GPIO TCP Server for Raspberry Pi
 *
 * Implements a subset of pigpiod protocol for minimal latency:
 * - WRITE command (4): Set GPIO level
 * - TRIG command (37): Generate pulse
 *
 * Uses direct /dev/gpiomem access for fastest possible GPIO control.
 *
 * Compile: gcc -O3 -o gpio_server gpio_server.c -lpthread
 * Run: sudo ./gpio_server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>

// GPIO Memory Map
#define BCM2835_PERI_BASE   0x3F000000  // RPi 2/3
#define BCM2711_PERI_BASE   0xFE000000  // RPi 4
#define GPIO_BASE_OFFSET    0x200000
#define BLOCK_SIZE          (4*1024)

// GPIO Register offsets
#define GPFSEL0     0   // Function select
#define GPSET0      7   // Pin output set
#define GPCLR0      10  // Pin output clear
#define GPLEV0      13  // Pin level

// Command codes
#define PI_CMD_WRITE    4
#define PI_CMD_TRIG     37
#define PI_CMD_PIGPV    26

#define PI_OUTPUT       1

// Global GPIO memory pointer
volatile uint32_t *gpio_map = NULL;

// Microsecond delay using nanosleep
void delay_us(uint32_t us)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = us * 1000;
    nanosleep(&ts, NULL);
}

// Initialize GPIO memory mapping
int init_gpio()
{
    int mem_fd;
    void *gpio_base;
    uint32_t peri_base;

    // Try to detect Pi version by trying different base addresses
    mem_fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Failed to open /dev/gpiomem");
        return -1;
    }

    // Map GPIO memory
    gpio_base = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);
    close(mem_fd);

    if (gpio_base == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    gpio_map = (volatile uint32_t *)gpio_base;
    printf("GPIO initialized\n");
    return 0;
}

// Set GPIO function (INPUT=0, OUTPUT=1)
void gpio_set_mode(int gpio, int mode)
{
    int reg = gpio / 10;
    int shift = (gpio % 10) * 3;

    uint32_t value = gpio_map[GPFSEL0 + reg];
    value &= ~(7 << shift);  // Clear 3 bits
    if (mode == PI_OUTPUT) {
        value |= (1 << shift);  // Set to output
    }
    gpio_map[GPFSEL0 + reg] = value;
}

// Set GPIO high
void gpio_set(int gpio)
{
    gpio_map[GPSET0 + gpio/32] = 1 << (gpio % 32);
}

// Set GPIO low
void gpio_clear(int gpio)
{
    gpio_map[GPCLR0 + gpio/32] = 1 << (gpio % 32);
}

// Write GPIO level
void gpio_write(int gpio, int level)
{
    if (level) {
        gpio_set(gpio);
    } else {
        gpio_clear(gpio);
    }
}

// Trigger pulse (set high, delay, set low)
void gpio_trig(int gpio, uint32_t pulse_us, int level)
{
    if (level) {
        // High pulse
        gpio_set(gpio);
        delay_us(pulse_us);
        gpio_clear(gpio);
    } else {
        // Low pulse
        gpio_clear(gpio);
        delay_us(pulse_us);
        gpio_set(gpio);
    }
}

// Handle client connection
void handle_client(int client_fd)
{
    uint8_t cmd_buf[16];
    uint32_t cmd, p1, p2, p3;
    int n;

    // Set TCP_NODELAY for minimal latency
    int flag = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    printf("Client connected\n");

    while (1) {
        // Read 16-byte command header
        n = recv(client_fd, cmd_buf, 16, MSG_WAITALL);
        if (n != 16) {
            if (n == 0) {
                printf("Client disconnected\n");
            } else {
                perror("recv failed");
            }
            break;
        }

        // Parse command
        memcpy(&cmd, cmd_buf + 0, 4);
        memcpy(&p1, cmd_buf + 4, 4);
        memcpy(&p2, cmd_buf + 8, 4);
        memcpy(&p3, cmd_buf + 12, 4);

        // Response buffer
        uint8_t res_buf[16] = {0};
        int32_t status = 0;

        switch (cmd) {
            case PI_CMD_WRITE: {
                // WRITE: p1=gpio, p2=level
                gpio_set_mode(p1, PI_OUTPUT);
                gpio_write(p1, p2);
                break;
            }

            case PI_CMD_TRIG: {
                // TRIG: p1=gpio, p2=pulse_us, p3=ext_size
                // Read extension data (level)
                uint32_t level = 1;
                if (p3 == 4) {
                    recv(client_fd, &level, 4, MSG_WAITALL);
                }

                // Ensure GPIO is in output mode
                gpio_set_mode(p1, PI_OUTPUT);

                // Trigger pulse
                gpio_trig(p1, p2, level);
                break;
            }

            case PI_CMD_PIGPV: {
                // Version command
                status = 79;  // Pretend to be pigpio v79
                break;
            }

            default:
                printf("Unknown command: %u\n", cmd);
                status = -1;
                break;
        }

        // Send response
        memcpy(res_buf, &status, 4);
        send(client_fd, res_buf, 16, 0);
    }

    close(client_fd);
}

int main(int argc, char *argv[])
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int port = 8888;

    // Set real-time scheduling priority for minimal latency
    struct sched_param param;
    param.sched_priority = 99;  // Highest priority
    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        fprintf(stderr, "Warning: Failed to set real-time priority (run with sudo): %s\n", strerror(errno));
    } else {
        printf("Real-time priority enabled (SCHED_FIFO, priority 99)\n");
    }

    // Lock memory to prevent paging delays
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        fprintf(stderr, "Warning: Failed to lock memory: %s\n", strerror(errno));
    } else {
        printf("Memory locked to prevent paging\n");
    }

    // Initialize GPIO
    if (init_gpio() < 0) {
        fprintf(stderr, "Failed to initialize GPIO\n");
        return 1;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    // Set SO_REUSEADDR to allow quick restarts
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Bind to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return 1;
    }

    // Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    printf("GPIO server listening on port %d\n", port);

    // Accept connections
    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // Handle client (single-threaded for now)
        handle_client(client_fd);
    }

    close(server_fd);
    return 0;
}
