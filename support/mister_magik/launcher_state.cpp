#include "launcher_state.h"

const char *magik_launcher_state_name(MagikLauncherState state)
{
	switch (state)
	{
	case MagikLauncherState::Unconfigured: return "Unconfigured";
	case MagikLauncherState::BootingMain: return "BootingMain";
	case MagikLauncherState::EnteringLauncher: return "EnteringLauncher";
	case MagikLauncherState::LauncherStarting: return "LauncherStarting";
	case MagikLauncherState::LauncherActive: return "LauncherActive";
	case MagikLauncherState::HandoffToGame: return "HandoffToGame";
	case MagikLauncherState::HandoffToStockMenu: return "HandoffToStockMenu";
	case MagikLauncherState::LauncherSuspending: return "LauncherSuspending";
	case MagikLauncherState::LauncherSuspended: return "LauncherSuspended";
	case MagikLauncherState::LauncherRebooting: return "LauncherRebooting";
	case MagikLauncherState::LauncherCrashed: return "LauncherCrashed";
	}
	return "Unknown";
}

const char *magik_launcher_event_name(MagikLauncherEvent event)
{
	switch (event)
	{
	case MagikLauncherEvent::Configured: return "Configured";
	case MagikLauncherEvent::BeginEnterLauncher: return "BeginEnterLauncher";
	case MagikLauncherEvent::ChildSpawned: return "ChildSpawned";
	case MagikLauncherEvent::LauncherReady: return "LauncherReady";
	case MagikLauncherEvent::LaunchRequested: return "LaunchRequested";
	case MagikLauncherEvent::ExitRequested: return "ExitRequested";
	case MagikLauncherEvent::ChildExitedUnexpectedly: return "ChildExitedUnexpectedly";
	case MagikLauncherEvent::ChildCrashed: return "ChildCrashed";
	case MagikLauncherEvent::SuspendRequested: return "SuspendRequested";
	case MagikLauncherEvent::ChildExitedExpectedly: return "ChildExitedExpectedly";
	case MagikLauncherEvent::ResumeRequested: return "ResumeRequested";
	case MagikLauncherEvent::RebootRequested: return "RebootRequested";
	case MagikLauncherEvent::HandoffComplete: return "HandoffComplete";
	case MagikLauncherEvent::ResetToUnconfigured: return "ResetToUnconfigured";
	}
	return "Unknown";
}

const char *magik_launcher_restart_action_name(MagikLauncherRestartAction action)
{
	switch (action)
	{
	case MagikLauncherRestartAction::Reject: return "Reject";
	case MagikLauncherRestartAction::ResumeSuspended: return "ResumeSuspended";
	case MagikLauncherRestartAction::RespawnCrashed: return "RespawnCrashed";
	case MagikLauncherRestartAction::RestartActive: return "RestartActive";
	}
	return "Unknown";
}

bool magik_launcher_is_active(MagikLauncherState state)
{
	return state == MagikLauncherState::LauncherActive;
}

bool magik_launcher_owns_session(MagikLauncherState state)
{
	return state == MagikLauncherState::EnteringLauncher ||
	       state == MagikLauncherState::LauncherStarting ||
	       state == MagikLauncherState::LauncherActive ||
	       state == MagikLauncherState::LauncherSuspending ||
	       state == MagikLauncherState::LauncherSuspended ||
	       state == MagikLauncherState::LauncherRebooting ||
	       state == MagikLauncherState::LauncherCrashed;
}

MagikLauncherRestartAction magik_launcher_restart_action(MagikLauncherState state)
{
	switch (state)
	{
	case MagikLauncherState::LauncherSuspended: return MagikLauncherRestartAction::ResumeSuspended;
	case MagikLauncherState::LauncherCrashed: return MagikLauncherRestartAction::RespawnCrashed;
	case MagikLauncherState::LauncherActive: return MagikLauncherRestartAction::RestartActive;
	default: return MagikLauncherRestartAction::Reject;
	}
}

bool magik_launcher_accepts_handoff(MagikLauncherState state)
{
	return state == MagikLauncherState::LauncherActive;
}

bool magik_launcher_accepts_video_reinit(MagikLauncherState state)
{
	return state == MagikLauncherState::LauncherActive;
}

bool magik_launcher_polls_commands(MagikLauncherState state)
{
	return state == MagikLauncherState::LauncherActive ||
	       state == MagikLauncherState::LauncherStarting ||
	       state == MagikLauncherState::LauncherSuspended ||
	       state == MagikLauncherState::LauncherCrashed;
}

