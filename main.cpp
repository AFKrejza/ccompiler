#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

/*
	g++ main.cpp -o main && ./main source.c
	include -DDEBUG compiler flag for extra info

	nasm -felf64 output.asm && ld output.o && ./a.out
	echo $?

	TODO: create a testing setup for each part of the compiler, not just codegen.
*/

void throw_error(int code, std::string msg);
void throw_warn(int code, std::string msg);
bool is_whitespace(char c);
std::string read_source_code(char *file);
char peek();
char advance();
bool parse_int(std::string lexeme);
bool parse_identifier(std::string lexeme);
void parse_string();
bool isnum(char c);
bool isAtEnd();
void throw_invalid_identifier();
void throw_invalid_identifier_start();
void scanToken();
bool is_alnum_underscore(char c);
bool is_identifier_start(char c);
std::string scanLexeme(char c);
void populateKeywords();
void populatePrintmap();
bool parseInteger(std::string lexeme);
int ctoi(char c);

int current = 0; // i.e. the next unconsumed character is source[current]
int column = 0;
int line = 1;

// like a union
using Literal = std::variant<
	std::monostate,
	int,
	std::string,
	bool
>;

struct LiteralVisitor {
	std::string operator()(std::monostate v) const {
		return "empty";
	}
	std::string operator()(int v) const {
		return std::to_string(v);
	}
	std::string operator()(std::string v) const {
		return v;
	}
	std::string operator()(bool v) const {
		return v ? "True" : "False";
	}
};

enum TokenType {
	SEMICOLON, PLUS, MINUS, MULT, ASSIGNMENT, OPEN_PARENTHESES, CLOSED_PARENTHESES, OPEN_SQUARE_BRACKET, CLOSED_SQUARE_BRACKET, OPEN_CURLY_BRACE, CLOSED_CURLY_BRACE,

	GREATER_THAN, LESS_THAN, EQUAL_TO, GREATER_OR_EQUAL, LESSER_OR_EQUAL,
	NOT_EQUAL,

	LOGICAL_NOT, LOGICAL_AND, LOGICAL_OR,
	BITWISE_NOT, BITWISE_AND, BITWISE_OR,

	IDENTIFIER, CHAR, SHORT, INT, LONG, STRING_LITERAL, INTEGER,

	IF, ELSE, 

	RETURN,

	END_OF_FILE,
};

std::unordered_map<TokenType, std::string> printmap;

class Token {
	public:
		TokenType type;
		std::string lexeme; // exact source text
		Literal literal;
		int line;
		int column; // TODO: broken
		int length; // TODO: broken

		Token(TokenType type, int length, std::string lexeme, Literal literal, int line, int column) {
			this->type = type;
			this->lexeme = lexeme;
			this->literal = literal;
			this->line = line;
			this->column = column;
			this->length = length;
		}

		void print()
		{
			std::cout << "   Type: " << token_type_to_string(type) << std::endl;
			std::cout << " Lexeme: " << lexeme << std::endl;
			std::cout << "Literal: " << std::visit(LiteralVisitor{}, literal) << std::endl;
			std::cout << "   Line: " << line << std::endl;
			std::cout << " Column: " << column << std::endl;
			std::cout << " Length: " << length << "\n" << std::endl;
		}

	private:
		std::string token_type_to_string(TokenType type)
		{
			int int_type = (int)type;

			auto it = printmap.find(type);
			if (it != printmap.end()) {
				return it->second;
			}
			throw_error(3, "Couldn't find token to be printed. ???");
			return "";
		}

		std::string literal_to_string(Literal literal)
		{
			std::string string = typeid(literal).name();
			return string;
		}
};



void addToken(TokenType type, int length, std::string lexeme, Literal literal = std::monostate{});

std::string source;
std::vector<Token> tokenlist;
std::unordered_map<std::string, TokenType> keywords;

int main(int argc, char *argv[])
{
	if (argc < 2) throw_error(1, "Missing argument");
	if (argc > 2) throw_error(1, "Too many arguments (only takes one file)");
	
	source = read_source_code(argv[1]);	

	populatePrintmap();
	populateKeywords();

	while (!isAtEnd())
	{
		scanToken();
	}
	addToken(TokenType::END_OF_FILE, 1, "");

	int len = tokenlist.size();
	std::cout << "tokenlist size: " << len << std::endl;
	for (int i = 0; i < len; i++)
	{
		std::cout << tokenlist[i].lexeme << std::endl;
	}
	
	
	// output.close();
	std::cout << "success" << std::endl;
	return 0;
}

void scanToken()
{
	char c = advance();
	#ifdef DEBUG
		std::cout << "Pre-switch: " << c << std::endl;
	#endif

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
				addToken(TokenType::GREATER_OR_EQUAL, 2, ">=");
			}
			else if (peek() == '<') {
				addToken(TokenType::LESSER_OR_EQUAL, 2, "<=");
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
			std::cout << "New lexeme: " << lexeme << std::endl;
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

	#ifdef DEBUG
		std::cout << "parse_identifier: Start: " << start
		<< ", Current: " << current << ", Substr: " << 
		source.substr(start, current - start) << std::endl;
	#endif

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
	tokenlist.push_back(token);
	token.print();
}


// TODO: find a way to print where the error was thrown in this file (a C++ exception or something?)
void throw_invalid_identifier()
{
	std::string s = "Invalid identifier symbol on line ";
	s.append(std::to_string(line))
	 .append(" column ")
	 .append(std::to_string(column));
	throw_error(2, s);
}

void throw_invalid_identifier_start()
{
	std::string s = "Invalid identifier start symbol on line ";
	s.append(std::to_string(line))
	 .append(" column ")
	 .append(std::to_string(column));
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
	std::cerr << "Error: " << msg << std::endl;
	exit(code);
}

void throw_warn(int code, std::string msg)
{
	std::cerr << "Warning :" << msg << std::endl;
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

	#ifdef DEBUG
		std::cout << "NEW LEXEME: " << lexeme << "length: " << lexeme.length() << std::endl;
	#endif

	return lexeme;
}

bool parse_keyword(std::string lexeme)
{
	return true;
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

// contains all symbols and keywords
void populatePrintmap()
{
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
}

void populateKeywords()
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