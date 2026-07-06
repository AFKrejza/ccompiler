#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

#include "main.hpp"

/*
	g++ main.cpp parser.cpp -o main && ./main source.c
	-DDEBUG to print all tokens

	nasm -felf64 output.asm && ld output.o && ./a.out
	echo $?

	TODO: create a testing setup for each part of the compiler, not just codegen. In python.

	int a = 5;
	int b = 10;
	int c = a + b;
	return c;
*/


static std::string read_source_code(char *file);

// all related to the tokenizer
static bool is_whitespace(char c);
static char peek();
static char advance();
static bool parse_identifier(std::string lexeme);
static void parse_string();
static bool isnum(char c);
static bool isAtEnd();
static void scanToken();
static bool is_alnum_underscore(char c);
static bool is_identifier_start(char c);
static std::string scanLexeme(char c);
static void populateKeywords(std::unordered_map<TokenType, std::string> printmap);
static bool parseInteger(std::string lexeme);
static int ctoi(char c);
static void printTokens();

// contains all symbols and keywords
std::unordered_map<TokenType, std::string> populatePrintmap();

static int current = 0; // i.e. the next unconsumed character is source[current]
static int column = 0;
static int line = 1;

const std::unordered_map<TokenType, std::string> Token::printmap = populatePrintmap();
void addToken(TokenType type, int length, std::string lexeme, Literal literal = std::monostate{});

std::string source;
std::vector<Token> tokenList;
std::unordered_map<std::string, TokenType> keywords;

int main(int argc, char *argv[])
{
	if (argc < 2) throw_error(1, "Missing argument");
	if (argc > 2) throw_error(1, "Too many arguments (only takes one file)");
	
	source = read_source_code(argv[1]);	

	populateKeywords(Token::printmap);

	while (!isAtEnd())
	{
		scanToken();
	}
	addToken(TokenType::END_OF_FILE, 1, "");

	int len = tokenList.size();
	std::cout << "tokenList size: " << len << std::endl;
	#ifdef DEBUG
		printTokens();
	#endif

	parser();



	// output.close();
	std::cout << "success" << std::endl;
	return 0;
}

void scanToken()
{
	char c = advance();

	switch (c)
	{
		case ' ':
			break;
		case '\r':
			break;
		case '\t':
			break;
		case '\n':
			break;
		case ';':
			addToken(TokenType::SEMICOLON, 1, ";");
			break;
		case '+':
			addToken(TokenType::PLUS, 1, "+");
			break;
		case '-':
			addToken(TokenType::MINUS, 1, "-");
			break;
		case '*':
			addToken(TokenType::MULT, 1, "*");
			break;
		case '(':
			addToken(TokenType::OPEN_PARENTHESES, 1, "(");
			break;
		case ')':
			addToken(TokenType::CLOSED_PARENTHESES, 1, ")");
			break;
		case '{':
			addToken(TokenType::OPEN_CURLY_BRACE, 1, "{");
			break;
		case '}':
			addToken(TokenType::CLOSED_CURLY_BRACE, 1, "}");
			break;
		case '[':
			addToken(TokenType::OPEN_SQUARE_BRACKET, 1, "[");
			break;
		case ']':
			addToken(TokenType::CLOSED_SQUARE_BRACKET, 1, "]");
			break;
		case '=':
			if (peek() == '>') {
				addToken(TokenType::GREATER_OR_EQUAL, 2, "=>");
			}
			else if (peek() == '<') {
				addToken(TokenType::LESSER_OR_EQUAL, 2, "=<");
			}
			else if (peek() == '=') {
				addToken(TokenType::EQUAL_TO, 2, "==");
			}
			else {
				addToken(TokenType::ASSIGNMENT, 1, "=");
			}
			break;
		case '>':
			if (peek() == '=') {
				addToken(TokenType::GREATER_OR_EQUAL, 2, ">=");
			}
			else {
				addToken(TokenType::GREATER_THAN, 1, ">");
			}
			break;
		case '<':
			if (peek() == '=') {
				addToken(TokenType::LESSER_OR_EQUAL, 2, "<=");
			}
			else {
				addToken(TokenType::LESS_THAN, 1, "<");
			}
			break;
		case '!':
			if (peek() == '=') {
				addToken(TokenType::NOT_EQUAL, 2, "!=");
			}
			else {
				addToken(TokenType::LOGICAL_NOT, 1, "!");
			}
			break;
		case '&':
			if (peek() == '&') {
				addToken(TokenType::LOGICAL_AND, 2, "&&");
			}
			else {
				addToken(TokenType::BITWISE_AND, 1, "&"); // won't work with pointers
			}
			break;
		case '"':
			parse_string();
			break;
		default:
			// now it's more than a 1-character token, so parse it FIRST, THEN match
			int start = current;
			std::string lexeme = scanLexeme(c);

			TokenType type;
			auto it = keywords.find(lexeme);
			if (it != keywords.end()) {
				type = it->second;
				addToken(type, current - start, lexeme);
				break;
			} else if (isnum(lexeme[0])) {
				parseInteger(lexeme);
				break;
			}
			else {
				addToken(IDENTIFIER, current - start, lexeme);
				break;
			}

			std::cerr << "Unexpected character: " << c << std::endl;
	}
}

