#pragma once

#include <string>

void throw_error(int code, std::string msg);
void throw_error_line(int code, int line, std::string msg);
void throw_warn(int code, int line, std::string msg);