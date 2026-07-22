#include <assert.h>
#include <string.h>
#include "support/mister_magik/launcher_state.h"

static MagikLauncherState step(MagikLauncherState from, MagikLauncherEvent event)
{
	MagikLauncherState next = from;
	assert(magik_launcher_transition(from, event, &next));
	return next;
}

int main()
{
	MagikLauncherState s = MagikLauncherState::Unconfigured;
	s = step(s, MagikLauncherEvent::Configured);
	assert(s == MagikLauncherState::BootingMain);
	s = step(s, MagikLauncherEvent::BeginEnterLauncher);
	assert(s == MagikLauncherState::EnteringLauncher);
	assert(magik_launcher_is_active(s));
	s = step(s, MagikLauncherEvent::ChildSpawned);
	assert(s == MagikLauncherState::LauncherActive);
	assert(magik_launcher_accepts_handoff(s));
	assert(magik_launcher_polls_commands(s));
	assert(magik_launcher_idle_waits(s));

	MagikLauncherState rejected = s;
	assert(!magik_launcher_transition(s, MagikLauncherEvent::Configured, &rejected));
	assert(rejected == s);

	MagikLauncherState launch = step(s, MagikLauncherEvent::LaunchRequested);
	assert(launch == MagikLauncherState::HandoffToGame);
	assert(!magik_launcher_accepts_handoff(launch));
	assert(!magik_launcher_polls_commands(launch));
	assert(!magik_launcher_idle_waits(launch));
	assert(!magik_launcher_transition(launch, MagikLauncherEvent::ExitRequested, &rejected));

	MagikLauncherState menu = step(s, MagikLauncherEvent::ExitRequested);
	assert(menu == MagikLauncherState::HandoffToStockMenu);
	assert(step(menu, MagikLauncherEvent::HandoffComplete) == MagikLauncherState::Unconfigured);

	MagikLauncherState crash = step(s, MagikLauncherEvent::ChildCrashed);
	assert(crash == MagikLauncherState::LauncherCrashed);
	assert(magik_launcher_is_active(crash));
	assert(magik_launcher_polls_commands(crash));
	assert(magik_launcher_idle_waits(crash));
	assert(magik_launcher_restart_action(crash) == MagikLauncherRestartAction::RespawnCrashed);
	assert(!strcmp(magik_launcher_restart_action_name(magik_launcher_restart_action(crash)), "RespawnCrashed"));
	assert(strcmp(magik_launcher_state_name(crash), "LauncherCrashed") == 0);
	MagikLauncherState crash_restart = step(crash, MagikLauncherEvent::BeginEnterLauncher);
	assert(crash_restart == MagikLauncherState::EnteringLauncher);
	assert(step(crash_restart, MagikLauncherEvent::ChildSpawned) == MagikLauncherState::LauncherActive);

	MagikLauncherState suspending = step(s, MagikLauncherEvent::SuspendRequested);
	assert(suspending == MagikLauncherState::LauncherSuspending);
	assert(magik_launcher_is_active(suspending));
	assert(!magik_launcher_accepts_handoff(suspending));
	assert(!magik_launcher_polls_commands(suspending));
	assert(!magik_launcher_idle_waits(suspending));
	assert(magik_launcher_restart_action(suspending) == MagikLauncherRestartAction::Reject);

	MagikLauncherState suspended = step(suspending, MagikLauncherEvent::ChildExitedExpectedly);
	assert(suspended == MagikLauncherState::LauncherSuspended);
	assert(magik_launcher_is_active(suspended));
	assert(!magik_launcher_accepts_handoff(suspended));
	assert(magik_launcher_polls_commands(suspended));
	assert(magik_launcher_idle_waits(suspended));
	assert(magik_launcher_restart_action(suspended) == MagikLauncherRestartAction::ResumeSuspended);
	assert(magik_launcher_restart_resets_tty(MagikLauncherRestartAction::ResumeSuspended));
	assert(!magik_launcher_restart_resets_tty(MagikLauncherRestartAction::RestartActive));
	assert(!magik_launcher_restart_resets_tty(MagikLauncherRestartAction::RespawnCrashed));
	MagikLauncherState restarting = step(suspended, MagikLauncherEvent::ResumeRequested);
	assert(restarting == MagikLauncherState::EnteringLauncher);
	assert(step(restarting, MagikLauncherEvent::ChildSpawned) == MagikLauncherState::LauncherActive);

	assert(magik_launcher_restart_action(s) == MagikLauncherRestartAction::RestartActive);

	MagikLauncherState rebooting = step(s, MagikLauncherEvent::RebootRequested);
	assert(rebooting == MagikLauncherState::LauncherRebooting);
	assert(magik_launcher_is_active(rebooting));
	assert(!magik_launcher_accepts_handoff(rebooting));
	assert(!magik_launcher_polls_commands(rebooting));
	assert(!magik_launcher_idle_waits(rebooting));
	assert(step(rebooting, MagikLauncherEvent::ChildExitedUnexpectedly) == MagikLauncherState::LauncherRebooting);
	assert(step(rebooting, MagikLauncherEvent::ChildExitedExpectedly) == MagikLauncherState::LauncherRebooting);
	assert(strcmp(magik_launcher_state_name(rebooting), "LauncherRebooting") == 0);

	return 0;
}
