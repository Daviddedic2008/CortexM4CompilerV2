#include "3opIrGen.h"
#include <stdint.h>
#include "../../helper/arenaAlloc.h"
#include <stdlib.h>

/*
descend to bottom left fo ast, parsing along the way.
for each node, attempt to transform into 3ac. if theres children linearize them first left to right
if a child is another operator or something and u need it for an argument, use most recent vReg.
*/

#define nullSymbol ((symbol){.type = invalidSymbol, .vReg = -1})

uint32_t numQuads; arena quadPool;

const char* op_names[] = {
    "ADD", "SUB", "MUL", "DIV", "AND", "NOT", "OR", "XOR",
    "LOAD", "STORE", "MOV", "LOADIMM",
    "CMP", "JMP", "JMPCND",
    "CALL", "ARG", "FNCDEF", "RET",
    "REF", "DEREF",
    "PUSH", "POP"
};

void printSymbol(symbol s) {
    switch (s.type) {
        case physical:
            if (s.vReg == 31)      printf("sp");
            else if (s.vReg == 30) printf("lr");
            else if (s.vReg == 29) printf("fp");
            else                   printf("x%d", s.vReg);
            break;
        case local:
            printf("loc_off(%d)", s.vReg);
            break;
        case global:
            printf("global_id(%d)", s.vReg);
            break;
        case arg:
            printf("arg_%d", s.vReg);
            break;
        case literalSymbol:
            printf("#%d", s.vReg);
            break;
        case invalidSymbol:
            printf("???");
            break;
        default:
            // Assuming default/other values are virtual registers for temps
            printf("v%d", s.vReg);
            break;
    }
}

void printQuad(quad q) {
    // Print the instruction index for jump-tracking

    switch (q.op) {
        /* --- 3-Operand Arithmetic --- */
        case ADD: case SUB: case MUL: case DIV:
        case AND: case OR:  case XOR:
            printSymbol(q.o1);
            printf(" = ");
            printSymbol(q.o2);
            printf(" %s ", op_names[q.op]);
            printSymbol(q.o3);
            break;

        /* --- Memory & Assignment --- */
        case MOV: case LOADIMM: case LOAD: case DEREF: case REf:
            printSymbol(q.o1);
            printf(" = %s ", op_names[q.op]);
            printSymbol(q.o2);
            break;

        case STORE:
            printf("STORE ");
            printSymbol(q.o2); // Value
            printf(" -> [");
            printSymbol(q.o1); // Destination Address
            printf("]");
            break;

        /* --- Control Flow --- */
        case JMP:
            printf("JMP LABEL_%d", q.o1.vReg);
            break;

        case JMPCND:
            printf("IF ");
            printSymbol(q.o2);
            printf(" JMP LABEL_%d", q.o1.vReg);
            break;

        case CMP:
            printf("CMP ");
            printSymbol(q.o1);
            printf(", ");
            printSymbol(q.o2);
            break;

        /* --- Functions --- */
        case FNCDEF:
            printf("\n--- FUNC DEF ---");
            break;

        case RET:
            printf("RET ");
            if (q.o1.type != invalidSymbol) printSymbol(q.o1);
            break;

        case CALL:
            if (q.o1.type != invalidSymbol) {
                printSymbol(q.o1);
                printf(" = ");
            }
            printf("CALL ");
            printSymbol(q.o2);
            break;

        case ARG:
            printf("PARAM ");
            printSymbol(q.o1);
            break;

        /* --- Stack Operations --- */
        case PUSH: case POP:
            printf("%s ", op_names[q.op]);
            printSymbol(q.o1);
            break;

        case NOT:
            printSymbol(q.o1);
            printf(" = NOT ");
            printSymbol(q.o2);
            break;

        default:
            printf("UNKNOWN_OP(%d)", q.op);
            break;
    }
    printf("\n");
}

void emitQuad(const quad q){
	printQuad(q);
	writeElement(&quadPool, &q, sizeof(quad));
	numQuads++;
}

int32_t curTempVReg;

const operation operationMap[] = {
	[opPlus] = ADD, [opMinus] = SUB, [opMul] = MUL, [opDiv] = DIV
};

uint32_t* fncJmpLabels; uint32_t fncsEncountered;

