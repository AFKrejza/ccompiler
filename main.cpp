#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

#include "main.hpp"

/*
	g++ main.cpp parser.cpp codegen.cpp -o main -lfmt && ./main source.c
	
	-DDEBUG to print all tokens

	-Wall -Wextra -Werror -Wshadow -Wuninitialized

	Also use
	-fsanitize=address,undefined
	-O2 for testing and non-debugging builds

	nasm -felf64 output.asm && ld output.o && ./a.out
	echo $?

	TODO: create a testing setup for each part of the compiler, not just codegen. In python.
*/

static std::string read_source_code(char *file);

int main(int argc, char *argv[])
{
	if (argc < 2) throw_error(1, "Missing argument");
	if (argc > 2) throw_error(1, "Too many arguments (only takes one file)");
	
	std::string source = read_source_code(argv[1]);

	lexer(source);

	parser();

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


void throw_error_line(int code, int line, std::string msg)
{
	std::cerr << "Error on line " << line << ": " << msg << std::endl;
	exit(code);
}


void throw_error(int code, std::string msg)
{
	fmt::print("Error {}, {}\n", code, msg);
}


void throw_warn(int code, int line, std::string msg)
{
	std::cerr << "Warning on line " << line << ": " << msg << std::endl;
}


std::string read_source_code(char *filename)
{
	std::ifstream file(filename);
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
	printmap[MULT] = "mult";
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
