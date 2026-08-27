#pragma once

enum class MagikLauncherState
{
	Unconfigured,
	BootingMain,
	EnteringLauncher,
	LauncherStarting,
	LauncherActive,
	HandoffToGame,
	HandoffToStockMenu,
	LauncherSuspending,
	LauncherSuspended,
	LauncherRebooting,
	LauncherCrashed,
};

enum class MagikLauncherEvent
{
	Configured,
	BeginEnterLauncher,
	ChildSpawned,
	LauncherReady,
	LaunchRequested,
	ExitRequested,
	ChildExitedUnexpectedly,
	ChildCrashed,
	SuspendRequested,
	ChildExitedExpectedly,
	ResumeRequested,
	RebootRequested,
	HandoffComplete,
	ResetToUnconfigured,
};

enum class MagikLauncherRestartAction
{
	Reject,
	ResumeSuspended,
	RespawnCrashed,
	RestartActive,
};

const char *magik_launcher_state_name(MagikLauncherState state);
const char *magik_launcher_event_name(MagikLauncherEvent event);
const char *magik_launcher_restart_action_name(MagikLauncherRestartAction action);
bool magik_launcher_is_active(MagikLauncherState state);
bool magik_launcher_owns_session(MagikLauncherState state);
bool magik_launcher_accepts_handoff(MagikLauncherState state);
bool magik_launcher_accepts_video_reinit(MagikLauncherState state);
bool magik_launcher_polls_commands(MagikLauncherState state);
bool magik_launcher_idle_waits(MagikLauncherState state);
MagikLauncherRestartAction magik_launcher_restart_action(MagikLauncherState state);
bool magik_launcher_transition(
    MagikLauncherState current,
    MagikLauncherEvent event,
    MagikLauncherState *next);
