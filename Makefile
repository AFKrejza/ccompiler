CXX := g++
CXXFLAGS := -DTAC -DAST -DASM
LDFLAGS := -lfmt

SOURCES := main.cpp lexer.cpp parser.cpp sema.cpp taco.cpp codegen.cpp
OBJECTS:= $(SOURCES:.cpp=.o)

.PHONY: all clean
all: main

main: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)

clean:
	rm -f main $(OBJECTS) $(OBJECTS:.o=.d)
