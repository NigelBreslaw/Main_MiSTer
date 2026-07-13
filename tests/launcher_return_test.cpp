#include <assert.h>
#include <stdlib.h>

#include "support/mister_magik/launcher_return.h"

int main()
{
	unsetenv("MISTER_MAGIK_RETURN_TO_LAUNCHER");
	assert(!magik_launcher_mark_latch_menu_return(""));
	assert(!magik_launcher_consume_return_spawn("menu-magik-vblank-latch.rbf", "/media/fat/mister-magik/experiments/menu-magik-vblank-latch.rbf"));

	assert(magik_launcher_mark_latch_menu_return("menu.rbf"));
	assert(magik_launcher_consume_return_spawn("menu-magik-vblank-latch.rbf", "/media/fat/mister-magik/experiments/menu-magik-vblank-latch.rbf"));
	assert(!getenv("MISTER_MAGIK_RETURN_TO_LAUNCHER"));
	assert(!magik_launcher_consume_return_spawn("menu-magik-vblank-latch.rbf", "/media/fat/mister-magik/experiments/menu-magik-vblank-latch.rbf"));

	assert(magik_launcher_consume_return_spawn("menu.rbf", "/media/fat/menu.rbf"));
	return 0;
}
