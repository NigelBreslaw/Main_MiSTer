#include "neogeo_memory.h"

bool magik_neogeo_graphics_memory_warning_needed(
    uint16_t ram_sz,
    bool dual_sdr,
    bool digital_io,
    uint32_t crom_start,
    uint32_t crom_sz_max)
{
	uint32_t start = crom_start < 0x300000 ? 0x300000 : crom_start;
	uint32_t crom_max = start + crom_sz_max;
	return ((!dual_sdr || !digital_io) &&
	        ((ram_sz == 2 && crom_max > 0x4000000) ||
	         (ram_sz == 1 && crom_max > 0x2000000) ||
	         !ram_sz));
}
