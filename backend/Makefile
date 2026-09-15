# Makefile for the SensorDataManager-C backend.
#
# Uses pkg-config to discover compiler and linker flags for libpq,
# libmicrohttpd, and json-c, falling back to plain -l flags if pkg-config
# does not have entries for a given library on this system.

CC ?= gcc
PKG_CONFIG ?= pkg-config

CFLAGS ?= -std=c11 -Wall -Wextra -O2 -Iinclude

PG_CFLAGS   := $(shell $(PKG_CONFIG) --cflags libpq 2>/dev/null)
PG_LIBS     := $(shell $(PKG_CONFIG) --libs libpq 2>/dev/null || echo -lpq)

MHD_CFLAGS  := $(shell $(PKG_CONFIG) --cflags libmicrohttpd 2>/dev/null)
MHD_LIBS    := $(shell $(PKG_CONFIG) --libs libmicrohttpd 2>/dev/null || echo -lmicrohttpd)

JSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags json-c 2>/dev/null)
JSON_LIBS   := $(shell $(PKG_CONFIG) --libs json-c 2>/dev/null || echo -ljson-c)

CFLAGS  += $(PG_CFLAGS) $(MHD_CFLAGS) $(JSON_CFLAGS)
LDFLAGS += $(PG_LIBS) $(MHD_LIBS) $(JSON_LIBS) -lpthread

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
TARGET    := $(BIN_DIR)/sensor_data_manager

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