bool magik_launcher_idle_waits(MagikLauncherState state)
{
	return state == MagikLauncherState::LauncherActive ||
	       state == MagikLauncherState::LauncherSuspended ||
	       state == MagikLauncherState::LauncherCrashed;
}

bool magik_launcher_transition(
    MagikLauncherState current,
    MagikLauncherEvent event,
    MagikLauncherState *next)
{
	if (!next) return false;

	if (event == MagikLauncherEvent::ResetToUnconfigured)
	{
		*next = MagikLauncherState::Unconfigured;
		return true;
	}

	switch (current)
	{
	case MagikLauncherState::Unconfigured:
		if (event == MagikLauncherEvent::Configured)
		{
			*next = MagikLauncherState::BootingMain;
			return true;
		}
		break;
	case MagikLauncherState::BootingMain:
		if (event == MagikLauncherEvent::BeginEnterLauncher)
		{
			*next = MagikLauncherState::EnteringLauncher;
			return true;
		}
		break;
	case MagikLauncherState::EnteringLauncher:
		if (event == MagikLauncherEvent::ChildSpawned)
		{
			*next = MagikLauncherState::LauncherStarting;
			return true;
		}
		if (event == MagikLauncherEvent::ChildCrashed ||
		    event == MagikLauncherEvent::ChildExitedUnexpectedly)
		{
			*next = MagikLauncherState::LauncherCrashed;
			return true;
		}
		break;
	case MagikLauncherState::LauncherStarting:
		if (event == MagikLauncherEvent::LauncherReady)
		{
			*next = MagikLauncherState::LauncherActive;
			return true;
		}
		if (event == MagikLauncherEvent::ChildCrashed ||
		    event == MagikLauncherEvent::ChildExitedUnexpectedly)
		{
			*next = MagikLauncherState::LauncherCrashed;
			return true;
		}
		break;
	case MagikLauncherState::LauncherActive:
		if (event == MagikLauncherEvent::LaunchRequested)
		{
			*next = MagikLauncherState::HandoffToGame;
			return true;
		}
		if (event == MagikLauncherEvent::ExitRequested)
		{
			*next = MagikLauncherState::HandoffToStockMenu;
			return true;
		}
		if (event == MagikLauncherEvent::ChildCrashed ||
		    event == MagikLauncherEvent::ChildExitedUnexpectedly)
		{
			*next = MagikLauncherState::LauncherCrashed;
			return true;
		}
		if (event == MagikLauncherEvent::SuspendRequested)
		{
			*next = MagikLauncherState::LauncherSuspending;
			return true;
		}
		if (event == MagikLauncherEvent::RebootRequested)
		{
			*next = MagikLauncherState::LauncherRebooting;
			return true;
		}
		break;
	case MagikLauncherState::HandoffToGame:
	case MagikLauncherState::HandoffToStockMenu:
		if (event == MagikLauncherEvent::HandoffComplete)
		{
			*next = MagikLauncherState::Unconfigured;
			return true;
		}
		break;
	case MagikLauncherState::LauncherSuspending:
		if (event == MagikLauncherEvent::ChildExitedExpectedly)
		{
			*next = MagikLauncherState::LauncherSuspended;
			return true;
		}
		if (event == MagikLauncherEvent::ChildCrashed ||
		    event == MagikLauncherEvent::ChildExitedUnexpectedly)
		{
			*next = MagikLauncherState::LauncherCrashed;
			return true;
		}
		break;
	case MagikLauncherState::LauncherSuspended:
		if (event == MagikLauncherEvent::ResumeRequested)
		{
			*next = MagikLauncherState::EnteringLauncher;
			return true;
		}
		break;
	case MagikLauncherState::LauncherRebooting:
		if (event == MagikLauncherEvent::ChildExitedExpectedly ||
		    event == MagikLauncherEvent::ChildExitedUnexpectedly ||
		    event == MagikLauncherEvent::ChildCrashed)
		{
			*next = MagikLauncherState::LauncherRebooting;
			return true;
		}
		break;
	case MagikLauncherState::LauncherCrashed:
		if (event == MagikLauncherEvent::BeginEnterLauncher)
		{
			*next = MagikLauncherState::EnteringLauncher;
			return true;
		}
		break;
	}

	return false;
}
