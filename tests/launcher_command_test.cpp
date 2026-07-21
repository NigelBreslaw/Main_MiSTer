#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "support/mister_magik/launcher_command.h"

static const char *valid_plan = "schema=1&launch_ref=magik-plan:test&title=Test%20Game&system_id=neogeo&core_path=NeoGeo&payload_path=/media/fat/games/NEOGEO/Test%20Game.neo&mount_kind=mount-image&mount_index=0&delay_secs=1";

static void assert_invalid_plan(const char *encoded)
{
	MagikLauncherCommand cmd;
	char line[2048];
	snprintf(line, sizeof(line), "mister_magik_launch_plan_v1 %s", encoded);
	assert(magik_launcher_parse_command(line, &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, "structured launch plan"));
}

int main()
{
	MagikLauncherCommand cmd;
	assert(magik_launcher_parse_command("mister_magik_exit_to_menu", &cmd));
	assert(cmd.type == MagikLauncherCommandType::ExitToMenu);
	assert(magik_launcher_parse_command("mister_magik_return_to_launcher", &cmd));
	assert(cmd.type == MagikLauncherCommandType::ReturnToLauncher);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "ReturnToLauncher"));

	assert(magik_launcher_parse_command("mister_magik_suspend", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Suspend);

	assert(magik_launcher_parse_command("mister_magik_resume", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Resume);

	assert(magik_launcher_parse_command("mister_magik_restart_launcher", &cmd));
	assert(cmd.type == MagikLauncherCommandType::RestartLauncher);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "RestartLauncher"));

	assert(magik_launcher_parse_command("mister_magik_hdmi_power_cycle", &cmd));
	assert(cmd.type == MagikLauncherCommandType::HdmiPowerCycle);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "HdmiPowerCycle"));

	assert(magik_launcher_parse_command("mister_magik_video_adjust", &cmd));
	assert(cmd.type == MagikLauncherCommandType::VideoAdjust);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "VideoAdjust"));

	assert(magik_launcher_parse_command("mister_magik_video_reinit", &cmd));
	assert(cmd.type == MagikLauncherCommandType::VideoReinit);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "VideoReinit"));

	assert(magik_launcher_parse_command("mister_magik_reboot", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Reboot);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "Reboot"));

	assert(magik_launcher_parse_command("mister_magik_direct_reset", &cmd));
	assert(cmd.type == MagikLauncherCommandType::DirectReset);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "DirectReset"));

	assert(magik_launcher_parse_command("mister_magik_direct_reset_no_sync", &cmd));
	assert(cmd.type == MagikLauncherCommandType::DirectResetNoSync);
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "DirectResetNoSync"));

	assert(magik_launcher_parse_command("mister_magik_settings_get_v1", &cmd));
	assert(cmd.type == MagikLauncherCommandType::SettingsGetV1);
	assert(magik_launcher_parse_command("mister_magik_settings_set_v1 output=hdmi", &cmd));
	assert(cmd.type == MagikLauncherCommandType::SettingsSetV1);
	assert(cmd.runtime_output == MagikRuntimeOutput::Hdmi);
	assert(magik_launcher_parse_command("mister_magik_settings_set_v1 output=crt-240p60", &cmd));
	assert(cmd.runtime_output == MagikRuntimeOutput::Crt240p60);
	assert(magik_launcher_parse_command("mister_magik_settings_set_v1 output=auto", &cmd));
	assert(cmd.runtime_output == MagikRuntimeOutput::Auto);
	assert(magik_launcher_parse_command("mister_magik_settings_set_v1 output=pal", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);

	assert(magik_launcher_parse_command("mister_magik_launch /media/fat/_Arcade/game.mra", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Launch);
	assert(!strcmp(cmd.path, "/media/fat/_Arcade/game.mra"));

	assert(magik_launcher_parse_command("mister_magik_launch relative/game.mra", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, "absolute"));

	char line[2048];
	snprintf(line, sizeof(line), "mister_magik_launch_plan_v1 %s", valid_plan);
	assert(magik_launcher_parse_command(line, &cmd));
	assert(cmd.type == MagikLauncherCommandType::LaunchPlan);
	assert(!strcmp(cmd.plan.core_path, "NeoGeo"));
	assert(!strcmp(cmd.plan.payload_path, "/media/fat/games/NEOGEO/Test Game.neo"));
	assert(!strcmp(cmd.plan.mount_kind, "mount-image"));
	assert(cmd.plan.mount_index == 0);
	assert(cmd.plan.delay_secs == 1);
	assert(magik_launcher_is_plan_arg(cmd.plan.arg));

	MagikStructuredLaunchPlan plan;
	assert(magik_launcher_parse_plan_arg(cmd.plan.arg, &plan));
	assert(!strcmp(plan.core_path, "NeoGeo"));
	assert(!strcmp(plan.payload_path, "/media/fat/games/NEOGEO/Test Game.neo"));
	assert(!strcmp(plan.mount_kind, "mount-image"));

	assert_invalid_plan("launch_ref=magik-plan:test&core_path=NeoGeo&payload_path=/media/fat/test.neo&mount_kind=mount-image&mount_index=0&delay_secs=1");
	assert_invalid_plan("schema=2&core_path=NeoGeo&payload_path=/media/fat/test.neo&mount_kind=mount-image&mount_index=0&delay_secs=1");
	assert_invalid_plan("schema=1&payload_path=/media/fat/test.neo&mount_kind=mount-image&mount_index=0&delay_secs=1");
	assert_invalid_plan("schema=1&core_path=NeoGeo&payload_path=/media/fat/test.neo&mount_kind=wrong&mount_index=0&delay_secs=1");
	assert_invalid_plan("schema=1&core_path=NeoGeo&payload_path=/media/fat/test.neo&mount_kind=mount-image&mount_index=abc&delay_secs=1");

	assert(magik_launcher_parse_command("load_core /tmp/external-last-launch.mgl", &cmd));
	assert(cmd.type == MagikLauncherCommandType::ExternalLaunch);
	assert(!strcmp(cmd.path, "/tmp/external-last-launch.mgl"));
	assert(!strcmp(magik_launcher_command_type_name(cmd.type), "ExternalLaunch"));

	assert(magik_launcher_parse_command("load_core /media/fat/_Arcade/game.mra", &cmd));
	assert(cmd.type == MagikLauncherCommandType::ExternalLaunch);
	assert(magik_launcher_parse_command("load_core /media/fat/_Console/core.RBF", &cmd));
	assert(cmd.type == MagikLauncherCommandType::ExternalLaunch);

	assert(magik_launcher_parse_command("load_core", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, "absolute"));
	assert(magik_launcher_parse_command("load_core relative/game.mgl", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, "absolute"));
	assert(magik_launcher_parse_command("load_core /media/fat/game.zip", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, ".mgl"));
	assert(magik_launcher_parse_command("load_core /media/fat/game.mgl\r", &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, "control character"));

	char oversized[1200];
	snprintf(oversized, sizeof(oversized), "load_core /%01080d.mgl", 0);
	assert(magik_launcher_parse_command(oversized, &cmd));
	assert(cmd.type == MagikLauncherCommandType::Invalid);
	assert(strstr(cmd.error, "too long"));
	return 0;
}
