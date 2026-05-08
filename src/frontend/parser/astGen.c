#include "astGen.h"
#include "../../helper/arenaAlloc.h"

/*
arena allocator for nodes, start with body node. child nodes will be allocated in linked list type of form.
allowing for sibling nodes to be children of the same parent and many children from one parent, no reallocation
*/

const char* nodeNames[] = {
    [bodyNode] = "bodyNode", [operatorNode] = "operatorNode", 
	[conditionalNode] = "conditionalNode", [literalNode] = "literalNode", 
	[identifierNode] = "identifierNode", [declarationNode] = "declarationNode", 
	[castNode] = "castNode", [funcDefNode] = "funcDefNode", [funcCallNode] = "funcCallNode"
};

const char* tokenNames[] = {
    [opPlus] = "opPlus", [opMinus] = "opMinus", [opIncrement] = "opIncrement",
	[opDecrement] = "opDecrement", [opEqual] = "opEqual", [keywordCharPtr] = "keywordCharPtr",
    [opMul] = "opMul", [opDiv] = "opDiv", [opLogicalOr] = "opLogicalOr",
    [opLogicalAnd] = "opLogicalAnd", [opLogicalNot] = "opLogicalNot",
    [opBitwiseNot] = "opBitwiseNot", [opBitwiseOr] = "opBitwiseOr",
    [opShiftRight] = "opShiftRight", [opShiftLeft] = "opShiftLeft",
    [opBitwiseAnd] = "opBitwiseAnd", [opDereference] = "opDereference",
    [opReference] = "opReference", [opCmpEquals] = "opCmpEquals",
    [opCmpGreater] = "opCmpGreater", [opCmpLess] = "opCmpLess",
    [opCmpGrEq] = "opCmpGrEq", [opCmpLeEq] = "opCmpLeEq",
    [curlyBraceR] = "curlyBraceR", [curlyBraceL] = "curlyBraceL",
    [parenthesesL] = "parenthesesL", [parenthesesR] = "parenthesesR",
    [keywordIf] = "keywordIf", [keywordElse] = "keywordElse",
    [keywordWhile] = "keywordWhile", [keywordInt] = "keywordInt",
    [keywordChar] = "keywordChar", [keywordIntPtr] = "keywordIntPtr",
    [endStatement] = "endStatement", [identifier] = "identifier",
    [literal] = "literal", [nullToken] = "nullToken"
};

#define nodeStep 2048

arena nodePool; jmp_buf compRetEnv;

void initPool(){nodePool = newArena(nodeStep * sizeof(node));}

