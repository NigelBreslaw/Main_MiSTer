#include <assert.h>
#include "support/mister_magik/neogeo_memory.h"

int main()
{
	const uint32_t mslug3_crom_start = 0xb00000;
	const uint32_t mslug3_crom_size = 0x4000000;

	assert(magik_neogeo_graphics_memory_warning_needed(
	    0, false, true, mslug3_crom_start, mslug3_crom_size));
	assert(magik_neogeo_graphics_memory_warning_needed(
	    2, false, true, mslug3_crom_start, mslug3_crom_size));
	assert(!magik_neogeo_graphics_memory_warning_needed(
	    2, true, true, mslug3_crom_start, mslug3_crom_size));
	assert(!magik_neogeo_graphics_memory_warning_needed(
	    3, false, true, mslug3_crom_start, mslug3_crom_size));
	assert(!magik_neogeo_graphics_memory_warning_needed(
	    0, true, true, mslug3_crom_start, mslug3_crom_size));
	return 0;
}
