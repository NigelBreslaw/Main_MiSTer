#include "launcher_command.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char plan_arg_prefix[] = "magik-plan-v1:";

static void set_error(MagikLauncherCommand *cmd, const char *message)
{
	if (!cmd) return;
	cmd->type = MagikLauncherCommandType::Invalid;
	snprintf(cmd->error, sizeof(cmd->error), "%s", message ? message : "invalid command");
}

static void magik_launch_plan_init(MagikStructuredLaunchPlan *plan)
{
	if (!plan) return;
	memset(plan, 0, sizeof(*plan));
	plan->mount_index = 0;
	plan->delay_secs = 1;
}

void magik_launcher_command_init(MagikLauncherCommand *cmd)
{
	if (!cmd) return;
	cmd->type = MagikLauncherCommandType::None;
	cmd->path[0] = 0;
	cmd->runtime_output = MagikRuntimeOutput::Auto;
	magik_launch_plan_init(&cmd->plan);
	cmd->error[0] = 0;
}

const char *magik_resolved_output_name(bool direct_video, bool menu_pal, bool forced_scandoubler)
{
	if (!direct_video) return "hdmi";
	if (forced_scandoubler) return menu_pal ? "crt-576p50" : "crt-480p60";
	return menu_pal ? "crt-288p50" : "crt-240p60";
}

const char *magik_launcher_command_type_name(MagikLauncherCommandType type)
{
	switch (type)
	{
	case MagikLauncherCommandType::None: return "None";
	case MagikLauncherCommandType::Launch: return "Launch";
	case MagikLauncherCommandType::ExternalLaunch: return "ExternalLaunch";
	case MagikLauncherCommandType::LaunchPlan: return "LaunchPlan";
	case MagikLauncherCommandType::ExitToMenu: return "ExitToMenu";
	case MagikLauncherCommandType::ReturnToLauncher: return "ReturnToLauncher";
	case MagikLauncherCommandType::Suspend: return "Suspend";
	case MagikLauncherCommandType::Resume: return "Resume";
	case MagikLauncherCommandType::RestartLauncher: return "RestartLauncher";
	case MagikLauncherCommandType::SupervisedRestartLauncher: return "SupervisedRestartLauncher";
	case MagikLauncherCommandType::HdmiPowerCycle: return "HdmiPowerCycle";
	case MagikLauncherCommandType::VideoAdjust: return "VideoAdjust";
	case MagikLauncherCommandType::VideoReinit: return "VideoReinit";
	case MagikLauncherCommandType::Reboot: return "Reboot";
	case MagikLauncherCommandType::DirectReset: return "DirectReset";
	case MagikLauncherCommandType::DirectResetNoSync: return "DirectResetNoSync";
	case MagikLauncherCommandType::SettingsGetV1: return "SettingsGetV1";
	case MagikLauncherCommandType::SettingsSetV1: return "SettingsSetV1";
	case MagikLauncherCommandType::DisplayGetV1: return "DisplayGetV1";
	case MagikLauncherCommandType::DisplayApplyV1: return "DisplayApplyV1";
	case MagikLauncherCommandType::DisplayApplyHeadlessV1: return "DisplayApplyHeadlessV1";
	case MagikLauncherCommandType::DisplayConfirmV1: return "DisplayConfirmV1";
	case MagikLauncherCommandType::DisplayCancelV1: return "DisplayCancelV1";
	case MagikLauncherCommandType::Invalid: return "Invalid";
	}
	return "Unknown";
}

const char *magik_runtime_output_name(MagikRuntimeOutput output)
{
	switch (output)
	{
	case MagikRuntimeOutput::Auto: return "auto";
	case MagikRuntimeOutput::Hdmi: return "hdmi";
	case MagikRuntimeOutput::Crt240p60: return "crt-240p60";
	case MagikRuntimeOutput::Crt288p50: return "crt-288p50";
	case MagikRuntimeOutput::Crt480p60: return "crt-480p60";
	case MagikRuntimeOutput::Crt576p50: return "crt-576p50";
	case MagikRuntimeOutput::Hdmi720p60: return "hdmi-1280x720p60";
	case MagikRuntimeOutput::Hdmi768p60: return "hdmi-1366x768p60";
	case MagikRuntimeOutput::Hdmi1080p60: return "hdmi-1920x1080p60";
	case MagikRuntimeOutput::Hdmi1200p60: return "hdmi-1920x1200p60";
	case MagikRuntimeOutput::Hdmi1536p60: return "hdmi-2048x1536p60";
	case MagikRuntimeOutput::Hdmi1440p60: return "hdmi-2560x1440p60";
	}
	return "unknown";
}