symbol linearizeNode(node* n, symbol targetReg){
	switch(n->type){
		case identifierNode: 
		if(targetReg.vReg != -1) emitQuad((quad){.op = MOV, .o1 = targetReg, .o2 = n->symbolData});
		return n->symbolData;
		case literalNode:{const symbol tmpLit = (symbol){.type = literalSymbol, .vReg = n->val.val}; if(targetReg.vReg != -1) 
		emitQuad((quad){.op = LOADIMM, .o1 = targetReg, .o2 = tmpLit}); 
		return tmpLit;
		}
		case operatorNode:{switch(n->val.type){
			case opEqual:
			const symbol resultReg = linearizeNode(n->firstChild, nullSymbol);
			linearizeNode(n->firstChild->sibling, resultReg);
			return resultReg;
			case opPlus: case opMinus: case opMul: case opDiv:{
				const symbol o1 = linearizeNode(n->firstChild, nullSymbol);
				const symbol o2 = linearizeNode(n->firstChild->sibling, nullSymbol);
				if(targetReg.vReg == -1) targetReg.vReg = curTempVReg++;
				emitQuad((quad){.op = operationMap[n->val.type], .o1 = targetReg, .o2 = o1, .o3 = o2});
				return targetReg;
			}
			case opReference: case opDereference: case opBitwiseNot: case opLogicalNot:{
				const symbol o1 = linearizeNode(n->firstChild, nullSymbol);
				if(targetReg.vReg == -1) targetReg.vReg = curTempVReg++;
				emitQuad((quad){.op = operationMap[n->val.type], .o1 = targetReg, .o2 = o1});
				return targetReg;
			}
			case keywordReturn:{
				if(n->firstChild == NULL) return nullSymbol;
				const symbol retV = linearizeNode(n->firstChild, (symbol){.type = physical, .vReg = 0});
				emitQuad((quad){.op = RET, .o1 = retV});
				return retV;
			}
		}
		break;
		}
		case funcDefNode:{
			emitQuad((quad){.op = FNCDEF});
			fncJmpLabels[fncsEncountered++] = numQuads;
			node* cn = n->firstChild->sibling; uint32_t numArgs = 0;
			while(cn != n->lastChild && cn != NULL){
				if(numArgs < 8) emitQuad((quad){.op = MOV, .o1 = cn->symbolData, .o2 = (symbol){.type = physical, .vReg = numArgs}});
				else emitQuad((quad){.op = POP, .o1 = cn->symbolData});
				numArgs++; cn = cn->sibling;
			}
			linearizeNode(n->lastChild, nullSymbol);
			return nullSymbol;
		}
		case funcCallNode:{
			node* cn = n->firstChild->sibling; uint32_t numArgs = 0;
			while(cn != n->lastChild && cn != NULL){
				if(numArgs < 8) emitQuad((quad){.op = MOV, .o2 = cn->symbolData, .o1 = (symbol){.type = physical, .vReg = numArgs}});
				else emitQuad((quad){.op = PUSH, .o1 = cn->symbolData});
				numArgs++; cn = cn->sibling;
			} emitQuad((quad){.op = CALL});
			return (symbol){.type = physical, .vReg = 0};
		}
		case declarationNode:{
			return n->firstChild->symbolData;
		}
		case castNode:{
			node* o1 = n->firstChild;
			token castType = n->firstChild->sibling->val;
			o1->val = castType;
			o1->symbolData.varType = castType.type;
			return o1->symbolData;
		}
		case bodyNode:{
			node* cn = n->firstChild;
			if(cn != NULL) do{
				if(cn == NULL) break;
				linearizeNode(cn, nullSymbol);
				if(cn == n->lastChild) break;
				cn = cn->sibling;
			}while(true);
		}
		default: return nullSymbol;
	}
}

#define maxQuads 2048

quad* linearizeAST(const node* baseNode){
	fncJmpLabels = malloc(sizeof(uint32_t) * getNumFuncs()); numQuads = 0;
	quadPool = newArena(sizeof(quad) * maxQuads);
	curTempVReg = getUsedVRegs(); fncsEncountered = 0;
	linearizeNode(baseNode, nullSymbol);
	return quadPool.pool;
}