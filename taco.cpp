/*
	Intermediate representation using three-address code (TAC)

*/

#include <cassert>
#include <optional>
#include <vector>

#include "ast.hpp"
#include "taco.hpp"
#include "utils.hpp"

static void emit(std::unique_ptr<Instruction> instr);
static std::optional<Operand> genExpression(Node *node,
											ScopeNode* parent,
											std::optional<Operand> dest);
static void genCond(Node* expr,
					ScopeNode* parent,
					std::shared_ptr<Label> ifTrue,
					std::shared_ptr<Label> ifFalse);
static void genOr(BinaryOpNode* node, Operand dest, ScopeNode* parent);
static void genAnd(BinaryOpNode* node, Operand dest, ScopeNode* parent);
static void genBody(std::vector<std::unique_ptr<Node>>& body, ScopeNode* parent);
static Operand genCall(CallNode* node, ScopeNode* parent);
static Operand genAssignment(int line, std::string name, Node* expr, ScopeNode* parent);
static Operand emitPrefix(UnaryOpNode* node, ScopeNode* parent);
static Operand emitPostFix(UnaryOpNode* node, ScopeNode* parent);

static int current = 0; // temp register
static int labelCount = 0;
static std::vector<std::unique_ptr<Instruction>> ir;
static GodNode* program;

std::vector<std::unique_ptr<Instruction>> taco(GodNode *prog)
{
	program = prog;

	for (const std::unique_ptr<Node>& node : program->body)
	{
		node->gen(program);
	}

	fmt::print("IR generated\n");
	return std::move(ir);
}

static int newVreg()
{
	return ++current;
}

static void genStatement(Node* node, ScopeNode* parent)
{
	node->gen(parent);
}

static void genBody(std::vector<std::unique_ptr<Node>>& body, ScopeNode* parent)
{
	for (size_t i = 0; i < body.size(); i++) {
		genStatement(body[i].get(), parent);
	}
}

static void emit(std::unique_ptr<Instruction> instr)
{
	ir.emplace_back(std::move(instr));
}

// optimization: prevents needless temp variables if a dest Operand is defined
static std::optional<Operand> genExpression(Node *node,
							 ScopeNode* parent,
							 std::optional<Operand> dest = std::nullopt)
{
	Operand vreg;
	if (dest.has_value()) vreg = dest.value();

	if (auto* n = dynamic_cast<ImmediateNode*>(node)) {
		vreg = Operand::Immediate(n->value);
	}
	else if (auto* binOp = dynamic_cast<BinaryOpNode*>(node))
	{
		if (binOp->op == BinaryOp::LOGICAL_AND ||
			binOp->op == BinaryOp::LOGICAL_OR)
		{
			if (!vreg.exists) vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
			if (binOp->op == BinaryOp::LOGICAL_OR)
				genOr(binOp, vreg, parent);

			if (binOp->op == BinaryOp::LOGICAL_AND)
				genAnd(binOp, vreg, parent);
		}
		else {
			Operand left = genExpression(binOp->left.get(), parent).value();
			Operand right = genExpression(binOp->right.get(), parent).value();
	
			if (!vreg.exists) vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
			auto binInstr = std::make_unique<BinaryInstr>(vreg,
											 binOp->op,
											 left,
											 right);
	
			emit(std::move(binInstr));
		}
		return vreg;
	}
	else if (auto* unOp = dynamic_cast<UnaryOpNode*>(node)) {
		std::unique_ptr<UnaryInstr> unInstr;
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
				src = genExpression(unOp->expression.get(), parent).value();
				// optimization: inserts negativ immediates directly.
				if (src.kind == OperandKind::Immediate)
				{
					if (unOp->op == UnaryOp::NEGATE)
						src.val = -src.val;
					else
						src.val = src.val ? 0 : 1;
					
					vreg = src;
				} else {
					vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
					unInstr = std::make_unique<UnaryInstr>(vreg, src, unOp->op);
					emit(std::move(unInstr));
				}
				break;
			default:
				throw_error_line(1, unOp->line,
					fmt::format("genExpression: no rule for unary {}", unOp->typeName()));
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
		return genAssignment(assign->line, assign->name, assign->expression.get(), parent); 
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
	Operand dest = genExpression(this->expression.get(), parent).value();
	emit(std::make_unique<ReturnInstr>(dest));
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

static Operand genAssignment(int line, std::string name, Node* expr, ScopeNode* parent)
{
	Attrs variable = parent->getSymbol(name, line, parent);
	Operand dest = Operand::Variable(name,
									 variable.offset,
									 variable.type->size);
	Operand src;

	if (auto* binOp = dynamic_cast<BinaryOpNode*>(expr))
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
				src = genExpression(expr, parent, dest).value();
		}
	}
	else {
		src = genExpression(expr, parent).value();
		emit(std::make_unique<AssignmentInstr>(dest, src));
	}

	return src;
}

