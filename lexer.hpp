#pragma once

#include <fmt/core.h>
#include <unordered_map>
#include <variant>

#include "error.hpp"
#include "operators.hpp"

// like a union
using Literal = std::variant<
	std::monostate,
	int,
	std::string,
	bool
>;

struct LiteralPrintVisitor {
	std::string operator()(std::monostate v) const {
		(void) v;
		return "empty";
	}
	std::string operator()(int v) const {
		return std::to_string(v);
	}
	std::string operator()(std::string v) const {
		return v;
	}
	std::string operator()(bool v) const {
		return v ? "True" : "False";
	}
};


class Token {
	public:
		TokenType type;
		std::string lexeme; // exact source text
		Literal literal;
		int line;
		int column; // TODO: broken
		int length; // TODO: broken

		Token(TokenType tokenType,
			  int length,
			  std::string lexeme,
			  Literal literal,
			  int line,
			  int column) : type(tokenType),
			  				lexeme(lexeme),
							literal(literal),
							line(line),
			  				column(column),
							length(length) {}

		// only for 0-initialization
		Token() : type(TokenType::END_OF_FILE), lexeme(""), line(0), column(0), length(0) {}

		void print()
		{
			fmt::print("   TokenType: {} \n", Token::toStr(this->type));
			fmt::print(" Lexeme: {}\n", lexeme);
			fmt::print("Literal: {}\n", std::visit(LiteralPrintVisitor{}, literal));
			fmt::print("   Line: {}\n", line);
			// fmt::print(" Column: {}\n", column);
			// fmt::print(" Length: {}\n", length);
		}
		
		// contains all symbols and keywords
		static const std::unordered_map<TokenType, std::string> printmap;

		static std::string toStr(TokenType type)
		{
			auto it = printmap.find(type);
			if (it != printmap.end()) {
				return it->second;
			}
			throw_error(3, "Couldn't find token to be printed. ???");
			return "";
		}

		static BinaryOp toBinaryOp(TokenType op) {
			switch (op)
			{
				case TokenType::PLUS:
					return BinaryOp::ADD;
				case TokenType::MINUS:
					return BinaryOp::SUB;
				case TokenType::ASTERISK:
					return BinaryOp::MUL;
				case TokenType::LOGICAL_AND:
					return BinaryOp::LOGICAL_AND;
				case TokenType::LOGICAL_OR:
					return BinaryOp::LOGICAL_OR;
				case TokenType::EQUAL_TO:
					return BinaryOp::EQUAL_TO;
				case TokenType::NOT_EQUAL:
					return BinaryOp::NOT_EQUAL;
				case TokenType::LESS_THAN:
					return BinaryOp::LESS_THAN;
				case TokenType::LESSER_OR_EQUAL:
					return BinaryOp::LESSER_OR_EQUAL;
				case TokenType::GREATER_THAN:
					return BinaryOp::GREATER_THAN;
				case TokenType::GREATER_OR_EQUAL:
					return BinaryOp::GREATER_OR_EQUAL;
				default:
					throw_error(1, fmt::format(
						"Invalid TokenType to BinaryOp conversion: type {}", Token::toStr(op)));
					exit(1);
			}
		}
};
