#include "frontend/parser/astGen.h"
#include <stdio.h>

int main(int argc, char* argv[]){
	jmp_buf compRetEnv;
	if(!setjmp(compRetEnv)){
		setJmpBuf(compRetEnv);
		verifyAlphabeticalOrder();
		char src[] = "int x(int a, int b){}";
		tokenArray a = tokenizeSource(src);
		node b = constructTree(a);
		printTree(&b, 0);
	} else{
		fprintf(stderr, "ERRORS ENCOUNTERED IN COMPILATION\n\n");
	}
	return 0;
}