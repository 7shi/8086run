TARGET   = 8086run
CXX      = g++
CXXFLAGS = -Wall -g
LDFLAGS  =
SOURCES  = main.cpp
OBJECTS  = $(SOURCES:%.cpp=%.o)

all: $(TARGET)

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJECTS) *core
