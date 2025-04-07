# Toolchain
CXX := arm-linux-gnueabihf-g++
CFLAGS := -Wall -Wextra -O2 -std=c++11
VORTEX_PATH := $(abspath ../../../../ext/vortex)

INCLUDES := -I$(VORTEX_PATH)/runtime/include \
			-I$(VORTEX_PATH)/kernel/include \
			-I$(VORTEX_PATH)/build/hw \
			-I$(VORTEX_PATH)/runtime

BUILD_ROOT := $(VORTEX_PATH)/build
LINK_PATHS := -L$(BUILD_ROOT)/runtime \
              -L$(BUILD_ROOT)/kernel \

LIBS := -L$(LINK_PATHS) -lvortex

# Source files
SRCS := main.cpp
OBJS := $(SRCS:.cpp=.o)
TARGET := app.elf

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking with libraries from: $(LINK_PATH)"
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

%.o: %.cpp common.h
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)