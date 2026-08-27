#include "support/mister_magik/launcher_display_timing.h"

#include <assert.h>

int main()
{
	assert(MAGIK_DISPLAY_CONFIRM_TIMEOUT_MS == 20000);
	assert(MAGIK_DISPLAY_FALLBACK_TIMEOUT_MS == 30000);
	assert(MAGIK_DISPLAY_FALLBACK_TIMEOUT_MS > MAGIK_DISPLAY_CONFIRM_TIMEOUT_MS);
	const MagikDisplayDeadlines deadlines = magik_display_confirmation_deadlines(1234);
	assert(deadlines.confirmation_ms == 21234);
	assert(deadlines.fallback_ms == 31234);
	assert(deadlines.fallback_ms - deadlines.confirmation_ms == 10000);
	return 0;
}
