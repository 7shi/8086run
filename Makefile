TARGET   = 8086run
CC       = gcc
CXX      = g++
CFLAGS   = -O1 -Wall -g
CXXFLAGS = $(CFLAGS)
LDFLAGS  =
SOURCES  = main.cpp

all: $(TARGET) hcopy.com

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

hcopy.com: hcopy.s
	nasm -o $@ hcopy.s

clean:
	rm -f $(TARGET) $(TARGET).exe *core hcopy.com
