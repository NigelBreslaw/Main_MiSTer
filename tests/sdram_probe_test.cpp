#include <assert.h>
#include "support/mister_magik/sdram_config.h"

static void assert_config(uint16_t cached, uint16_t osdmask, bool valid, uint16_t size_code)
{
	MagikSdramConfig config = magik_sdram_config(cached, osdmask);
	assert(config.valid == valid);
	assert(config.size_code == size_code);
}

int main()
{
	assert_config(0, 0, false, 0);
	assert_config(0, 0x8007, true, 3);
	assert_config(0, 0x8003, true, 2);
	assert_config(0, 0x8001, true, 1);
	assert_config(0, 0x8000, true, 0);
	assert_config(0x8002, 0x8007, true, 2);
	return 0;
}
