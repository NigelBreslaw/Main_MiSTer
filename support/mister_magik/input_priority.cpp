#include "input_priority.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

static bool read_nice(int *nice)
{
	errno = 0;
	*nice = getpriority(PRIO_PROCESS, 0);
	return errno == 0;
}

static bool write_nice(int nice)
{
	return setpriority(PRIO_PROCESS, 0, nice) == 0;
}

static void report_failure(const char *operation)
{
	// Report each operation independently so an earlier failure cannot hide restoration failure.
	static bool reported[3] = {};
	const unsigned index = !strcmp(operation, "restore") ? 2 : !strcmp(operation, "apply") ? 1 : 0;
	if (!reported[index]) fprintf(stderr, "MiSTer MagiK input priority: %s failed (errno=%d)\n", operation, errno);
	reported[index] = true;
}

MagikInputPriority::MagikInputPriority()
	: MagikInputPriority(MagikInputPriorityOps{read_nice, write_nice, report_failure})
{
}

MagikInputPriority::MagikInputPriority(const MagikInputPriorityOps &ops) : ops_(ops)
{
	if (!ops_.read(&previous_))
	{
		ops_.failed("read");
		return;
	}
	if (previous_ == -20) return;
	changed_ = ops_.write(-20);
	if (!changed_) ops_.failed("apply");
}

MagikInputPriority::~MagikInputPriority()
{
	if (changed_ && !ops_.write(previous_)) ops_.failed("restore");
}
