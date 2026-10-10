#include <assert.h>
#include <stdlib.h>

#include "support/mister_magik/session_main.h"

int main()
{
	unsetenv("MISTER_MAGIK_SESSION_MAIN");
	assert(!magik_session_keeps_main("/media/fat/MiSTer_MagiKDev"));

	setenv("MISTER_MAGIK_SESSION_MAIN", "/media/fat/MiSTer_MagiKDev", 1);
	assert(magik_session_keeps_main("/media/fat/MiSTer_MagiKDev"));
	assert(!magik_session_keeps_main("/media/fat/MiSTer"));
	assert(!magik_session_keeps_main("/media/fat/MiSTer_MagiK"));
	assert(!magik_session_keeps_main(NULL));

	setenv("MISTER_MAGIK_SESSION_MAIN", "MiSTer_MagiKDev", 1);
	assert(!magik_session_keeps_main("MiSTer_MagiKDev"));
	setenv("MISTER_MAGIK_SESSION_MAIN", "", 1);
	assert(!magik_session_keeps_main(""));
	return 0;
}
