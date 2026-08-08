/*
	Semantic analysis phase to validate the structure of the program

	Just does basic type checking

	Returns a fully validated AST to the codegen stage

	Rules for return type int:
		Each term must be an int or be promotable to an int (char, short).

	walk the ast and for each return node
	check that its expression's type is the same as the function it's in
*/

#include "main.hpp"
#include <type_traits>

bool typesEqual(Type *first, Type *second);
Type* evalType(Node *node);

ProgramNode *sema(ProgramNode *ast)
{
	FuncDefNode *main = dynamic_cast<FuncDefNode*>(ast->children[0]);
	if (main == nullptr ||
		main->typeName() != "FuncDefNode" ||
		main->getLexeme() != "main"){
		throw_error(1, "Only the main function is currently supported.");
	}
	
	
	for (Node *node : main->body)
	{
		if (node->typeName() == "ReturnNode") {
			ReturnNode *retNode = static_cast<ReturnNode*>(node);
			
			Type* exprType = evalType(retNode->expression);
			
			if (!typesEqual(main->returnType, exprType))
				throw_error_line(1, node->getLine(), "Invalid return type");
			
			retNode->expression->type = exprType;
		}
	}
	
	fmt::print("AST validation complete\n");
	return ast;
}

// takes 2 Types and walks through them in lockstep. Types are linked lists of size 1 or greater.
bool typesEqual(Type *first, Type *second)
{
	if (dynamic_cast<IntType*>(first) && dynamic_cast<IntType*>(second)) {
		return true;
	}
	
	auto *p1 = dynamic_cast<PointerType*>(first);
	auto *p2 = dynamic_cast<PointerType*>(second);

	if (p1 && p2)
		return typesEqual(p1->pointee, p2->pointee);
	
	return false;
}

// Creating a evalType method in each Node class would get rid of all these casts
Type* getExpressionType(Node *node)
{
	Type* leftType;
	Type* rightType;

	leftType = evalType(node);
	rightType = evalType(node);

	if (typesEqual(leftType, rightType)) {
		return leftType;
	}
	else {
		throw_error_line(1, node->getLine(), "Operand type mismatch");
		exit(1);
	}
}

// bottom-up typechecking
Type* evalType(Node *node)
{
    Type* leftType;
    Type* rightType;

	if (node == nullptr) {
		return new VoidType();
	}

    if (auto *binOp = dynamic_cast<BinaryOpNode*>(node)) {
        leftType = evalType(binOp->left);
		rightType = evalType(binOp->right);
    }
	else if (node == nullptr) {
		return new VoidType();
	}
    else if (auto *intNode = dynamic_cast<IntegerNode*>(node)) {
        leftType = new IntType();
    }
    else {
        throw_error_line(1, node->getLine(), "Invalid Node type");
    }

    if (auto *intNode = dynamic_cast<IntegerNode*>(node)) {
        rightType = new IntType();
    }
    else {
        throw_error_line(1, node->getLine(), "Invalid Node type");
    }

	if (typesEqual(leftType, rightType)) {
		return leftType;
	}
	else {
		throw_error_line(1, node->getLine(), "Operand type mismatch");
		exit(1);
	}
}