bool magik_display_should_return_to_settings(bool confirm_ui)
{
	return confirm_ui;
}

static bool parse_runtime_output(const char *value, MagikRuntimeOutput *output)
{
	if (!value || !output) return false;
	for (int raw = (int)MagikRuntimeOutput::Auto; raw <= (int)MagikRuntimeOutput::Hdmi1440p60; raw++)
	{
		MagikRuntimeOutput candidate = (MagikRuntimeOutput)raw;
		if (!strcmp(value, magik_runtime_output_name(candidate)))
		{
			*output = candidate;
			return true;
		}
	}
	return false;
}

static int hex_value(char ch)
{
	if (ch >= '0' && ch <= '9') return ch - '0';
	if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
	return -1;
}

static bool percent_decode_field(const char *src, size_t len, char *dst, size_t dst_len)
{
	if (!dst || !dst_len) return false;
	size_t out = 0;
	for (size_t i = 0; i < len; i++)
	{
		unsigned char ch = (unsigned char)src[i];
		if (ch == '%' && i + 2 < len)
		{
			int hi = hex_value(src[i + 1]);
			int lo = hex_value(src[i + 2]);
			if (hi >= 0 && lo >= 0)
			{
				ch = (unsigned char)((hi << 4) | lo);
				i += 2;
			}
		}
		if (out + 1 >= dst_len) return false;
		dst[out++] = (char)ch;
	}
	dst[out] = 0;
	return true;
}

static bool parse_nonnegative_int_field(const char *value, size_t value_len, int *out)
{
	char tmp[16];
	if (!out || !percent_decode_field(value, value_len, tmp, sizeof(tmp)) || !tmp[0]) return false;
	for (const char *ch = tmp; *ch; ch++)
	{
		if (!isdigit((unsigned char)*ch)) return false;
	}
	*out = atoi(tmp);
	return true;
}

static bool valid_mount_kind(const char *mount_kind)
{
	return mount_kind && (!strcasecmp(mount_kind, "load-file") || !strcasecmp(mount_kind, "mount-image"));
}

static bool copy_plan_field(MagikStructuredLaunchPlan *plan, const char *key, const char *value, size_t value_len)
{
	if (!strcmp(key, "schema"))
		return value_len == 1 && value[0] == '1';
	if (!strcmp(key, "mount_index"))
		return parse_nonnegative_int_field(value, value_len, &plan->mount_index);
	if (!strcmp(key, "delay_secs"))
		return parse_nonnegative_int_field(value, value_len, &plan->delay_secs);
	if (!strcmp(key, "launch_ref"))
		return percent_decode_field(value, value_len, plan->launch_ref, sizeof(plan->launch_ref));
	if (!strcmp(key, "title"))
		return percent_decode_field(value, value_len, plan->title, sizeof(plan->title));
	if (!strcmp(key, "system_id"))
		return percent_decode_field(value, value_len, plan->system_id, sizeof(plan->system_id));
	if (!strcmp(key, "core_path"))
		return percent_decode_field(value, value_len, plan->core_path, sizeof(plan->core_path));
	if (!strcmp(key, "payload_path"))
		return percent_decode_field(value, value_len, plan->payload_path, sizeof(plan->payload_path));
	if (!strcmp(key, "mount_kind"))
		return percent_decode_field(value, value_len, plan->mount_kind, sizeof(plan->mount_kind));
	return true;
}

