#include "main.hpp"

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

static void addToken(TokenType type, int length, std::string lexeme, Literal literal = std::monostate{});

// contains all symbols and keywords
std::unordered_map<TokenType, std::string> populatePrintmap();

const std::unordered_map<TokenType, std::string> Token::printmap = populatePrintmap();

std::vector<Token> tokenList;

static int current = 0; // i.e. the next unconsumed character is source[current]
static int column = 0;
static int line = 1;

std::string source;
std::unordered_map<std::string, TokenType> keywords;

void lexer(std::string src)
{
	source = src;

	populateKeywords(Token::printmap);

	while (!isAtEnd())
	{
		scanToken();
	}

	addToken(TokenType::END_OF_FILE, 1, "");

	int len = tokenList.size();
	fmt::print("tokenList size: {}\n", len);
	#ifdef DEBUG
		printTokens();
	#endif
	
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
		case ',':
			addToken(TokenType::COMMA, 1, ",");
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


// defines all permitted keywords
static void populateKeywords(std::unordered_map<TokenType, std::string> printmap)
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
			addToken(TokenType::STRING_LITERAL, str.size(), str, str);
			advance();
			return;
		}
		advance();
	}
	if (isAtEnd())
	{
		throw_warn(4, line, std::string{"Unterminated string on line "}.append(std::to_string(line)));
	}
}


// https://en.cppreference.com/cpp/language/identifiers
static bool parse_identifier(std::string lexeme)
{
	int start = current;
	char c = source[current];
	int len = lexeme.length();
	if (!is_identifier_start(lexeme[0]))
	{
		throw_invalid_identifier_start(line);
	}
	for (int i = 0; i < len; i++)
	{
		if (!is_alnum_underscore(lexeme[i]))
		{
			throw_invalid_identifier(line);
		}
	}
	addToken(TokenType::IDENTIFIER, current - start, lexeme);

	return true;
}


static bool isnum(char c)
{
	if (c <= '9' && c >= '0')
		return true;

	return false;
}


static void addToken(TokenType type, int length, std::string lexeme, Literal literal)
{
	Token token(type, length, lexeme, literal, line, column);
	tokenList.push_back(token);
}


static bool is_whitespace(char c)
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


static bool is_identifier_start(char c)
{
	if (isupper(c) || islower(c) || c == '_')
	{
		return true;
	}
	return false;
}


static bool is_alnum_underscore(char c)
{
	if (isupper(c) || islower(c) || c == '_' || isnum(c))
	{
		return true;
	}
	return false;
}


static std::string scanLexeme(char c)
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
static bool parseInteger(std::string lexeme)
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


static int ctoi(char c)
{
	return c - '0';
}


static void printTokens()
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
