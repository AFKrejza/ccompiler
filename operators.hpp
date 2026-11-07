#pragma once

class BinOp {
	public:
	enum en {
		ADD,
		SUB,
		MUL,
		DIV,
		LOGICAL_AND,
		LOGICAL_OR,
		EQUAL_TO,
		NOT_EQUAL,
		LESS_THAN,
		LESSER_OR_EQUAL,
		GREATER_THAN,
		GREATER_OR_EQUAL
	};

	inline static const std::vector<BinOp::en> relational = {
		BinOp::en::EQUAL_TO,
		BinOp::en::NOT_EQUAL,
		BinOp::en::LESS_THAN,
		BinOp::en::LESSER_OR_EQUAL,
		BinOp::en::GREATER_THAN,
		BinOp::en::GREATER_OR_EQUAL
	};

	static std::string toStr(BinOp::en op) {
		switch (op) {
			case BinOp::en::ADD:
				return std::string{"+"};
			case BinOp::en::SUB:
				return std::string{"-"};
			case BinOp::en::MUL:
				return std::string{"*"};
			case BinOp::en::DIV:
				return std::string{"/"};
			case BinOp::en::LOGICAL_AND:
				return std::string{"&&"};
			case BinOp::en::LOGICAL_OR:
				return std::string{"||"};
			case BinOp::en::EQUAL_TO:
				return std::string{"=="};
			case BinOp::en::NOT_EQUAL:
				return std::string{"!="};
			case BinOp::en::LESS_THAN:
				return std::string{"<"};
			case BinOp::en::LESSER_OR_EQUAL:
				return std::string{"<="};
			case BinOp::en::GREATER_THAN:
				return std::string{">"};
			case BinOp::en::GREATER_OR_EQUAL:
				return std::string{">="};

			default:
				throw_error(1, "Error in BinaryInstr->toStr: Invalid operator");
				exit(1);
		}
	}

	static std::string toAsm(BinOp::en op)
	{
		switch (op)
		{
			case BinOp::en::ADD:
				return "add";
			case BinOp::en::SUB:
				return "sub";
			case BinOp::en::MUL:
				return "imul";
			case BinOp::en::LESS_THAN:
				return "setl";
			case BinOp::en::GREATER_THAN:
				return "setg";
			case BinOp::en::LESSER_OR_EQUAL:
				return "setle";
			case BinOp::en::GREATER_OR_EQUAL:
				return "setge";
			case BinOp::en::NOT_EQUAL:
				return "setne";
			case BinOp::en::EQUAL_TO:
				return "sete";
			case BinOp::en::LOGICAL_AND:
			case BinOp::en::LOGICAL_OR:
				throw_error(1, "Logical AND/OR don't exist at the assembly level");
				break;
			case BinOp::en::DIV:
				throw_error(1, "Division isn't supported");
				break;			
		}
		throw_error(1, "Error in BinOp::toAsm: Missing op translation");
		exit(1);
	}

	static std::string relationalToAsm(BinOp::en op, bool cond)
	{
		switch (op)
		{
			case BinOp::en::EQUAL_TO:
				return cond ? "je" : "jne";
			case BinOp::en::NOT_EQUAL:
				return cond ? "jne" : "je" ;
			case BinOp::en::LESS_THAN:
				return cond ? "jl" : "jge" ;
			case BinOp::en::LESSER_OR_EQUAL:
				return cond ? "jle" : "jg" ;
			case BinOp::en::GREATER_THAN:
				return cond ? "jg" : "jle" ;
			case BinOp::en::GREATER_OR_EQUAL:
				return cond ? "jge" : "jl" ;
			case BinOp::en::LOGICAL_AND:
			case BinOp::en::LOGICAL_OR:
			case BinOp::en::ADD:
			case BinOp::en::SUB:
			case BinOp::en::MUL:
			case BinOp::en::DIV:
				throw_error(1, fmt::format("'{}' is not a relational operator.", BinOp::toStr(op)));
				break;
		}
		throw_error(1, fmt::format("'{}' is not handled correctly", BinOp::toStr(op)));
		exit(1);
	}

	static bool isRelational(BinOp::en op)
	{
		for (auto i : BinOp::relational) {
			if (op == i) return true;
		}
		return false;
	}
};

class UnOp {
	public:
	enum en {
		NEGATE,
		LOGICAL_NOT,
		PREINC,
		POSTINC,
		PREDEC,
		POSTDEC
	};

	static std::string toStr(UnOp::en op)
	{
		switch (op)
		{
			case UnOp::en::NEGATE:
				return "NEGATE";
			case UnOp::en::LOGICAL_NOT:
				return "LOGICAL_NOT";
			case UnOp::en::PREINC:
				return "PREINC";
			case UnOp::en::POSTINC:
				return "POSTINC";
			case UnOp::en::PREDEC:
				return "PREDEC";
			case UnOp::en::POSTDEC:
				return "POSTDEC";
			default:
				throw_error(1, fmt::format("Error in UnOp::toStr: Missing op translation for {}",
					UnOp::toStr(op)));
				exit(1);
		}
	}
};
