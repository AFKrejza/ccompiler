#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

/*
	g++ main.cpp -o main && ./main source.c
	nasm -felf64 output.asm && ld output.o && ./a.out
	echo $?

	TODO: create a testing setup. Do Test-driven development for this stuff.

	// look at first commit of chibicc: https://github.com/rui314/chibicc/commit/0522e2d77e3ab82d3b80a5be8dbbdc8d4180561c
	// go through them in order!
	// the first one just takes a value and returns it.

	// https://gpfault.net/posts/asm-tut-3.txt.html

	// https://craftinginterpreters.com/scanning.html

	Goals: create a scanner. create a list of tokens for reserved keywords as an enum
	then create a token class

	consider adding this to the Token class as stated in 4.2.3 'Location Information'
	int offset/column;
	int length;

	look into: std::vector, std::unordered_map, std::string, std::variant

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

// https://gist.github.com/MangaD/eb7dd67de08072edb6fa83c534716a80
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
	// single character
	SEMICOLON, PLUS, MINUS, ASSIGNMENT,

	// one or two character

	// literals
	IDENTIFIER, CHAR, SHORT, INT, LONG, STRING_LITERAL, INTEGER,

	// keywords
	IF, ELSE, 

	// MAIN, // must contain int main() ? idk dude
	RETURN,

	END_OF_FILE, // is this necessary? probably not.
};

// TODO: write a function which populates tokentypes by string. it could just read this source file directly?

// TODO: add length to make debugging easier
class Token {
	public:
		TokenType type;
		std::string lexeme; // exact source text
		Literal literal;
		int line;
		int column;
		int length;

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
		// TODO: this it outdated and simply a bad implementation.
		// put all possible keywords into the hashmap and just read it from there each time.
		std::string token_type_to_string(TokenType type)
		{
			int int_type = (int)type;
			switch (int_type)
			{
				case 0: return "SEMICOLON";
				case 1: return "PLUS";
				case 2: return "MINUS";
				case 3: return "ASSIGNMENT";
				case 4: return "IDENTIFIER";
				case 5: return "CHAR";
				case 6: return "SHORT";
				case 7: return "INT";
				case 8: return "LONG";
				case 9: return "STRING_LITERAL";
				case 10: return "IF";
				case 11: return "ELSE";
				case 12: return "RETURN";
				case 13: return "END_OF_FILE";
			};
			throw_error(3, "lmfao dumbass");
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
	// enum TokenType a = TokenType::INT;
	// Token token(a, "testlexeme", "meow", 4, 0, 4);

	if (argc < 2) throw_error(1, "Missing argument");
	if (argc > 2) throw_error(1, "Too many arguments (only takes one file)");
	
	source = read_source_code(argv[1]);	

	populateKeywords();


	
	// greed
	while (!isAtEnd())
	{
		scanToken();
	}

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
	// first get the lexeme, then select it from the list

	char c = advance();
	// std::cout << "Pre-switch: " << c << std::endl;
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
		case '=':
			addToken(TokenType::ASSIGNMENT, 1, "=", false);
			break;
		case ';':
			addToken(TokenType::SEMICOLON, 1, ";", false);
			break;
		// add stuff like >= <= != == as well aka 2-char tokens
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

bool parse_int(std::string lexeme)
{
	int start = current;
	std::string match = "int";

	for (int i = 0; i < match.length(); i++)
	{
		if (source[current] != match[i] || is_whitespace(peek()))
			return false;
		std::cout << "debug" << std::endl;
		advance();
	}
	if (is_whitespace(source[current]) || source[current] == ';')
	{
		addToken(TokenType::INT, current - start, source.substr(start, current - start));
		return true;
	}
	return false;


	// if (source[current + 1] == 'n' && source[current + 2] == 't' && is_whitespace(source[current + 3]))
	// {
	// 	Token token(TokenType::INT, source.substr(current, current + 2), false, line, column);
	// 	tokenlist.push_back(token);
	// 	current += 3;
	// 	return true;
	// }
	// return false;
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

	// while (!is_whitespace(peek()))
	// {
	// 	if (!is_alnum_underscore(c))
	// 	{
	// 		// throw_invalid_identifier();
	// 		break;
	// 	}
	// 	c = advance();
	// }
	std::cout << start << ", " << current << std::endl;
	std::cout << source.substr(start, current - start) << std::endl;
	addToken(TokenType::IDENTIFIER, current - start, lexeme);
	return true;
}

bool isnum(char c)
{
	if (c <= '9' && c >= '0')
		return true;

	return false;
}


// TODO: learn how unions work here, like adding stuff to it. Learning it for C will also be useful.
void addToken(TokenType type, int length, std::string lexeme, Literal literal)
{
	Token token(type, length, lexeme, literal, line, column);
	tokenlist.push_back(token);
	token.print();
}


// TODO: find a way to print where the error was thrown in this file (surely a C++ exception or something?)
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

// TODO: make a string for identifier start chars and identifier chars

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

	// if (!is_identifier_start(c))
	// {
	// 	std::cout << "meow" << std::endl;
	// 	return "";
	// }
	std::cout << "At: " << source.substr(start, current - start) << std::endl;
	std::cout << "Start: " << start << " End: " << current << std::endl;
	if (is_alnum_underscore(source[current]))
	{
		while (is_alnum_underscore(peek())) // is this right?
		{
			advance();
		}
	}

	// while (!isAtEnd() && is_alnum_underscore(c))
	// {
	// 	if (!is_alnum_underscore(peek()))
	// 		break;
	// 	c = advance();
	// }
	// std::cout << "Lexeme scan end: " << c << std::endl;
	lexeme = source.substr(start, current - start);
	// std::cout << "NEW LEXEME: " << lexeme << "length: " << lexeme.length() << std::endl;
	return lexeme;
}

bool parse_keyword(std::string lexeme)
{
	return true;
}

void populateKeywords() // single- and double- character keywords can be removed from here
{
	keywords["semicolon"] = SEMICOLON;
	keywords["plus"] = PLUS;
	keywords["minus"] = MINUS;
	keywords["assignment"] = ASSIGNMENT;
	keywords["identifier"] = IDENTIFIER;
	keywords["char"] = CHAR;
	keywords["short"] = SHORT;
	keywords["int"] = INT;
	keywords["long"] = LONG;
	keywords["string_literal"] = STRING_LITERAL;
	keywords["if"] = IF;
	keywords["else"] = ELSE;
	keywords["return"] = RETURN;
	keywords["end_of_file"] = END_OF_FILE;
	keywords["integer"] = INTEGER;
}

// only does positive ints
bool parseInteger(std::string lexeme)
{
	//bool isPositive = true;
	//if (lexeme[0] == '-')
	//{
	//	isPositive = false;
	//}
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