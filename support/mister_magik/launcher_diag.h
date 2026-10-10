#pragma once

#include <stddef.h>

struct MagikLauncherInvariant
{
	char kind[96];
	char detail[256];
};

void magik_launcher_invariant_init(
    MagikLauncherInvariant *event,
    const char *kind,
    const char *detail);
