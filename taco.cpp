/*
	Intermediate representation using three-address code (TAC)

*/

#include <cassert>
#include <vector>

#include "ast.hpp"
#include "taco.hpp"
#include "utils.hpp"

static void emit(Instruction* instr);
static Operand genExpression(Node *node, ScopeNode* parent);
static void genOr(BinaryOpNode* node, Operand dest, ScopeNode* parent);
static void genAnd(BinaryOpNode* node, Operand dest, ScopeNode* parent);
static void genBody(std::vector<Node*> body, ScopeNode* parent);
static Operand genCall(CallNode* node, ScopeNode* parent);
static Operand genAssignment(AssignmentNode* node, ScopeNode* parent);
static Operand emitPrefix(UnaryOpNode* node, ScopeNode* parent);
static Operand emitPostFix(UnaryOpNode* node, ScopeNode* parent);

static int current = 0; // temp register
static int labelCount = 0;
static std::vector<Instruction*> ir;

std::vector<Instruction*> taco(GodNode *program)
{
	for (Node* node : program->body)
	{
		node->gen(program);
	}

	fmt::print("IR generated\n");
	return ir;
}

static int newVreg()
{
	return ++current;
}

static void genStatement(Node* node, ScopeNode* parent)
{
	node->gen(parent);
}

static void genBody(std::vector<Node*> body, ScopeNode* parent)
{
	for (size_t i = 0; i < body.size(); i++) {
		genStatement(body[i], parent);
	}
}

static void emit(Instruction* instr)
{
	ir.push_back(instr);
}

static Operand genExpression(Node *node, ScopeNode* parent)
{
	Operand vreg;

	if (auto* n = dynamic_cast<ImmediateNode*>(node)) {
		vreg = Operand::Immediate(n->value);
	}
	else if (auto* binOp = dynamic_cast<BinaryOpNode*>(node)) {
		if (binOp->op == BinaryOp::LOGICAL_AND ||
			binOp->op == BinaryOp::LOGICAL_OR) {
			return Operand::Temp(newVreg(), parent->changeFrameSize(4));
		}
		
		Operand left = genExpression(binOp->left, parent);
		Operand right = genExpression(binOp->right, parent);
		vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));

		auto* binInstr = new BinaryInstr(vreg,
										 binOp->op,
										 left,
										 right);

		emit(binInstr);
		return vreg;
	}
	else if (auto* unOp = dynamic_cast<UnaryOpNode*>(node)) {
		UnaryInstr* unInstr;
		Operand src;
		Attrs var{nullptr, 0, 0};

		switch (unOp->op)
		{
			case UnaryOp::PREINC:
			case UnaryOp::PREDEC:
				vreg = emitPrefix(unOp, parent);
				break;
			case UnaryOp::POSTINC:
			case UnaryOp::POSTDEC:
				vreg = emitPostFix(unOp, parent);
				break;
			case UnaryOp::NEGATE:
			case UnaryOp::LOGICAL_NOT:
				vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
				src = genExpression(unOp->expression, parent);
				unInstr = new UnaryInstr(vreg, src, unOp->op);
				emit(unInstr);
				break;
		}
		return vreg;
	}
	else if (auto* var = dynamic_cast<VariableNode*>(node)) {
		Attrs variable = parent->getSymbol(var->name, var->line, parent);
		return Operand::Variable(var->name,
								 variable.offset,
								 variable.type->size);
	}
	else if (auto* call = dynamic_cast<CallNode*>(node)) {
		return genCall(call, parent);
	}
	else if (auto* assign = dynamic_cast<AssignmentNode*>(node)) {
		return genAssignment(assign, parent); 
	}
	else {
		throw_error_line(1, 
						 node->line, 
						 fmt::format("genExpression: no rule for {}", node->typeName()));
	}
	return vreg;
}

void ReturnNode::gen(ScopeNode* parent)
{
	Operand dest = genExpression(this->expression, parent);

	if (auto* binOp = dynamic_cast<BinaryOpNode*>(this->expression))
	{
		if (binOp->op == BinaryOp::LOGICAL_OR)
				genOr(binOp, dest, parent);
		else if (binOp->op == BinaryOp::LOGICAL_AND)
				genAnd(binOp, dest, parent);
	}
	emit(new ReturnInstr(dest));
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
			fmt::print("\nOperand: {}\n", operand.name, operand.offset, operand.size, operand.val);
			throw_error(1, "Operand has invalid kind. Hello??");
			exit(1);
	}
}

