ROOT_DIR :=  $(realpath ../../../..)

INC_DIR :=   $(ROOT_DIR)/ext/vortex/runtime/include
COMMON_DIR := $(ROOT_DIR)/ext/vortex/runtime/common
HW_DIR := $(ROOT_DIR)/ext/vortex/build/hw
SIM_DIR := $(ROOT_DIR)/ext/vortex/sim

TARGET       := main_test

CXX          := arm-linux-gnueabihf-g++
CXXFLAGS     := -std=c++17 -O2 -Wall
CXXFLAGS 	 += -I$(INC_DIR) -I$(COMMON_DIR) -I$(HW_DIR) -I$(SIM_DIR)/common
LDFLAGS      := -L$(ROOT_DIR)/ext/vortex/build/runtime
LDFLAGS      += -lvortex -lvortex-opaesimx -lopaesimx-c-sim
LDFLAGS 	 += -static


# Sources
SRCS         := main.cpp
OBJS         := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)