# Toolchain
CXX := arm-linux-gnueabihf-g++
CFLAGS := -Wall -Wextra -O2 -std=c++17
VORTEX_PATH := $(abspath ../../../../ext/vortex)

INCLUDES := -I$(VORTEX_PATH)/runtime/include \
			-I$(VORTEX_PATH)/kernel/include \
			-I$(VORTEX_PATH)/build/hw \
			-I$(VORTEX_PATH)/runtime

BUILD_ROOT := $(VORTEX_PATH)/build
LINK_PATHS := -L$(BUILD_ROOT)/runtime \
              #-L$(BUILD_ROOT)/kernel \

LIBS := -lvortex

# Source files
SRCS := main.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := app.elf

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LINK_PATHS) $(LIBS)

%.o: %.cpp common.h
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)