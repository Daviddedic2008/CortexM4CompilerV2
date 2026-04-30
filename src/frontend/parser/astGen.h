#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lexer/tokenizer.h"

typedef enum nodeType : uint8_t{
	bodyNode, operatorNode, conditionalNode, literalNode, identifierNode, castNode, declarationNode
}nodeType;

typedef struct node{
	nodeType type; token val;
	struct node* firstChild; struct node* lastChild; struct node* sibling;
}node;

node constructTree(tokenArray arr);

void printTree(node* n, int depth);