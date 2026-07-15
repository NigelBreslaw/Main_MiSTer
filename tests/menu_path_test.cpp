#include <assert.h>
#include <string.h>

#include "support/mister_magik/menu_path.h"

int main()
{
	const char *latch_path = "/media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf";
	assert(!strcmp(magik_latch_menu_path(), latch_path));
	assert(!strcmp(magik_menu_browser_path_override(latch_path), "menu.rbf"));
	assert(!magik_menu_browser_path_override(NULL));
	assert(!magik_menu_browser_path_override("menu.rbf"));
	assert(!magik_menu_browser_path_override("/media/fat/menu.rbf"));
	assert(!magik_menu_browser_path_override("/media/fat/mister-magik/fpga/menu-magik-vblank-latch-copy.rbf"));
	return 0;
}
