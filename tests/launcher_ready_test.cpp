#include <assert.h>
#include <string.h>

#include <string>

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
	const char *valid_v3 =
		"ready-v3 token=0123456789abcdef0123456789abcdef pid=42 main_pid=7 main_generation=11 owner_epoch=13 protocol=5 capabilities=03ff base=229e9000 width=960 height=540 stride=1920 first_sequence=1 first_route_epoch=9 first_slot=1 first_receipt_crc=0000 second_sequence=2 second_route_epoch=10 second_slot=2 second_receipt_crc=abcd source_nonblank=1";
	const char *valid_v2 =
		"ready-v2 token=0123456789abcdef0123456789abcdef pid=42 main_pid=7 main_generation=11 owner_epoch=13 protocol=5 capabilities=03ff base=229e9000 width=960 height=540 stride=1920 first_sequence=1 first_route_epoch=9 first_slot=1 first_receipt_crc=0000 second_sequence=2 second_route_epoch=10 second_slot=2 second_receipt_crc=abcd source_sha256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef source_nonzero=99";
	MagikLauncherReadyReport report = {};
	assert(magik_launcher_parse_ready(valid_v3, &report));
	assert(!strcmp(report.token, "0123456789abcdef0123456789abcdef"));
	assert(report.ready_version == 3);
	assert(report.source_nonblank);
	assert(report.pid == 42);
	assert(report.owner_epoch == 13);
	assert(report.first_receipt_crc == 0);
	assert(report.second_receipt_crc == 0xabcd);
	assert(magik_launcher_parse_ready(valid_v2, &report));
	assert(report.ready_version == 2);
	assert(report.source_nonblank);

	const char *invalid[] = {
		"ready-v2 token=bad pid=42",
		"ready-v1 token=0123456789abcdef0123456789abcdef pid=42",
		"ready-v2 token=0123456789abcdef0123456789abcdef pid=42 main_pid=7",
		" ready-v2 token=0123456789abcdef0123456789abcdef pid=42",
	};
	for (const char *line : invalid)
		assert(!magik_launcher_parse_ready(line, &report));
	auto replace_once = [](const char *input, const char *before, const char *after) {
		std::string value(input);
		size_t position = value.find(before);
		assert(position != std::string::npos);
		value.replace(position, strlen(before), after);
		return value;
	};
	assert(!magik_launcher_parse_ready((std::string(valid_v3) + " extra").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v3, "pid=42", "pid=042").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v3, "base=229e9000", "base=100000000").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v3, "width=960", "width=70000").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v3, "first_sequence=1", "first_sequence=70000").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v3, "source_nonblank=1", "source_nonblank=0").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v3, "source_nonblank=1", "source_nonblank=2").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid_v2, "source_nonzero=99", "source_nonzero=999999").c_str(), &report));
	assert(magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 42, 7, 11, 13));
	assert(!magik_launcher_ready_matches(report, "1123456789abcdef0123456789abcdef", 42, 7, 11, 13));
	assert(!magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 43, 7, 11, 13));
	assert(!magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 42, 8, 11, 13));
	assert(!magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 42, 7, 12, 13));
	assert(!magik_launcher_ready_matches(report, "0123456789abcdef0123456789abcdef", 42, 7, 11, 14));

	assert(!magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Idle, 8000, 8000));
	assert(!magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Awaiting, 7999, 8000));
	assert(magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Awaiting, 8000, 8000));
	assert(magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Awaiting, 8001, 8000));
	assert(!magik_launcher_ready_timed_out(MagikLauncherReadyPhase::Ready, 8001, 8000));
	assert(magik_launcher_ready_begin_attempt(0) == 1);
	assert(magik_launcher_ready_begin_attempt(2) == 2);

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

	MagikLauncherReadyPhase terminal_phase = MagikLauncherReadyPhase::Failed;
	unsigned int terminal_attempt = second.next_attempt;
	unsigned long terminal_deadline_ms = 9000;
	magik_launcher_ready_rearm_after_terminal_failure(
		&terminal_phase,
		&terminal_attempt,
		&terminal_deadline_ms);
	assert(terminal_phase == MagikLauncherReadyPhase::Idle);
	assert(terminal_attempt == 0);
	assert(terminal_deadline_ms == 0);

	// The next independent entry starts a new attempt 1 and once again gets
	// exactly one fresh-child retry. Terminal recovery itself never loops.
	unsigned int later_attempt = magik_launcher_ready_begin_attempt(terminal_attempt);
	assert(later_attempt == 1);
	MagikLauncherReadyRecoveryPlan later_first_failure =
		magik_launcher_ready_recovery_plan(later_attempt, false);
	expect_steps(later_first_failure, first_failure, 2);
	assert(later_first_failure.next_attempt == 2);
	expect_steps(
		magik_launcher_ready_recovery_plan(later_first_failure.next_attempt, false),
		final_startup_failure,
		3);
	return 0;
}
