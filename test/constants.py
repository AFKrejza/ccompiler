
# class Constants:
U8_MAX = 255
I8_MAX = 127

U16_MAX = 65535
I16_MAX = 32767

U32_MAX = 4294967295
I32_MAX = 2147483647

U64_MAX = 18446744073709551615
I64_MAX = 9223372036854775807

def u_max(bits):
	return (1 << bits) - 1

def i_max(bits):
	return (1 << bits - 1) - 1