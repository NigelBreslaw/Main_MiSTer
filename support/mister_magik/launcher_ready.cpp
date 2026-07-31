#include "launcher_ready.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char *magik_launcher_ready_phase_name(MagikLauncherReadyPhase phase)
{
	switch (phase)
	{
	case MagikLauncherReadyPhase::Idle: return "idle";
	case MagikLauncherReadyPhase::Awaiting: return "awaiting";
	case MagikLauncherReadyPhase::Ready: return "ready";
	case MagikLauncherReadyPhase::Failed: return "failed";
	}
	return "unknown";
}

bool magik_launcher_parse_ready(const char *line, MagikLauncherReadyReport *report)
{
	if (!line || !report) return false;
	char command[16] = {};
	char token[33] = {};
	unsigned long pid = 0;
	char trailing = 0;
	if (sscanf(line, "%15s token=%32s pid=%lu %c", command, token, &pid, &trailing) != 3)
		return false;
	if (strcmp(command, "ready-v1") || strlen(token) != 32 || !pid)
		return false;
	for (const char *ch = token; *ch; ch++)
		if (!isdigit((unsigned char)*ch) && (*ch < 'a' || *ch > 'f')) return false;
	char expected[96];
	snprintf(expected, sizeof(expected), "ready-v1 token=%s pid=%lu", token, pid);
	if (strcmp(line, expected)) return false;
	snprintf(report->token, sizeof(report->token), "%s", token);
	report->pid = pid;
	return true;
}

bool magik_launcher_ready_matches(
	const MagikLauncherReadyReport &report,
	const char *token,
	unsigned long supervised_pid)
{
	return token && !strcmp(report.token, token) && report.pid == supervised_pid;
}

bool magik_launcher_ready_timed_out(
	MagikLauncherReadyPhase phase,
	unsigned long now_ms,
	unsigned long deadline_ms)
{
	return phase == MagikLauncherReadyPhase::Awaiting && deadline_ms && now_ms >= deadline_ms;
}

MagikLauncherReadyRecoveryPlan magik_launcher_ready_recovery_plan(
	unsigned int attempt,
	bool display_change_pending)
{
	MagikLauncherReadyRecoveryPlan plan = {};
	plan.steps[plan.count++] = MagikLauncherReadyRecoveryStep::StopChild;
	if (attempt == 1)
	{
		plan.steps[plan.count++] = MagikLauncherReadyRecoveryStep::Retry;
		plan.next_attempt = 2;
		return plan;
	}
	plan.next_attempt = attempt;
	if (display_change_pending)
		plan.steps[plan.count++] = MagikLauncherReadyRecoveryStep::RollbackDisplay;
	plan.steps[plan.count++] = MagikLauncherReadyRecoveryStep::RestoreStockMenu;
	plan.steps[plan.count++] = MagikLauncherReadyRecoveryStep::FinishPendingReply;
	return plan;
}
