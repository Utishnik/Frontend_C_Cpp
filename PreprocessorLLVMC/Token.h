#pragma once
#include <string>
#include "tcc.h"

static long long tok;

struct Token {
	std::string token_text;
	CValue value;
	CType type;
};