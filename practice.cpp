#include <iostream>
#include <vector>
#include <cassert>

class Node {

	public:
		Node *parent;

		int type;
		int literal;
		int line;

		Node(int type, int literal, int line) {
			this->type = type;
			this->literal = literal;
			this->line = line;
		}

		virtual void addParent(Node *newParent) {
			this->parent = newParent;
		}

		virtual void sayType() {
			std::cout << "Node" << std::endl;
		}


};

class BinaryOpNode : public Node {
	public:
		Node *left;
		Node *right;
		int op; // operator

		BinaryOpNode(int op, int literal, int line) : Node(op, literal, line) {
			this->op = op;
		}

		void sayType() override {
			std::cout << "BinaryOpNode" << std::endl;
		}
};

Node *function()
{
	BinaryOpNode *node = new BinaryOpNode(2, 2, 2);
	return node;
}

int main()
{
	Node *nodeA = new Node(1, 1, 1);

	BinaryOpNode *nodeB = new BinaryOpNode(2, 2, 2);

	nodeA->addParent(nodeB);
	std::cout << nodeA->line << std::endl;
	std::cout << nodeA->parent->line << std::endl;

	Node *another = function();

	another->sayType(); // i get it now

	// std::vector<int> arr;
	// arr.
	int i = 5;
	assert(i == 5);
}
