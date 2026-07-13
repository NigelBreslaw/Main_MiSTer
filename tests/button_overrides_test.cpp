#include "support/mister_magik/button_overrides.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
	char path[256];
	snprintf(path, sizeof(path), "/tmp/mister-magik-button-overrides-test-%d", getpid());

	FILE *f = fopen(path, "w");
	assert(f);
	fprintf(f, "schema=1\n");
	fprintf(f, "4=Start\n");
	fprintf(f, "5= select \n");
	fprintf(f, "6=L\n");
	fprintf(f, "8=unmap\n");
	fprintf(f, "bad=R\n");
	fprintf(f, "9=bad-value\n");
	fprintf(f, "99=A\n");
	fclose(f);

	char values[12][32];
	bool unmap[12];
	int loaded = magik_button_overrides_load(path, values, unmap, 12);

	assert(loaded == 4);
	assert(!strcmp(values[4], "Start"));
	assert(!strcmp(values[5], "Select"));
	assert(!strcmp(values[6], "L"));
	assert(values[8][0] == 0);
	assert(unmap[8]);
	assert(values[9][0] == 0);
	assert(!unmap[9]);

	unlink(path);
	return 0;
}
