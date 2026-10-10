#include "session_main.h"

#include <stdlib.h>
#include <string.h>

static const char s_session_main_env[] = "MISTER_MAGIK_SESSION_MAIN";

bool magik_session_keeps_main(const char *current_exe)
{
	const char *session_main = getenv(s_session_main_env);
	// Deliberately exact, unlike upstream's case-insensitive main= comparison:
	// a mismatch only ends the session by following MiSTer.ini as usual.
	return session_main && session_main[0] == '/' && current_exe &&
	       !strcmp(session_main, current_exe);
}
