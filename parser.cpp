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

static std::unique_ptr<Node> parseExpression(ScopeNode* parent);
static std::unique_ptr<Node> parseAssignment(ScopeNode* parent);
static std::unique_ptr<Node> parseLogicalOr(ScopeNode* parent);
static std::unique_ptr<Node> parseLogicalAnd(ScopeNode* parent);
static std::unique_ptr<Node> parseEquality(ScopeNode* parent);
static std::unique_ptr<Node> parseRelational(ScopeNode* parent);
static std::unique_ptr<Node> parseAdditive(ScopeNode* parent);
static std::unique_ptr<Node> parseTerm(ScopeNode* parent);
static std::unique_ptr<Node> parseUnary(ScopeNode* parent);
static std::unique_ptr<Node> parseFactor(ScopeNode* parent);
static std::unique_ptr<FuncNode> parseFunc(GodNode* parent);
static std::vector<Parameter> parseParamList();
static std::vector<std::unique_ptr<Node>> parseStatements(bool isCompound, ScopeNode* parent);
static std::unique_ptr<Type> parseType();
static std::unique_ptr<DeclarationNode> parseDeclaration(ScopeNode* parent);
static std::unique_ptr<IfNode> parseIf(ScopeNode* parent);
static std::unique_ptr<ElseNode> parseElse(ScopeNode* parent);
static std::unique_ptr<WhileNode> parseWhile(ScopeNode* parent);
static std::unique_ptr<DoWhileNode> parseDoWhile(ScopeNode* parent);
static std::unique_ptr<ForNode> parseFor(ScopeNode* parent);
static std::unique_ptr<CallNode> parseCall(ScopeNode* parent);

