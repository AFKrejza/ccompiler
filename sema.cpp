/*
	Semantic analysis phase to validate the structure of the program
	(types, variable scope, break/continue in a loop, )

	Returns a fully validated AST
*/

#include <algorithm>
#include <iterator>
#include <type_traits>

#include "ast.hpp"
#include "error.hpp"
#include "utils.hpp"

static void evalDeclaration(DeclarationNode* node, ScopeNode* parent);
static Type* evalType(Node *node, ScopeNode* parent);
static bool typesEqual(Type *first, Type *second);
static void evalAssignment(AssignmentNode* node, ScopeNode* parent);
static void evalReturn(ReturnNode* node, ScopeNode* parent);
static void evalIf(IfNode* node, ScopeNode* parent);
static void evalElse(StatementNode* node, ScopeNode* parent, int index);
static void evalStatements(StatementNode* block, ScopeNode* parent);
static void evalLoop(LoopNode* node);
static void evalBreak(BreakNode* node);
static void evalContinue(ContinueNode* node);
static bool evalExpression(Node* expr, ScopeNode* parent);

GodNode *sema(GodNode *program)
{
	FuncDefNode *main = dynamic_cast<FuncDefNode*>(program->body[0]);
	if (main == nullptr ||
		main->typeName() != "FuncDefNode" ||
		main->name != "main"){
		throw_error(1, "Only the main function is currently supported.");
	}

	// add global vars

	main->parent = program;

	for (Node *node : program->body)
	{
		if (auto* func = dynamic_cast<FuncDefNode*>(node))
		{
			evalStatements(func, func);
		}
	}

	fmt::print("Semantic analysis completed\n");
	return program;
}

static void evalStatements(StatementNode* block, ScopeNode* parent)
{
	for (size_t i = 0; i < block->body.size(); i++)
	{
		if (auto* retNode = dynamic_cast<ReturnNode*>(block->body[i]))
		{
			evalReturn(retNode, parent);
		}
		// TODO: declaration and assignment shouldn't really be here cuz they're
		// not statements.
		else if (auto* declNode = dynamic_cast<DeclarationNode*>(block->body[i]))
		{
			evalDeclaration(declNode, parent);
		}
		else if (auto* asg = dynamic_cast<AssignmentNode*>(block->body[i]))
		{
			// check that lvalues exist
			evalAssignment(asg, parent);
		}
		else if (auto* ifs = dynamic_cast<IfNode*>(block->body[i]))
		{
			evalIf(ifs, parent);
		}
		else if (auto* elses = dynamic_cast<ElseNode*>(block->body[i]))
		{
			evalElse(elses, parent, i);
		}
		else if (auto* whilel = dynamic_cast<WhileNode*>(block->body[i]))
		{
			evalLoop(whilel);
		}
		else if (auto* doer = dynamic_cast<DoWhileNode*>(block->body[i]))
		{
			evalLoop(doer);
		}
		else if (auto* forl = dynamic_cast<ForNode*>(block->body[i]))
		{
			evalLoop(forl);
		}
		else if (auto* breaker = dynamic_cast<BreakNode*>(block->body[i]))
		{
			evalBreak(breaker);
		}
		else if (auto* conch = dynamic_cast<ContinueNode*>(block->body[i]))
		{
			evalContinue(conch);
		}
		else {
			throw_error_line(1, 
							 block->body[i]->line, 
							 fmt::format("No rule for node type {}", block->body[i]->typeName()));
		}
	}
}

// takes 2 Types and walks through them in lockstep. 
// Types are linked lists of size 1 or greater.
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
static Type* evalType(Node *node, ScopeNode* parent)
{
    if (auto *binOp = dynamic_cast<BinaryOpNode*>(node))
	{
        Type* leftType = evalType(binOp->left, parent);
		Type* rightType = evalType(binOp->right, parent);
		if (typesEqual(leftType, rightType)) {
			return leftType;
		}
		else {
			throw_error_line(1, node->line, "Operand type mismatch");
			exit(1);
		}
    }
	else if (auto* unOp = dynamic_cast<UnaryOpNode*>(node))
	{
		return evalType(unOp->expression, parent);
	}
    else if (auto *intNode = dynamic_cast<ImmediateNode*>(node))
	{
		(void) intNode;
        return new ImmediateType();
    }
	else if (auto* var = dynamic_cast<VariableNode*>(node))
	{
		Attrs attrs = parent->getSymbol(var->name, var->line, parent);
		return attrs.type;
	}
    else {
        throw_error_line(1, node->line, "Invalid Node type");
		exit(1);
    }
}

