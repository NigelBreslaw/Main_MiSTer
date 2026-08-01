#pragma once

#include <stddef.h>

enum class MagikLauncherCommandType
{
	None,
	Launch,
	ExternalLaunch,
	LaunchPlan,
	ExitToMenu,
	ReturnToLauncher,
	Suspend,
	Resume,
	RestartLauncher,
	SupervisedRestartLauncher,
	ReloadMain,
	HdmiPowerCycle,
	VideoAdjust,
	VideoReinit,
	Reboot,
	DirectReset,
	DirectResetNoSync,
	SettingsGetV1,
	SettingsSetV1,
	DisplayGetV1,
	DisplayApplyV1,
	DisplayApplyHeadlessV1,
	DisplayConfirmV1,
	DisplayCancelV1,
	Invalid,
};

enum class MagikRuntimeOutput
{
	Auto,
	Hdmi,
	Crt240p60,
	Crt288p50,
	Crt480p60,
	Crt576p50,
	Hdmi720p60,
	Hdmi768p60,
	Hdmi1080p60,
	Hdmi1200p60,
	Hdmi1536p60,
	Hdmi1440p60,
};

struct MagikStructuredLaunchPlan
{
	char encoded[4096];
	char arg[4112];
	char launch_ref[1024];
	char title[256];
	char system_id[64];
	char core_path[1024];
	char payload_path[1024];
	char mount_kind[64];
	int mount_index;
	int delay_secs;
};

struct MagikLauncherCommand
{
	MagikLauncherCommandType type;
	char path[1024];
	MagikStructuredLaunchPlan plan;
	MagikRuntimeOutput runtime_output;
	char error[160];
};

void magik_launcher_command_init(MagikLauncherCommand *cmd);
bool magik_launcher_parse_command(const char *line, MagikLauncherCommand *cmd);
bool magik_launcher_is_plan_arg(const char *arg);
bool magik_launcher_parse_plan_arg(const char *arg, MagikStructuredLaunchPlan *plan);
const char *magik_launcher_command_type_name(MagikLauncherCommandType type);
const char *magik_resolved_output_name(bool direct_video, bool menu_pal, bool forced_scandoubler);
const char *magik_runtime_output_name(MagikRuntimeOutput output);
bool magik_display_should_return_to_settings(bool confirm_ui);
