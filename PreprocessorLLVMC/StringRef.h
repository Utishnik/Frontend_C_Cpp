#include <string>
//todo переписать stringref из llvm C:\llvm\llvm-project\llvm\include\llvm\ADT\StringRef.h
struct StringRef {
	std::string* Data;
};