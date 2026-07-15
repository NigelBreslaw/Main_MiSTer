#include "menu_path.h"

#include <string.h>

static const char s_latch_menu_path[] = "/media/fat/mister-magik/fpga/menu-magik-vblank-latch.rbf";

const char *magik_latch_menu_path()
{
	return s_latch_menu_path;
}

const char *magik_menu_browser_path_override(const char *rbf_path)
{
	return rbf_path && !strcmp(rbf_path, s_latch_menu_path) ? "menu.rbf" : NULL;
}
