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
// static Token retreat(int retreatBy = 1);
static Token peek(int peekBy = 1);
static inline Token token();

static Node *parseExpression(ScopeNode* parent);
static Node* parseLogicalOr(ScopeNode* parent);
static Node* parseLogicalAnd(ScopeNode* parent);
static Node* parseEquality(ScopeNode* parent);
static Node* parseRelational(ScopeNode* parent);
static Node* parseAdditive(ScopeNode* parent);
static Node *parseTerm(ScopeNode* parent);
static Node* parseUnary(ScopeNode* parent);
static Node *parseFactor(ScopeNode* parent);
static Node *parseFunc(GodNode* parent);
static std::vector<Parameter> parseParamList();
static std::vector<Node*> parseStatements(bool isCompound, ScopeNode* parent);
static Type *parseType();
static Node* parseDeclaration(ScopeNode* parent);
static Node* parseAssignment(ScopeNode* parent);
static IfNode* parseIf(ScopeNode* parent);
static ElseNode* parseElse(ScopeNode* parent);
static WhileNode* parseWhile(ScopeNode* parent);
static DoWhileNode* parseDoWhile(ScopeNode* parent);
static ForNode* parseFor(ScopeNode* parent);
static CallNode* parseCall(ScopeNode* parent);

GodNode *parser()
{
	GodNode *program = new GodNode(0);

	while (token().tokenType != END_OF_FILE)
	{
		Node *node;
		if (token().tokenType == INT &&
			tokenList[current + 1].tokenType == IDENTIFIER &&
			tokenList[current + 2].tokenType == OPEN_PARENTHESES) {

			node = parseFunc(program);
			program->body.push_back(node);
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

// static Token retreat(int retreatBy)
// {
// 	current = current - retreatBy;
// 	return token();
// }

static Token peek(int peekBy)
{
	return tokenList.at(current + peekBy);
}

static Token token()
{ 
	return tokenList.at(current);
}

// starts at first token of expression
static Node *parseExpression(ScopeNode* parent)
{
	Node* root = nullptr;

	if (token().tokenType != END_OF_FILE &&
		token().tokenType != SEMICOLON)
	{
		root = parseAssignment(parent);
	}
	return root;
}

static Node* parseAssignment(ScopeNode* parent)
{
	Node* root = parseLogicalOr(parent);

	while (token().tokenType == ASSIGNMENT)
	{
		advance();
		auto* node = new AssignmentNode(root->line);
		node->lhs = root;

		if (auto* l = dynamic_cast<VariableNode*>(node->lhs)) {
			node->name = l->name;
		} else {
			throw_error_line(1, node->lhs->line,
							fmt::format("{} is not a valid lvalue", node->lhs->typeName()));
		}
		node->expression = parseAssignment(parent);
		root = node;
	}
	return root;
}

static Node* parseLogicalOr(ScopeNode* parent)
{
	Node* root = parseLogicalAnd(parent);

	while (token().tokenType == LOGICAL_OR)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		node->left = root;
		advance();
		node->right = parseLogicalAnd(parent);
		root = node;
	}
	return root;
}

static Node* parseLogicalAnd(ScopeNode* parent)
{
	Node* root = parseEquality(parent);

	while (token().tokenType == LOGICAL_AND)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		node->left = root;
		advance();
		node->right = parseEquality(parent);
		root = node;
	}
	return root;
}

static Node* parseEquality(ScopeNode* parent)
{
	Node* root = parseRelational(parent);

	while (token().tokenType == EQUAL_TO ||
		token().tokenType == NOT_EQUAL)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		advance();
		node->left = root;
		node->right = parseRelational(parent);
		root = node;
	}
	return root;
}

static Node* parseRelational(ScopeNode* parent)
{
	Node* root = parseAdditive(parent);

	while (token().tokenType == LESS_THAN ||
		   token().tokenType == LESSER_OR_EQUAL ||
		   token().tokenType == GREATER_THAN ||
		   token().tokenType == GREATER_OR_EQUAL)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		advance();
		node->left = root;
		node->right = parseAdditive(parent);
		root = node;
	}
	return root;
}