// add it to the local scope
static void evalDeclaration(DeclarationNode* node, ScopeNode* parent)
{
	if (node->assignment != nullptr) {
		node->assignment->type = evalType(node->assignment->expression, parent);
		if (!typesEqual(node->type, node->assignment->type)) {
			throw_error_line(1, node->line, "evalDeclaration: Unequal types");
		}
	}

	// check if not already declared in this scope
	if (parent->scope.count(node->name))
	{
		Attrs attrs = parent->getSymbol(node->name, node->line, parent);
		throw_error_line(1, 
						 node->line, 
						 fmt::format("'{}' was redeclared. First declared on line {}", 
									 node->name, 
									 attrs.line));
	}

	int offset = parent->changeFrameSize(parent, node->type->size);
	parent->scope.insert({node->name, Attrs{node->type, offset, node->line}});
}

static void evalAssignment(AssignmentNode* node, ScopeNode* parent)
{
	// TODO: verify that the left side is actually an lvalue
	
	Attrs var = parent->getSymbol(node->name, node->line, parent);
	node->expression->type = evalType(node->expression, parent);

	if (!typesEqual(var.type, node->expression->type)) {
		throw_error_line(1, node->line, "evalAssignment: Unequal types");
	}

}

static void evalReturn(ReturnNode* node, ScopeNode* parent)
{
	evalExpression(node->expression, parent);
	FuncDefNode* func = node->findParentFunction(node->parent);
	if (func == nullptr)
		throw_error(1, "Return has no parent function");

	Type* exprType = evalType(node->expression, func);
	if (!typesEqual(func->returnType, exprType))
		throw_error_line(1, node->line, "Invalid return type");
	
	node->expression->type = exprType;
}

static void evalIf(IfNode* node, ScopeNode* parent)
{
	evalExpression(node->expression, node);
	evalStatements(node, parent);
}

static void evalElse(StatementNode* node, ScopeNode* parent, int index)
{
	if (index == 0 || !dynamic_cast<IfNode*>(parent->body.at(index -1)))
	{
		throw_error_line(1, node->line, "Missing if statement before else");
	}
	evalStatements(node, parent);
}

static void evalLoop(LoopNode* node)
{
	if (auto* whilel = dynamic_cast<ForNode*>(node)) {
		if (!dynamic_cast<VoidNode*>(whilel->prologue)) {
			evalExpression(whilel->prologue, whilel);
		}
		if (!dynamic_cast<VoidNode*>(whilel->epilogue)) {
			evalExpression(whilel->epilogue, whilel);
		}
	}
	if (!dynamic_cast<VoidNode*>(node->expression))
		evalExpression(node->expression, node);

	evalStatements(node, node);
}

static void evalBreak(BreakNode* node)
{
	// if no ancestor is a loop throw error
	if (node->findParentLoop(node->parent) == nullptr) {
		throw_error_line(1, node->line, "Break can only be used in loops");
	}
}

// again copied
static void evalContinue(ContinueNode* node)
{
	if (node->findParentLoop(node->parent) == nullptr) {
		throw_error_line(1, node->line, "Continue can only be used in loops");
	}
}

// validate that all variables in an expression are in scope
// parent = parent of the expression
static bool evalExpression(Node* expr, ScopeNode* parent)
{
	Operand vreg;

	if (dynamic_cast<ImmediateNode*>(expr)) {
		return true;
	}
	else if (auto* binOp = dynamic_cast<BinaryOpNode*>(expr))
	{
		evalExpression(binOp->left, parent);
		evalExpression(binOp->right, parent);
	}
	else if (auto* unOp = dynamic_cast<UnaryOpNode*>(expr))
	{
		evalExpression(unOp->expression, parent);
	}
	else if (auto* var = dynamic_cast<VariableNode*>(expr))
	{
		parent->getSymbol(var->name, var->line, parent);
	}
	else if (auto* declNode = dynamic_cast<DeclarationNode*>(expr))
	{
		evalDeclaration(declNode, parent);
	}
	else if (auto* asg = dynamic_cast<AssignmentNode*>(expr))
	{
		// check that lvalues exist
		evalAssignment(asg, parent);
	}
	else {
		throw_error_line(1, 
						 expr->line, 
						 fmt::format("genExpression: no rule for {}", expr->typeName()));
	}
	return true;
}
