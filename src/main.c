#include "frontend/parser/astGen.h"
#include <stdio.h>

int main(int argc, char* argv[]){
	verifyAlphabeticalOrder();
	char src[] = "if(x = 5){} else{x = 7;}";
	tokenArray a = tokenizeSource(src);
	node b = constructTree(a);
	printTree(&b, 0);
	return 0;
}