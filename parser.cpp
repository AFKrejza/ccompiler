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
		std::vector<Node*> children;

		Node(Token *token) {
			this->token = token;
		}

		void print(int indent) { // TODO: also print the type of the Node itself
			std::string indentation{};
			std::string bar = "|";
			for (int i = 0; i < indent - 1; i++) {
				indentation.append(bar)
						   .append("   ");
			}
			indentation.append(bar)
					   .append("--");
			std::cout << indentation;
			std::cout << "Type: " << Token::token_type_to_string(this->getType())
			<< " Lexeme: " << getLexeme() << std::endl;
		}

		// runtime polymorphism!
		virtual void printChildren(int indent) {
			print(indent);
			for (Node *i: children) {
				i->printChildren(indent + 1);
			}
		}

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
		TokenType op; // operator. prob not necessary

		BinaryOpNode(Token *token) : Node(token) {
			this->op = token->type;
		}

		void printChildren(int indent) override {
			print(indent);
			if (left) left->printChildren(indent + 1);
			if (right) right->printChildren(indent + 1);
		}
};

class IntegerNode : public Node {
	public:
		IntegerNode(Token *token) : Node(token) {
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
	root->printChildren(1);

	// }
}

// current points to the token that was returned to make
// it easier to track state across function calls
// (tokenizer did [current++])
static Token advance()
{
	return tokenList[++current];
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
