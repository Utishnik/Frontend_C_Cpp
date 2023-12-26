#include "StringRef.h"
#include "Token.h"
namespace Preprocessor {
	void SuggestTypoedDirective(const Token& Tok,
		StringRef Directive);
}