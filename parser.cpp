/*
	recursive descent parser
*/

#include <cassert>
#include <vector>

#include "ast.hpp"
#include "lexer.hpp"

extern std::vector<Token> tokenList;
static int current = 0;

static Token advance(int advanceBy = 1);
static Token retreat(int retreatBy = 1);
static Token peek(int peekBy = 1);
static inline Token token();

static Node *parseExpression();
static Node* parseLogicalOr();
static Node* parseLogicalAnd();
static Node* parseEquality();
static Node* parseRelational();
static Node* parseAdditive();
static Node *parseTerm();
static Node* parseUnary();
static Node *parseFactor();
static Node *parseFuncDef(GodNode* parent);
static std::vector<Parameter> parseParamList();
static std::vector<Node*> parseStatements(bool isCompound, FuncDefNode *func);
static Type *parseType();
static Node* parseDeclaration();
static AssignmentNode* parseAssignment();

static Node* parseIf(FuncDefNode* func);

GodNode *parser()
{
	GodNode *program = new GodNode(0);

	while (token().tokenType != END_OF_FILE)
	{
		Node *node;
		if (token().tokenType == INT &&
			tokenList[current + 1].tokenType == IDENTIFIER &&
			tokenList[current + 2].tokenType == OPEN_PARENTHESES) {

			node = parseFuncDef(program);
			program->body.push_back(node);
			break;
		}
		else {
			throw_error_line(1, 
							 token().line, 
							 fmt::format("Parser failure: no rule for token {}",
								token().tokenTypeToStr(token().tokenType)));
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

// starts at first token of expression, ends after semicolon
static Node *parseExpression()
{
	Node *root;

	if (token().tokenType != END_OF_FILE &&
		token().tokenType != SEMICOLON)
	{
		root = parseLogicalOr();
	}
	return root;
}

static Node* parseLogicalOr()
{
	Node* root = parseLogicalAnd();

	while (token().tokenType == LOGICAL_OR)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		node->left = root;
		advance();
		node->right = parseLogicalAnd();
		root = node;
	}
	return root;
}

static Node* parseLogicalAnd()
{
	Node* root = parseEquality();

	while (token().tokenType == LOGICAL_AND)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		node->left = root;
		advance();
		node->right = parseEquality();
		root = node;
	}
	return root;
}

static Node* parseEquality()
{
	Node* root = parseRelational();

	while (token().tokenType == EQUAL_TO ||
		token().tokenType == NOT_EQUAL)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		advance();
		node->left = root;
		node->right = parseRelational();
		root = node;
	}
	return root;
}

static Node* parseRelational()
{
	Node* root = parseAdditive();

	while (token().tokenType == LESS_THAN ||
		   token().tokenType == LESSER_OR_EQUAL ||
		   token().tokenType == GREATER_THAN ||
		   token().tokenType == GREATER_OR_EQUAL)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		advance();
		node->left = root;
		node->right = parseAdditive();
		root = node;
	}
	return root;
}

static Node* parseAdditive()
{
	Node* root = parseTerm();

	while (token().tokenType == PLUS ||
		   token().tokenType == MINUS)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		advance();
		node->left = root;
		node->right = parseTerm();
		root = node;
	}
	return root;
}

static Node *parseTerm()
{
	Node *root;

	root = parseUnary();

	while (token().tokenType == ASTERISK)
	{
		BinaryOpNode *newRoot = new BinaryOpNode(token().line, token().tokenType);
		advance();

		Node *newFactor = parseUnary();

		newRoot->left = root;
		newRoot->right = newFactor;
		root = newRoot;
	}
	return root;
}

static Node* parseUnary()
{
	Node* root;

	if (token().tokenType == MINUS)
	{
		UnaryOpNode *unop = new UnaryOpNode(token().line, token().tokenType, NULL);
		advance();
		unop->expression = parseFactor();
		root = unop;
	}
	else {
		root = parseFactor();
	}
	return root;
}

static Node *parseFactor()
{
	Node* factor;

	if (token().tokenType == INTEGER)
	{
		factor = new ImmediateNode(token().line, std::get<int>(token().literal));
		advance();
	}
	else if (token().tokenType == IDENTIFIER)
	{
		factor = new VariableNode(token().line, token().lexeme);
		advance();
	}
	else if (token().tokenType == OPEN_PARENTHESES)
	{
		advance();
		factor = parseExpression();
		assert(token().tokenType == CLOSED_PARENTHESES);
		advance();
	}
	else {
		throw_error_line(1, 
						 token().line, 
						 fmt::format("Invalid factor: '{}', type '{}'", 
							token().lexeme, 
							token().tokenTypeToStr(token().tokenType)));
	}
	return factor;
}

