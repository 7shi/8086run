TARGET   = 8086run
CC       = gcc
CXX      = g++
CFLAGS   = -O1 -Wall -g
CXXFLAGS = $(CFLAGS)
LDFLAGS  =

all: $(TARGET)

$(TARGET): main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(TARGET).exe *.o *core
