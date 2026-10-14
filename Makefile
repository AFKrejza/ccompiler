CXX := g++
# SANITIZE := -fsanitize=address,undefined 
CXXFLAGS := -Wall -Werror -Wextra -g -Wpedantic -Wreturn-type -Wuninitialized \
			$(SANITIZE)
#			-Wpessimizing-move
# 			-Wimplicit-fallthrough #-Wswitch-enum #-Wshadow

# 	makefile flags:
# 	-DLEX to print token list
# 	-DAST to print validated AST
# 	-DTAC to print TAC IR
# 	-DASM to print assembly
MYFLAGS := -DTAC -DAST -DASM #-DLEX #-O2
LDFLAGS := $(SANITIZE)
LDLIBS := -lfmt

BUILD := build
SOURCES := main.cpp lexer.cpp parser.cpp sema.cpp taco.cpp codegen.cpp
OBJECTS:= $(SOURCES:%.cpp=$(BUILD)/%.o)
DEPS := $(OBJECTS:.o=.d)

# for fun
# C++_ASMFLAGS := -save-temps=obj -masm=intel -fverbose-asm -fno-asynchronous-unwind-tables

# for cleaner gcc output, -01 
CFLAGS := -S -O0 -masm=intel -fno-asynchronous-unwind-tables -fcf-protection=none -fno-pie
# extras for more complicated programs
# -fno-unwind-tables -fno-stack-protector -fno-pic -fno-ident -g0

.PHONY: all clean re r gcc
all: rat

rat: $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $(MYFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

-include $(DEPS)

$(OBJECTS): Makefile

gcc:
	gcc $(CFLAGS) src.c -o src.s

clean:
	rm -rf $(BUILD) rat

re:
	$(MAKE) clean
	$(MAKE) all

