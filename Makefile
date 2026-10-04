CXX := g++
CXXFLAGS := -Wall -Werror -Wextra -g -Wpedantic -Wreturn-type -Wuninitialized
 #-fsanitize=address,undefined #-Wimplicit-fallthrough #-Wswitch-enum #-Wshadow

# 	makefile flags:
# 	-DLEX to print token list
# 	-DAST to print validated AST
# 	-DTAC to print TAC IR
# 	-DASM to print assembly
MYFLAGS := -DTAC -DAST -DASM #-DLEX #-O2
LDFLAGS := -lfmt

SOURCES := main.cpp lexer.cpp parser.cpp sema.cpp taco.cpp codegen.cpp
OBJECTS:= $(SOURCES:.cpp=.o)

.PHONY: all clean re
all: rat

rat: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(MYFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)

$(OBJECTS): Makefile

clean:
	rm -f main $(OBJECTS) $(OBJECTS:.o=.d)

re:
	$(MAKE) clean
	$(MAKE) all

r:
	$(MAKE) all
	$(MAKE) run
