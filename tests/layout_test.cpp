#include <assert.h>
#include <string.h>

#include "support/mister_magik/layout.h"

int main(int argc, char **argv)
{
	const char *name = argc > 0 ? strrchr(argv[0], '/') : NULL;
	name = name ? name + 1 : (argc > 0 ? argv[0] : "");
	bool dev = !strcmp(name, "MiSTer_MagiKDev");
	assert(magik_dev_layout() == dev);
	assert(!strcmp(
	    magik_app_dir(),
	    dev ? "/media/fat/mister-magik-dev" : "/media/fat/mister-magik"));
	assert(!strcmp(
	    magik_main_path(),
	    dev ? "/media/fat/MiSTer_MagiKDev" : "/media/fat/MiSTer_MagiK"));
	assert(!strcmp(
	    magik_launcher_relative_path(),
	    dev ? "mister-magik-dev/mister-magik-fb" : "mister-magik/mister-magik-fb"));
	char path[256] = {};
	magik_app_path(path, sizeof(path), "platform-v1.manifest");
	assert(!strcmp(
	    path,
	    dev ? "/media/fat/mister-magik-dev/platform-v1.manifest"
	        : "/media/fat/mister-magik/platform-v1.manifest"));
	return 0;
}