static Node* parseAdditive(ScopeNode* parent)
{
	Node* root = parseTerm(parent);

	while (token().tokenType == PLUS ||
		   token().tokenType == MINUS)
	{
		auto* node = new BinaryOpNode(token().line, token().tokenType);
		advance();
		node->left = root;
		node->right = parseTerm(parent);
		root = node;
	}
	return root;
}

static Node *parseTerm(ScopeNode* parent)
{
	Node *root;

	root = parseUnary(parent);

	while (token().tokenType == ASTERISK)
	{
		BinaryOpNode *newRoot = new BinaryOpNode(token().line, token().tokenType);
		advance();

		Node *newFactor = parseUnary(parent);

		newRoot->left = root;
		newRoot->right = newFactor;
		root = newRoot;
	}
	return root;
}

static Node* parseUnary(ScopeNode* parent)
{
	Node* root;
	UnaryOpNode *unop;

	if (token().tokenType != MINUS &&
		token().tokenType != LOGICAL_NOT &&
		token().tokenType != INC &&
		token().tokenType != DEC)
	{
		root = parseFactor(parent);
	}
	else {
		TokenType type;
		switch (token().tokenType)
		{
			case MINUS:
				type = MINUS;
				break;
			case LOGICAL_NOT:
				type = LOGICAL_NOT;
				break;
			case INC:
				type = PREINC;
				break;
			case DEC:
				type = PREDEC;
				break;
			default:
				throw_error_line(1, token().line, "Missed a rule for a unary op");
		}
			unop = new UnaryOpNode(token().line, type, NULL);
			advance();
			unop->expression = parseUnary(parent);
			root = unop;
	}
	return root;
}

static Node *parseFactor(ScopeNode* parent)
{
	Node* factor;

	if (token().tokenType == INTEGER)
	{
		factor = new ImmediateNode(token().line, std::get<int>(token().literal));
		advance();
		return factor;
	}
	else if (token().tokenType == IDENTIFIER)
	{
		if (peek().tokenType == OPEN_PARENTHESES) {
			factor = parseCall(parent);
		}
		else {
			factor = new VariableNode(token().line, token().lexeme);
			advance();
			if (token().tokenType == INC)
			{
				auto* un = new UnaryOpNode(token().line, POSTINC, factor);
				advance();
				factor = un;
			}
			else if (token().tokenType == DEC)
			{
				auto* un = new UnaryOpNode(token().line, POSTDEC, factor);
				advance();
				factor = un;
			}
		}
		return factor;
	}
	else if (token().tokenType == OPEN_PARENTHESES)
	{
		advance();
		factor = parseExpression(parent);
		assert(token().tokenType == CLOSED_PARENTHESES);
		advance();
		return factor;
	}
	else {
		throw_error_line(1, 
						 token().line, 
						 fmt::format("Invalid factor: '{}', type '{}'", 
							token().lexeme, 
							token().tokenTypeToStr(token().tokenType)));
		exit(1);
	}
}

static Node *parseFunc(GodNode* parent)
{
	Type *returnType = parseType();
	int line = token().line;
	std::string lexeme = token().lexeme;
	
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	std::vector<Parameter> paramList = parseParamList();

	FuncNode* func;

	if (token().tokenType == SEMICOLON)
	{
		auto* node = new FuncNode(line, parent, lexeme, returnType, paramList, false);
		advance();
		func = node;
	}
	else {
		auto* node = new FuncNode(line, parent, lexeme, returnType, paramList, true);
		node->body = parseStatements(true, node);
		func = node;
	}
	return func;
}