void DeclarationNode::gen(ScopeNode* parent)
{
	if (this->expression == nullptr) return;

	genAssignment(this->line, this->name, this->expression.get(), parent);
}

std::unique_ptr<Label> newLocalLabel(std::string text)
{
	for (char c : text) {
		if (isNumber(c)) throw_error(1, fmt::format("Labels cannot contain numbers!"));
	}
	if (text.back() == ':')
		throw_error(1, fmt::format("newLocalLabel can't receive label definitions"));

	return std::make_unique<Label>(
		std::string{".L_"}.append(text.append("_").append(std::to_string(++labelCount)))
	);
}

void AssignmentNode::gen(ScopeNode* parent)
{
	genAssignment(this->line, this->name, this->expression.get(), parent);
}

static void genOr(BinaryOpNode* node, Operand dest, ScopeNode* parent)
{
	std::shared_ptr<Label> ifTrue = newLocalLabel("iftrue");
	std::shared_ptr<Label> cont = newLocalLabel("cont");
	std::shared_ptr<Label> ifFalse = newLocalLabel("iffalse");

	Operand left = genExpression(node->left.get(), parent).value();
	Operand right = genExpression(node->right.get(), parent).value();

	emit(std::make_unique<JumpIfInstr>(true, ifTrue, left));
	emit(std::make_unique<JumpIfInstr>(false, ifFalse, right));
	emit(ifTrue->def());
	emit(std::make_unique<AssignmentInstr>(dest, Operand::Immediate(1)));
	emit(std::make_unique<JumpInstr>(cont));
	emit(ifFalse->def());
	emit(std::make_unique<AssignmentInstr>(dest, Operand::Immediate(0)));
	emit(cont->def());
}

static void genAnd(BinaryOpNode* node, Operand dest, ScopeNode* parent)
{
	std::shared_ptr<Label> skip = newLocalLabel("skip");
	std::shared_ptr<Label> cont = newLocalLabel("cont");

	Operand left = genExpression(node->left.get(), parent).value();
	Operand right = genExpression(node->right.get(), parent).value();

	emit(std::make_unique<JumpIfInstr>(false, skip, left));
	emit(std::make_unique<JumpIfInstr>(false, skip, right));
	emit(std::make_unique<AssignmentInstr>(dest, Operand::Immediate(1)));
	emit(std::make_unique<JumpInstr>(cont));
	emit(skip->def());
	emit(std::make_unique<AssignmentInstr>(dest, Operand::Immediate(0)));
	emit(cont->def());
}

void IfNode::gen(ScopeNode* parent)
{
	std::shared_ptr<Label> skip;
	std::shared_ptr<Label> isTrue = newLocalLabel("is_true");
	
	if (this->elseBranch) {
		skip = newLocalLabel("else");
	}
	else {
		skip = newLocalLabel("cont");
	}
	genCond(this->expression.get(), parent, isTrue, skip);
	emit(isTrue->def());
	genBody(this->body, this);
	emit(skip->def());

	if (this->elseBranch)
		genBody(this->elseBranch->body, parent);
}

// optimization: prevents unnecessary creation of temps used in conditionals.
// e.g. if (a == b): previously it'd do the comparison, store it, load it, check if it's true,
// and then do the jump. Now it just does the comparison and uses the result directly when possible
static void genCond(Node* expr, 
					ScopeNode* parent,
					std::shared_ptr<Label> ifTrue, 
					std::shared_ptr<Label> ifFalse)
{
	if (auto e = dynamic_cast<BinaryOpNode*>(expr))
	{
		std::shared_ptr<Label> local;
		Operand operand;
		Operand left;
		Operand right;
		switch(e->op)
		{
			case BinaryOp::LOGICAL_AND:
				local = newLocalLabel("iftrue");
				genCond(e->left.get(), parent, local, ifFalse);
				emit(local->def());
				break;
			case BinaryOp::LOGICAL_OR:
				local = newLocalLabel("nextcond");
				genCond(e->left.get(), parent, ifTrue, local);
				emit(local->def());
				genCond(e->right.get(), parent, ifTrue, ifFalse);
				break;
			case BinaryOp::EQUAL_TO:
			case BinaryOp::NOT_EQUAL:
			case BinaryOp::LESS_THAN:
			case BinaryOp::LESSER_OR_EQUAL:
			case BinaryOp::GREATER_THAN:
			case BinaryOp::GREATER_OR_EQUAL:
				left = genExpression(e->left.get(), parent).value();
				right = genExpression(e->right.get(), parent).value();
				emit(std::make_unique<CondJump>(e->op, left, right, ifTrue, ifFalse));
				break;
			default:
				operand = genExpression(expr, parent).value();
				emit(std::make_unique<CondJump>(e->op, operand,
					Operand::Immediate(0),
					std::move(ifTrue),
					std::move(ifFalse)));
				break;
		}
	}
	else if (auto u = dynamic_cast<UnaryOpNode*>(expr))
	{
		if (u->op == UnaryOp::LOGICAL_NOT)
		{
			genCond(u->expression.get(), parent, ifFalse, ifTrue);
		}

	}
	else {
		Operand operand = genExpression(expr, parent).value();
		emit(std::make_unique<CondJump>(BinaryOp::NOT_EQUAL, 
										operand, 
										Operand::Immediate(0), 
										ifTrue, 
										ifFalse));
	}
}

