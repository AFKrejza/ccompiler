// Contains the type system for the semantic analysis phase

#include <string>

class Type {
	public:
		int size;
		bool isBase;
		Type *pointee = nullptr;

		Type(int size, bool isBase) {
			this->size = size;
			this->isBase = isBase;
		}

		virtual std::string typeName() {
			return "Type";
		}
};

class IntType : public Type {
	public:
		IntType() : Type(4, true) {}

		std::string typeName() override {
			std::string name = "IntType";
			return name;
		}
};

class PointerType : public Type {
	public:
		PointerType(Type *pointee) : Type(8, false) {
			this->pointee = pointee;
		}

		std::string typeName() override {
			std::string name = "PointerType";
			name.append(" " + this->pointee->typeName());
			return name;
		}
};

class VoidType : public Type {
	public:
		VoidType() : Type(0, true) {}

		std::string typeName() override {
			std::string name = "VoidType";
			return name;
		}
};
