/*
	compile with make

	makefile flags:
	-DLEX to print token list
	-DAST to print validated AST
	-DTAC to print TAC IR
	-DASM to print assembly

	-Wall -Wextra -Werror -Wshadow -Wuninitialized

	Also use
	-fsanitize=address,undefined
	-O2 for testing and non-debugging builds

	echo $?

	To check gcc assembly: gcc INPUT.c -S -masm=intel
	Try with various optimizations e.g. -O2

	2026/04/09: Compiles into out.s, assembles into a.out

	TODO: create a testing setup for each part of the compiler, not just codegen
*/

#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"
#include "taco.hpp"

static std::string preprocess(std::string fileName);
void lexer(std::string src);
GodNode *parser();
GodNode *sema(GodNode *ast);
std::vector<Instruction*> taco(GodNode *program);
std::string codegen(std::string fileName, std::vector<Instruction*> ir, GodNode* ast);
std::string readFile(std::string filename);
void printTokens();

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

	GodNode *ast = parser();

	GodNode *vAst = sema(ast); // validated ast

	#ifdef AST
	vAst->printChildren(1);
	fmt::print("\n");
	#endif

	std::vector<Instruction*> ir = taco(vAst);

	#ifdef TAC
	for (Instruction *i : ir) {
		i->print(1);
	}
	#endif

	std::string outputFilename = codegen(fileName, ir, vAst);

	#ifdef ASM
	std::string result = readFile(outputFilename);
	std::cout << "\n" << result << "\n";
	#endif

	fmt::print("Compiled to a.out\n");

	return 0;
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

void printIndentLines(int indent)
{
	std::string indentation{};
	std::string bar = "|";
	for (int i = 0; i < indent - 1; i++) {
		indentation.append(bar)
					.append("   ");
	}
	indentation.append(bar)
				.append("--");
	fmt::print(indentation);
}
