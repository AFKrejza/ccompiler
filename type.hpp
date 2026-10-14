// Contains the type system for the semantic analysis phase
#pragma once

#include <string>

class Type {
	public:
		int size;
		bool isBase;

		Type(int size, bool isBase) : size(size), isBase(isBase) {}

		virtual ~Type() = default;

		virtual std::string typeName() const {
			return "Type";
		}

		static std::unique_ptr<Type> clone(const Type* input);
};

// TODO: sizes will have to be redone since you can do stuff like char c = int64max
// learn how gcc does it with instructions like movsx and movzx
class ImmediateType : public Type {
	public:
		ImmediateType() : Type(4, true) {}

		~ImmediateType() = default;

		std::string typeName() const override {
			return "ImmediateType";
		}
};

class IntType : public Type {
	public:
		IntType() : Type(4, true) {}

		~IntType() = default;

		std::string typeName() const override {
			return "IntType";
		}
};

class PointerType : public Type {
	public:
		std::unique_ptr<Type> pointee;

		PointerType(std::unique_ptr<Type>& pointee) : Type(8, false), pointee(std::move(pointee)) {}

		~PointerType() = default;

		std::string typeName() const override {
			std::string name = "PointerType";
			name.append(" " + this->pointee->typeName());
			return name;
		}
};

class VoidType : public Type {
	public:
		VoidType() : Type(0, true) {}

		~VoidType() = default;

		std::string typeName() const override {
			return "VoidType";
		}
};

class FuncType : public Type {
	public:
		FuncType() : Type(0, true) {}

		~FuncType() = default;

		std::string typeName() const override {
			return "FuncType";
		}
};
