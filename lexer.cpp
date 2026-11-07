#include <vector>

#include "lexer.hpp"
#include "utils.hpp"

// all related to the tokenizer
static char peek();
static char advance();
static void parse_string();
static bool isAtEnd();
static void scanToken();
static bool is_alnum_underscore(char c);
static std::string scanLexeme();
static void populateKeywords(std::unordered_map<Token::en, std::string> printmap);
static bool parseInteger(std::string lexeme);
static int ctoi(char c);
void throw_invalid_identifier(int line);
void throw_invalid_identifier_start(int line);

static void addToken(Token::en type, 
					 int length, 
					 std::string lexeme, 
					 Literal literal = std::monostate{});

// contains all symbols and keywords
std::unordered_map<Token::en, std::string> populatePrintmap();

const std::unordered_map<Token::en, std::string> Token::printmap = populatePrintmap();

std::vector<Token> tokenList;

static unsigned long int current = 0; // i.e. the next unconsumed character is source[current]
static int column = 0;
static int line = 1;

std::string source;
std::unordered_map<std::string, Token::en> keywords;

void lexer(std::string src)
{
	source = src;

	populateKeywords(Token::printmap);

	while (!isAtEnd())
	{
		scanToken();
	}

	addToken(Token::en::END_OF_FILE, 1, "");

	int len = tokenList.size();
	fmt::print("Tokenizer done. tokenList size: {}\n", len);
}

static void scanToken()
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
			addToken(Token::en::SEMICOLON, 1, ";");
			break;
		case '+':
			if (peek() == '+') {
				addToken(Token::en::INC, 2, "++");
				advance();
			}
			else {
				addToken(Token::en::PLUS, 1, "+");
			}
			break;
		case '-':
			if (peek() == '-') {
				addToken(Token::en::DEC, 2, "--");
				advance();
			}
			else {
				addToken(Token::en::MINUS, 1, "-");
			}
			break;
		case '*':
			addToken(Token::en::ASTERISK, 1, "*");
			break;
		case '(':
			addToken(Token::en::OPEN_PARENTHESES, 1, "(");
			break;
		case ')':
			addToken(Token::en::CLOSED_PARENTHESES, 1, ")");
			break;
		case '{':
			addToken(Token::en::OPEN_CURLY_BRACE, 1, "{");
			break;
		case '}':
			addToken(Token::en::CLOSED_CURLY_BRACE, 1, "}");
			break;
		case '[':
			addToken(Token::en::OPEN_SQUARE_BRACKET, 1, "[");
			break;
		case ']':
			addToken(Token::en::CLOSED_SQUARE_BRACKET, 1, "]");
			break;
		case ',':
			addToken(Token::en::COMMA, 1, ",");
			break;
		case '=':
			if (peek() == '>') {
				addToken(Token::en::GREATER_OR_EQUAL, 2, "=>");
				advance();
			}
			else if (peek() == '<') {
				addToken(Token::en::LESSER_OR_EQUAL, 2, "=<");
				advance();
			}
			else if (peek() == '=') {
				addToken(Token::en::EQUAL_TO, 2, "==");
				advance();
			}
			else {
				addToken(Token::en::ASSIGNMENT, 1, "=");
			}
			break;
		case '>':
			if (peek() == '=') {
				addToken(Token::en::GREATER_OR_EQUAL, 2, ">=");
				advance();
			}
			else {
				addToken(Token::en::GREATER_THAN, 1, ">");
			}
			break;
		case '<':
			if (peek() == '=') {
				addToken(Token::en::LESSER_OR_EQUAL, 2, "<=");
				advance();
			}
			else {
				addToken(Token::en::LESS_THAN, 1, "<");
			}
			break;
		case '!':
			if (peek() == '=') {
				addToken(Token::en::NOT_EQUAL, 2, "!=");
				advance();
			}
			else {
				addToken(Token::en::LOGICAL_NOT, 1, "!");
			}
			break;
		case '&':
			if (peek() == '&') {
				addToken(Token::en::LOGICAL_AND, 2, "&&");
				advance();
			}
			else {
				addToken(Token::en::BITWISE_AND, 1, "&"); // won't work with pointers
			}
			break;
		case '|':
			if (peek() == '|') {
				addToken(Token::en::LOGICAL_OR, 2, "||");
				advance();
			}
			else {
				addToken(Token::en::BITWISE_OR, 1, "|");
			}
			break;
		case '"':
			parse_string();
			break;
		default:
			// now it's more than a 1-character token, so parse it FIRST, THEN match
			int start = current;
			std::string lexeme = scanLexeme();

			Token::en type;
			auto it = keywords.find(lexeme);
			if (it != keywords.end()) {
				type = it->second;
				addToken(type, current - start, lexeme);
				break;
			} else if (isNumber(lexeme[0])) {
				parseInteger(lexeme);
				break;
			}
			else {
				addToken(Token::en::IDENTIFIER, current - start, lexeme);
				break;
			}

			throw_error_line(1, line, fmt::format("Unexpected character: {}", c));
	}
}

