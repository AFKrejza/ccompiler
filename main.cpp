#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

#include "main.hpp"

/*
	g++ main.cpp parser.cpp lexer.cpp sema.cpp taco.cpp codegen.cpp -o main -lfmt && ./main source.c
	
	-DLEX to print token list
	-DAST to print validated AST
	-DTAC to print TAC IR
	-DASM to print assembly

	-Wall -Wextra -Werror -Wshadow -Wuninitialized

	Also use
	-fsanitize=address,undefined
	-O2 for testing and non-debugging builds

	nasm -felf64 output.asm && ld output.o && ./a.out
	echo $?

	To check gcc assembly: gcc INPUT.c -S -masm=intel
	Try with various optimizations e.g. -O2

	TODO: create a testing setup for each part of the compiler, not just codegen. In python.
*/


static std::string preprocess(std::string fileName);

int main(int argc, char *argv[])
{
	if (argc < 2) throw_error(1, "Missing argument");
	if (argc > 2) throw_error(1, "Too many arguments (only takes one file)");
	
	std::string fileName = argv[1];
	assert(fileName.at(fileName.length() - 1) == 'c' &&
		   fileName.at(fileName.length() - 2) == '.');

	fileName = fileName.substr(0, fileName.length() - 2);


	std::string source = preprocess(fileName);

	lexer(source);

	#ifdef LEX
	printTokens();
	#endif

	ProgramNode *ast = parser();

	ProgramNode *vAst = sema(ast); // validated ast

	#ifdef AST
	vAst->printChildren(1);
	#endif

	std::vector<Instruction*> ir = taco(vAst);

	#ifdef TAC
	for (Instruction *i : ir) {
		i->print(1);
	}
	#endif

	std::string outputFilename = codegen(fileName, ir);

	#ifdef ASM
	std::string result = readFile(outputFilename);
	std::cout << "\n" << result << "\n";
	#endif

	fmt::print("success\n");

	return 0;
}


// TODO: find a way to print where the error was thrown in this file (a C++ exception or something?)
void throw_invalid_identifier(int line)
{
	std::string s = "Invalid identifier symbol on line ";
	s.append(std::to_string(line));
	//  .append(" column ")
	//  .append(std::to_string(column));
	throw_error_line(2, line, s);
}


void throw_invalid_identifier_start(int line)
{
	std::string s = "Invalid identifier start symbol on line ";
	s.append(std::to_string(line));
	//  .append(" column ")
	//  .append(std::to_string(column));
	throw_error_line(2, line, s);
}

// TODO: add yellow and red colors for warnings and errors
void throw_error_line(int code, int line, std::string msg)
{
	std::cerr << "Error on line " << line << ": " << msg << std::endl;
	exit(code);
}


void throw_error(int code, std::string msg)
{
	fmt::print("Error {}, {}\n", code, msg);
	exit(code);
}


void throw_warn(int code, int line, std::string msg)
{
	std::cerr << "Warning on line " << line << ": " << msg << std::endl;
}


std::string readFile(std::string filename)
{
	const char *c_filename = filename.c_str();
	std::ifstream file(c_filename);
	if (!file.is_open()) throw_error_line(1, 0, "Couldn't open source file");

	std::string source_code{
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	};
	return source_code;
}


// contains all symbols and keywords
std::unordered_map<TokenType, std::string> populatePrintmap()
{
	std::unordered_map<TokenType, std::string> printmap;
	
	printmap[SEMICOLON] = "semicolon";
	printmap[PLUS] = "plus";
	printmap[MINUS] = "minus";
	printmap[ASTERISK] = "asterisk";
	printmap[ASSIGNMENT] = "assignment";
	printmap[OPEN_PARENTHESES] = "open_parentheses";
	printmap[CLOSED_PARENTHESES] = "closed_parentheses";
	printmap[OPEN_SQUARE_BRACKET] = "open_square_bracket";
	printmap[CLOSED_SQUARE_BRACKET] = "closed_square_bracket";
	printmap[OPEN_CURLY_BRACE] = "open_curly_brace";
	printmap[CLOSED_CURLY_BRACE] = "closed_curly_brace";
	printmap[COMMA] = "comma";
	printmap[GREATER_THAN] = "greater_than";
	printmap[LESS_THAN] = "less_than";
	printmap[EQUAL_TO] = "equal_to";
	printmap[GREATER_OR_EQUAL] = "greater_or_equal";
	printmap[LESSER_OR_EQUAL] = "lesser_or_equal";
	printmap[NOT_EQUAL] = "not_equal";
	printmap[LOGICAL_NOT] = "logical_not";
	printmap[LOGICAL_AND] = "logical_and";
	printmap[LOGICAL_OR] = "logical_or";
	printmap[BITWISE_NOT] = "bitwise_not";
	printmap[BITWISE_AND] = "bitwise_and";
	printmap[BITWISE_OR] = "bitwise_or";
	printmap[IDENTIFIER] = "identifier";
	printmap[CHAR] = "char";
	printmap[SHORT] = "short";
	printmap[INT] = "int";
	printmap[LONG] = "long";
	printmap[STRING_LITERAL] = "string_literal";
	printmap[INTEGER] = "integer";
	printmap[IF] = "if";
	printmap[ELSE] = "else";
	printmap[RETURN] = "return";
	printmap[END_OF_FILE] = "end_of_file";
	
	return printmap;
};

static std::string preprocess(std::string fileName)
{
	std::string preprocessed = fileName;
	preprocessed.append(".i");
	std::string script = fmt::format("gcc -E -P {}.c -o {}", fileName, preprocessed);
	fmt::print("{}\n", script);
	system(script.c_str());
	
	std::string source = readFile(preprocessed);
	
	script = fmt::format("rm {}", preprocessed);
	system(script.c_str());

	return source;
}