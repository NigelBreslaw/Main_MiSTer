#include "launcher_diag.h"

#include <stdio.h>

void magik_launcher_invariant_init(
    MagikLauncherInvariant *event,
    const char *kind,
    const char *detail)
{
	if (!event) return;
	snprintf(event->kind, sizeof(event->kind), "%s", kind && kind[0] ? kind : "launcher_invariant");
	snprintf(event->detail, sizeof(event->detail), "%s", detail && detail[0] ? detail : "unspecified");
}