static Operand genAssignment(AssignmentNode* node, ScopeNode* parent)
{
	Attrs variable = parent->getSymbol(node->name, node->line, parent);
	Operand dest = Operand::Variable(node->name,
									 variable.offset,
									 variable.type->size);
	Operand src;

	if (auto* binOp = dynamic_cast<BinaryOpNode*>(node->expression))
	{
		switch (binOp->op)
		{
			case BinaryOp::LOGICAL_OR:
				genOr(binOp, dest, parent);
				break;
			case BinaryOp::LOGICAL_AND:
				genAnd(binOp, dest, parent);
				break;
			default:
				src = genExpression(node->expression, parent);
				emit(new AssignmentInstr(dest, src));
		}
	}
	else {
		src = genExpression(node->expression, parent);
		emit(new AssignmentInstr(dest, src));
	}

	return src;
}

void DeclarationNode::gen(ScopeNode* parent)
{
	if (this->expression == nullptr) return;

	auto* assign = new AssignmentNode(this->line);
	assign->name = this->name;
	assign->expression = this->expression;
	genAssignment(assign, parent);
}

Label* newLocalLabel(std::string text)
{
	for (char c : text) {
		if (isNumber(c)) throw_error(1, fmt::format("Labels cannot contain numbers!"));
	}
	if (text.back() == ':')
		throw_error(1, fmt::format("newLocalLabel can't receive label definitions"));
	return new Label(std::string{".L_"}.append(text.append(std::to_string(++labelCount))));
}

void AssignmentNode::gen(ScopeNode* parent)
{
	genAssignment(this, parent);
}

static void genOr(BinaryOpNode* node, Operand dest, ScopeNode* parent)
{
	Label* ifTrue = newLocalLabel("iftrue");
	Label* ifFalse = newLocalLabel("iffalse");
	Label* cont = newLocalLabel("cont");

	Operand left = genExpression(node->left, parent);
	Operand right = genExpression(node->right, parent);

	emit(new JumpIfTrueInstr(ifTrue, left));
	emit(new JumpIfFalseInstr(ifFalse, right));
	emit(ifTrue->def());
	emit(new AssignmentInstr(dest, Operand::Immediate(1)));
	emit(new JumpInstr(cont));
	emit(ifFalse->def());
	emit(new AssignmentInstr(dest, Operand::Immediate(0)));
	emit(cont->def());
}

static void genAnd(BinaryOpNode* node, Operand dest, ScopeNode* parent)
{
	Label* skip = newLocalLabel("skip");
	Label* cont = newLocalLabel("cont");

	Operand left = genExpression(node->left, parent);
	Operand right = genExpression(node->right, parent);

	emit(new JumpIfFalseInstr(skip, left));
	emit(new JumpIfFalseInstr(skip, right));
	emit(new AssignmentInstr(dest, Operand::Immediate(1)));
	emit(new JumpInstr(cont));
	emit(skip->def());
	emit(new AssignmentInstr(dest, Operand::Immediate(0)));
	emit(cont->def());
}

void IfNode::gen(ScopeNode* parent)
{
	Label* skip;
	Operand expr = genExpression(this->expression, this);

	if (this->elseBranch) {
		skip = newLocalLabel("else");
	}
	else
		skip = newLocalLabel("cont");

	emit(new JumpIfFalseInstr(skip, expr));
	genBody(this->body, this);
	emit(skip->def());

	if (this->elseBranch)
		genBody(this->elseBranch->body, parent);
}

static void genLoopStatements(std::vector<Node*> block, LoopNode* parent)
{
	for (size_t i = 0; i < block.size(); ++i)
	{
		block[i]->gen(parent);
	}
}

void WhileNode::gen(ScopeNode* parent)
{
	(void)parent;
	emit(this->startLabel->def());
	Operand expr = genExpression(this->expression, this);
	emit(new JumpIfFalseInstr(this->endLabel, expr));
	genLoopStatements(this->body, this);
	emit(new JumpInstr(this->startLabel));
	emit(this->endLabel->def());
}

