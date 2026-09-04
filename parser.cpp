/*
	recursive descent parser
*/

#include "main.hpp"

extern std::vector<Token> tokenList;
static int current = 0;

static Token advance(int advanceBy = 1);
static Token retreat(int retreatBy = -1);
static Token peek(int peekBy = 1);
static Token token();

static Node *parseExpression();
static Node *parseTerm();
static Node *parseFactor();
static Node *parseFuncDef(GodNode* parent);
static std::vector<Parameter> parseParamList();
static std::vector<Node*> parseFuncBody(FuncDefNode *funcNode);
static Type *parseType();
static Node* parseDeclaration();
static AssignmentNode* parseAssignment();

GodNode *parser()
{
	fmt::print("parseTokens\n");

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
			throw_error_line(1, token().line, fmt::format("Parser failure: no rule for token {}", token().token_type_to_string(token().tokenType)));
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
	static int nested; // i.e. expr(expr)

	Node *root;

	while (token().tokenType != END_OF_FILE &&
		   token().tokenType != SEMICOLON &&
		   token().tokenType != CLOSED_CURLY_BRACE)
	{
		if (token().tokenType == PLUS || token().tokenType == MINUS || token().tokenType == ASTERISK)
		{
			BinaryOpNode *newRoot = new BinaryOpNode(token().line, token().tokenType);
			newRoot->left = root;
			advance();
			if (token().tokenType == OPEN_PARENTHESES) {
				advance();
				nested++;
				newRoot->right = parseExpression();
			}
			else {
				newRoot->right = parseTerm();
			}
			root = newRoot;
		}
		else if (token().tokenType == OPEN_PARENTHESES) {
			nested++;
			advance();
			if (nested > 1)
				root = parseExpression();
		}
		else if (token().tokenType == CLOSED_PARENTHESES) {
			nested--;
			advance();
		}
		else {
			root = parseTerm();
		}
	}
	// fmt::print("nested: {}\n", nested);
	if (token().tokenType == SEMICOLON) 
	{
		advance();
		if (nested != 0)
		{
			std::string type = nested > 0 ? "closing" : "opening";
			int count = nested < 0 ? nested * -1 : nested;
			throw_error_line(1, token().line, fmt::format("Missing {} {} parentheses", count, type));
		}
	}

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
		return new ImmediateNode(token().line, std::get<int>(token().literal));
	}
	else if (token().tokenType == IDENTIFIER)
	{
		return new VariableNode(token().line, token().lexeme);
	}
	else {
		throw_error_line(1, token().line, fmt::format("Invalid factor: '{}', type '{}'", token().lexeme, token().token_type_to_string(token().tokenType)));
		return NULL;
	}
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
		   token().tokenType != CLOSED_CURLY_BRACE)
	{
		if (token().tokenType == RETURN)
		{
			ReturnNode *retNode = new ReturnNode(token().line);
			advance();
			retNode->expression = parseExpression();
			body.push_back(retNode);
		}
		else if (token().tokenType == INT &&
				 peek(1).tokenType == IDENTIFIER)
		{
			body.push_back(parseDeclaration());
		}
		else if (token().tokenType == IDENTIFIER &&
				 peek(1).tokenType == ASSIGNMENT)
		{
			body.push_back(parseAssignment());
		}
		else {
			throw_error_line(1, token().line, fmt::format("parseFuncBody failure to parse {}", token().token_type_to_string(token().tokenType)));
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
			advance();
			break;
		
		default:
			throw_error_line(1, tokenList.at(current).line, "parseType failure: Invalid or missing type");
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
	else if (peek(1).tokenType == SEMICOLON) {
		advance(2);
	}
	else {
		throw_error_line(1, token().line, fmt::format("Non-initializing declaration of variable {} was not terminated with a semicolon", token().lexeme));
	}

	return node;
}

static AssignmentNode* parseAssignment()
{
	assert(peek(1).tokenType == ASSIGNMENT);

	auto* node = new AssignmentNode(token().line, token().lexeme);

	advance(2);
	node->expression = parseExpression();
	node->printChildren(1);
	return node;	
}