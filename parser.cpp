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
#include <cassert>

extern std::vector<Token> tokenList;
static int current = 0;

static void printIndentLines(int indent)
{
	std::string indentation{};
	std::string bar = "|";
	for (int i = 0; i < indent - 1; i++) {
		indentation.append(bar)
					.append("   ");
	}
	indentation.append(bar)
				.append("--");
	std::cout << indentation;
}

class Node {

	public:
		Node *parent; // TODO: this is never set. Is this a mistake?

		// TODO: the Program root node is the only one with children in this form.
		// Make a GodNode class and remove children from here. Confusing.
		std::vector<Node*> children;

		Node(Token *token) : token(token) {} // TODO: learn initializer list syntax properly and use it in other places

		virtual void print(int indent) {
			printIndentLines(indent);
			std::cout << typeName() << std::endl;
		}

		// runtime polymorphism
		virtual void printChildren(int indent) {
			print(indent);
			for (Node *i: children) {
				i->printChildren(indent + 1);
			}
		}

		virtual std::string typeName() {
			return "Node";
		}


		// TODO: these shouldn't exist for a lot of nodes. Reconsider storing the Token pointer as well.
		// TODO: just store line or smth. Only what's necessary for ALL nodes.
		TokenType getType() const {
			return this->token->type;
		}
		Literal getLiteral() const {
			return this->token->literal;
		}
		int getLine() const {
			return this->token->line;
		}
		std::string getLexeme() const {
			return this->token->lexeme;
		}
		
		private:
			Token *token;
};

class BinaryOpNode : public Node {
	public:
		Node *left;
		Node *right;
		TokenType op;

		BinaryOpNode(Token *token) : Node(token) {
			this->op = token->type;
		}

		void printChildren(int indent) override {
			print(indent);
			if (left) left->printChildren(indent + 1);
			if (right) right->printChildren(indent + 1);
		}

		std::string typeName() override {
			return "BinaryOpNode";
		}
};

class IntegerNode : public Node {
	public:
		IntegerNode(Token *token) : Node(token) {
		}

		std::string typeName() override {
			return "IntegerNode";
		}
};

class Parameter {
	public:
		TokenType type;
		std::string identifier;

		Parameter(TokenType type, std::string identifier) {
			this->type = type;
			this->identifier = identifier;
		}

		void print(int indent) {
			std::cout << Token::token_type_to_string(type) << " " << identifier << ", ";
		}
};

class FuncDefNode : public Node {
	public:
		std::string identifier;
		TokenType returnType;
		std::vector<Parameter> paramList;

		// must contain at least one return for each control path if returnType isn't void.
		// should only contain statements?
		std::vector<Node*> body;

		FuncDefNode(Token *identifierToken, TokenType returnType) : Node(identifierToken) {
			this->identifier = identifierToken->lexeme;
			this->returnType = returnType;
		}

		void print(int indent) override {
			printIndentLines(indent);
			std::cout << typeName() << " " << Token::token_type_to_string(returnType) << " " << identifier;
			std::cout << "(";
			for (Parameter i : paramList) {
				i.print(indent + 1);
			}
			std::cout << ")" << std::endl;

		}

		void printChildren(int indent) override {
			print(indent);
			for (Node *i : body) {
				i->printChildren(indent + 1);
			}

		}

		std::string typeName() override {
			return "FuncDefNode";
		}
};

// TODO: consider creating this. Perhaps it could make functions easier
// to reason about?
// class StatementNode : public Node {
// 	public:

// };

class ReturnNode : public Node {
	public:
		Node *expression; // can be almost anything, including nothing.

		ReturnNode(Token *token) : Node(token) {

		}

		std::string typeName() override {
			return "ReturnNode";
		}

		void print(int indent) override {
			printIndentLines(indent);
			std::cout << typeName() << " " << std::endl;
		}

		void printChildren(int indent) override {
			print(indent);
			expression->printChildren(indent + 1);
		}
};

// class IdentifierNode : Node {
// 	public:
// 		IdentifierNode()
// }

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

void parser()
{
	std::cout << "parseTokens" << std::endl;

	Token *temp = new Token();
	Node *program = new Node(temp);

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

	program->printChildren(1);
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
	
	if (root == NULL) {
		throw_error(2, "Missing term at start of expression");
	}

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
			throw_error(2, "Error in parseExpression: unexpected token");
		}
	}
	if (tokenList[current].type == SEMICOLON) advance();

	return root;
}

static Node *parseTerm()
{
	Node *root;

	root = parseFactor();
	if (root == NULL) {
		throw_error(2, "parseTerm: Missing factor");
	}
	advance();

	// (("*") factor)*
	while (tokenList[current].type == MULT && peek().type == INTEGER) {
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
	advance();
	FuncDefNode *funcNode = new FuncDefNode(&tokenList[current], tokenList[current - 1].type);
	advance();
	assert(tokenList[current].type == OPEN_PARENTHESES);
	funcNode->paramList = parseParamList();
	assert(tokenList.at(current).type == CLOSED_PARENTHESES);
	advance();

	// TODO: implement function declarations.
	if (tokenList.at(current).type != OPEN_CURLY_BRACE) {
		throw_error(2, "Function declarations are not implemented. Only function definitions.");
	}

	funcNode->body = parseFuncBody(funcNode);
	std::cout << funcNode->body[0]->typeName() << std::endl;

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
		Parameter param{token().type,
						peek(1).lexeme};
		
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
		   token().type != CLOSED_CURLY_BRACE) { // statements will consume their own closing braces
		if (token().type == RETURN)
		{
			ReturnNode *retNode = new ReturnNode(&tokenList[current]);
			std::cout << retNode->typeName() << std::endl;	
			advance();
			retNode->expression = parseExpression();
			body.push_back(retNode);
		}
	}
	return body;
}
