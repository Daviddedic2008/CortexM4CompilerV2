#include "frontend/parser/astGen.h"
#include <stdio.h>

int main(int argc, char* argv[]){
	verifyAlphabeticalOrder();
	char src[] = "x(y, 7);";
	tokenArray a = tokenizeSource(src);
	node b = constructTree(a);
	printTree(&b, 0);
	return 0;
}