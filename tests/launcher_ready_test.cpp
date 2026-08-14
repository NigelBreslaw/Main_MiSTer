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
	const char *valid =
		"ready-v2 token=0123456789abcdef0123456789abcdef pid=42 main_pid=7 main_generation=11 owner_epoch=13 protocol=5 capabilities=03ff base=229e9000 width=960 height=540 stride=1920 first_sequence=1 first_route_epoch=9 first_slot=1 first_receipt_crc=0000 second_sequence=2 second_route_epoch=10 second_slot=2 second_receipt_crc=abcd source_sha256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef source_nonzero=99";
	MagikLauncherReadyReport report = {};
	assert(magik_launcher_parse_ready(valid, &report));
	assert(!strcmp(report.token, "0123456789abcdef0123456789abcdef"));
	assert(report.pid == 42);
	assert(report.owner_epoch == 13);
	assert(report.first_receipt_crc == 0);
	assert(report.second_receipt_crc == 0xabcd);

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
	assert(!magik_launcher_parse_ready((std::string(valid) + " extra").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid, "pid=42", "pid=042").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid, "base=229e9000", "base=100000000").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid, "width=960", "width=70000").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid, "first_sequence=1", "first_sequence=70000").c_str(), &report));
	assert(!magik_launcher_parse_ready(replace_once(valid, "source_nonzero=99", "source_nonzero=999999").c_str(), &report));
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
