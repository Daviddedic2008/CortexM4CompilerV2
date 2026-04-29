#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum tokenType : uint8_t{
	opPlus, opMinus, opEqual, opMul, opDiv, opLogicalOr, opLogicalAnd, opLogicalNot, opBitwiseNot, opBitwiseOr, opShiftRight, opShiftLeft, opBitwiseAnd, opDereference, opReference, opCmpEquals, opCmpGreater, opCmpLess, opCmpGrEq, opCmpLeEq,
	curlyBraceR, curlyBraceL, parenthesesL, parenthesesR,
	keywordIf, keywordElse, keywordWhile, keywordInt, keywordChar, keywordPtr,
	endStatement, identifier, literal, nullToken
}tokenType;

typedef struct{
	tokenType type;
	union{
		int val; uint32_t len;
	}; char* str;
}token;

typedef struct{
	token* tokens;
	uint32_t numTokens;
}tokenArray;

tokenArray tokenizeSource(char* src);

void verifyAlphabeticalOrder();
