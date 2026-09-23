/*
	Intermediate representation using three-address code (TAC)

*/

#include <cassert>
#include <vector>

#include "ast.hpp"
#include "taco.hpp"
#include "utils.hpp"

static void emit(Instruction* instr);
static void genDeclaration(DeclarationNode* node, FuncDefNode* func);
static Operand genExpression(Node *node, FuncDefNode* func);
static void genReturn(ReturnNode *node, FuncDefNode* func);
static int newVreg();
static void genAssignment(AssignmentNode* node, FuncDefNode* func);
static Label* newLocalLabel(std::string text);
static void genOr(BinaryOpNode* node, Operand dest, FuncDefNode* func);
static void genAnd(BinaryOpNode* node, Operand dest, FuncDefNode* func);
static void genStatements(std::vector<Node*> body, FuncDefNode* func);
static void genIf(IfNode* node, FuncDefNode* func);

static int current = 0; // temp register

static int labelCount = 0;

static std::vector<Instruction*> ir;

std::vector<Instruction*> taco(GodNode *program)
{
	auto* main = dynamic_cast<FuncDefNode*>(program->body[0]);
	assert(main->name == "main");

	for (Node* node : program->body)
	{
		if (auto* func = dynamic_cast<FuncDefNode*>(node))
		{
			genStatements(func->body, func);
			int pad = func->frameSize % 16;
			func->frameSize = func->frameSize + 16 - (pad ? pad : 16);
		}
	}

	fmt::print("IR generated\n");
	return ir;
}

static void genStatements(std::vector<Node*> body, FuncDefNode* func)
{
	for (int i = 0; i < body.size(); i++) {
		if (auto* ret = dynamic_cast<ReturnNode*>(body[i])) {
			genReturn(ret, func);
		}
		else if (auto* decl = dynamic_cast<DeclarationNode*>(body[i])) {
			genDeclaration(decl, func);
		}
		else if (auto* assign = dynamic_cast<AssignmentNode*>(body[i])) {
			genAssignment(assign, func);
		}
		else if (auto* ifs = dynamic_cast<IfNode*>(body[i])) {
			genIf(ifs, func);
		}
		else {
			throw_error_line(1, 
							 body[i]->line, 
							 fmt::format("Taco: no rule for {}", body[i]->typeName()));
		}
	}
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
		if (binOp->op == BinaryOp::LOGICAL_AND ||
			binOp->op == BinaryOp::LOGICAL_OR) {
			return vreg;
		}
		auto* binInstr = new BinaryInstr(vreg,
										 binOp->op,
										 genExpression(binOp->left, func),
										 genExpression(binOp->right, func));

		emit(binInstr);
		return vreg;
	}
	else if (auto* unOp = dynamic_cast<UnaryOpNode*>(node)) {
		vreg = Operand::Temp(newVreg(), func->frameSize -= 4);
		Operand src = genExpression(unOp->expression, func);
		auto* unInstr = new UnaryInstr(vreg, src, unOp->op);
		emit(unInstr);
		return vreg;
	}
	else if (auto* var = dynamic_cast<VariableNode*>(node)) {
		return Operand::Variable(var->name, func->scope.at(var->name).offset);
	}
	else {
		throw_error_line(1, 
						 node->line, 
						 fmt::format("genExpression: no rule for {}", node->typeName()));
	}
	return vreg;
}

static void genReturn(ReturnNode *node, FuncDefNode* func)
{
	Operand dest = genExpression(node->expression, func);

	if (auto* binOp = dynamic_cast<BinaryOpNode*>(node->expression))
	{
		switch (binOp->op)
		{
			case BinaryOp::LOGICAL_OR:
				genOr(binOp, dest, func);
				break;
			case BinaryOp::LOGICAL_AND:
				genAnd(binOp, dest, func);
				break;
		}
	}
	emit(new ReturnInstr(dest));
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
		case LOGICAL_AND:
			return BinaryOp::LOGICAL_AND;
		case LOGICAL_OR:
			return BinaryOp::LOGICAL_OR;
		case EQUAL_TO:
			return BinaryOp::EQUAL_TO;
		case NOT_EQUAL:
			return BinaryOp::NOT_EQUAL;
		case LESS_THAN:
			return BinaryOp::LESS_THAN;
		case LESSER_OR_EQUAL:
			return BinaryOp::LESSER_OR_EQUAL;
		case GREATER_THAN:
			return BinaryOp::GREATER_THAN;
		case GREATER_OR_EQUAL:
			return BinaryOp::GREATER_OR_EQUAL;
		default:
			throw_error(1, fmt::format("Invalid TokenType to BinaryOp conversion: type {}", op));
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
		case BinaryOp::LOGICAL_AND:
			return std::string{"&&"};
		case BinaryOp::LOGICAL_OR:
			return std::string{"||"};
		case BinaryOp::EQUAL_TO:
			return std::string{"=="};
		case BinaryOp::NOT_EQUAL:
			return std::string{"!="};
		case BinaryOp::LESS_THAN:
			return std::string{"<"};
		case BinaryOp::LESSER_OR_EQUAL:
			return std::string{"<="};
		case BinaryOp::GREATER_THAN:
			return std::string{">"};
		case BinaryOp::GREATER_OR_EQUAL:
			return std::string{">="};

		default:
			// TODO: use a C++ feature to print the function automatically.
			throw_error(1, "Error in BinaryInstr->toStr: Invalid operator");
			exit(1);
	}
}

