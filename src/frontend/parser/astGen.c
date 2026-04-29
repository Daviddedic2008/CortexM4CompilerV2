#include "astGen.h"
#include "../../helper/arenaAlloc.h"

/*
arena allocator for nodes, start with body node. child nodes will be allocated in linked list type of form.
allowing for sibling nodes to be children of the same parent and many children from one parent, no reallocation
*/

#define nodeStep 128

arena nodePool;

void initPool(){nodePool = newArena(nodeStep * sizeof(node));}

node constructNode(const nodeType type){
	return (node){.type = type, .firstChild = NULL, .lastChild = NULL, .sibling = NULL};
}

node* addNode(const nodeType type){
	const node tmp = constructNode(type);
	return writeElement(&nodePool, &tmp, sizeof(node));
}

void addChildFromPtr(node* n, const node* add){
	if(n->lastChild == NULL){
		n->firstChild = add; n->lastChild = add;
		return;
	} n->lastChild->sibling = add; n->lastChild = add;
}

node* retrieveChild(node* n, uint32_t idx){
	n = n->firstChild; if(n == NULL) return NULL; while(idx--){
		if(n->sibling == NULL) return NULL;
		n = n->sibling;
	} return n;
}

void addChild(node* n, const node add){
	const node* ptr = writeElement(&nodePool, &add, sizeof(node));
	addChildFromPtr(n, ptr);
}

tokenArray srcArr; uint32_t tokensScanned;

token peekToken(){return tokensScanned >= srcArr.numTokens ? (token){.type = nullToken} : *srcArr.tokens;}
token eatToken(){return tokensScanned++ >= srcArr.numTokens ? (token){.type = nullToken} : *(srcArr.tokens++);}

node* parseBody(); node* parseExpression(const uint16_t minPrecedence);

node* parseArgument(){
	const token t = eatToken();
	node* n;
	switch(t.type){
		case literal: n = addNode(literalNode); break;
		case identifier: n = addNode(identifierNode); break;
		case parenthesesL: n = parseExpression(0); eatToken(); return n;
		case opMinus: opReference: opDereference: opLogicalNot: opBitwiseNot:
		n->type = operatorNode; addChildFromPtr(n, parseExpression(110));
	}
	return n->val = t, n;
}

uint16_t getPrecedence(const uint8_t t) {
    switch(t) {
        case opMul: case opDiv: return 100;
        case opPlus: case opMinus: return 90;
        case opShiftLeft: case opShiftRight: return 80;
        case opCmpGreater: case opCmpLess: case opCmpGrEq: case opCmpLeEq: return 70;
        case opCmpEquals: return 60;
        case opBitwiseAnd: return 50;
        case opBitwiseOr: return 40;
        case opLogicalAnd: return 30;
        case opLogicalOr: return 20;
        case opEqual: return 10;
        default: return 0;
    }
}

node* parseExpression(const uint16_t minPrecedence){
	node* left = parseArgument();
	while(1){
		token op = peekToken(); const uint16_t p = getPrecedence(op.type);
		if(p < minPrecedence || !p) break;
		eatToken();
		node* right = parseExpression(p+1);
		node* parent = addNode(operatorNode); parent->val = op;
		addChildFromPtr(parent, left); addChildFromPtr(parent, right); left = parent;
	} return left;
}

node* parseIf(){
	node* ifNode = addNode(conditionalNode);
	addChildFromPtr(ifNode, parseExpression(0));
	addChildFromPtr(ifNode, parseBody());
	if(peekToken().type == keywordElse){
		addChildFromPtr(ifNode, parseBody());
	} ifNode->val = (token){.type = keywordIf};
} // includes else parsing

node* parseWhile(){
	node* whileNode = addNode(conditionalNode);
	addChildFromPtr(whileNode, parseExpression(0));
	addChildFromPtr(whileNode, parseBody());
	whileNode->val = (token){.type = keywordWhile};
} 

node* parseBody(){
// parse until located } or EOF
	node* ret = addNode(bodyNode); token ct;
	while(ct = peekToken(), !(ct.type == nullToken | ct.type == curlyBraceR)){
		token ft = eatToken();
		switch(ft.type){
			case curlyBraceL: addChildFromPtr(ret, parseBody()); break;
			case keywordIf: addChildFromPtr(ret, parseIf()); break;
			case keywordWhile: addChildFromPtr(ret, parseWhile()); break;
			default: addChildFromPtr(ret, parseExpression(0));
		}
	}eatToken(); return ret;
}

node constructTree(tokenArray arr){
	srcArr = arr; tokensScanned = 0;
}

