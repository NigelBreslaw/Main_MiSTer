#include "menu_path.h"
#include "layout.h"

#include <string.h>

const char *magik_latch_menu_path()
{
	static char path[512] = {};
	if (!path[0]) magik_app_path(path, sizeof(path), "fpga/menu-magik-vblank-latch.rbf");
	return path;
}

const char *magik_menu_browser_path_override(const char *rbf_path)
{
	return rbf_path && !strcmp(rbf_path, magik_latch_menu_path()) ? "menu.rbf" : NULL;
}