std::string operandToStr(Operand operand)
{
	switch (operand.kind)
	{
		case OperandKind::Immediate:
			return fmt::format("Immediate({})", operand.val);
		case OperandKind::Temp:
			return fmt::format("Temp({}, {})", operand.val, operand.offset);
		case OperandKind::Variable:
			return fmt::format("Variable({}, {})", operand.name, operand.offset);
		default:
			throw_error(1, "Operand has invalid kind. Hello??");
			exit(1);
	}
}

static void genDeclaration(DeclarationNode* node, FuncDefNode* func)
{
	if (node->assignment == nullptr) return;
	genAssignment(node->assignment, func);
}

static Label* newLocalLabel(std::string text)
{
	for (char c : text) {
		if (isNumber(c)) throw_error(1, fmt::format("Labels cannot contain numbers!"));
	}
	return new Label(std::string{".L"}.append(text.append(std::to_string(++labelCount))));
}

static void genAssignment(AssignmentNode* node, FuncDefNode* func)
{
	Operand dest = Operand::Variable(node->name, func->getSymbol(node->name).offset);
	Operand src;

	if (auto* binOp = dynamic_cast<BinaryOpNode*>(node->expression))
	{
		switch (binOp->op)
		{
			case BinaryOp::LOGICAL_OR:
				genOr(binOp, dest, func);
				break;
			case BinaryOp::LOGICAL_AND:
				genAnd(binOp, dest, func);
				break;
			default:
				src = genExpression(node->expression, func);
				emit(new AssignmentInstr(dest, src));
		}
	}
	else {
		src = genExpression(node->expression, func);
		emit(new AssignmentInstr(dest, src));
	}
}

static void genOr(BinaryOpNode* node, Operand dest, FuncDefNode* func)
{
	Label* ifTrue = newLocalLabel(".iftrue");
	Label* ifFalse = newLocalLabel(".iffalse");
	Label* cont = newLocalLabel(".cont");

	Operand left = genExpression(node->left, func);
	Operand right = genExpression(node->right, func);

	emit(new JumpIfTrueInstr(ifTrue, left));
	emit(new JumpIfFalseInstr(ifFalse, right));
	emit(ifTrue);
	emit(new AssignmentInstr(dest, Operand::Immediate(1)));
	emit(new JumpInstr(cont));
	emit(ifFalse);
	emit(new AssignmentInstr(dest, Operand::Immediate(0)));
	emit(cont);
}

static void genAnd(BinaryOpNode* node, Operand dest, FuncDefNode* func)
{
	Label* skip = newLocalLabel(".skip");
	Label* cont = newLocalLabel(".cont");

	Operand left = genExpression(node->left, func);
	Operand right = genExpression(node->right, func);

	emit(new JumpIfFalseInstr(skip, left));
	emit(new JumpIfFalseInstr(skip, right));
	emit(new AssignmentInstr(dest, Operand::Immediate(1)));
	emit(new JumpInstr(cont));
	emit(skip);
	emit(new AssignmentInstr(dest, Operand::Immediate(0)));
	emit(cont);
}

static void genIf(IfNode* node, FuncDefNode* func)
{
	Label* skip;
	Operand expr = genExpression(node->expression, func);

	if (node->elseBranch) {
		skip = newLocalLabel(".else");
	}
	else
		skip = newLocalLabel(".cont");

	emit(new JumpIfFalseInstr(skip, expr));
	genStatements(node->body, func);
	emit(skip);

	if (node->elseBranch)
		genStatements(node->elseBranch->body, func);
}
