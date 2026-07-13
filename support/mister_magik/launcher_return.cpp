#include "launcher_return.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char s_return_env[] = "MISTER_MAGIK_RETURN_TO_LAUNCHER";

static bool is_menu_path(const char *path)
{
	return path && (!strcasecmp(path, "menu.rbf") || !strcasecmp(path, "/media/fat/menu.rbf"));
}

bool magik_launcher_mark_latch_menu_return(const char *path)
{
	if (!is_menu_path(path)) return false;
	setenv(s_return_env, "1", 1);
	return true;
}

bool magik_launcher_consume_return_spawn(const char *rbf_name, const char *rbf_path)
{
	const char *marked = getenv(s_return_env);
	bool return_spawn = is_menu_path(rbf_name) || is_menu_path(rbf_path) || (marked && !strcmp(marked, "1"));
	unsetenv(s_return_env);
	return return_spawn;
}
