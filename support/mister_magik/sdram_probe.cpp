#include "sdram_probe.h"

bool magik_sdram_cache_valid(uint16_t cached)
{
	return (cached & 0x8000) != 0;
}

uint16_t magik_sdram_size_from_osdmask(uint16_t osdmask)
{
	if (!(osdmask & 0x8000)) return 0;
	switch (osdmask & 7)
	{
	case 7:
		return 3;
	case 3:
		return 2;
	case 1:
		return 1;
	default:
		return 0;
	}
}

uint16_t magik_sdram_ensure_cached_size(uint16_t cached, uint16_t osdmask, bool *probed)
{
	if (magik_sdram_cache_valid(cached))
	{
		if (probed) *probed = false;
		return cached & 0x7fff;
	}
	if (probed) *probed = true;
	return magik_sdram_size_from_osdmask(osdmask);
}
