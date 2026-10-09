/*
	Semantic analysis phase to validate the structure of the program
	(types, variable scope, break/continue in a loop, )

	Returns a fully validated AST
*/

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <type_traits>

#include "ast.hpp"
#include "error.hpp"
#include "utils.hpp"

static Type* evalType(Node *node, ScopeNode* parent);
static bool typesEqual(Type *first, Type *second);
static void evalStatements(StatementNode* block, ScopeNode* parent);
static void evalLoop(LoopNode* node);
static bool evalExpression(Node* expr, ScopeNode* parent);
static void evalFunction(FuncDefNode* func);

GodNode* program;

GodNode *sema(GodNode *prog)
{
	program = prog;

	// add global vars

	FuncDefNode* main = nullptr;

	for (Node *node : program->body)
	{
		if (auto* func = dynamic_cast<FuncDefNode*>(node))
		{
			if (func->name == "main")
			{
				if (main) throw_error_line(1, func->line, 
					fmt::format("Function 'main' was redefined. Original definition on"
								"line {}", main->line));
				else main = func;
			}

			auto it = program->scope.find(func->name);
			if (it != program->scope.end())
				throw_error_line(1, func->line,
						fmt::format("Redefinition of function {}, "
									"first defined on line {}", it->first, it->second.line));

			func->parent = program;
			evalFunction(func);
			program->scope.insert({func->name, Attrs{new FuncDefType{}, 0, func->line, func }});
		}
	}
	if (main == nullptr)
		throw_error(1, "Program is missing the main function");

	fmt::print("Semantic analysis completed\n");
	return program;
}

static void evalStatements(StatementNode* block, ScopeNode* parent)
{
	for (size_t i = 0; i < block->body.size(); i++)
	{
		block->body[i]->eval(parent);
	}
}