node constructNode(const nodeType type){
	return (node){.type = type, .val = (token){.type = nullToken}, .firstChild = NULL, .lastChild = NULL, .sibling = NULL};
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
token peekAdvToken(const uint8_t i){return tokensScanned+i >= srcArr.numTokens ? (token){.type = nullToken} : *(srcArr.tokens+i);}
token eatToken(){return tokensScanned++ >= srcArr.numTokens ? (token){.type = nullToken} : *(srcArr.tokens++);}
token expect(const uint8_t type){if(peekToken().type == type){return eatToken();} else{longjmp(compRetEnv, 1);}}

node* parseBody(); node* parseExpression(const uint16_t minPrecedence); node* parseFuncCall(const token t);

node* parseArgument(){
	token t = eatToken(); node* n;
	switch(t.type){
		case literal: n = addNode(literalNode); break;
		case identifier: switch(peekToken().type){
			case parenthesesL: n = parseFuncCall(t); eatToken(); return n;
			default: n = addNode(identifierNode);
		} break;
		case keywordInt: case keywordChar:
		if(peekToken().type == opMul){uint8_t pd = 0; while(peekToken().type == opMul){eatToken(); pd++;}t.type = t.type == keywordInt ?  keywordIntPtr : keywordCharPtr; t.val = pd;}		
		n = addNode(declarationNode); n->val = t; t = eatToken(); addChild(n, (node){.type = identifierNode, .val = t}); return n;
		case parenthesesL: if(const uint8_t tt = peekToken().type; (tt == keywordInt || tt == keywordChar)){token t1 = eatToken(); t1.val = 0; 
		while(peekToken().type == opMul){eatToken(); if(!t1.val)t1.type = t1.type == keywordInt ? keywordIntPtr : keywordCharPtr; t1.val++;}
		eatToken(); n = addNode(castNode); n->val = t1; addChildFromPtr(n, parseArgument()); return n;}
		n = parseExpression(0); expect(parenthesesR); return n;
		case opMinus: case opMul: case opBitwiseAnd:
		n = addNode(operatorNode); addChildFromPtr(n, parseExpression(110)); break;
		default:;
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
        case opEqual: case opIncrement: case opDecrement: return 10;
        default: return 0;
    }
}

node* parseFuncCall(const token t){
	node* fcNode = addNode(funcCallNode);
	fcNode->val = t; expect(parenthesesL);
	while(peekToken().type != parenthesesR){
		addChildFromPtr(fcNode, parseExpression(0)); if(peekToken().type == parenthesesR) break; expect(endStatement);
	}return fcNode;
}

token peekOperator(){
	token ret = peekToken(); const token tc = ret;
	switch(peekAdvToken(1).type){
		case opEqual: switch(ret.type){
			case opEqual: ret.type = opCmpEquals; break;
			case opPlus: ret.type = opIncrement; break;
			case opMinus: ret.type = opDecrement; break;
			case opCmpGreater: ret.type = opCmpGrEq; break;
			case opCmpLess: ret.type = opCmpLeEq; break;
		} 
	} if(ret.type != tc.type) eatToken(); return ret;
}

node* parseExpression(const uint16_t minPrecedence){
	node* left = parseArgument();
	while(1){
		token op = peekOperator(); const uint16_t p = getPrecedence(op.type);
		if(p < minPrecedence || !p) break;
		eatToken();
		node* right = parseExpression(p+1);
		node* parent = addNode(operatorNode); parent->val = op;
		addChildFromPtr(parent, left); addChildFromPtr(parent, right); left = parent;
	} return left;
}

node* parseIf(){
	node* ifNode = addNode(conditionalNode);
	addChildFromPtr(ifNode, parseExpression(0)); expect(curlyBraceL); // eat curly to prevent double nested body stuff
	addChildFromPtr(ifNode, parseBody()); expect(curlyBraceR);
	if(peekToken().type == keywordElse){ eatToken(); expect(curlyBraceL);
		addChildFromPtr(ifNode, parseBody());
	} ifNode->val = (token){.type = keywordIf}; expect(curlyBraceR);
	return ifNode;
} // includes else parsing

node* parseWhile(){
	node* whileNode = addNode(conditionalNode);
	addChildFromPtr(whileNode, parseExpression(0));
	addChildFromPtr(whileNode, parseBody());
	whileNode->val = (token){.type = keywordWhile};
	return whileNode;
} 

node* parseFuncDef(){
	node* fdNode = addNode(funcDefNode);
	node n = constructNode(literalNode);if(peekAdvToken(1).type == opMul){const uint8_t t = eatToken().type; srcArr.tokens->type = t == keywordInt ? keywordIntPtr : keywordCharPtr;}
	n.val = eatToken();
	addChild(fdNode, n);
	fdNode->val = eatToken(); eatToken();
	while(peekToken().type != parenthesesR){addChildFromPtr(fdNode, parseArgument()); if(peekToken().type == endStatement) eatToken();}
	eatToken(); eatToken();
	addChildFromPtr(fdNode, parseBody());
	return fdNode;
}

node* parseBody(){
// parse until located } or EOF 
	node* ret = addNode(bodyNode); token ct;
	while(ct = peekToken(), !(ct.type == nullToken | ct.type == curlyBraceR)){
		switch(ct.type){
			case curlyBraceL: eatToken(); addChildFromPtr(ret, parseBody()); continue;
			case keywordIf: eatToken(); addChildFromPtr(ret, parseIf()); continue;
			case keywordWhile: eatToken(); addChildFromPtr(ret, parseWhile()); continue;
			case keywordInt: case keywordChar: uint8_t pd = 0; if(peekAdvToken(1).type == opMul) pd = 1;
			while(peekAdvToken(pd+2).type == opMul){pd++;} if(peekAdvToken(2+pd).type == parenthesesL && peekAdvToken(1+pd).type == identifier){addChildFromPtr(ret, parseFuncDef()); continue;}
			default: addChildFromPtr(ret, parseExpression(0));
		} eatToken();
	}eatToken(); return ret;
}

node constructTree(tokenArray arr){
	tokensScanned = 0;
	srcArr = arr; initPool();
	node* b = parseBody();
	return *b;
}

void printTree(node* n, int depth) {
    if (!n) return;
    for (int i = 0; i < depth; i++) printf("  ");
    const char* nName = (n->type <= declarationNode) ? nodeNames[n->type] : "UNKNOWN_NODE";
    const char* tName = (n->val.type <= nullToken) ? tokenNames[n->val.type] : "UNKNOWN_TOKEN";

    printf("[%s | %s", nName, tName);

    printf("]\n");
    fflush(stdout);
    node* child = n->firstChild;
    while (child) {
        printTree(child, depth + 1);
        child = child->sibling;
    }
}

void setJmpBuf(jmp_buf f){memcpy(compRetEnv, f, sizeof(jmp_buf));}