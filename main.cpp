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
std::unordered_map<Token::en, std::string> populatePrintmap()
{
	std::unordered_map<Token::en, std::string> printmap;
	
	printmap[ Token::en::SEMICOLON ] = "semicolon";
	printmap[ Token::en::PLUS ] = "plus";
	printmap[ Token::en::MINUS ] = "minus";
	printmap[ Token::en::ASTERISK ] = "asterisk";
	printmap[ Token::en::ASSIGNMENT ] = "assignment";
	printmap[ Token::en::OPEN_PARENTHESES ] = "open_parentheses";
	printmap[ Token::en::CLOSED_PARENTHESES ] = "closed_parentheses";
	printmap[ Token::en::OPEN_SQUARE_BRACKET ] = "open_square_bracket";
	printmap[ Token::en::CLOSED_SQUARE_BRACKET ] = "closed_square_bracket";
	printmap[ Token::en::OPEN_CURLY_BRACE ] = "open_curly_brace";
	printmap[ Token::en::CLOSED_CURLY_BRACE ] = "closed_curly_brace";
	printmap[ Token::en::COMMA ] = "comma";
	printmap[ Token::en::GREATER_THAN ] = "greater_than";
	printmap[ Token::en::LESS_THAN ] = "less_than";
	printmap[ Token::en::EQUAL_TO ] = "equal_to";
	printmap[ Token::en::GREATER_OR_EQUAL ] = "greater_or_equal";
	printmap[ Token::en::LESSER_OR_EQUAL ] = "lesser_or_equal";
	printmap[ Token::en::NOT_EQUAL ] = "not_equal";
	printmap[ Token::en::LOGICAL_NOT ] = "logical_not";
	printmap[ Token::en::LOGICAL_AND ] = "logical_and";
	printmap[ Token::en::LOGICAL_OR ] = "logical_or";
	printmap[ Token::en::BITWISE_NOT ] = "bitwise_not";
	printmap[ Token::en::BITWISE_AND ] = "bitwise_and";
	printmap[ Token::en::BITWISE_OR ] = "bitwise_or";
	printmap[ Token::en::IDENTIFIER ] = "identifier";
	printmap[ Token::en::CHAR ] = "char";
	printmap[ Token::en::SHORT ] = "short";
	printmap[ Token::en::INT ] = "int";
	printmap[ Token::en::LONG ] = "long";
	printmap[ Token::en::STRING_LITERAL ] = "string_literal";
	printmap[ Token::en::INTEGER ] = "integer";
	printmap[ Token::en::IF ] = "if";
	printmap[ Token::en::ELSE ] = "else";
	printmap[ Token::en::RETURN ] = "return";
	printmap[ Token::en::END_OF_FILE ] = "end_of_file";
	printmap[ Token::en::WHILE ] = "while";
	printmap[ Token::en::DO ] = "do";
	printmap[ Token::en::FOR ] = "for";
	printmap[ Token::en::BREAK ] = "break";
	printmap[ Token::en::CONTINUE ] = "continue";
	printmap[ Token::en::PREINC ] = "preinc";
	printmap[ Token::en::PREDEC ] = "predec";
	printmap[ Token::en::POSTINC ] = "postinc";
	printmap[ Token::en::POSTDEC ] = "postdec";
	printmap[ Token::en::INC ] = "inc";
	printmap[ Token::en::DEC ] = "dec";
	printmap[ Token::en::EXTERN ] = "extern";
	
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
