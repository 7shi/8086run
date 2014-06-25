TARGET   = 8086run
CC       = gcc
CXX      = g++
CFLAGS   = -O1 -Wall -Wno-unused-result -g
CXXFLAGS = $(CFLAGS)
LDFLAGS  =
SOURCES  = main.cpp
OBJECTS  = $(SOURCES:.cpp=.o)

all: $(TARGET) $(COMS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) main.cpp

clean:
	rm -f $(TARGET) $(TARGET).exe *.o *core
