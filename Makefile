TARGET   = 8086run
CC       = gcc
CXX      = g++
CFLAGS   = -O1 -Wall -g
CXXFLAGS = $(CFLAGS)
LDFLAGS  =

all: $(TARGET) bios

bios: bios.asm
	nasm $<

$(TARGET): main.o 8086tiny.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

8086tiny.o: 8086tiny.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(TARGET).exe *.o *core bios
