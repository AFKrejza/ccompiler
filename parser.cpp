/*
	recursive descent parser

	expression -> term -> factor naming convention:
	expression for loose-binding + -
	term for tighter-binding like * /
	factor for things that have no operators or parentheses that explicitly group things.

	using EBNF (more or less) example:

	declaration	->	type IDENTIFIER "=" expression ";"
				|	type IDENTIFIER ";"
	expression	->	term (("+"|"-") term)*
	term 		->	factor (("*") factor)*
	factor		->	IDENTIFIER | INTEGER | "(" expression ")"
*/

#include "main.hpp"

extern std::vector<Token> tokenList;
static int current = 0;

class Node {

	public:
		Node *parent;
		TokenType type;
		Literal literal;
		int line;
		// uuuh maybe store a pointer to the original token?
		std::vector<Node*> children;

		Node(TokenType type, Literal literal, int line) {
			this->type = type;
			this->literal = literal;
			this->line = line;
		}

		void print() {
			std::cout << "   Type: " << Token::token_type_to_string(type) << std::endl;
			std::cout << "Literal: " << std::visit(LiteralVisitor{}, literal) << std::endl;
		}

		// runtime polymorphism!
		virtual void printChildren() {
			print();
			for (Node *i: children) {
				i->printChildren();
			}
		}
};

class BinaryOpNode : public Node {
	public:
		Node *left;
		Node *right;
		TokenType op; // operator

		BinaryOpNode(TokenType op, int line) : Node(op, literal, line) {
			this->op = op;
		}

		void printChildren() override {
			print();
			if (left) left->printChildren();
			if (right) right->printChildren();
		}
};

class IntegerNode : public Node {
	public:
		IntegerNode(Literal literal, int line) : Node(INTEGER, literal, line) {
		}
};

// class IdentifierNode : Node {
// 	public:
// 		IdentifierNode()
// }

static Token advance();
// static Node *parseDeclaration();
static Node *parseExpression();
static void addNode(TokenType type, Literal literal);
static Token peek();
static Node *parseTerm();
static Node *parseFactor();

void parser()
{
	std::cout << "parseTokens" << std::endl;

	// while (token.type != END_OF_FILE)
	// {
	Node *root = parseExpression();
	root->printChildren();

	// }
}

// current points to the token that was returned to make
// it easier to track state across function calls
// (tokenizer did [current++])
static Token advance()
{
	return tokenList[++current];
}

static void addNode(TokenType type, Literal literal)
{

}

static Token peek()
{
	return tokenList[current + 1];
}


// static Node *parseDeclaration()
// {
// 	// if (tokenList[current].type == INT)
// 	// {

// 	// }
// }



static Node *parseExpression()
{
	Node *root = NULL;
	
	Node *node = parseTerm();
	if (node == NULL) {
		throw_error(2, "Missing term at start of expression");
	}
	root = node;

	while (tokenList[current].type != END_OF_FILE && tokenList[current].type != SEMICOLON)
	{		
		if (tokenList[current].type == PLUS || tokenList[current].type == MINUS)
		{
			BinaryOpNode *newRoot = new BinaryOpNode(tokenList[current].type,
													 tokenList[current].line);
													 
			advance();
			newRoot->left = node;
			newRoot->right = parseTerm();
			root = newRoot;
		}
	}
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
		BinaryOpNode *newRoot = new BinaryOpNode(MULT, tokenList[current].line);
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
		IntegerNode *node = new IntegerNode(token.literal, token.line);
		return node;
	} else {
		return NULL;
	}
}
