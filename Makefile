TARGET   = 8086run
CC       = gcc
CXX      = g++
CFLAGS   = -O1 -Wall -g
CXXFLAGS = $(CFLAGS)
LDFLAGS  =
SOURCES  = main.cpp
OBJECTS  = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(TARGET).exe *.o *core
