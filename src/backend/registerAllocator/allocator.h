typedef struct{
	uint32_t startAddr, endAddr;
}liveRange;

void createGraph(liveRange* ranges, const uint16_t numRanges);