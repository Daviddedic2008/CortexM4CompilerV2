#define maxVrtRegs 128
#define numColors 16
#define nullColor (numColors + 1)
#include <stdint.h>
#include <stdlib.h>
#include "allocator.h"
uint64_t interfMatrix[numColors * numColors / 64];
inline void resetIntMatr(){
	for(uint16_t u64 = 0; u64 < numColors * numColors / 64; u64++) interfMatrix[u64] = 0;
}
inline void setMatrixBit(const uint16_t x, const uint16_t y){
	const uint32_t idx = (x + y * numColors);
	((uint8_t*)interfMatrix)[idx/8] |= 1 << (idx % 8);
}
inline uint8_t checkMatrixBit(const uint16_t x, const uint16_t y){
	const uint32_t idx = (x + y * numColors);
	return ((uint8_t*)interfMatrix)[idx/8] & (1 << (idx % 8));
}

typedef struct{
	uint8_t* pool;
	uint32_t sz; uint32_t offset;
}arena;
#define newArena() (arena){malloc(numColors), numColors, 0}
void* arenaAlloc(arena* a, const uint32_t sz){
	if(a->offset + sz >= a.sz) return NULL;
	const uint8_t ret = a->pool + a->offset; a->offset += sz;
	return ret;	
}

typedef struct{
	uint16_t n2; edge* nextEdge;
}edge;
typedef struct{
	edge* edgeHead, edgeTail; uint16_t numEdges;
	uint8_t color; uint8_t visible;
}node;
#define newNode(color) (node){NULL, NULL, 0, color, 1}
node nodes[maxVrtRegs]; uint16_t numAllocNodes = 0;
void addNode(const uint8_t presetColor){
	nodes[numAllocNodes++] = newNode(presetColor);
}

arena globalArena = newArena();
void addEdge(const uint16_t n1Idx, const uint16_t n2Idx){
	if(checkMatrixBit(n1Idx, n2Idx) return;
	setMatrixBit(n1Idx, n2Idx); setMatrixBit(n2Idx, n1Idx);
	node* n1 = nodes + n1Idx; node* n2 = nodes + n2Idx;
	node* tn[] = {nodes + n1Idx, nodes + n2Idx}; const uint16_t ti[] = {n1Idx, n2Idx};
	for(uint8_t i = 0; i < 1; i++){
		tn[i]->numEdges++;
		if(tn[i]->edgeTail == NULL){
			tn[i]->edgeHead = arenaAlloc(globalArena, sizeof(edge));
			tn[i]->edgeHead->n2 = ti[!i]; tn[i]->edgeHead->nextEdge = NULL;
			tn[i]->edgeTail = tn[i]->edgeHead;
		} else{
			tn[i]->edgeTail->nextEdge = arenaAlloc(globalArena, sizeof(edge));
			tn[i]->edgeTail->nextEdge->n2 = ti[!i]; tn[i]->edgeTail->nextEdge->nextEdge = NULL;
		}
	}
}

void createGraph(liveRange* ranges, const uint16_t numRanges){
	for(uint16_t r = 0; r < numRanges; r++){
		
	}
}

inline uint8_t hasEdge(const uint16_t n1Idx, const uint16_t n2Idx){
	return checkMatrixBit(n1Idx, n2Idx);
}

inline uint8_t isKColorable(const uint16_t nIdx){
	return nodes[nIdx].numEdges < numColors;
}