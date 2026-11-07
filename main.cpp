/*
	compile with make

	echo $?
	To check gcc assembly: gcc INPUT.c -S -masm=intel
	Try with various optimizations e.g. -O2

	2026-09-27: Compiles into out.s, assembles into a.out

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
std::unique_ptr<GodNode> parser();
void sema(GodNode* prog);
std::vector<std::unique_ptr<Instruction>> taco(GodNode *program);
std::string codegen(std::string fileName,
					std::vector<std::unique_ptr<Instruction>> ir,
					GodNode* ast);
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

	std::unique_ptr<GodNode> ast = parser();

	sema(ast.get());

	#ifdef AST
	ast->printChildren(1);
	fmt::print("\n");
	#endif

	std::vector<std::unique_ptr<Instruction>> ir = taco(ast.get());

	#ifdef TAC
	for (const std::unique_ptr<Instruction>& i : ir) {
		i->print(1);
	}
	#endif

	std::string outputFilename = codegen(fileName, std::move(ir), ast.get());

	#ifdef ASM
	std::string result = readFile(outputFilename);
	std::cout << "\n" << result << "\n";
	#endif

	fmt::print("Compiled to a.out\n");

	return 0;
}

void throw_error_line(int code, int line, std::string msg)
{
	fmt::print("Error {} : line {} : {}\n", code, line, msg);
	// segfaulting for backtraces. gotta be a better way.
	int c = *(int*)0;
	(void)c;
	exit(code);
}

void throw_error(int code, std::string msg)
{
	fmt::print("Error {} : {}\n", code, msg);
	int c = *(int*)0;
	(void)c;
	exit(code);
}

void throw_warn(int line, std::string msg)
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
	
	printmap[ TokenType::SEMICOLON ] = "semicolon";
	printmap[ TokenType::PLUS ] = "plus";
	printmap[ TokenType::MINUS ] = "minus";
	printmap[ TokenType::ASTERISK ] = "asterisk";
	printmap[ TokenType::ASSIGNMENT ] = "assignment";
	printmap[ TokenType::OPEN_PARENTHESES ] = "open_parentheses";
	printmap[ TokenType::CLOSED_PARENTHESES ] = "closed_parentheses";
	printmap[ TokenType::OPEN_SQUARE_BRACKET ] = "open_square_bracket";
	printmap[ TokenType::CLOSED_SQUARE_BRACKET ] = "closed_square_bracket";
	printmap[ TokenType::OPEN_CURLY_BRACE ] = "open_curly_brace";
	printmap[ TokenType::CLOSED_CURLY_BRACE ] = "closed_curly_brace";
	printmap[ TokenType::COMMA ] = "comma";
	printmap[ TokenType::GREATER_THAN ] = "greater_than";
	printmap[ TokenType::LESS_THAN ] = "less_than";
	printmap[ TokenType::EQUAL_TO ] = "equal_to";
	printmap[ TokenType::GREATER_OR_EQUAL ] = "greater_or_equal";
	printmap[ TokenType::LESSER_OR_EQUAL ] = "lesser_or_equal";
	printmap[ TokenType::NOT_EQUAL ] = "not_equal";
	printmap[ TokenType::LOGICAL_NOT ] = "logical_not";
	printmap[ TokenType::LOGICAL_AND ] = "logical_and";
	printmap[ TokenType::LOGICAL_OR ] = "logical_or";
	printmap[ TokenType::BITWISE_NOT ] = "bitwise_not";
	printmap[ TokenType::BITWISE_AND ] = "bitwise_and";
	printmap[ TokenType::BITWISE_OR ] = "bitwise_or";
	printmap[ TokenType::IDENTIFIER ] = "identifier";
	printmap[ TokenType::CHAR ] = "char";
	printmap[ TokenType::SHORT ] = "short";
	printmap[ TokenType::INT ] = "int";
	printmap[ TokenType::LONG ] = "long";
	printmap[ TokenType::STRING_LITERAL ] = "string_literal";
	printmap[ TokenType::INTEGER ] = "integer";
	printmap[ TokenType::IF ] = "if";
	printmap[ TokenType::ELSE ] = "else";
	printmap[ TokenType::RETURN ] = "return";
	printmap[ TokenType::END_OF_FILE ] = "end_of_file";
	printmap[ TokenType::WHILE ] = "while";
	printmap[ TokenType::DO ] = "do";
	printmap[ TokenType::FOR ] = "for";
	printmap[ TokenType::BREAK ] = "break";
	printmap[ TokenType::CONTINUE ] = "continue";
	printmap[ TokenType::PREINC ] = "preinc";
	printmap[ TokenType::PREDEC ] = "predec";
	printmap[ TokenType::POSTINC ] = "postinc";
	printmap[ TokenType::POSTDEC ] = "postdec";
	printmap[ TokenType::INC ] = "inc";
	printmap[ TokenType::DEC ] = "dec";
	printmap[ TokenType::EXTERN ] = "extern";
	
	return printmap;
};

static std::string preprocess(std::string fileName)
{
	std::string preprocessed = fileName;
	preprocessed.append(".i");
	std::string script = fmt::format("gcc -E -P {}.c -o {}", fileName, preprocessed);
	fmt::print("{}\n", script);
	int call = system(script.c_str());
	if (call != 0) throw_error(call, "Preprocessor failure");
	
	std::string source = readFile(preprocessed);
	
	script = fmt::format("rm {}", preprocessed);
	call = system(script.c_str());
	if (call != 0) throw_error(call, 
							   fmt::format("Preprocessor issue deleting file {}", preprocessed));

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