std::unique_ptr<GodNode> parser()
{
	std::unique_ptr<GodNode> program = std::make_unique<GodNode>();

	while (token().tokenType != END_OF_FILE)
	{
		std::unique_ptr<Node> node;

		if (token().tokenType == EXTERN) advance();

		if (token().tokenType == INT &&
			tokenList.at(current + 1).tokenType == IDENTIFIER &&
			tokenList.at(current + 2).tokenType == OPEN_PARENTHESES)
		{
			program->body.emplace_back(parseFunc(program.get()));
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
static std::unique_ptr<Node> parseExpression(ScopeNode* parent)
{
	std::unique_ptr<Node> root = nullptr;

	if (token().tokenType != END_OF_FILE &&
		token().tokenType != SEMICOLON)
	{
		root = parseAssignment(parent);
	}
	return root;
}

static std::unique_ptr<Node> parseAssignment(ScopeNode* parent)
{
	std::unique_ptr<Node> root = parseLogicalOr(parent);

	while (token().tokenType == ASSIGNMENT)
	{
		advance();
		auto node = std::make_unique<AssignmentNode>(root->line);
		node->lhs = std::move(root);

		if (auto* l = dynamic_cast<VariableNode*>(node->lhs.get())) {
			node->name = l->name;
		} else {
			throw_error_line(1, node->lhs->line,
							fmt::format("{} is not a valid lvalue", node->lhs->typeName()));
		}
		node->expression = parseAssignment(parent);
		root = std::move(node);
	}
	return root;
}

static std::unique_ptr<Node> parseLogicalOr(ScopeNode* parent)
{
	std::unique_ptr<Node> root = parseLogicalAnd(parent);

	while (token().tokenType == LOGICAL_OR)
	{
		auto node = std::make_unique<BinaryOpNode>(token().line, token().tokenType);
		node->left = std::move(root);
		advance();
		node->right = parseLogicalAnd(parent);
		root = std::move(node);
	}
	return root;
}

static std::unique_ptr<Node> parseLogicalAnd(ScopeNode* parent)
{
	std::unique_ptr<Node> root = parseEquality(parent);

	while (token().tokenType == LOGICAL_AND)
	{
		auto node = std::make_unique<BinaryOpNode>(token().line, token().tokenType);
		node->left = std::move(root);
		advance();
		node->right = parseEquality(parent);
		root = std::move(node);
	}
	return root;
}

static std::unique_ptr<Node> parseEquality(ScopeNode* parent)
{
	std::unique_ptr<Node> root = parseRelational(parent);

	while (token().tokenType == EQUAL_TO ||
		token().tokenType == NOT_EQUAL)
	{
		auto node = std::make_unique<BinaryOpNode>(token().line, token().tokenType);
		advance();
		node->left = std::move(root);
		node->right = parseRelational(parent);
		root = std::move(node);
	}
	return root;
}

static std::unique_ptr<Node> parseRelational(ScopeNode* parent)
{
	std::unique_ptr<Node> root = parseAdditive(parent);

	while (token().tokenType == LESS_THAN ||
		   token().tokenType == LESSER_OR_EQUAL ||
		   token().tokenType == GREATER_THAN ||
		   token().tokenType == GREATER_OR_EQUAL)
	{
		auto node = std::make_unique<BinaryOpNode>(token().line, token().tokenType);
		advance();
		node->left = std::move(root);
		node->right = parseAdditive(parent);
		root = std::move(node);
	}
	return root;
}

static std::unique_ptr<Node> parseAdditive(ScopeNode* parent)
{
	std::unique_ptr<Node> root = parseTerm(parent);

	while (token().tokenType == PLUS ||
		   token().tokenType == MINUS)
	{
		auto node = std::make_unique<BinaryOpNode>(token().line, token().tokenType);
		advance();
		node->left = std::move(root);
		node->right = parseTerm(parent);
		root = std::move(node);
	}
	return root;
}

static std::unique_ptr<Node> parseTerm(ScopeNode* parent)
{
	std::unique_ptr<Node> root;

	root = parseUnary(parent);

	while (token().tokenType == ASTERISK)
	{
		auto newRoot = std::make_unique<BinaryOpNode>(token().line, token().tokenType);
		advance();

		std::unique_ptr<Node> newFactor = parseUnary(parent);

		newRoot->left = std::move(root);
		newRoot->right = std::move(newFactor);
		root = std::move(newRoot);
	}
	return root;
}

static std::unique_ptr<Node> parseUnary(ScopeNode* parent)
{
	std::unique_ptr<Node> root;
	std::unique_ptr<UnaryOpNode> unop;

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
			int line = token().line;
			advance();
			std::unique_ptr<Node> expr = parseUnary(parent);
			root = std::make_unique<UnaryOpNode>(line, type, std::move(expr));
	}
	return root;
}

static std::unique_ptr<Node> parseFactor(ScopeNode* parent)
{
	std::unique_ptr<Node> factor;

	if (token().tokenType == INTEGER)
	{
		factor = std::make_unique<ImmediateNode>(token().line, std::get<int>(token().literal));
		advance();
		return factor;
	}
	else if (token().tokenType == IDENTIFIER)
	{
		if (peek().tokenType == OPEN_PARENTHESES) {
			factor = parseCall(parent);
		}
		else {
			factor = std::make_unique<VariableNode>(token().line, token().lexeme);
			advance();
			if (token().tokenType == INC)
			{
				factor = std::make_unique<UnaryOpNode>(token().line, POSTINC, std::move(factor));
				advance();
			}
			else if (token().tokenType == DEC)
			{
				factor = std::make_unique<UnaryOpNode>(token().line, POSTDEC, std::move(factor));
				advance();
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

// always begins parsing from the type
static std::unique_ptr<FuncNode> parseFunc(GodNode* parent)
{
	bool isExtern = false;

	if (current > 0 &&
		peek(-1).tokenType == EXTERN) {
		isExtern = true;
	}

	std::unique_ptr<Type> returnType = parseType();
	int line = token().line;
	std::string lexeme = token().lexeme;
	
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	std::vector<Parameter> paramList = parseParamList();

	std::unique_ptr<FuncNode> func;

	if (token().tokenType == SEMICOLON)
	{
		func = std::make_unique<FuncNode>(line, parent, lexeme, std::move(returnType),
								  std::move(paramList), false, isExtern);
		advance();
	}
	else {
		func = std::make_unique<FuncNode>(line, parent, lexeme, std::move(returnType),
								  std::move(paramList), true, isExtern);
		func->body = parseStatements(true, func.get());
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
		paramList.emplace_back(token().line, token().lexeme, parseType());
		
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
static std::vector<std::unique_ptr<Node>> parseStatements(bool isCompound, ScopeNode* parent)
{
	if (isCompound) {
		assert(token().tokenType == OPEN_CURLY_BRACE);
		advance();
	}

	std::vector<std::unique_ptr<Node>> body;

	while (token().tokenType != END_OF_FILE)
	{
		if (isCompound && token().tokenType == CLOSED_CURLY_BRACE) {
			advance();
			return body;
		}

		std::unique_ptr<ReturnNode> retNode;
		switch (token().tokenType)
		{
			case RETURN:
				retNode = std::make_unique<ReturnNode>(token().line, parent);
				advance();
				retNode->expression = parseExpression(parent);
				assert(token().tokenType == SEMICOLON);
				advance();
				body.emplace_back(std::move(retNode));
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
				body.push_back(std::make_unique<BreakNode>(token().line, parent));
				assert(peek().tokenType == SEMICOLON);
				advance(2);
				break;
			case CONTINUE:
				body.push_back(std::make_unique<ContinueNode>(token().line, parent));
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

static std::unique_ptr<Type> parseType()
{
	std::unique_ptr<Type> type = nullptr;
	switch (tokenList.at(current).tokenType)
	{
		case INT:
			type = std::make_unique<IntType>();
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
		type = std::make_unique<PointerType>(type);
		advance();
	}

	return type;
}

static std::unique_ptr<DeclarationNode> parseDeclaration(ScopeNode* parent)
{
	std::unique_ptr<Type> type = parseType();
	auto node = std::make_unique<DeclarationNode>(token().line, token().lexeme, std::move(type));

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

static std::vector<std::unique_ptr<Node>> parseConditionalStatements(ScopeNode* parent)
{
	std::vector<std::unique_ptr<Node>> statements;
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

static std::unique_ptr<IfNode> parseIf(ScopeNode* parent)
{
	int line = token().line;
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	advance();
	std::unique_ptr<Node> expr = parseExpression(parent);
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();

	auto ifNode = std::make_unique<IfNode>(line, parent, std::move(expr));
	ifNode->body = parseConditionalStatements(ifNode.get());

	if (token().tokenType == ELSE)
	{
		ifNode->elseBranch = parseElse(parent);
	}

	return ifNode;
}

static std::unique_ptr<ElseNode> parseElse(ScopeNode* parent)
{
	auto elseNode = std::make_unique<ElseNode>(token().line, parent);
	advance();
	elseNode->body = parseConditionalStatements(elseNode.get());

	return elseNode;
}

static std::unique_ptr<WhileNode> parseWhile(ScopeNode* parent)
{
	int line = token().line;
	advance();
	assert(token().tokenType == OPEN_PARENTHESES);
	advance();

	std::unique_ptr<Node> expr;

	if (token().tokenType == CLOSED_PARENTHESES) {
		expr = std::make_unique<VoidNode>(token().line);
	} else expr = parseExpression(parent);
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();

	auto whileNode = std::make_unique<WhileNode>(line, parent, std::move(expr));
	whileNode->body = parseConditionalStatements(whileNode.get());
	return whileNode;
}

static std::unique_ptr<DoWhileNode> parseDoWhile(ScopeNode* parent)
{
	int line = token().line;
	advance();
	auto loop = std::make_unique<DoWhileNode>(line, parent);

	loop->body = parseConditionalStatements(loop.get());
	assert(token().tokenType == WHILE);
	assert(peek().tokenType == OPEN_PARENTHESES);
	advance(2);
	loop->expression = parseExpression(parent);
	assert(token().tokenType == CLOSED_PARENTHESES);
	assert(peek().tokenType == SEMICOLON);
	advance(2);

	return loop;	
}

static std::unique_ptr<Node> parseForPrologue(ScopeNode* parent)
{
	std::unique_ptr<Node> ret;
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
		ret = std::make_unique<VoidNode>(token().line);
		advance();
	}
	else {
		ret = parseExpression(parent);
	}
	return ret;
}

static std::unique_ptr<ForNode> parseFor(ScopeNode* parent)
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

	std::unique_ptr<Node> prologue = parseForPrologue(parent);

	std::unique_ptr<Node> expression;
	if (token().tokenType == SEMICOLON) // second ;
	{
		expression = std::make_unique<VoidNode>(token().line);
		advance();
	} else {
		expression = parseExpression(parent);
		assert(token().tokenType == SEMICOLON);
		advance();
	}
		
	std::unique_ptr<Node> epilogue;
	if (token().tokenType == CLOSED_PARENTHESES) {
		epilogue = std::make_unique<VoidNode>(token().line);
	} else {
		if (peek().tokenType == ASSIGNMENT) {
			epilogue = parseAssignment(parent);
		}
		else epilogue = parseExpression(parent);
	}

	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();
	auto loop = std::make_unique<ForNode>(line, parent,
										  std::move(expression),
										  std::move(prologue),
										  std::move(epilogue));

	loop->body = parseConditionalStatements(loop.get());
	return loop;
}

static std::unique_ptr<CallNode> parseCall(ScopeNode* parent)
{
	auto node = std::make_unique<CallNode>(token().line, parent, token().lexeme);
	advance(2);

	while (token().tokenType != CLOSED_PARENTHESES)
	{
		std::unique_ptr<Node> arg = parseExpression(parent);
		node->args.emplace_back(std::move(arg));
		if (token().tokenType == COMMA) advance();
	}
	assert(token().tokenType == CLOSED_PARENTHESES);
	advance();
	return node;
}

std::unique_ptr<Type> Type::clone(const Type* input) {
	std::unique_ptr<Type> dest;

	if (input == nullptr) throw_error(1, "Attempt to clone nullptr");

	if (!input->isBase)
	{
		if (auto* p = dynamic_cast<const PointerType*>(input))
		{
			if (p->pointee == nullptr) {
				throw_error(1, "Attempt to clone pointer without a type");
			}
			std::unique_ptr<Type> pointee;
			pointee = Type::clone(p->pointee.get());
			dest = std::make_unique<PointerType>(pointee);
		}
		else {
			throw_error(1, fmt::format("Couldn't non-base type {}", input->typeName()));
		}
	}

	if (dynamic_cast<const ImmediateType*>(input)) {
		dest = std::make_unique<ImmediateType>();
	}
	else if (dynamic_cast<const IntType*>(input)) {
		dest = std::make_unique<IntType>();
	}
	else if (dynamic_cast<const FuncType*>(input)) {
		dest = std::make_unique<FuncType>();
	}
	else if (dynamic_cast<const VoidType*>(input)) {
		dest = std::make_unique<VoidType>();
	}
	else {
		throw_error(1, fmt::format("Couldn't base type {}", input->typeName()));
	}
	return dest;
}
