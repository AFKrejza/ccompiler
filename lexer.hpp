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
		TokenType tokenType;
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
			  int column)
		{
			this->tokenType = tokenType;
			this->lexeme = lexeme;
			this->literal = literal;
			this->line = line;
			this->column = column;
			this->length = length;
		}

		// only for 0-initialization
		Token() {
			this->tokenType = END_OF_FILE;
			this->lexeme = "";
			this->line = 0;
			this->column = 0;
			this->length = 0;
		}

		void print()
		{
			fmt::print("   TokenType: {} \n", tokenTypeToStr(tokenType));
			fmt::print(" Lexeme: {}\n", lexeme);
			fmt::print("Literal: {}\n", std::visit(LiteralPrintVisitor{}, literal));
			fmt::print("   Line: {}\n", line);
			fmt::print(" Column: {}\n", column);
			fmt::print(" Length: {}\n", length);
		}
		
		// contains all symbols and keywords
		static const std::unordered_map<TokenType, std::string> printmap;

		static std::string tokenTypeToStr(TokenType type)
		{
			int int_type = (int)type;

			auto it = printmap.find(type);
			if (it != printmap.end()) {
				return it->second;
			}
			throw_error(3, "Couldn't find token to be printed. ???");
			return "";
		}
};
