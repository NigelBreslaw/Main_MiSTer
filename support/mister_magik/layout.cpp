#include "layout.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

bool magik_dev_layout()
{
	static int cached = -1;
	if (cached >= 0) return cached != 0;
	char path[PATH_MAX] = {};
#ifdef __APPLE__
	uint32_t size = sizeof(path);
	if (_NSGetExecutablePath(path, &size)) path[0] = 0;
#else
	ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
	if (length > 0) path[length] = 0;
#endif
	const char *name = strrchr(path, '/');
	name = name ? name + 1 : path;
	cached = !strcmp(name, "MiSTer_MagiKDev");
	return cached != 0;
}

const char *magik_app_dir()
{
	return magik_dev_layout() ? "/media/fat/mister-magik-dev" : "/media/fat/mister-magik";
}

const char *magik_main_path()
{
	return magik_dev_layout() ? "/media/fat/MiSTer_MagiKDev" : "/media/fat/MiSTer_MagiK";
}

const char *magik_launcher_relative_path()
{
	return magik_dev_layout()
	    ? "mister-magik-dev/mister-magik-fb"
	    : "mister-magik/mister-magik-fb";
}

void magik_app_path(char *out, size_t size, const char *relative)
{
	snprintf(out, size, "%s/%s", magik_app_dir(), relative);
}
