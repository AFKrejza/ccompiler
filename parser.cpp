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

	Token *temp = new Token();
	ProgramNode *program = new ProgramNode(temp);

	while (tokenList[current].type != END_OF_FILE)
	{
		Node *node;
		if (tokenList[current].type == INT &&
			tokenList[current + 1].type == IDENTIFIER &&
			tokenList[current + 2].type == OPEN_PARENTHESES) {

			node = parseFuncDef();
			program->children.push_back(node);
			break;
		}
	}

	// program->printChildren(1);

	fmt::print("AST parsing completed\n");
	return program;
}

// current points to the token that was returned to make
// it easier to track state across function calls
// (tokenizer did [current++])
static Token advance(int advanceBy)
{
	current = current + advanceBy;
	return tokenList[current];
}

static Token retreat(int retreatBy)
{
	current = current - retreatBy;
	return tokenList[current];
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
// 	// if (tokenList[current].type == INT)
// 	// {

// 	// }
// }

static Node *parseExpression()
{
	Node *root = parseTerm();

	fmt::print("Type: {}\n", tokenList[current].token_type_to_string(tokenList[current].type));
	while (tokenList[current].type != END_OF_FILE && tokenList[current].type != SEMICOLON)
	{		
		if (tokenList[current].type == PLUS || tokenList[current].type == MINUS)
		{
			BinaryOpNode *newRoot = new BinaryOpNode(&tokenList[current]);
			advance();			
			newRoot->left = root;
			newRoot->right = parseTerm();
			root = newRoot;
		}
		else {
			break;
		}
	}
	if (tokenList[current].type == SEMICOLON) advance();

	return root;
}

static Node *parseTerm()
{
	Node *root = parseFactor();
	advance();

	// (("*") factor)*
	while (tokenList[current].type == ASTERISK && peek().type == INTEGER) {
		BinaryOpNode *newRoot = new BinaryOpNode(&tokenList[current]);
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
	Token token = tokenList[current];
	if (token.type == INTEGER)
	{
		IntegerNode *node = new IntegerNode(&tokenList[current]);
		return node;
	} else {
		return NULL;
	}
}

static Node *parseFuncDef()
{
	Type *returnType = parseType();

	FuncDefNode *funcNode = new FuncDefNode(&tokenList[current], returnType);
	advance();
	assert(tokenList[current].type == OPEN_PARENTHESES);
	funcNode->paramList = parseParamList();
	assert(tokenList.at(current).type == CLOSED_PARENTHESES);
	advance();

	funcNode->body = parseFuncBody(funcNode);
	return funcNode;
}

static std::vector<Parameter> parseParamList()
{
	std::vector<Parameter> paramList;

	advance();
	if (token().type == CLOSED_PARENTHESES) {
		return paramList;
	}

	while (token().type == INT && peek(1).type == IDENTIFIER)
	{
		Type *paramType = parseType();
		Parameter param{paramType, peek(1).lexeme};
		
		paramList.push_back(param);
		
		if (peek(2).type == COMMA)
			advance(3);
		else advance(2);
	}

	return paramList;
}

static std::vector<Node*> parseFuncBody(FuncDefNode *funcNode)
{
	std::vector<Node*> body;
	advance();

	while (token().type != END_OF_FILE &&
		   token().type != CLOSED_CURLY_BRACE) {
		if (token().type == RETURN)
		{
			ReturnNode *retNode = new ReturnNode(&tokenList[current]);
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
	switch (tokenList.at(current).type)
	{
		case INT:
			type = new IntType();
			current++;
			break;
		
		default:
			throw_error_line(1, tokenList.at(current).line, "parseType failure: Invalid or missing type");
			return nullptr;
	}

	while (tokenList.at(current).type == ASTERISK)
	{
		type = new PointerType(type);
		current++;
	}

	return type;
}