# Toolchain
CXX := arm-linux-gnueabihf-g++
CFLAGS := -Wall -Wextra -O2 -std=c++11
VORTEX_PATH := $(abspath ../../../../ext/vortex/)

INCLUDES := -I$(VORTEX_PATH)/runtime/include
INCLUDES += -I$(VORTEX_PATH)/kernel/include
INCLUDES += -I$(VORTEX_PATH)/build/hw
INCLUDES += -I$(VORTEX_PATH)/runtime

PROJECT_ROOT := $(abspath ../../../../ext/vortex/build)
LINK_PATHS := -L$(PROJECT_ROOT)/runtime \
              -L$(PROJECT_ROOT)/kernel \

LIBS := -L$(LINK_PATHS) -lvortex -lsimx -lvortex-simx -lopae-c-sim -lvortex-opae

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