// defines all permitted keywords
static void populateKeywords(std::unordered_map<Token::en, std::string> printmap)
{
	const std::vector<Token::en> keywordFilter = {
		Token::en::CHAR,
		Token::en::SHORT,
		Token::en::INT,
		Token::en::LONG,
		Token::en::IF,
		Token::en::ELSE,
		Token::en::RETURN,
		Token::en::WHILE,
		Token::en::DO,
		Token::en::FOR,
		Token::en::BREAK,
		Token::en::CONTINUE,
		Token::en::EXTERN
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

static void parse_string()
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
			addToken(Token::en::STRING_LITERAL, str.size(), str, str);
			advance();
			return;
		}
		advance();
	}
	if (isAtEnd())
	{
		throw_warn(line, 
				   std::string{"Unterminated string on line "}.append(std::to_string(line)));
	}
}

bool isNumber(char c)
{
	if (c <= '9' && c >= '0')
		return true;

	return false;
}

static void addToken(Token::en type, int length, std::string lexeme, Literal literal)
{
	Token token(type, length, lexeme, literal, line, column);
	tokenList.push_back(token);
}

static bool isAtEnd()
{
	return current >= source.length();
}

static char peek()
{
	return source[current];
}

static char advance()
{
	char c = source[current++];
	column++;
	if (c == '\n') {
		line++;
		column = 1;
	}
	return c;
}

static bool is_alnum_underscore(char c)
{
	if (isupper(c) || islower(c) || c == '_' || isNumber(c))
	{
		return true;
	}
	return false;
}

static std::string scanLexeme()
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

// negative numbers are managed in the parser with UnOp::en::NEGATE
static bool parseInteger(std::string lexeme)
{
	int number = 0;	
	int len = lexeme.length();
	for (int i = 0; i < len; i++)
	{
		if (!isNumber(lexeme[i]))
		{
			return false;
		}
		number *= 10;
		number += ctoi(lexeme[i]);
	}
	addToken(Token::en::INTEGER, current - len, lexeme, number);
	return true;
}

static int ctoi(char c)
{
	return c - '0';
}

void printTokens()
{
	int len = tokenList.size();
	for (int i = 0; i < len; i++)
	{
		tokenList[i].print();
	}
	fmt::print("End of Tokens. Lexeme List:\n");
	for (int i = 0; i < len; i++)
	{
		fmt::print("{}\n", tokenList[i].lexeme);
	}
}

void throw_invalid_identifier(int line)
{
	std::string s = "Invalid identifier symbol on line ";
	s.append(std::to_string(line));
	throw_error_line(2, line, s);
}

void throw_invalid_identifier_start(int line)
{
	std::string s = "Invalid identifier start symbol on line ";
	s.append(std::to_string(line));
	throw_error_line(2, line, s);
}
