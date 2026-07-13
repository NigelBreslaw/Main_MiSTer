#include "button_overrides.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void trim(char *s)
{
	char *p = s;
	while (*p && isspace((unsigned char)*p)) p++;
	if (p != s) memmove(s, p, strlen(p) + 1);

	size_t len = strlen(s);
	while (len && isspace((unsigned char)s[len - 1])) s[--len] = 0;
}

static const char *canonical_button(const char *value)
{
	if (!strcasecmp(value, "A")) return "A";
	if (!strcasecmp(value, "B")) return "B";
	if (!strcasecmp(value, "X")) return "X";
	if (!strcasecmp(value, "Y")) return "Y";
	if (!strcasecmp(value, "L")) return "L";
	if (!strcasecmp(value, "R")) return "R";
	if (!strcasecmp(value, "Select")) return "Select";
	if (!strcasecmp(value, "Start")) return "Start";
	return NULL;
}

void magik_button_overrides_clear(char values[][32], bool unmap[], int count)
{
	for (int i = 0; i < count; i++)
	{
		values[i][0] = 0;
		unmap[i] = false;
	}
}

int magik_button_overrides_load(const char *path, char values[][32], bool unmap[], int count)
{
	magik_button_overrides_clear(values, unmap, count);

	FILE *f = fopen(path, "r");
	if (!f) return 0;

	int loaded = 0;
	char line[96];
	while (fgets(line, sizeof(line), f))
	{
		trim(line);
		if (!line[0] || line[0] == '#') continue;
		if (!strncasecmp(line, "schema=", 7)) continue;

		char *eq = strchr(line, '=');
		if (!eq) continue;
		*eq = 0;
		char *key = line;
		char *value = eq + 1;
		trim(key);
		trim(value);

		char *end = NULL;
		long idx = strtol(key, &end, 10);
		if (!key[0] || (end && *end) || idx < 0 || idx >= count) continue;

		if (!strcasecmp(value, "unmap"))
		{
			values[idx][0] = 0;
			unmap[idx] = true;
			loaded++;
			continue;
		}

		const char *button = canonical_button(value);
		if (!button) continue;

		snprintf(values[idx], 32, "%s", button);
		unmap[idx] = false;
		loaded++;
	}
	fclose(f);
	return loaded;
}