static bool parse_plan_payload(const char *encoded, MagikStructuredLaunchPlan *plan)
{
	if (!encoded || !plan) return false;
	magik_launch_plan_init(plan);
	if (strlen(encoded) >= sizeof(plan->encoded)) return false;
	snprintf(plan->encoded, sizeof(plan->encoded), "%s", encoded);
	snprintf(plan->arg, sizeof(plan->arg), "%s%s", plan_arg_prefix, encoded);

	bool saw_schema = false;
	const char *field = encoded;
	while (*field)
	{
		const char *next = strchr(field, '&');
		size_t field_len = next ? (size_t)(next - field) : strlen(field);
		const char *eq = (const char*)memchr(field, '=', field_len);
		if (eq)
		{
			char key[64];
			size_t key_len = (size_t)(eq - field);
			if (key_len == 0 || key_len >= sizeof(key)) return false;
			memcpy(key, field, key_len);
			key[key_len] = 0;
			if (!strcmp(key, "schema")) saw_schema = true;
			if (!copy_plan_field(plan, key, eq + 1, field_len - key_len - 1))
				return false;
		}
		if (!next) break;
		field = next + 1;
	}

	if (!saw_schema || !plan->core_path[0] || !plan->payload_path[0] || !valid_mount_kind(plan->mount_kind))
		return false;
	return true;
}

bool magik_launcher_is_plan_arg(const char *arg)
{
	return arg && !strncmp(arg, plan_arg_prefix, sizeof(plan_arg_prefix) - 1);
}

bool magik_launcher_parse_plan_arg(const char *arg, MagikStructuredLaunchPlan *plan)
{
	if (!magik_launcher_is_plan_arg(arg)) return false;
	return parse_plan_payload(arg + sizeof(plan_arg_prefix) - 1, plan);
}

