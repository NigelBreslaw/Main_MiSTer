#include <assert.h>
#include <string.h>
#include "support/mister_magik/launcher_diag.h"

int main()
{
	MagikLauncherInvariant event;
	magik_launcher_invariant_init(&event, "unexpected_osd_call_while_launcher_owned", "OsdUpdate");
	assert(!strcmp(event.kind, "unexpected_osd_call_while_launcher_owned"));
	assert(!strcmp(event.detail, "OsdUpdate"));

	magik_launcher_invariant_init(&event, 0, 0);
	assert(!strcmp(event.kind, "launcher_invariant"));
	assert(!strcmp(event.detail, "unspecified"));

	char long_detail[400];
	memset(long_detail, 'x', sizeof(long_detail));
	long_detail[sizeof(long_detail) - 1] = 0;
	magik_launcher_invariant_init(&event, "k", long_detail);
	assert(strlen(event.detail) == sizeof(event.detail) - 1);
	assert(event.detail[sizeof(event.detail) - 1] == 0);

	return 0;
}
