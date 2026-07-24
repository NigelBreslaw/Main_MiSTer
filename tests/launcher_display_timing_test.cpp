#include "support/mister_magik/launcher_display_timing.h"

#include <assert.h>

int main()
{
	assert(MAGIK_DISPLAY_CONFIRM_TIMEOUT_MS == 20000);
	assert(MAGIK_DISPLAY_FALLBACK_TIMEOUT_MS == 30000);
	assert(MAGIK_DISPLAY_FALLBACK_TIMEOUT_MS > MAGIK_DISPLAY_CONFIRM_TIMEOUT_MS);
	return 0;
}
