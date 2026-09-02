#include <unordered_map>

#include "type.hpp"

// symbol tables

struct Attrs {
	Type* type;
	int line; // declaration line
	int offset;

	Attrs(Type* type, int offset, int line) {
		this->type = type;
		this->offset = offset;
		this->line = line;
	}
};
