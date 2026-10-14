#include <iostream>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <memory>
#include <fmt/core.h>

/*
g++ practice.cpp -lfmt -fsanitize=address,undefined
*/

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

class Child {
	int n = 10;
	public:
		Child() {}

		~Child() {
			fmt::print("Called Child destructor\n");
		}

		void print() {
			fmt::print("Called Child print, {}\n", n);
		}
};

class Rib {
	public:
		int num;
		std::unique_ptr<Child> child;

		Rib(int n) : num(n) {}

		~Rib() {
			fmt::print("Called Rib destructor\n");
		}
};

int main()
{
	std::unique_ptr<Rib> p = std::make_unique<Rib>(1);
	std::unique_ptr<Child> c = std::make_unique<Child>();

	p->child = std::move(c);

	p->child->print();
	c->print(); // segfault cuz of move
}



	// Node *nodeA = new Node(1, 1, 1);
	// BinaryOpNode *nodeB = new BinaryOpNode(2, 2, 2);
	// nodeA->addParent(nodeB);
	// std::cout << nodeA->line << std::endl;
	// std::cout << nodeA->parent->line << std::endl;
	// Node *another = function();
	// another->sayType();
	// // std::vector<int> arr;
	// // arr.
	// int i = 5;
	// assert(i == 5);
	// std::unordered_map<int, int> vregMap;
	// vregMap.insert({1, 1});
	// int a = vregMap.at(1);
	// std::cout << a << "\n";