static std::vector<Parameter> parseParamList()
{
	std::vector<Parameter> paramList;

	advance();
	if (token().tokenType == CLOSED_PARENTHESES) {
		advance();
		return paramList;
	}

	while (token().tokenType == INT && peek(1).tokenType == IDENTIFIER)
	{
		Type *paramType = parseType();
		Parameter param{token().line, token().lexeme, paramType};
		
		paramList.push_back(param);
		
		if (peek(2).tokenType == COMMA)
			advance(3);
		else if (peek(2).tokenType == CLOSED_PARENTHESES)
			advance(2);
		else advance(2);
	}

	return paramList;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
static std::vector<Node*> parseStatements(bool isCompound, ScopeNode* parent)
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

		switch (token().tokenType)
		{
			ReturnNode* retNode;
			case RETURN:
				retNode = new ReturnNode(token().line, parent);
				advance();
				retNode->expression = parseExpression(parent);
				assert(token().tokenType == SEMICOLON);
				advance();
				body.push_back(retNode);
				break;
			case IF:
				body.push_back(parseIf(parent));
				break;
			case ELSE:
				throw_error_line(1, token().line, "Else ain't got no preceding if");
				break;
			case WHILE:
				body.push_back(parseWhile(parent));
				break;
			case BREAK:
				body.push_back(new BreakNode(token().line, parent));
				assert(peek().tokenType == SEMICOLON);
				advance(2);
				break;
			case CONTINUE:
				body.push_back(new ContinueNode(token().line, parent));
				assert(peek().tokenType == SEMICOLON);
				advance(2);
				break;
			case DO:
				body.push_back(parseDoWhile(parent));
				break;
			case FOR:
				body.push_back(parseFor(parent));
				break;
			case POSTINC:
			case POSTDEC:
				parseExpression(parent);
				break;
			default:
				if (token().tokenType == INT &&
					peek(1).tokenType == IDENTIFIER)
				{
					body.push_back(parseDeclaration(parent));
					assert(token().tokenType == SEMICOLON);
					advance();
				}
				else {
					body.push_back(parseExpression(parent));
					advance();
				}
		}

		if (!isCompound)
			return body;
	}
	return body;
}
#pragma GCC diagnostic pop

