CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = 

TARGET = wifi_controller
SRC = wifi_controller.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean install help

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	@echo "Note: Run with: sudo ./$(TARGET)"

clean:
	rm -f $(OBJ) $(TARGET)
	rm -f wifi_fix.log
	@echo "Cleaned build artifacts"

help:
	@echo "Available targets:"
	@echo "  make all       - Build the wifi_controller"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make install   - Verify build (no actual install)"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make           - Build the tool"
	@echo "  sudo ./$(TARGET)  - Run the recovery tool"
