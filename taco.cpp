/*
	Intermediate representation using three-address code (TAC)

*/

#include <cassert>
#include <vector>

#include "ast.hpp"
#include "taco.hpp"
#include "utils.hpp"

static void emit(Instruction* instr);
static void genDeclaration(DeclarationNode* node, ScopeNode* parent);
static Operand genExpression(Node *node, ScopeNode* parent);
static void genReturn(ReturnNode *node, ScopeNode* parent);
static void genAssignment(AssignmentNode* node, ScopeNode* parent);
static void genOr(BinaryOpNode* node, Operand dest, ScopeNode* parent);
static void genAnd(BinaryOpNode* node, Operand dest, ScopeNode* parent);
static void genBody(std::vector<Node*> body, ScopeNode* parent);
static void genIf(IfNode* node, ScopeNode* parent);
static void genWhile(WhileNode* node);
static void genDoWhile(DoWhileNode* node);
static void genFor(ForNode* node);
static void genBreak(BreakNode* node);
static void genContinue(ContinueNode* node);

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
			genBody(func->body, func);
			int pad = func->frameSize % 16;
			func->frameSize = func->frameSize + 16 - (pad ? pad : 16);
		}
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
	if (auto* ret = dynamic_cast<ReturnNode*>(node)) {
		genReturn(ret, parent);
	}
	else if (auto* decl = dynamic_cast<DeclarationNode*>(node)) {
		genDeclaration(decl, parent);
	}
	else if (auto* assign = dynamic_cast<AssignmentNode*>(node)) {
		genAssignment(assign, parent);
	}
	else if (auto* ifs = dynamic_cast<IfNode*>(node)) {
		genIf(ifs, parent);
	}
	else if (auto* whilel = dynamic_cast<WhileNode*>(node)) {
		genWhile(whilel);
	}
	else if (auto* doWhile = dynamic_cast<DoWhileNode*>(node)) {
		genDoWhile(doWhile);
	}
	else if (auto* forl = dynamic_cast<ForNode*>(node)) {
		genFor(forl);
	}
	else if (auto* breaker = dynamic_cast<BreakNode*>(node)) {
		genBreak(breaker);
	}
	else if (auto* conch = dynamic_cast<ContinueNode*>(node)) {
		genContinue(conch);
	}
	else {
		throw_error_line(1, 
						 node->line, 
						 fmt::format("Taco: no rule for {}", node->typeName()));
	}
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
		vreg = Operand::Temp(newVreg(), parent->changeFrameSize(parent, 4));
		if (binOp->op == BinaryOp::LOGICAL_AND ||
			binOp->op == BinaryOp::LOGICAL_OR) {
			return vreg;
		}
		auto* binInstr = new BinaryInstr(vreg,
										 binOp->op,
										 genExpression(binOp->left, parent),
										 genExpression(binOp->right, parent));

		emit(binInstr);
		return vreg;
	}
	else if (auto* unOp = dynamic_cast<UnaryOpNode*>(node)) {
		vreg = Operand::Temp(newVreg(), parent->changeFrameSize(parent, 4));
		Operand src = genExpression(unOp->expression, parent);
		auto* unInstr = new UnaryInstr(vreg, src, unOp->op);
		emit(unInstr);
		return vreg;
	}
	else if (auto* var = dynamic_cast<VariableNode*>(node)) {
		return Operand::Variable(var->name,
								 parent->getSymbol(var->name, var->line, parent).offset);
	}
	else {
		throw_error_line(1, 
						 node->line, 
						 fmt::format("genExpression: no rule for {}", node->typeName()));
	}
	return vreg;
}

static void genReturn(ReturnNode *node, ScopeNode* parent)
{
	Operand dest = genExpression(node->expression, parent);

	if (auto* binOp = dynamic_cast<BinaryOpNode*>(node->expression))
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
			throw_error(1, "Operand has invalid kind. Hello??");
			exit(1);
	}
}

static void genDeclaration(DeclarationNode* node, ScopeNode* parent)
{
	if (node->assignment == nullptr) return;
	genAssignment(node->assignment, parent);
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

static void genAssignment(AssignmentNode* node, ScopeNode* parent)
{
	Operand dest = Operand::Variable(node->name,
									 parent->getSymbol(node->name, node->line, parent).offset);
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

static void genIf(IfNode* node, ScopeNode* parent)
{
	Label* skip;
	Operand expr = genExpression(node->expression, node);

	if (node->elseBranch) {
		skip = newLocalLabel("else");
	}
	else
		skip = newLocalLabel("cont");

	emit(new JumpIfFalseInstr(skip, expr));
	genBody(node->body, node);
	emit(skip->def());

	if (node->elseBranch)
		genBody(node->elseBranch->body, parent);
}

static void genLoopStatements(std::vector<Node*> block, LoopNode* parent)
{
	for (size_t i = 0; i < block.size(); ++i)
	{
		if (dynamic_cast<BreakNode*>(block[i]))
		{
			emit(new JumpInstr(parent->endLabel));
		}
		else if (dynamic_cast<ContinueNode*>(block[i])) {
			if (auto* forl = dynamic_cast<ForNode*>(parent)) {
				emit(new JumpInstr(forl->epilogueLabel));
			}
			else emit(new JumpInstr(parent->startLabel));
		}
		else {
			genStatement(block[i], parent);
		}
	}
}

static void genWhile(WhileNode* node)
{
	emit(node->startLabel->def());
	Operand expr = genExpression(node->expression, node);
	emit(new JumpIfFalseInstr(node->endLabel, expr));
	genLoopStatements(node->body, node);
	emit(new JumpInstr(node->startLabel));
	emit(node->endLabel->def());
}

static void genDoWhile(DoWhileNode* node)
{
	emit(node->startLabel->def());
	genLoopStatements(node->body, node);
	Operand expr = genExpression(node->expression, node);
	emit(new JumpIfTrueInstr(node->startLabel, expr));
	emit(node->endLabel->def());
}

static void genFor(ForNode* node)
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
	if (auto* decl = dynamic_cast<DeclarationNode*>(node->prologue))
	{
		genDeclaration(decl, node);
	}
	else if (dynamic_cast<VoidNode*>(node->prologue))
	{}
	else {
		genExpression(node->prologue, node);
	}
	emit(node->startLabel->def());

	if (!dynamic_cast<VoidNode*>(node->expression)) {
		Operand expr = genExpression(node->expression, node);
		emit(new JumpIfFalseInstr(node->endLabel, expr));
	}

	genLoopStatements(node->body, node);

	emit(node->epilogueLabel->def());
	if (auto* assign = dynamic_cast<AssignmentNode*>(node->epilogue)) {
		genAssignment(assign, node);
	}
	else if (dynamic_cast<VoidNode*>(node->epilogue)) {}
	else {
		Operand expr = genExpression(node->epilogue, node);
	}

	emit(new JumpInstr(node->startLabel));
	emit(node->endLabel->def());	
}

static void genBreak(BreakNode* node)
{
	emit(new JumpInstr(node->findParentLoop(node->parent)->endLabel));
}

static void genContinue(ContinueNode* node)
{
	Label* jmpLabel;
	LoopNode* parent = node->findParentLoop(node->parent);
	if (auto* forl = dynamic_cast<ForNode*>(parent)) {
		jmpLabel = forl->epilogueLabel;
	}
	else {
		jmpLabel = parent->startLabel;
	}
	emit(new JumpInstr(jmpLabel));
}