static Node *parseFuncDef(GodNode* parent)
{
	Type *returnType = parseType();

	FuncDefNode *funcNode = new FuncDefNode(token().line, parent, token().lexeme, returnType);
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	funcNode->paramList = parseParamList();
	assert(tokenList.at(current).tokenType == CLOSED_PARENTHESES);
	advance();

	funcNode->body = parseStatements(true, funcNode);
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

static std::vector<Node*> parseStatements(bool isCompound, FuncDefNode *func)
{
	if (isCompound) {
		assert(token().tokenType == OPEN_CURLY_BRACE);
		advance();
	}

	std::vector<Node*> body;

	while (token().tokenType != END_OF_FILE)
	{
		if (isCompound && token().tokenType == CLOSED_CURLY_BRACE) {
			advance();
			return body;
		}

		if (token().tokenType == RETURN)
		{
			ReturnNode *retNode = new ReturnNode(token().line);
			advance();
			retNode->expression = parseExpression();
			assert(token().tokenType == SEMICOLON);
			advance();
			body.push_back(retNode);
		}
		else if (token().tokenType == INT &&
				 peek(1).tokenType == IDENTIFIER)
		{
			body.push_back(parseDeclaration());
			assert(token().tokenType == SEMICOLON);
			advance();
		}
		else if (token().tokenType == IDENTIFIER &&
				 peek(1).tokenType == ASSIGNMENT)
		{
			body.push_back(parseAssignment());
			assert(token().tokenType == SEMICOLON);
			advance();
		}
		else if (token().tokenType == IF)
		{
			body.push_back(parseIf(func));
		}
		else {
			throw_error_line(1, 
							 token().line, 
							 fmt::format("parseStatements failure to parse {}", 
							 token().tokenTypeToStr(token().tokenType)));
		}

		if (!isCompound)
			return body;
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
			advance();
			break;
		
		default:
			throw_error_line(1, 
							 tokenList.at(current).line, 
							 "parseType failure: Invalid or missing type");
			return nullptr;
	}

	while (tokenList.at(current).tokenType == ASTERISK)
	{
		type = new PointerType(type);
		advance();
	}

	return type;
}

static Node* parseDeclaration()
{
	Type* type = parseType();
	auto* node = new DeclarationNode(token().line, token().lexeme, type);

	if (peek().tokenType == ASSIGNMENT) {
		node->assignment = parseAssignment();
	}
	else if (peek().tokenType == SEMICOLON) {
		advance();
	}
	else {
		throw_error_line(1, 
						 token().line, 
						 fmt::format("Non-initializing declaration of variable {} was "
									 "not terminated with a semicolon", token().lexeme));
	}

	return node;
}

static AssignmentNode* parseAssignment()
{
	assert(peek(1).tokenType == ASSIGNMENT);

	auto* node = new AssignmentNode(token().line, token().lexeme);

	advance(2);
	node->expression = parseExpression();
	return node;	
}


UnaryOp TokenTypeToUnaryOp(TokenType op) {
	switch (op)
	{
		case MINUS:
			return UnaryOp::NEGATE;
		default:
			throw_error(1, fmt::format("TokenTypeToUnaryOp: no rule for token {}",
				Token::tokenTypeToStr(op)));
			exit(1);
	}
}

std::string unaryOpToStr(UnaryOp op)
{
	switch (op)
	{
		case UnaryOp::NEGATE:
			return "NEGATE";
		default:
			throw_error(1, fmt::format("Error in binaryOpToAsm: Missing op translation for {}",
				unaryOpToStr(op)));
			exit(1);
	}
}

static Node* parseIf(FuncDefNode* func)
{
	int line = token().line;
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	advance();
	Node* expr = parseExpression();
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();

	std::vector<Node*> statements;
	if (token().tokenType == OPEN_CURLY_BRACE)
	{
		// parse compound statement
		statements = parseStatements(true, func);
	}
	else {
		// parse one statement
		statements = parseStatements(false, func);
	}

	auto* ifNode = new IfNode(line, expr);
	ifNode->body = statements;
	return ifNode;
}
