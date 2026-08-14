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
	char source_sha256[65] = {};
	unsigned long pid = 0;
	unsigned long main_pid = 0, source_nonzero = 0;
	unsigned long long main_generation = 0, owner_epoch = 0, base = 0;
	unsigned int protocol = 0, capabilities = 0, width = 0, height = 0, stride = 0;
	unsigned int first_sequence = 0, first_route_epoch = 0, first_slot = 0, first_receipt_crc = 0;
	unsigned int second_sequence = 0, second_route_epoch = 0, second_slot = 0, second_receipt_crc = 0;
	char trailing = 0;
	int fields = sscanf(
		line,
		"%15s token=%32s pid=%lu main_pid=%lu main_generation=%llu owner_epoch=%llu protocol=%u capabilities=%x base=%llx width=%u height=%u stride=%u first_sequence=%u first_route_epoch=%u first_slot=%u first_receipt_crc=%x second_sequence=%u second_route_epoch=%u second_slot=%u second_receipt_crc=%x source_sha256=%64s source_nonzero=%lu %c",
		command, token, &pid, &main_pid, &main_generation, &owner_epoch, &protocol,
		&capabilities, &base, &width, &height, &stride, &first_sequence,
		&first_route_epoch, &first_slot, &first_receipt_crc, &second_sequence,
		&second_route_epoch, &second_slot, &second_receipt_crc, source_sha256,
		&source_nonzero, &trailing);
	if (fields != 22)
		return false;
	if (strcmp(command, "ready-v2") || strlen(token) != 32 || !pid || !main_pid ||
	    !main_generation || !owner_epoch || protocol != 5 || capabilities != 0x03ff ||
	    !base || base > 0xffffffffULL || !width || width > 0xffff || !height ||
	    height > 0xffff || stride > 0xffff || stride < width * 2 ||
	    first_sequence > 0xffff || first_route_epoch > 0xffff ||
	    first_receipt_crc > 0xffff || second_sequence > 0xffff ||
	    second_route_epoch > 0xffff || second_receipt_crc > 0xffff ||
	    first_slot < 1 || first_slot > 2 || second_slot < 1 || second_slot > 2 ||
	    first_slot == second_slot || !source_nonzero ||
	    (unsigned long long)source_nonzero > (unsigned long long)width * height ||
	    strlen(source_sha256) != 64)
		return false;
	for (const char *ch = token; *ch; ch++)
		if (!isdigit((unsigned char)*ch) && (*ch < 'a' || *ch > 'f')) return false;
	for (const char *ch = source_sha256; *ch; ch++)
		if (!isdigit((unsigned char)*ch) && (*ch < 'a' || *ch > 'f')) return false;
	unsigned int sequence_delta = (second_sequence - first_sequence) & 0xffff;
	unsigned int route_delta = (second_route_epoch - first_route_epoch) & 0xffff;
	if (!sequence_delta || sequence_delta >= 0x8000 || !route_delta || route_delta >= 0x8000)
		return false;
	char expected[1024];
	snprintf(expected, sizeof(expected),
		"ready-v2 token=%s pid=%lu main_pid=%lu main_generation=%llu owner_epoch=%llu protocol=%u capabilities=%04x base=%08llx width=%u height=%u stride=%u first_sequence=%u first_route_epoch=%u first_slot=%u first_receipt_crc=%04x second_sequence=%u second_route_epoch=%u second_slot=%u second_receipt_crc=%04x source_sha256=%s source_nonzero=%lu",
		token, pid, main_pid, main_generation, owner_epoch, protocol, capabilities, base,
		width, height, stride, first_sequence, first_route_epoch, first_slot,
		first_receipt_crc, second_sequence, second_route_epoch, second_slot,
		second_receipt_crc, source_sha256, source_nonzero);
	if (strcmp(line, expected)) return false;
	snprintf(report->token, sizeof(report->token), "%s", token);
	snprintf(report->source_sha256, sizeof(report->source_sha256), "%s", source_sha256);
	report->pid = pid;
	report->main_pid = main_pid;
	report->main_generation = main_generation;
	report->owner_epoch = owner_epoch;
	report->protocol = protocol;
	report->capabilities = capabilities;
	report->base = base;
	report->width = width;
	report->height = height;
	report->stride = stride;
	report->first_sequence = first_sequence;
	report->first_route_epoch = first_route_epoch;
	report->first_slot = first_slot;
	report->first_receipt_crc = first_receipt_crc;
	report->second_sequence = second_sequence;
	report->second_route_epoch = second_route_epoch;
	report->second_slot = second_slot;
	report->second_receipt_crc = second_receipt_crc;
	report->source_nonzero = source_nonzero;
	return true;
}

bool magik_launcher_ready_matches(
	const MagikLauncherReadyReport &report,
	const char *token,
	unsigned long supervised_pid,
	unsigned long main_pid,
	unsigned long long main_generation,
	unsigned long long owner_epoch)
{
	return token && !strcmp(report.token, token) && report.pid == supervised_pid &&
	       report.main_pid == main_pid && report.main_generation == main_generation &&
	       report.owner_epoch == owner_epoch;
}

bool magik_launcher_ready_timed_out(
	MagikLauncherReadyPhase phase,
	unsigned long now_ms,
	unsigned long deadline_ms)
{
	return phase == MagikLauncherReadyPhase::Awaiting && deadline_ms && now_ms >= deadline_ms;
}

unsigned int magik_launcher_ready_begin_attempt(unsigned int attempt)
{
	return attempt ? attempt : 1;
}

void magik_launcher_ready_rearm_after_terminal_failure(
	MagikLauncherReadyPhase *phase,
	unsigned int *attempt,
	unsigned long *deadline_ms)
{
	if (phase) *phase = MagikLauncherReadyPhase::Idle;
	if (attempt) *attempt = 0;
	if (deadline_ms) *deadline_ms = 0;
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