bool magik_launcher_parse_command(const char *line, MagikLauncherCommand *cmd)
{
	magik_launcher_command_init(cmd);
	if (!line || !cmd)
		return false;

	while (*line == ' ' || *line == '\t') line++;
	if (!*line)
		return false;

	if (!strcmp(line, "mister_magik_exit_to_menu"))
	{
		cmd->type = MagikLauncherCommandType::ExitToMenu;
		return true;
	}
	if (!strcmp(line, "mister_magik_return_to_launcher"))
	{
		cmd->type = MagikLauncherCommandType::ReturnToLauncher;
		return true;
	}
	if (!strcmp(line, "mister_magik_suspend"))
	{
		cmd->type = MagikLauncherCommandType::Suspend;
		return true;
	}
	if (!strcmp(line, "mister_magik_resume"))
	{
		cmd->type = MagikLauncherCommandType::Resume;
		return true;
	}
	if (!strcmp(line, "mister_magik_restart_launcher"))
	{
		cmd->type = MagikLauncherCommandType::RestartLauncher;
		return true;
	}
	if (!strcmp(line, "mister_magik_supervised_restart_launcher"))
	{
		cmd->type = MagikLauncherCommandType::SupervisedRestartLauncher;
		return true;
	}
	if (!strcmp(line, "mister_magik_hdmi_power_cycle"))
	{
		cmd->type = MagikLauncherCommandType::HdmiPowerCycle;
		return true;
	}
	if (!strcmp(line, "mister_magik_video_adjust"))
	{
		cmd->type = MagikLauncherCommandType::VideoAdjust;
		return true;
	}
	if (!strcmp(line, "mister_magik_video_reinit"))
	{
		cmd->type = MagikLauncherCommandType::VideoReinit;
		return true;
	}
	if (!strcmp(line, "mister_magik_reboot"))
	{
		cmd->type = MagikLauncherCommandType::Reboot;
		return true;
	}
	if (!strcmp(line, "mister_magik_direct_reset"))
	{
		cmd->type = MagikLauncherCommandType::DirectReset;
		return true;
	}
	if (!strcmp(line, "mister_magik_direct_reset_no_sync"))
	{
		cmd->type = MagikLauncherCommandType::DirectResetNoSync;
		return true;
	}
	if (!strcmp(line, "mister_magik_settings_get_v1"))
	{
		cmd->type = MagikLauncherCommandType::SettingsGetV1;
		return true;
	}
	static const char settings_prefix[] = "mister_magik_settings_set_v1 output=";
	if (!strncmp(line, settings_prefix, sizeof(settings_prefix) - 1))
	{
		const char *value = line + sizeof(settings_prefix) - 1;
		if (!strcmp(value, "auto")) cmd->runtime_output = MagikRuntimeOutput::Auto;
		else if (!strcmp(value, "hdmi")) cmd->runtime_output = MagikRuntimeOutput::Hdmi;
		else if (!strcmp(value, "crt-240p60")) cmd->runtime_output = MagikRuntimeOutput::Crt240p60;
		else
		{
			set_error(cmd, "runtime output must be auto, hdmi, or crt-240p60");
			return true;
		}
		cmd->type = MagikLauncherCommandType::SettingsSetV1;
		return true;
	}
	if (!strcmp(line, "mister_magik_display_get_v1"))
	{
		cmd->type = MagikLauncherCommandType::DisplayGetV1;
		return true;
	}
	static const char display_prefix[] = "mister_magik_display_apply_v1 mode=";
	if (!strncmp(line, display_prefix, sizeof(display_prefix) - 1))
	{
		if (!parse_runtime_output(line + sizeof(display_prefix) - 1, &cmd->runtime_output))
		{
			set_error(cmd, "unsupported display mode");
			return true;
		}
		cmd->type = MagikLauncherCommandType::DisplayApplyV1;
		return true;
	}
	static const char display_headless_prefix[] = "mister_magik_display_apply_headless_v1 mode=";
	if (!strncmp(line, display_headless_prefix, sizeof(display_headless_prefix) - 1))
	{
		if (!parse_runtime_output(line + sizeof(display_headless_prefix) - 1, &cmd->runtime_output))
		{
			set_error(cmd, "unsupported display mode");
			return true;
		}
		cmd->type = MagikLauncherCommandType::DisplayApplyHeadlessV1;
		return true;
	}
	if (!strcmp(line, "mister_magik_display_confirm_v1"))
	{
		cmd->type = MagikLauncherCommandType::DisplayConfirmV1;
		return true;
	}
	if (!strcmp(line, "mister_magik_display_cancel_v1"))
	{
		cmd->type = MagikLauncherCommandType::DisplayCancelV1;
		return true;
	}

	static const char stock_launch_command[] = "load_core";
	static const char stock_launch_prefix[] = "load_core ";
	if (!strcmp(line, stock_launch_command) ||
	    !strncmp(line, stock_launch_prefix, sizeof(stock_launch_prefix) - 1))
	{
		const char *path = line + sizeof(stock_launch_command) - 1;
		while (*path == ' ' || *path == '\t') path++;
		if (path[0] != '/')
		{
			set_error(cmd, "external load_core path must be absolute");
			return true;
		}
		if (strlen(path) >= sizeof(cmd->path))
		{
			set_error(cmd, "external load_core path is too long");
			return true;
		}
		for (const unsigned char *ch = (const unsigned char*)path; *ch; ch++)
		{
			if (iscntrl(*ch))
			{
				set_error(cmd, "external load_core path contains a control character");
				return true;
			}
		}
		const char *extension = strrchr(path, '.');
		if (!extension || (strcasecmp(extension, ".mgl") &&
		                   strcasecmp(extension, ".mra") &&
		                   strcasecmp(extension, ".rbf")))
		{
			set_error(cmd, "external load_core path must end in .mgl, .mra, or .rbf");
			return true;
		}
		snprintf(cmd->path, sizeof(cmd->path), "%s", path);
		cmd->type = MagikLauncherCommandType::ExternalLaunch;
		return true;
	}

	static const char launch_prefix[] = "mister_magik_launch ";
	if (!strncmp(line, launch_prefix, sizeof(launch_prefix) - 1))
	{
		const char *path = line + sizeof(launch_prefix) - 1;
		while (*path == ' ' || *path == '\t') path++;
		if (path[0] != '/')
		{
			set_error(cmd, "launch path must be absolute");
			return true;
		}
		if (strlen(path) >= sizeof(cmd->path))
		{
			set_error(cmd, "launch path is too long");
			return true;
		}
		snprintf(cmd->path, sizeof(cmd->path), "%s", path);
		cmd->type = MagikLauncherCommandType::Launch;
		return true;
	}

	static const char plan_prefix[] = "mister_magik_launch_plan_v1 ";
	if (!strncmp(line, plan_prefix, sizeof(plan_prefix) - 1))
	{
		const char *encoded = line + sizeof(plan_prefix) - 1;
		while (*encoded == ' ' || *encoded == '\t') encoded++;
		if (!parse_plan_payload(encoded, &cmd->plan))
		{
			set_error(cmd, "invalid structured launch plan");
			return true;
		}
		cmd->type = MagikLauncherCommandType::LaunchPlan;
		return true;
	}

	return false;
}
