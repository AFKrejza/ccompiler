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

# for fun
# C++_ASMFLAGS := -save-temps=obj -masm=intel -fverbose-asm -fno-asynchronous-unwind-tables

# for cleaner gcc output, -01 
CFLAGS := -S -O0 -masm=intel -fno-asynchronous-unwind-tables -fcf-protection=none -fno-pie
# extras for more complicated programs
# -fno-unwind-tables -fno-stack-protector -fno-pic -fno-ident -g0

.PHONY: all clean re
all: rat

rat: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) -save-temps=obj -masm=intel -fverbose-asm

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(MYFLAGS) $(ASMFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)

$(OBJECTS): Makefile

gcc:
	gcc $(CFLAGS) src.c -o src.s

clean:
	rm -f rat $(OBJECTS) $(OBJECTS:.o=.d)

re:
	$(MAKE) clean
	$(MAKE) all

r:
	$(MAKE) all
	$(MAKE) run