void parse_string()
{
	int start = current;

	std::string str;
	std::string lexeme;
	advance();
	while (!isAtEnd())
	{
		if (source[current] == '"')
		{
			str = source.substr(start, current - start);
			addToken(TokenType::STRING_LITERAL, str.size(), str, str);
			advance();
			return;
		}
		advance();
	}
	if (isAtEnd())
	{
		throw_warn(4, std::string{"Unterminated string on line "}.append(std::to_string(line)));
	}
}

// https://en.cppreference.com/cpp/language/identifiers
bool parse_identifier(std::string lexeme)
{
	int start = current;
	char c = source[current];
	int len = lexeme.length();
	if (!is_identifier_start(lexeme[0]))
	{
		throw_invalid_identifier_start();
	}
	for (int i = 0; i < len; i++)
	{
		if (!is_alnum_underscore(lexeme[i]))
		{
			throw_invalid_identifier();
		}
	}
	addToken(TokenType::IDENTIFIER, current - start, lexeme);

	return true;
}

bool isnum(char c)
{
	if (c <= '9' && c >= '0')
		return true;

	return false;
}

void addToken(TokenType type, int length, std::string lexeme, Literal literal)
{
	Token token(type, length, lexeme, literal, line, column);
	tokenList.push_back(token);
}


// TODO: find a way to print where the error was thrown in this file (a C++ exception or something?)
void throw_invalid_identifier()
{
	std::string s = "Invalid identifier symbol on line ";
	s.append(std::to_string(line));
	//  .append(" column ")
	//  .append(std::to_string(column));
	throw_error(2, s);
}

void throw_invalid_identifier_start()
{
	std::string s = "Invalid identifier start symbol on line ";
	s.append(std::to_string(line));
	//  .append(" column ")
	//  .append(std::to_string(column));
	throw_error(2, s);
}

bool is_whitespace(char c)
{
	switch (c)
	{
		case ' ':
			return true;
		case '\r':
			return true;
		case '\t':
			return true;
		case '\n':
			return true;
		default:
			return false;
	}
}

bool isAtEnd()
{
	return current >= source.length();
}

char peek()
{
	return source[current];
}

char advance()
{
	char c = source[current++];
	column++;
	if (c == '\n') {
		line++;
		column = 1;
	}
	return c;
}

void throw_error(int code, std::string msg)
{
	std::cerr << "Error on line " << line << ": " << msg << std::endl;
	exit(code);
}

void throw_warn(int code, std::string msg)
{
	std::cerr << "Warning on line " << line << ": " << msg << std::endl;
}

std::string read_source_code(char *filename)
{
	std::ifstream file(filename);
	if (!file.is_open()) throw_error(1, "Couldn't open source file");

	std::string source_code{
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	};
	return source_code;
}

bool is_identifier_start(char c)
{
	if (isupper(c) || islower(c) || c == '_')
	{
		return true;
	}
	return false;
}

bool is_alnum_underscore(char c)
{
	if (isupper(c) || islower(c) || c == '_' || isnum(c))
	{
		return true;
	}
	return false;
}

std::string scanLexeme(char c)
{
	std::string lexeme;
	int start = current - 1;

	if (is_alnum_underscore(source[current]))
	{
		while (is_alnum_underscore(peek()))
		{
			advance();
		}
	}
	lexeme = source.substr(start, current - start);

	return lexeme;
}

// only does positive ints
bool parseInteger(std::string lexeme)
{
	int number = 0;	
	int len = lexeme.length();
	for (int i = 0; i < len; i++)
	{
		if (!isnum(lexeme[i]))
		{
			return false;
		}
		number *= 10;
		number += ctoi(lexeme[i]);
	}
	addToken(INTEGER, current - len, lexeme, number);
	return true;
}

int ctoi(char c)
{
	return c - '0';
}

void populateKeywords(std::unordered_map<TokenType, std::string> printmap)
{
	const std::vector<TokenType> keywordFilter = {
		CHAR, SHORT, INT, LONG, IF, ELSE, RETURN
	};

	int len = keywordFilter.size();
	for (int i = 0; i < len; i++)
	{
		auto it = printmap.find(keywordFilter[i]);
		if (it != printmap.end()) {
			keywords[it->second] = it->first;
		}
	}
}

void printTokens()
{
	int len = tokenList.size();
	for (int i = 0; i < len; i++)
	{
		tokenList[i].print();
	}
	std::cout << "End of Tokens. Lexeme List:" << std::endl;
	for (int i = 0; i < len; i++)
	{
		std::cout << tokenList[i].lexeme << std::endl;
	}
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
