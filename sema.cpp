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

static void evalDeclaration(DeclarationNode* node, FuncDefNode* func);
static Type* evalType(Node *node, FuncDefNode* func);
static bool typesEqual(Type *first, Type *second);
static void evalAssignment(AssignmentNode* node, FuncDefNode* func);

GodNode *sema(GodNode *program)
{
	fmt::print("sema\n");

	FuncDefNode *main = dynamic_cast<FuncDefNode*>(program->body[0]);
	if (main == nullptr ||
		main->typeName() != "FuncDefNode" ||
		main->name != "main"){
		throw_error(1, "Only the main function is currently supported.");
	}

	// add global vars

	main->parent = program;
	for (Node *node : main->body)
	{
		if (auto* retNode = dynamic_cast<ReturnNode*>(node))
		{
			Type* exprType = evalType(retNode->expression, main);
			
			if (!typesEqual(main->returnType, exprType))
				throw_error_line(1, node->line, "Invalid return type");
			
			retNode->expression->type = exprType;
		}
		else if (auto* declNode = dynamic_cast<DeclarationNode*>(node))
		{
			evalDeclaration(declNode, main);
		}
		else if (auto* asg = dynamic_cast<AssignmentNode*>(node))
		{
			// check that lvalues exist
			evalAssignment(asg, main);

		}
	}
	
	fmt::print("AST validation complete\n");
	return program;
}

// takes 2 Types and walks through them in lockstep. Types are linked lists of size 1 or greater.
bool typesEqual(Type *first, Type *second)
{
	// TODO: look into how C manages differently sized integers.

	// could just use switch typeName(), this seems kinda dumb
	if (dynamic_cast<IntType*>(first) && dynamic_cast<IntType*>(second)) {
		return true;
	}
	if (dynamic_cast<IntType*>(first) && dynamic_cast<ImmediateType*>(second)) {
		return true;
	}
	if (dynamic_cast<ImmediateType*>(first) && dynamic_cast<IntType*>(second)) {
		return true;
	}
	if (dynamic_cast<ImmediateType*>(first) && dynamic_cast<ImmediateType*>(second)) {
		return true;
	}
	
	
	auto *p1 = dynamic_cast<PointerType*>(first);
	auto *p2 = dynamic_cast<PointerType*>(second);

	if (p1 && p2)
		return typesEqual(p1->pointee, p2->pointee);
	
	return false;
}

// bottom-up typechecking
static Type* evalType(Node *node, FuncDefNode* func)
{
    if (auto *binOp = dynamic_cast<BinaryOpNode*>(node))
	{
        Type* leftType = evalType(binOp->left, func);
		Type* rightType = evalType(binOp->right, func);
		if (typesEqual(leftType, rightType)) {
			return leftType;
		}
		else {
			throw_error_line(1, node->line, "Operand type mismatch");
			exit(1);
		}
    }
    else if (auto *intNode = dynamic_cast<ImmediateNode*>(node))
	{
        return new ImmediateType();
    }
	else if (auto* var = dynamic_cast<VariableNode*>(node))
	{
		if (!func->findSymbolScope(var->name)) {
			throw_error_line(1, var->line, fmt::format("Use of uninitialized variable {}", var->name));
		}

		Attrs attrs = func->scope.at(var->name);
		return attrs.type;
	}
    else {
        throw_error_line(1, node->line, "Invalid Node type");
		exit(1);
    }
}

// add it to the local scope
static void evalDeclaration(DeclarationNode* node, FuncDefNode* func)
{
	if (node->assignment != nullptr) {
		node->assignment->type = evalType(node->assignment->expression, func);
		if (!typesEqual(node->type, node->assignment->type)) {
			throw_error_line(1, node->line, "evalDeclaration: Unequal types");
		}
	}

	// check if not already declared in this scope
	if (func->scope.count(node->name))
	{
		Attrs attrs = func->getSymbol(node->name);
		throw_error_line(1, node->line, fmt::format("'{}' was redeclared. First declared on line {}", node->name, attrs.line));
	}

	func->frameSize -= node->type->size;
	func->scope.insert({node->name, Attrs{node->type, func->frameSize, node->line}});
}

static void evalAssignment(AssignmentNode* node, FuncDefNode* func)
{
	// TODO: verify that the left side is actually an lvalue
	
	if (func->findSymbolScope(node->name) != 1) {
		throw_error_line(1, node->line, fmt::format("Variable '{}' isn't in scope", node->name));
	}

	Attrs var = func->getSymbol(node->name);
	node->expression->type = evalType(node->expression, func);

	fmt::print("node: {}\nexpression: {}\n", var.type->typeName(), node->expression->typeName());

	if (!typesEqual(var.type, node->expression->type)) {
		throw_error_line(1, node->line, "evalAssignment: Unequal types");
	}

}
