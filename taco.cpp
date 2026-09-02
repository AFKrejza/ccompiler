/*
	Intermediate representation using three-address code (TAC)

*/

#include "main.hpp"

static void emit(Instruction* instr);
static void genDeclaration(DeclarationNode* node, FuncDefNode* func);
static Operand genExpression(Node *node, FuncDefNode* func);
static void genReturn(ReturnNode *node, FuncDefNode* func);
static int newVreg();
static std::string operandToStr(Operand operand);
static void genAssignment(AssignmentNode* node, FuncDefNode* func);

static int current = 0; // temp register

static std::vector<Instruction*> ir;

std::vector<Instruction*> taco(GodNode *program)
{
	fmt::print("taco\n");

	auto* main = dynamic_cast<FuncDefNode*>(program->body[0]);
	assert(main->name == "main");

	for (Node* node : main->body) {
		if (auto* ret = dynamic_cast<ReturnNode*>(node)) {
			genReturn(ret, main);
		}
		else if (auto* decl = dynamic_cast<DeclarationNode*>(node)) {
			genDeclaration(decl, main);
		}
		else if (auto* assign = dynamic_cast<AssignmentNode*>(node)) {
			genAssignment(assign, main);
		}
		else {
			throw_error_line(1, node->line, fmt::format("Taco no rule for {}\n", node->typeName()));
		}
	}

	return ir;
}

static void emit(Instruction* instr)
{
	ir.push_back(instr);
}

static Operand genExpression(Node *node, FuncDefNode* func)
{
	Operand vreg;

	if (auto* n = dynamic_cast<ImmediateNode*>(node)) {
		vreg = Operand::Immediate(n->value);
	}
	else if (auto* binOp = dynamic_cast<BinaryOpNode*>(node)) {
		vreg = Operand::Temp(newVreg(), func->frameSize -= 4);
		auto* binInstr = new BinaryInstr(vreg,
										 binOp->op,
										 genExpression(binOp->left, func),
										 genExpression(binOp->right, func));

		emit(binInstr);
		return vreg;
	}
	else if (auto* var = dynamic_cast<VariableNode*>(node)) {
		return Operand::Variable(var->name, func->scope.at(var->name).offset);
	}
	else {
		throw_error_line(1, node->line, fmt::format("genExpression: no rule for {}", node->typeName()));
	}
	return vreg;
}

static void genReturn(ReturnNode *node, FuncDefNode* func)
{
	Operand operand = genExpression(node->expression, func);
	emit(new ReturnInstr(operand));
}

static int newVreg()
{
	return ++current;
}

BinaryOp TokenTypeToBinaryOp(TokenType op) {
	switch (op)
	{
		case PLUS:
			return BinaryOp::ADD;
		case MINUS:
			return BinaryOp::SUB;
		case ASTERISK:
			return BinaryOp::MUL;
		default:
			throw_error(1, "Invalid TokenType to BinaryOp conversion");
			exit(1);
	}
}

std::string binaryOpToStr(BinaryOp op) {
	switch (op) {
		case BinaryOp::ADD:
			return std::string{"+"};
		case BinaryOp::SUB:
			return std::string{"-"};
		case BinaryOp::MUL:
			return std::string{"*"};
		case BinaryOp::DIV:
			return std::string{"/"};
		default:
			throw_error(1, "Error in BinaryInstr->toStr: Invalid operator"); // TODO: use a C++ feature to do this automatically.
			exit(1);
	}
}

static void genDeclaration(DeclarationNode* node, FuncDefNode* func)
{
	if (node->assignment == nullptr) return;
	genAssignment(node->assignment, func);
}

static void genAssignment(AssignmentNode* node, FuncDefNode* func)
{
	Operand dest = Operand::Variable(node->name, func->getSymbol(node->name).offset);
	Operand src = genExpression(node->expression, func);
	emit(new AssignmentInstr(dest, src));
}
