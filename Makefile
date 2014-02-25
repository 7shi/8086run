TARGET   = 8086run
CC       = gcc
CXX      = g++
CFLAGS   = -O1 -Wall -g
CXXFLAGS = $(CFLAGS)
LDFLAGS  =
SOURCES  = main.cpp

all: $(TARGET) $(COMS)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).exe *core
