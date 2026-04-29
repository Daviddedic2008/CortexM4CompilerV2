#include "arenaAlloc.h"
#include <string.h>

arena newArena(const uint32_t sz){return (arena){.pool = malloc(sz), .allocated = sz, .used = 0};}

void* writeElement(arena* a, const void* data, const uint32_t wrSz){
	if((a->used += wrSz) > a->allocated) return NULL;
	memcpy(a->pool+a->used-wrSz, data, wrSz);
	return a->pool + a->used - wrSz;
}
void* blankElement(arena* a, const uint32_t wrSz){
	if((a->used += wrSz) > a->allocated) return NULL;
	return a->pool + a->used - wrSz;
}