static Type *parseType()
{
	Type *type = nullptr;
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

static Node* parseDeclaration(ScopeNode* parent)
{
	Type* type = parseType();
	auto* node = new DeclarationNode(token().line, token().lexeme, type);

	if (peek().tokenType == ASSIGNMENT) {
		advance(2);
		node->expression = parseExpression(parent);
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

UnaryOp TokenTypeToUnaryOp(TokenType type) {
	UnaryOp op;
	switch (type)
	{
		case MINUS:
			op = UnaryOp::NEGATE;
			break;
		case LOGICAL_NOT:
			op = UnaryOp::LOGICAL_NOT;
			break;
		case PREINC:
			op = UnaryOp::PREINC;
			break;
		case POSTINC:
			op = UnaryOp::POSTINC;
			break;
		case PREDEC:
			op = UnaryOp::PREDEC;
			break;
		case POSTDEC:
			op = UnaryOp::POSTDEC;
			break;
		default:
			throw_error(1, fmt::format("TokenTypeToUnaryOp: no rule for token {}",
				Token::tokenTypeToStr(type)));
			exit(1);
	}
	return op;
}

std::string unaryOpToStr(UnaryOp op)
{
	switch (op)
	{
		case UnaryOp::NEGATE:
			return "NEGATE";
		case UnaryOp::LOGICAL_NOT:
			return "LOGICAL_NOT";
		case UnaryOp::PREINC:
			return "PREINC";
		case UnaryOp::POSTINC:
			return "POSTINC";
		case UnaryOp::PREDEC:
			return "PREDEC";
		case UnaryOp::POSTDEC:
			return "POSTDEC";
		default:
			throw_error(1, fmt::format("Error in binaryOpToAsm: Missing op translation for {}",
				unaryOpToStr(op)));
			exit(1);
	}
}

static std::vector<Node*> parseConditionalStatements(ScopeNode* parent)
{
	std::vector<Node*> statements;
	if (token().tokenType == OPEN_CURLY_BRACE)
	{
		// parse compound statement
		statements = parseStatements(true, parent);
	}
	else {
		// parse one statement
		statements = parseStatements(false, parent);
	}
	return statements;
}

static IfNode* parseIf(ScopeNode* parent)
{
	int line = token().line;
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	advance();
	Node* expr = parseExpression(parent);
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();

	auto* ifNode = new IfNode(line, parent, expr);
	ifNode->body = parseConditionalStatements(ifNode);

	if (token().tokenType == ELSE)
	{
		ifNode->elseBranch = parseElse(parent);
	}

	return ifNode;
}

static ElseNode* parseElse(ScopeNode* parent)
{
	auto* elseNode = new ElseNode(token().line, parent);
	advance();
	elseNode->body = parseConditionalStatements(elseNode);

	return elseNode;
}

static WhileNode* parseWhile(ScopeNode* parent)
{
	int line = token().line;
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	advance();

	Node* expr;

	if (token().tokenType == CLOSED_PARENTHESES) {
		expr = new VoidNode(token().line);
	} else expr = parseExpression(parent);
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();

	auto* whileNode = new WhileNode(line, parent, expr);
	whileNode->body = parseConditionalStatements(whileNode);
	return whileNode;
}

static DoWhileNode* parseDoWhile(ScopeNode* parent)
{
	int line = token().line;
	advance();
	auto* loop = new DoWhileNode(line, parent);

	loop->body = parseConditionalStatements(loop);
	assert(token().tokenType == WHILE);
	assert(peek().tokenType == OPEN_PARENTHESES);
	advance(2);
	loop->expression = parseExpression(parent);
	assert(token().tokenType == CLOSED_PARENTHESES);
	assert(peek().tokenType == SEMICOLON);
	advance(2);

	return loop;	
}

static Node* parseForPrologue(ScopeNode* parent)
{
	Node* ret;
	if (token().tokenType == INT)
	{
		ret = parseDeclaration(parent);
		assert(token().tokenType == SEMICOLON);
		advance();
	}
	else if (token().tokenType == IDENTIFIER &&
			 peek().tokenType == ASSIGNMENT)
	{
		ret = parseAssignment(parent);
		assert(token().tokenType == SEMICOLON);
		advance();
	}
	else if (token().tokenType == SEMICOLON)
	{
		ret = new VoidNode(token().line);
		advance();
	}
	else {
		ret = parseExpression(parent);
	}
	return ret;
}

static ForNode* parseFor(ScopeNode* parent)
{
	/*
		for (;;)
		{
			return 1;
		}

		from first token after (

		if token is ; prologue = voidnode; advance();
		else parseExpression

		second one parseExpression

		third one: if token is ')', voidnode
	*/

	int line = token().line;
	assert(peek().tokenType == OPEN_PARENTHESES);
	advance(2);

	Node* prologue = parseForPrologue(parent);

	Node* expression;
	if (token().tokenType == SEMICOLON) // second ;
	{
		expression = new VoidNode(token().line);
		advance();
	} else {
		expression = parseExpression(parent);
		assert(token().tokenType == SEMICOLON);
		advance();
	}
		
	Node* epilogue;
	if (token().tokenType == CLOSED_PARENTHESES) {
		epilogue = new VoidNode(token().line);
	} else {
		if (peek().tokenType == ASSIGNMENT) {
			epilogue = parseAssignment(parent);
		}
		else epilogue = parseExpression(parent);
	}

	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();
	auto* loop = new ForNode(line, parent, expression, prologue, epilogue);
	loop->body = parseConditionalStatements(loop);
	return loop;
}

static CallNode* parseCall(ScopeNode* parent)
{
	auto* node = new CallNode(token().line, parent, token().lexeme);
	advance(2);

	while (token().tokenType != CLOSED_PARENTHESES)
	{
		Node* arg = parseExpression(parent);
		node->args.push_back(arg);
		if (token().tokenType == COMMA) advance();
	}
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();
	return node;
}
