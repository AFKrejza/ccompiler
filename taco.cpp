/*
	Intermediate representation using three-address code (TAC)

*/

#include "main.hpp"

static void emit(Instruction* instr);
Operand genExpr(Node *node);
void genReturn(ReturnNode *node);
static std::string operandToStr(Operand operand);
static int newVreg();

static int current = 0; // temp register

static std::vector<Instruction*> ir;

std::vector<Instruction*> taco(ProgramNode *program)
{
	fmt::print("taco\n");

	auto* main = dynamic_cast<FuncDefNode*>(program->children[0]);
	assert(main->name == "main");

	auto* ret = dynamic_cast<ReturnNode*>(main->body[0]);
	assert(ret != nullptr);
	
	genReturn(ret);

	return ir;

}

static void emit(Instruction* instr)
{
	ir.push_back(instr);
}

Operand genExpr(Node *node)
{
	Operand vreg;

	if (auto* n = dynamic_cast<IntegerNode*>(node)) {
		vreg = Operand::Immediate(n->value);
	}
	else if (auto* binOp = dynamic_cast<BinaryOpNode*>(node)) {
		vreg = Operand::Temp(newVreg());
		auto* binInstr = new BinaryInstr(vreg,
										 binOp->op,
										 genExpr(binOp->left),
										 genExpr(binOp->right));

		emit(binInstr);
	}
	return vreg;
}

void genReturn(ReturnNode *node)
{
	Operand operand = genExpr(node->expression);
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