#include "frontend/lexer/tokenizer.h"
#include <stdio.h>

int main(int argc, char* argv[]){
	verifyAlphabeticalOrder();
	char src[] = "x = 500;";
	tokenizeSource(src);
	
	return 0;
}