static void genLoopStatements(std::vector<std::unique_ptr<Node>>& block, LoopNode* parent)
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
	genCond(this->expression.get(), this, this->bodyLabel, this->endLabel);
	emit(this->bodyLabel->def());
	genLoopStatements(this->body, this);
	emit(std::make_unique<JumpInstr>(this->startLabel));
	emit(this->endLabel->def());
}

void DoWhileNode::gen(ScopeNode* parent)
{
	(void)parent;

	emit(this->startLabel->def());
	genLoopStatements(this->body, this);
	genCond(this->expression.get(), this, this->startLabel, this->endLabel);
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

	if (!dynamic_cast<VoidNode*>(this->expression.get())) {
		genCond(this->expression.get(), this, this->bodyLabel, this->endLabel);
	}
	emit(bodyLabel->def());
	genLoopStatements(this->body, this);
	emit(this->epilogueLabel->def());

	this->epilogue->gen(this);
	emit(std::make_unique<JumpInstr>(this->startLabel));
	emit(this->endLabel->def());	
}

void BreakNode::gen(ScopeNode* parent)
{
	(void)parent;
	emit(std::make_unique<JumpInstr>(this->findParentLoop(this->parent)->endLabel));
}

void ContinueNode::gen(ScopeNode* parent)
{
	(void)parent;
	std::shared_ptr<Label> jmpLabel;
	LoopNode* parentLoop = this->findParentLoop(this->parent);
	if (auto* forl = dynamic_cast<ForNode*>(parentLoop)) {
		jmpLabel = forl->epilogueLabel;
	}
	else {
		jmpLabel = parentLoop->startLabel;
	}
	emit(std::make_unique<JumpInstr>(jmpLabel));
}

Operand genCall(CallNode* node, ScopeNode* parent)
{
	Operand retReg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
	auto instr = std::make_unique<CallInstr>(node->name, true, retReg);

	for (size_t i = 0; i < node->args.size(); ++i)
	{
		instr->args.push_back(genExpression(node->args[i].get(), parent).value());
	}

	emit(std::move(instr));
	return retReg;
}

void FuncNode::gen(ScopeNode* parent)
{
	(void)parent;
	if (!this->isDef) return;

	auto prologue = std::make_unique<FuncPrologueInstr>(this->name);

	for (size_t i = 0; i < this->paramList.size(); i++)
	{
		Attrs attrs = this->getSymbol(this->paramList[i].name, this->line, this);
		Operand op = Operand::Variable(this->paramList[i].name,
										attrs.offset,
										attrs.type->size);

		prologue->args.push_back(op);
	}
	emit(std::move(prologue));
	size_t index = ir.size() - 1;

	genBody(this->body, this);

	int pad = this->frameSize % 16;
	this->frameSize = this->frameSize + 16 - (pad ? pad : 16);
	static_cast<FuncPrologueInstr*>(ir.at(index).get())->frameSize = this->frameSize;
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
	auto* v = static_cast<VariableNode*>(node->expression.get());
	Attrs var = parent->getSymbol(v->name, v->line, parent);
	Operand vreg = Operand::Variable(v->name, var.offset, var.type->size);
	emit(std::make_unique<UnaryInstr>(vreg, vreg, node->op));
	return vreg;
}

static Operand emitPostFix(UnaryOpNode* node, ScopeNode* parent)
{
	Operand vreg = Operand::Temp(newVreg(), parent->changeFrameSize(4));
	Operand src = genExpression(node->expression.get(), parent).value();
	emit(std::make_unique<UnaryInstr>(vreg, src, node->op));
	return vreg;
}

bool isbinOpRelationalOperator(BinaryOp op)
{
	for (auto i : binOpRelationals) {
		if (op == i) return true;
	}
	return false;
}
