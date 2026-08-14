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
	unsigned int protocol;
	unsigned int capabilities;
	unsigned long long base;
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int first_sequence;
	unsigned int first_route_epoch;
	unsigned int first_slot;
	unsigned int first_receipt_crc;
	unsigned int second_sequence;
	unsigned int second_route_epoch;
	unsigned int second_slot;
	unsigned int second_receipt_crc;
	char source_sha256[65];
	unsigned long source_nonzero;
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
