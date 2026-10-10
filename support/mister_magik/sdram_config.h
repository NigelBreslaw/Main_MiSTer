#pragma once

#include <stdint.h>

struct MagikSdramConfig
{
	bool valid;
	uint16_t size_code;
};

inline MagikSdramConfig magik_sdram_config(uint16_t cached, uint16_t osdmask)
{
	if (cached & 0x8000)
		return {true, static_cast<uint16_t>(cached & 0x7fff)};
	if (!(osdmask & 0x8000))
		return {false, 0};
	switch (osdmask & 7)
	{
	case 7: return {true, 3};
	case 3: return {true, 2};
	case 1: return {true, 1};
	default: return {true, 0};
	}
}