// takes 2 Types and walks through them in lockstep. 
// Types are linked lists of size 1 or greater.
bool typesEqual(Type *first, Type *second)
{
	// TODO: look into how C manages differently sized integers.

	// TODO: perhaps add a equalTo map or switch to each Type?
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
	if (dynamic_cast<FuncDefType*>(first) && dynamic_cast<FuncDefType*>(second)) {
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
	else if (auto* var = dynamic_cast<AssignmentNode*>(node))
	{
		Attrs attrs = parent->getSymbol(var->name, var->line, parent);
		return attrs.type;
	}
	else if (auto* func = dynamic_cast<CallNode*>(node))
	{
		Attrs attrs = program->getSymbol(func->name, func->line, program);
		return attrs.func->returnType;
	}
    else {
        throw_error_line(1, node->line, "Invalid Node type");
		exit(1);
    }
}

// add it to the local scope
void DeclarationNode::eval(ScopeNode* parent)
{
	if (this->expression != nullptr) {
		this->expression->type = evalType(this->expression, parent);
		if (!typesEqual(this->type, this->expression->type)) {
			throw_error_line(1, this->line, "evalDeclaration: Unequal types");
		}
	}

	// check if not already declared in this scope
	if (parent->scope.count(this->name))
	{
		Attrs attrs = parent->getSymbol(this->name, this->line, parent);
		throw_error_line(1, 
						 this->line, 
						 fmt::format("'{}' was redeclared. First declared on line {}", 
									 this->name, 
									 attrs.line));
	}

	int offset = parent->changeFrameSize(this->type->size);
	parent->scope.insert({this->name, Attrs{this->type, offset, this->line}});
}

void AssignmentNode::eval(ScopeNode* parent)
{
	Attrs var = parent->getSymbol(this->name, this->line, parent);
	this->expression->type = evalType(this->expression, parent);

	if (!typesEqual(var.type, this->expression->type)) {
		throw_error_line(1, this->line, "evalAssignment: Unequal types");
	}
}

void ReturnNode::eval(ScopeNode* parent)
{
	evalExpression(this->expression, parent);
	FuncDefNode* func = this->findParentFunction(this->parent);
	if (func == nullptr)
		throw_error(1, "Return has no parent function");

	Type* exprType = evalType(this->expression, func);
	if (!typesEqual(func->returnType, exprType))
		throw_error_line(1, this->line, "Invalid return type");
	
	this->expression->type = exprType;
}

void IfNode::eval(ScopeNode* parent)
{
	evalExpression(this->expression, this);
	evalStatements(this, parent);
}

void ElseNode::eval(ScopeNode* parent)
{
	if (parent->body.size() == 0 ||
		!dynamic_cast<IfNode*>(parent->body.at(parent->body.size() -2)))
	{
		throw_error_line(1, this->line, "Missing if statement before else");
	}
	evalStatements(this, parent);
}

void WhileNode::eval(ScopeNode* parent)
{
	(void)parent;
	evalLoop(this);
}

void DoWhileNode::eval(ScopeNode* parent)
{
	(void)parent;
	evalLoop(this);
}

void ForNode::eval(ScopeNode* parent)
{
	(void)parent;
	if (!dynamic_cast<VoidNode*>(this->prologue))
	{
		evalExpression(this->prologue, this);
	}
	if (!dynamic_cast<VoidNode*>(this->epilogue))
	{
		evalExpression(this->epilogue, this);
	}
	evalLoop(this);
}

void evalLoop(LoopNode* node)
{
	if (!dynamic_cast<VoidNode*>(node->expression))
		evalExpression(node->expression, node);

	evalStatements(node, node);
}

void BreakNode::eval(ScopeNode* parent)
{
	(void)parent;
	// if no ancestor is a loop throw error
	if (this->findParentLoop(this->parent) == nullptr) {
		throw_error_line(1, this->line, "Break can only be used in loops");
	}
}

// copied, not worth making another class imo
void ContinueNode::eval(ScopeNode* parent)
{
	(void)parent;
	if (this->findParentLoop(this->parent) == nullptr) {
		throw_error_line(1, this->line, "Continue can only be used in loops");
	}
}

// validate that all variables in an expression are in scope
// parent = parent of the expression
static bool evalExpression(Node* expr, ScopeNode* parent)
{
	if (dynamic_cast<ImmediateNode*>(expr)) {
		return true;
	}
	else expr->eval(parent);
	return true;
}

void VariableNode::eval(ScopeNode* parent)
{
	parent->getSymbol(this->name, this->line, parent);
}

void BinaryOpNode::eval(ScopeNode* parent)
{
	evalExpression(this->left, parent);
	evalExpression(this->right, parent);
}

void UnaryOpNode::eval(ScopeNode* parent)
{
	if (this->op == UnaryOp::PREINC ||
		this->op == UnaryOp::POSTINC ||
		this->op == UnaryOp::PREDEC ||
		this->op == UnaryOp::POSTDEC)
	{
		auto* var = dynamic_cast<VariableNode*>(this->expression);
		if (!var) {
			throw_error_line(1, this->line,
							 fmt::format("Target of {} operator is not a Variable "
										 "(other lvalues are not supported)",
										 unaryOpToStr(this->op)));
		}
	}
	evalExpression(this->expression, parent);
}

static void evalFunction(FuncDefNode* func)
{
	for (Parameter param : func->paramList)
	{
		int offset = func->changeFrameSize(param.type->size);
		func->scope.insert({param.name, Attrs{param.type, offset, param.line}});
	}
	// TODO: unused variable & parameter check

	evalStatements(func, func);
}

void CallNode::eval(ScopeNode* parent)
{
	Attrs attrs = program->getSymbol(this->name, this->line, program);

	auto* funcType = new FuncDefType{};
	if (!typesEqual(funcType, attrs.type))
	{
		throw_error_line(1,
						 this->line,
						 fmt::format("Call to symbol {} which is not a {} but a {}",
						 this->name, funcType->typeName(), attrs.type->typeName()));
	}

	FuncDefNode* func = attrs.func;

	if (this->args.size() != func->paramList.size())
	{
		throw_error_line(1,
			this->line,
			fmt::format("Invalid number of arguments: got {}, expected {}",
				this->args.size(), func->paramList.size()));
	}
	
	for (size_t i = 0; i < this->args.size(); ++i)
	{
		this->args[i]->type = evalType(this->args[i], parent);
		if (!typesEqual(this->args[i]->type, func->paramList[i].type))
		{
			throw_error_line(1, this->args[i]->line,
				fmt::format("Mismatched argument types: got {}, expected {}",
					this->args[i]->type->typeName(), func->paramList[i].type->typeName()));
		}

		evalExpression(this->args[i], parent);
	}
}

void ImmediateNode::eval(ScopeNode* parent)
{
	evalExpression(this, parent);
}
