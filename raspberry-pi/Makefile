CC = gcc
CFLAGS = -O3 -Wall -Wextra
LDFLAGS = -lpthread

TARGET = gpio_server
SRC = gpio_server.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

.PHONY: all clean install
