/*
	recursive descent parser

	expression -> term -> factor naming convention:
	expression for loose-binding + -
	term for tighter-binding like * /
	factor for things that have no operators or parentheses that explicitly group things.

	using EBNF (more or less):

	Terminals: type, IDENTIFIER, INTEGER

	function definition	->	type IDENTIFIER "(" paramList ")" functionBody
		  paramList	->	[ parameter ("," parameter)* ]
		      parameter ->	type IDENTIFIER
		   functionBody ->	(statement)* ret	// only return exists. The trailing mandatory ret will be removed eventually
				    ret ->	"return" expression
	

	declaration	->	type IDENTIFIER "=" expression ";"		// not implemented
				|	type IDENTIFIER ";"						// not implemented
	expression	->	term (("+"|"-") term)*
	term 		->	factor (("*") factor)*
	factor		->	IDENTIFIER | INTEGER | "(" expression ")"	// not implemented (only INTEGER is implemented)
*/

#include "main.hpp"

extern std::vector<Token> tokenList;
static int current = 0;

static Token advance(int advanceBy = 1);
static Token retreat(int retreatBy = -1);
static Token peek(int peekBy = 1);
static Token token();

// static Node *parseDeclaration();
static Node *parseExpression();
static Node *parseTerm();
static Node *parseFactor();
static Node *parseFuncDef();
static std::vector<Parameter> parseParamList();
static std::vector<Node*> parseFuncBody(FuncDefNode *funcNode);
static Type *parseType();

ProgramNode *parser()
{
	fmt::print("parseTokens\n");

	ProgramNode *program = new ProgramNode(0);

	while (token().tokenType != END_OF_FILE)
	{
		Node *node;
		if (token().tokenType == INT &&
			tokenList[current + 1].tokenType == IDENTIFIER &&
			tokenList[current + 2].tokenType == OPEN_PARENTHESES) {

			node = parseFuncDef();
			program->children.push_back(node);
			break;
		}
	}

	fmt::print("AST parsing completed\n");
	return program;
}

// current points to the token that was returned to make
// it easier to track state across function calls
// (tokenizer did [current++])
static Token advance(int advanceBy)
{
	current = current + advanceBy;
	return token();
}

static Token retreat(int retreatBy)
{
	current = current - retreatBy;
	return token();
}

static Token peek(int peekBy)
{
	return tokenList[current + peekBy];
}

static Token token()
{
	return tokenList.at(current);
}

// static Node *parseDeclaration()
// {
// 	// if (token().type == INT)
// 	// {

// 	// }
// }

static Node *parseExpression()
{
	Node *root = parseTerm();

	fmt::print("Type: {}\n", token().token_type_to_string(token().tokenType));
	while (token().tokenType != END_OF_FILE && token().tokenType != SEMICOLON)
	{		
		if (token().tokenType == PLUS || token().tokenType == MINUS)
		{
			BinaryOpNode *newRoot = new BinaryOpNode(token().line, token().tokenType);
			advance();			
			newRoot->left = root;
			newRoot->right = parseTerm();
			root = newRoot;
		}
		else {
			break;
		}
	}
	if (token().tokenType == SEMICOLON) advance();

	return root;
}

static Node *parseTerm()
{
	Node *root = parseFactor();
	advance();

	// (("*") factor)*
	while (token().tokenType == ASTERISK && peek().tokenType == INTEGER) {
		BinaryOpNode *newRoot = new BinaryOpNode(token().line, token().tokenType);
		advance();

		Node *newInt = parseFactor();
		advance();

		newRoot->left = root;
		newRoot->right = newInt;
		root = newRoot;
	}

	return root;
}

static Node *parseFactor()
{
	if (token().tokenType == INTEGER)
	{
		IntegerNode *node = new IntegerNode(token().line, std::get<int>(token().literal));
		return node;
	} else {
		return NULL;
	}
}

static Node *parseFuncDef()
{
	Type *returnType = parseType();

	FuncDefNode *funcNode = new FuncDefNode(token().line, token().lexeme, returnType);
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	funcNode->paramList = parseParamList();
	assert(tokenList.at(current).tokenType == CLOSED_PARENTHESES);
	advance();

	funcNode->body = parseFuncBody(funcNode);
	return funcNode;
}

static std::vector<Parameter> parseParamList()
{
	std::vector<Parameter> paramList;

	advance();
	if (token().tokenType == CLOSED_PARENTHESES) {
		return paramList;
	}

	while (token().tokenType == INT && peek(1).tokenType == IDENTIFIER)
	{
		Type *paramType = parseType();
		Parameter param{paramType, peek(1).lexeme};
		
		paramList.push_back(param);
		
		if (peek(2).tokenType == COMMA)
			advance(3);
		else advance(2);
	}

	return paramList;
}

static std::vector<Node*> parseFuncBody(FuncDefNode *funcNode)
{
	std::vector<Node*> body;
	advance();

	while (token().tokenType != END_OF_FILE &&
		   token().tokenType != CLOSED_CURLY_BRACE) {
		if (token().tokenType == RETURN)
		{
			ReturnNode *retNode = new ReturnNode(token().line);
			advance();
			retNode->expression = parseExpression();
			body.push_back(retNode);
		}
	}
	return body;
}

static Type *parseType()
{
	Type *type;
	switch (tokenList.at(current).tokenType)
	{
		case INT:
			type = new IntType();
			current++;
			break;
		
		default:
			throw_error_line(1, tokenList.at(current).line, "parseType failure: Invalid or missing type");
			return nullptr;
	}

	while (tokenList.at(current).tokenType == ASTERISK)
	{
		type = new PointerType(type);
		current++;
	}

	return type;
}