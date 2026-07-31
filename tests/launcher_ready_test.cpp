#include <assert.h>
#include <string.h>

#include "support/mister_magik/launcher_ready.h"

static void expect_steps(
	const MagikLauncherReadyRecoveryPlan &plan,
	const MagikLauncherReadyRecoveryStep *expected,
	unsigned int count)
{
	assert(plan.count == count);
	for (unsigned int i = 0; i < count; i++) assert(plan.steps[i] == expected[i]);
}

int main()
{
	MagikLauncherReadyReport report = {};
	assert(magik_launcher_parse_ready(
		"ready-v1 token=0123456789abcdef0123456789abcdef pid=42",
		&report));
	assert(!strcmp(report.token, "0123456789abcdef0123456789abcdef"));
	assert(report.pid == 42);

	const char *invalid[] = {
		"ready-v1 token=bad pid=42",
		"ready-v1 token=0123456789abcdef0123456789abcdef pid=0",
		"ready-v1 token=0123456789abcdef0123456789abcdeg pid=42",
		"ready-v1 token=0123456789abcdef0123456789abcdef pid=42 extra",
		" ready-v1 token=0123456789abcdef0123456789abcdef pid=42",
		"ready-v1  token=0123456789abcdef0123456789abcdef pid=42",
		"ready-v1 token=0123456789abcdef0123456789abcdef pid=042",
		"other-v1 token=0123456789abcdef0123456789abcdef pid=42",
	};
	for (const char *line : invalid)
		assert(!magik_launcher_parse_ready(line, &report));
	assert(magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 42));
	assert(!magik_launcher_ready_matches(report, "1123456789abcdef0123456789abcdef", 42));
	assert(!magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 43));

	assert(!magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Idle, 8000, 8000));
	assert(!magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Awaiting, 7999, 8000));
	assert(magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Awaiting, 8000, 8000));
	assert(magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Awaiting, 8001, 8000));
	assert(!magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Ready, 8001, 8000));

	using Step = MagikLauncherReadyRecoveryStep;
	const Step first_failure[] = {Step::StopChild, Step::Retry};
	MagikLauncherReadyRecoveryPlan first = magik_launcher_ready_recovery_plan(1, true);
	expect_steps(first, first_failure, 2);
	assert(first.next_attempt == 2);

	const Step final_display_failure[] = {
		Step::StopChild,
		Step::RollbackDisplay,
		Step::RestoreStockMenu,
		Step::FinishPendingReply,
	};
	MagikLauncherReadyRecoveryPlan second =
		magik_launcher_ready_recovery_plan(first.next_attempt, true);
	expect_steps(second, final_display_failure, 4);
	assert(second.next_attempt == 2);

	const Step final_startup_failure[] = {
		Step::StopChild,
		Step::RestoreStockMenu,
		Step::FinishPendingReply,
	};
	expect_steps(
		magik_launcher_ready_recovery_plan(first.next_attempt, false),
		final_startup_failure,
		3);
	expect_steps(magik_launcher_ready_recovery_plan(0, false), final_startup_failure, 3);
	return 0;
}