void DoWhileNode::gen(ScopeNode* parent)
{
	(void)parent;

	emit(this->startLabel->def());
	genLoopStatements(this->body, this);
	Operand expr = genExpression(this->expression, this);
	emit(new JumpIfTrueInstr(this->startLabel, expr));
	emit(this->endLabel->def());
}

void ForNode::gen(ScopeNode* parent)
{
	/*
		prologue
		.forstart:
		expression
		jumpiffalse .forend
		body
			if continue: jump epilogue
			if break: jump forend
		.epilogue:
		epilogue
		jump .forstart
		.forend:
	*/
	(void)parent;

	this->prologue->gen(this);
	emit(this->startLabel->def());

	if (!dynamic_cast<VoidNode*>(this->expression)) {
		Operand expr = genExpression(this->expression, this);
		emit(new JumpIfFalseInstr(this->endLabel, expr));
	}
	genLoopStatements(this->body, this);
	emit(this->epilogueLabel->def());

	this->epilogue->gen(this);
	emit(new JumpInstr(this->startLabel));
	emit(this->endLabel->def());	
}

void BreakNode::gen(ScopeNode* parent)
{
	(void)parent;
	emit(new JumpInstr(this->findParentLoop(this->parent)->endLabel));
}

void ContinueNode::gen(ScopeNode* parent)
{
	(void)parent;
	Label* jmpLabel;
	LoopNode* parentLoop = this->findParentLoop(this->parent);
	if (auto* forl = dynamic_cast<ForNode*>(parentLoop)) {
		jmpLabel = forl->epilogueLabel;
	}
	else {
		jmpLabel = parentLoop->startLabel;
	}
	emit(new JumpInstr(jmpLabel));
}

Operand genCall(CallNode* node, ScopeNode* parent)
{
	Operand retReg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
	std::vector<Operand> argRegs;

	for (size_t i = 0; i < node->args.size(); ++i)
	{
		argRegs.push_back(genExpression(node->args[i], parent));
	}
	for (size_t i = 0; i < argRegs.size(); ++i)
	{
		emit(new LoadArg(i, argRegs[i]));
	}
	
	emit(new CallInstr(node->name)); 
	emit(new SaveRet(retReg));
	return retReg;
}

void FuncNode::gen(ScopeNode* parent)
{
	(void)parent;
	if (!this->isDef) return;

	emit(new FuncPrologueInstr(this->name));
	size_t index = ir.size() - 1;

	for (size_t i = 0; i < this->paramList.size(); i++)
	{
		Attrs attrs = this->getSymbol(this->paramList[i].name, this->line, this);
		Operand op = Operand::Variable(this->paramList[i].name,
										attrs.offset,
										attrs.type->size);

		emit(new SaveArg(i, op));
	}
	genBody(this->body, this);

	this->frameSize = abs(this->frameSize);
	int pad = this->frameSize % 16;
	this->frameSize = this->frameSize + 16 - (pad ? pad : 16);
	static_cast<FuncPrologueInstr*>(ir.at(index))->frameSize = this->frameSize;
	emit(new FuncEpilogueInstr(this->name));
}

void UnaryOpNode::gen(ScopeNode* parent)
{
	genExpression(this, parent);
}

void ImmediateNode::gen(ScopeNode* parent)
{
	genExpression(this, parent);
}

void VariableNode::gen(ScopeNode* parent)
{
	genExpression(this, parent);
}

void ElseNode::gen(ScopeNode* parent)
{
	(void)parent;
	throw_error_line(1, this->line, "Else should never be alone. How.");
}

void BinaryOpNode::gen(ScopeNode* parent)
{
	genExpression(this, parent);
}

void CallNode::gen(ScopeNode* parent)
{
	genExpression(this, parent);
}

static Operand emitPrefix(UnaryOpNode* node, ScopeNode* parent)
{
	auto* v = static_cast<VariableNode*>(node->expression);
	Attrs var = parent->getSymbol(v->name, v->line, parent);
	Operand vreg = Operand::Variable(v->name, var.offset, var.type->size);
	emit(new UnaryInstr(vreg, vreg, node->op));
	return vreg;
}

static Operand emitPostFix(UnaryOpNode* node, ScopeNode* parent)
{
	Operand vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
	Operand src = genExpression(node->expression, parent);
	emit(new UnaryInstr(vreg, src, node->op));
	return vreg;
}
