#include "support/mister_magik/input_priority.h"
#include <assert.h>
#include <string.h>
#include <vector>

static int current_nice;
static bool read_ok, write_ok;
static std::vector<int> writes;
static std::vector<const char *> failures;
static bool read_priority(int *nice) { *nice = current_nice; return read_ok; }
static bool write_priority(int nice) {
	writes.push_back(nice);
	if (write_ok) current_nice = nice;
	return write_ok;
}
static void failed(const char *operation) { failures.push_back(operation); }
static const MagikInputPriorityOps ops = {read_priority, write_priority, failed};
static void reset(int nice) {
	current_nice = nice; read_ok = write_ok = true; writes.clear(); failures.clear();
}

int main()
{
	for (int original : {0, -10, 5}) {
		reset(original);
		{
			MagikInputPriority priority(ops);
			assert(current_nice == -20);
		}
		assert(current_nice == original);
		assert((writes == std::vector<int>{-20, original}));
		assert(failures.empty());
	}
	reset(-20);
	{ MagikInputPriority priority(ops); }
	assert(writes.empty());
	reset(0); read_ok = false;
	{ MagikInputPriority priority(ops); }
	assert(writes.empty() && failures.size() == 1 && !strcmp(failures[0], "read"));
	reset(0); write_ok = false;
	{ MagikInputPriority priority(ops); }
	assert(writes.size() == 1 && current_nice == 0 && !strcmp(failures[0], "apply"));
	reset(0);
	{ MagikInputPriority priority(ops); write_ok = false; }
	assert(failures.size() == 1 && !strcmp(failures[0], "restore"));
	return 0;
}
