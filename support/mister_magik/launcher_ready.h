#pragma once

#include <stddef.h>

enum class MagikLauncherReadyPhase
{
	Idle,
	Awaiting,
	Ready,
	Failed,
};

struct MagikLauncherReadyReport
{
	char token[33];
	unsigned long pid;
	unsigned long main_pid;
	unsigned long long main_generation;
	unsigned long long owner_epoch;
};

enum class MagikLauncherReadyRecoveryStep
{
	StopChild,
	Retry,
	RollbackDisplay,
	RestoreStockMenu,
	FinishPendingReply,
};

struct MagikLauncherReadyRecoveryPlan
{
	MagikLauncherReadyRecoveryStep steps[4];
	unsigned int count;
	unsigned int next_attempt;
};

const char *magik_launcher_ready_phase_name(MagikLauncherReadyPhase phase);
bool magik_launcher_parse_ready(const char *line, MagikLauncherReadyReport *report);
bool magik_launcher_ready_matches(
	const MagikLauncherReadyReport &report,
	const char *token,
	unsigned long supervised_pid,
	unsigned long main_pid,
	unsigned long long main_generation,
	unsigned long long owner_epoch);
bool magik_launcher_ready_timed_out(
	MagikLauncherReadyPhase phase,
	unsigned long now_ms,
	unsigned long deadline_ms);
unsigned int magik_launcher_ready_begin_attempt(unsigned int attempt);
void magik_launcher_ready_rearm_after_terminal_failure(
	MagikLauncherReadyPhase *phase,
	unsigned int *attempt,
	unsigned long *deadline_ms);
MagikLauncherReadyRecoveryPlan magik_launcher_ready_recovery_plan(
	unsigned int attempt,
	bool display_change_pending);
