#include <assert.h>
#include "support/mister_magik/sdram_probe.h"

static void assert_ensure(uint16_t cached, uint16_t osdmask, uint16_t expected, bool expected_probe)
{
	bool probed = false;
	assert(magik_sdram_ensure_cached_size(cached, osdmask, &probed) == expected);
	assert(probed == expected_probe);
}

int main()
{
	assert(!magik_sdram_cache_valid(0));
	assert(magik_sdram_cache_valid(0x8002));

	assert(magik_sdram_size_from_osdmask(0x8007) == 3);
	assert(magik_sdram_size_from_osdmask(0x8003) == 2);
	assert(magik_sdram_size_from_osdmask(0x8001) == 1);
	assert(magik_sdram_size_from_osdmask(0x8000) == 0);
	assert(magik_sdram_size_from_osdmask(0x0007) == 0);

	assert_ensure(0, 0x8007, 3, true);
	assert_ensure(0, 0x8003, 2, true);
	assert_ensure(0, 0x8001, 1, true);
	assert_ensure(0, 0x8000, 0, true);
	assert_ensure(0x8002, 0x8007, 2, false);
	return 0;
}
