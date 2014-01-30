TARGET   = 8086run
CXX      = g++
CXXFLAGS = -Wall -g
LDFLAGS  =
OBJECTS  = $(SOURCES:%.cpp=%.o)
SOURCES = main.cpp utils.cpp \
		  OpCode.cpp Operand.cpp VM.cpp VM.inst.cpp disasm.cpp

all: $(TARGET)

.SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJECTS) *core

depend:
	rm -f dependencies
	for cpp in $(SOURCES); do \
	  (echo -n `dirname $$cpp`/; \
	   g++ -MM $(CXXFLAGS) $$cpp) >> dependencies; \
	done

-include dependencies
