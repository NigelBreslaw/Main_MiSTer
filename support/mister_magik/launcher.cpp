#include "launcher.h"
#include "launcher_state.h"
#include "launcher_command.h"
#include "launcher_diag.h"
#include "launcher_return.h"
#include "launcher_wait.h"
#include "layout.h"
#include "menu_path.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <limits.h>

#include "file_io.h"
#include "fpga_io.h"
#include "hardware.h"
#include "input.h"
#include "support/arcade/mra_loader.h"
#include "user_io.h"
#include "osd.h"
#include "video.h"

extern int video_magik_route_black();

static const char s_script_path[] = "/tmp/mister_magik_launcher";
static const char s_log_path[] = "/tmp/mister-magik-main.log";
static const char s_status_dir[] = "/tmp/mister-magik";
static const char s_status_path[] = "/tmp/mister-magik/main-status.json";
static const char s_events_path[] = "/tmp/mister-magik/events.jsonl";
static const char s_input_policy_path[] = "/tmp/mister-magik/input-policy";
static const char s_cmd_fifo_path[] = "/dev/MiSTer_cmd";
static const char *layout_path(const char *relative)
{
	static char slots[8][PATH_MAX];
	static unsigned int next = 0;
	char *out = slots[next++ % 8];
	magik_app_path(out, PATH_MAX, relative);
	return out;
}

static const char *artifact_verify_command()
{
	static char command[8192];
	snprintf(command, sizeof(command),
	"set -e; "
	"app=%s; "
	"manifest=$app/platform-v2.manifest; "
	"legacy_manifest=$app/platform-v1.manifest; "
	"if test -r \"$manifest\"; then manifest_format=mister-magik-platform-v2; else manifest=$legacy_manifest; manifest_format=mister-magik-platform-v1; fi; "
	"get() { value=$(sed -n \"s/^$1=//p\" \"$manifest\"); test -n \"$value\"; test \"$(grep -c \"^$1=\" \"$manifest\")\" -eq 1; printf %%s \"$value\"; }; "
	"main=%s; "
	"gui=$app/mister-magik-fb; "
	"catalog_builder=$app/mister-magik-catalog-builder; "
	"module=$app/mister_magik_scanout_slots.ko; "
	"module_meta=$app/mister_magik_scanout_slots.metadata.txt; "
	"rbf=$app/fpga/menu-magik-vblank-latch.rbf; "
	"rbf_meta=$app/fpga/menu-magik-vblank-latch.metadata.txt; "
	"test -r \"$manifest\" -a -r \"$main\" -a -r \"$gui\" -a -r \"$module\" -a -r \"$module_meta\" -a -r \"$rbf\" -a -r \"$rbf_meta\"; "
	"test \"$(get format)\" = \"$manifest_format\"; "
	"test \"$(get main_path)\" = \"$main\"; "
	"test \"$(get gui_path)\" = \"$gui\"; "
	"test \"$(get scanout_module_path)\" = \"$module\"; "
	"test \"$(get scanout_metadata_path)\" = \"$module_meta\"; "
	"test \"$(get latch_rbf_path)\" = \"$rbf\"; "
	"test \"$(get latch_metadata_path)\" = \"$rbf_meta\"; "
	"main_hash=$(get main_sha256); "
	"gui_hash=$(get gui_sha256); "
	"module_hash=$(get scanout_module_sha256); "
	"module_meta_hash=$(get scanout_metadata_sha256); "
	"rbf_hash=$(get latch_rbf_sha256); "
	"rbf_meta_hash=$(get latch_metadata_sha256); "
	"contract=$(get platform_contract_sha256); "
	"main_revision=$(get main_revision); "
	"magik_revision=$(get magik_revision); "
	"menu_revision=$(get menu_revision); "
	"test ${#main_hash} -eq 64 -a ${#gui_hash} -eq 64 -a ${#module_hash} -eq 64 -a ${#module_meta_hash} -eq 64 -a ${#rbf_hash} -eq 64 -a ${#rbf_meta_hash} -eq 64 -a ${#contract} -eq 64; "
	"test ${#main_revision} -eq 40 -a ${#magik_revision} -eq 40 -a ${#menu_revision} -eq 40; "
	"case \"$main_hash$gui_hash$module_hash$module_meta_hash$rbf_hash$rbf_meta_hash$contract$main_revision$magik_revision$menu_revision\" in *[!0-9a-f]*) exit 1;; esac; "
	"test \"$(sha256sum \"$main\" | awk '{print $1}')\" = \"$main_hash\"; "
	"test \"$(sha256sum \"$gui\" | awk '{print $1}')\" = \"$gui_hash\"; "
	"if test \"$manifest_format\" = mister-magik-platform-v1; then catalog_builder_hash=$(get catalog_builder_sha256); test \"$(get catalog_builder_path)\" = \"$catalog_builder\"; test -r \"$catalog_builder\"; test ${#catalog_builder_hash} -eq 64; case \"$catalog_builder_hash\" in *[!0-9a-f]*) exit 1;; esac; test \"$(sha256sum \"$catalog_builder\" | awk '{print $1}')\" = \"$catalog_builder_hash\"; fi; "
	"test \"$(sha256sum \"$module\" | awk '{print $1}')\" = \"$module_hash\"; "
	"test \"$(sha256sum \"$module_meta\" | awk '{print $1}')\" = \"$module_meta_hash\"; "
	"test \"$(sha256sum \"$rbf\" | awk '{print $1}')\" = \"$rbf_hash\"; "
	"test \"$(sha256sum \"$rbf_meta\" | awk '{print $1}')\" = \"$rbf_meta_hash\"; "
	"grep -qx \"platform_contract_sha256=$contract\" \"$module_meta\"; "
	"grep -qx \"platform_contract_sha256=$contract\" \"$rbf_meta\"; "
	"grep -qx \"module_sha256=$module_hash\" \"$module_meta\"; "
	"grep -qx \"rbf_sha256=$rbf_hash\" \"$rbf_meta\"; "
	"grep -qx \"source_commit=$menu_revision\" \"$rbf_meta\"",
	magik_app_dir(), magik_main_path());
	return command;
}
static const int s_maintenance_poll_ms = 1000;
static const int s_vt = 2;
static const char s_tty[] = "tty2";
static const char s_tty_path[] = "/dev/tty2";

static MagikLauncherState s_state = MagikLauncherState::Unconfigured;
static pid_t s_pid = 0;
static bool s_spawn_pending = false;
static int s_cmd_fd = -1;
static unsigned long s_main_generation = 0;
static unsigned long s_command_ready_ms = 0;
static char s_executable_path[PATH_MAX] = {};
static char s_last_operation[64] = "startup";
static char s_last_operation_result[32] = "completed";
static unsigned long s_last_operation_ms = 0;
static bool s_video_diagnostic_active = false;
static unsigned long s_invariant_count = 0;
static unsigned long s_crash_count = 0;
static unsigned long s_restart_count = 0;
static char s_last_invariant_kind[96] = "";
static char s_last_invariant_detail[256] = "";
static char s_last_crash_reason[256] = "";
static char s_last_crash_report[256] = "";
static char s_last_crash_report_id[128] = "";
static char s_last_crash_kind[64] = "";
static char s_last_restart_error[256] = "";
static char s_last_spawn_error[256] = "";

static void clear_input_policy_marker(void)
{
	unlink(s_input_policy_path);
}

static void eventf(const char *event, const char *fmt, ...);
static void set_status_string(char *dst, size_t len, const char *fmt, ...);

static void ensure_status_dir(void)
{
	mkdir(s_status_dir, 0755);
}

static void read_trimmed(const char *path, char *buf, size_t len)
{
	if (!buf || !len) return;
	buf[0] = 0;
	FILE *f = fopen(path, "r");
	if (!f) return;
	if (fgets(buf, len, f))
	{
		size_t n = strlen(buf);
		while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == '\t' || buf[n - 1] == ' '))
			buf[--n] = 0;
	}
	fclose(f);
}

static void json_escape(FILE *f, const char *s)
{
	fputc('"', f);
	if (s)
	{
		for (; *s; s++)
		{
			switch (*s)
			{
			case '\\': fputs("\\\\", f); break;
			case '"': fputs("\\\"", f); break;
			case '\n': fputs("\\n", f); break;
			case '\r': fputs("\\r", f); break;
			case '\t': fputs("\\t", f); break;
			default: fputc(*s, f); break;
			}
		}
	}
	fputc('"', f);
}

static void json_file_string_field(FILE *f, const char *name, const char *path, int tail_lines)
{
	fprintf(f, ",\"");
	fputs(name, f);
	fprintf(f, "\":");
	FILE *in = fopen(path, "r");
	if (!in)
	{
		fputs("null", f);
		return;
	}
	char *lines[160];
	int cap = (tail_lines > 0 && tail_lines < 160) ? tail_lines : 160;
	int count = 0;
	char buf[512];
	memset(lines, 0, sizeof(lines));
	while (fgets(buf, sizeof(buf), in))
	{
		int slot = count % cap;
		free(lines[slot]);
		lines[slot] = strdup(buf);
		count++;
	}
	fclose(in);
	fputc('"', f);
	int start = count > cap ? count - cap : 0;
	for (int i = start; i < count; i++)
	{
		char *line = lines[i % cap];
		if (!line) continue;
		for (char *s = line; *s; s++)
		{
			switch (*s)
			{
			case '\\': fputs("\\\\", f); break;
			case '"': fputs("\\\"", f); break;
			case '\n': fputs("\\n", f); break;
			case '\r': fputs("\\r", f); break;
			case '\t': fputs("\\t", f); break;
			default: fputc(*s, f); break;
			}
		}
	}
	fputc('"', f);
	for (int i = 0; i < cap; i++) free(lines[i]);
}

static void json_file_raw_or_null(FILE *f, const char *name, const char *path)
{
	fprintf(f, ",\"");
	fputs(name, f);
	fprintf(f, "\":");
	FILE *in = fopen(path, "r");
	if (!in)
	{
		fputs("null", f);
		return;
	}
	int c;
	while ((c = fgetc(in)) != EOF)
		fputc(c, f);
	fclose(in);
}

static void write_crash_report_file(const char *report_id, const char *path, pid_t old_pid, int status, const char *kind)
{
	FILE *f = fopen(path, "w");
	if (!f) return;
	char active_vt[64];
	char fb_mode[128];
	read_trimmed("/sys/class/tty/tty0/active", active_vt, sizeof(active_vt));
	read_trimmed("/sys/module/MiSTer_fb/parameters/mode", fb_mode, sizeof(fb_mode));
	fprintf(f, "{");
	fprintf(f, "\"schema\":\"mister-magik-crash-report-v1\",");
	fprintf(f, "\"source\":\"main\",");
	fprintf(f, "\"kind\":"); json_escape(f, kind ? kind : "child-exit");
	fprintf(f, ",\"report_id\":"); json_escape(f, report_id);
	fprintf(f, ",\"ts_boot_ms\":%lu", GetTimer(0));
	fprintf(f, ",\"pid\":%d", getpid());
	fprintf(f, ",\"main\":{");
	fprintf(f, "\"state\":"); json_escape(f, magik_launcher_state_name(s_state));
	fprintf(f, ",\"active_vt\":"); json_escape(f, active_vt[0] ? active_vt : "unknown");
	fprintf(f, ",\"fb_mode\":"); json_escape(f, fb_mode[0] ? fb_mode : "unknown");
	fprintf(f, "}");
	fprintf(f, ",\"child\":{");
	fprintf(f, "\"launcher_pid\":%d", old_pid);
	fprintf(f, ",\"wait_status\":%d", status);
	fprintf(f, ",\"exited\":%s", WIFEXITED(status) ? "true" : "false");
	fprintf(f, ",\"exit_status\":%d", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
	fprintf(f, ",\"signaled\":%s", WIFSIGNALED(status) ? "true" : "false");
	fprintf(f, ",\"signal\":%d", WIFSIGNALED(status) ? WTERMSIG(status) : -1);
#ifdef WCOREDUMP
	fprintf(f, ",\"core_dumped\":%s", WIFSIGNALED(status) && WCOREDUMP(status) ? "true" : "false");
#else
	fprintf(f, ",\"core_dumped\":false");
#endif
	fprintf(f, "}");
	fprintf(f, ",\"files\":{");
	fprintf(f, "\"script_path\":"); json_escape(f, s_script_path);
	json_file_raw_or_null(f, "slint_status", "/tmp/mister-magik/status.json");
	json_file_raw_or_null(f, "main_status", s_status_path);
	json_file_string_field(f, "events_tail", s_events_path, 80);
	json_file_string_field(f, "slint_log_tail", "/tmp/mister-magik-slint.log", 120);
	json_file_string_field(f, "main_log_tail", s_log_path, 120);
	fprintf(f, "}");
	fprintf(f, "}\n");
	fflush(f);
	fsync(fileno(f));
	fclose(f);
}

static void record_launcher_crash_report(pid_t old_pid, int status, const char *kind)
{
	mkdir(magik_app_dir(), 0755);
	mkdir(layout_path("crashes"), 0755);
	char report_id[128];
	snprintf(report_id, sizeof(report_id), "report-main-%lu-%d-%lu", GetTimer(0), old_pid, s_crash_count);
	char path[256];
	char tmp_path[280];
	char latest_tmp_path[280];
	snprintf(path, sizeof(path), "%s/%s.json", layout_path("crashes"), report_id);
	snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
	snprintf(latest_tmp_path, sizeof(latest_tmp_path), "%s.tmp", layout_path("crashes/latest.json"));
	write_crash_report_file(report_id, tmp_path, old_pid, status, kind);
	if (rename(tmp_path, path) == 0)
	{
		write_crash_report_file(report_id, latest_tmp_path, old_pid, status, kind);
		rename(latest_tmp_path, layout_path("crashes/latest.json"));
		set_status_string(s_last_crash_report, sizeof(s_last_crash_report), "%s", path);
		set_status_string(s_last_crash_report_id, sizeof(s_last_crash_report_id), "%s", report_id);
		set_status_string(s_last_crash_kind, sizeof(s_last_crash_kind), "%s", kind ? kind : "child-exit");
		eventf("crash_report_written", "path=%s report_id=%s", path, report_id);
	}
	else
	{
		eventf("crash_report_failed", "path=%s errno=%d", path, errno);
	}
}

static void log_msg(const char *fmt, ...)
{
	FILE *f = fopen(s_log_path, "a");
	if (!f) return;
	va_list args;
	va_start(args, fmt);
	vfprintf(f, fmt, args);
	va_end(args);
	fputc('\n', f);
	fclose(f);
}

static void reboot_log(const char *stage, const char *fmt, ...)
{
	mkdir(magik_app_dir(), 0755);
	mkdir(layout_path("bootlogs"), 0755);
	FILE *f = fopen(layout_path("bootlogs/main-reboot.log"), "a");
	if (!f) return;
	fprintf(
	    f,
	    "%lu pid=%d state=%s stage=%s detail=",
	    GetTimer(0),
	    getpid(),
	    magik_launcher_state_name(s_state),
	    stage ? stage : "unknown");
	if (fmt)
	{
		va_list args;
		va_start(args, fmt);
		vfprintf(f, fmt, args);
		va_end(args);
	}
	fputc('\n', f);
	fclose(f);
}

static void exec_linux_reboot_detached(void)
{
	setsid();
	int null_fd = open("/dev/null", O_RDWR);
	if (null_fd >= 0)
	{
		dup2(null_fd, STDIN_FILENO);
		dup2(null_fd, STDOUT_FILENO);
		dup2(null_fd, STDERR_FILENO);
		if (null_fd > STDERR_FILENO)
			close(null_fd);
	}
	long open_max = sysconf(_SC_OPEN_MAX);
	if (open_max < 0 || open_max > 4096)
		open_max = 1024;
	for (int fd = STDERR_FILENO + 1; fd < open_max; fd++)
		close(fd);
	execl("/sbin/reboot", "reboot", NULL);
	reboot_log("linux_reboot_exec_failed", "errno=%d", errno);
	_exit(127);
}

static void event_jsonl(const char *event, const char *detail)
{
	ensure_status_dir();
	FILE *f = fopen(s_events_path, "a");
	if (!f) return;
	fprintf(f, "{\"ts_boot_ms\":%lu,\"source\":\"main\",\"pid\":%d,\"event\":", GetTimer(0), getpid());
	json_escape(f, event ? event : "unknown");
	fprintf(f, ",\"detail\":");
	json_escape(f, detail ? detail : "");
	fprintf(f, "}\n");
	fclose(f);
}

static void eventf(const char *event, const char *fmt, ...)
{
	char detail[512];
	detail[0] = 0;
	if (fmt)
	{
		va_list args;
		va_start(args, fmt);
		vsnprintf(detail, sizeof(detail), fmt, args);
		va_end(args);
		detail[sizeof(detail) - 1] = 0;
	}
	event_jsonl(event, detail);
	mister_magik_status_write();
}

static bool transition(MagikLauncherEvent event)
{
	MagikLauncherState next = s_state;
	if (!magik_launcher_transition(s_state, event, &next))
	{
		snprintf(s_last_operation, sizeof(s_last_operation), "%s", magik_launcher_event_name(event));
		snprintf(s_last_operation_result, sizeof(s_last_operation_result), "rejected");
		s_last_operation_ms = GetTimer(0);
		eventf("launcher_transition_rejected", "state=%s event=%s", magik_launcher_state_name(s_state), magik_launcher_event_name(event));
		mister_magik_status_write();
		return false;
	}
	eventf("launcher_transition", "from=%s event=%s to=%s", magik_launcher_state_name(s_state), magik_launcher_event_name(event), magik_launcher_state_name(next));
	s_state = next;
	snprintf(s_last_operation, sizeof(s_last_operation), "%s", magik_launcher_event_name(event));
	snprintf(s_last_operation_result, sizeof(s_last_operation_result), "completed");
	s_last_operation_ms = GetTimer(0);
	mister_magik_status_write();
	return true;
}

static bool deploy_lock_active(void)
{
	return access(layout_path("deploy.lock"), F_OK) == 0;
}

static void close_command_fifo(void)
{
	if (s_cmd_fd >= 0)
	{
		close(s_cmd_fd);
		s_cmd_fd = -1;
		s_command_ready_ms = 0;
		mister_magik_status_write();
	}
}

static void ensure_command_fifo(void)
{
	if (s_cmd_fd >= 0) return;
	if (access(s_cmd_fifo_path, F_OK) != 0)
		mkfifo(s_cmd_fifo_path, 0666);
	s_cmd_fd = open(s_cmd_fifo_path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (s_cmd_fd < 0)
		eventf("command_fifo_open_failed", "path=%s errno=%d", s_cmd_fifo_path, errno);
	else
	{
		if (!s_main_generation) s_main_generation = GetTimer(0);
		if (!s_executable_path[0])
		{
			ssize_t len = readlink("/proc/self/exe", s_executable_path, sizeof(s_executable_path) - 1);
			if (len > 0) s_executable_path[len] = 0;
		}
		s_command_ready_ms = GetTimer(0);
		mister_magik_status_write();
	}
}

static void reset_launcher_tty(void);
static void spawn_launcher(void);

static void set_status_string(char *dst, size_t len, const char *fmt, ...)
{
	if (!dst || !len) return;
	if (!fmt)
	{
		dst[0] = 0;
		return;
	}
	va_list args;
	va_start(args, fmt);
	vsnprintf(dst, len, fmt, args);
	va_end(args);
	dst[len - 1] = 0;
}

static void stop_launcher_child(void)
{
	if (!s_pid) return;
	pid_t pid = s_pid;
	kill(-pid, SIGTERM);
	kill(pid, SIGTERM);
	for (int i = 0; i < 50; i++)
	{
		if (waitpid(pid, NULL, WNOHANG) == pid)
		{
			s_pid = 0;
			return;
		}
		usleep(10000);
	}
	kill(-pid, SIGKILL);
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
	s_pid = 0;
}

static void complete_handoff_to_game(const char *path, bool external = false)
{
	if (!magik_launcher_accepts_handoff(s_state))
	{
		if (external)
			eventf("external_load_core_rejected", "state=%s", magik_launcher_state_name(s_state));
		else
			eventf("handoff_rejected", "command=launch state=%s", magik_launcher_state_name(s_state));
		return;
	}
	if (external)
		eventf("external_load_core_handoff", "path=%s", path ? path : "");
	transition(MagikLauncherEvent::LaunchRequested);
	s_spawn_pending = false;
	close_command_fifo();
	stop_launcher_child();
	reset_launcher_tty();
	input_switch(1);
	eventf("handoff_launch", "path=%s", path ? path : "");
	user_io_ensure_sdram_config();
	if (path && isXmlName(path)) xml_load(path);
	else if (path) fpga_load_rbf(path);
	transition(MagikLauncherEvent::HandoffComplete);
}

static void complete_handoff_to_game_plan(const MagikStructuredLaunchPlan *plan)
{
	if (!magik_launcher_accepts_handoff(s_state))
	{
		eventf("handoff_rejected", "command=launch_plan state=%s", magik_launcher_state_name(s_state));
		return;
	}
	if (!plan || !plan->core_path[0] || !plan->payload_path[0])
	{
		eventf("handoff_launch_plan_invalid", "reason=missing-fields");
		return;
	}
	const char *rbf = mra_resolve_rbf_name(plan->core_path, 0);
	if (!rbf)
	{
		eventf("handoff_launch_plan_invalid", "reason=rbf-not-found core=%s", plan->core_path);
		return;
	}
	transition(MagikLauncherEvent::LaunchRequested);
	s_spawn_pending = false;
	close_command_fifo();
	stop_launcher_child();
	reset_launcher_tty();
	input_switch(1);
	eventf("handoff_launch_plan", "core=%s payload=%s", rbf, plan->payload_path);
	user_io_ensure_sdram_config();
	fpga_load_rbf(rbf, NULL, plan->arg);
	transition(MagikLauncherEvent::HandoffComplete);
}

static void complete_handoff_to_menu(void)
{
	if (!magik_launcher_accepts_handoff(s_state))
	{
		eventf("handoff_rejected", "command=exit_to_menu state=%s", magik_launcher_state_name(s_state));
		return;
	}
	transition(MagikLauncherEvent::ExitRequested);
	clear_input_policy_marker();
	s_spawn_pending = false;
	close_command_fifo();
	stop_launcher_child();
	reset_launcher_tty();
	input_switch(1);
	transition(MagikLauncherEvent::HandoffComplete);
	video_fb_enable(0);
	video_menu_bg(user_io_status_get("[3:1]"));
	eventf("handoff_exit_to_menu", "done=1");
}

static void suspend_launcher(void)
{
	if (s_state == MagikLauncherState::LauncherSuspended)
	{
		eventf("launcher_suspend_ignored", "state=%s", magik_launcher_state_name(s_state));
		return;
	}
	if (s_state != MagikLauncherState::LauncherActive)
	{
		eventf("launcher_suspend_rejected", "state=%s", magik_launcher_state_name(s_state));
		return;
	}
	transition(MagikLauncherEvent::SuspendRequested);
	s_spawn_pending = false;
	stop_launcher_child();
	transition(MagikLauncherEvent::ChildExitedExpectedly);
	eventf("launcher_suspended", "done=1");
}

static void resume_launcher(void)
{
	if (s_state != MagikLauncherState::LauncherSuspended)
	{
		eventf("launcher_resume_rejected", "state=%s", magik_launcher_state_name(s_state));
		return;
	}
	transition(MagikLauncherEvent::ResumeRequested);
	s_spawn_pending = true;
	eventf("launcher_resume_requested", "done=1");
}

static void restart_launcher(void)
{
	s_restart_count++;
	s_last_restart_error[0] = 0;
	switch (magik_launcher_restart_action(s_state))
	{
	case MagikLauncherRestartAction::ResumeSuspended:
		transition(MagikLauncherEvent::ResumeRequested);
		s_spawn_pending = true;
		eventf("launcher_restart_requested", "state=LauncherSuspended");
		return;
	case MagikLauncherRestartAction::RespawnCrashed:
		reset_launcher_tty();
		user_io_osd_key_enable(0);
		OsdDisable();
		input_switch(0);
		if (!transition(MagikLauncherEvent::BeginEnterLauncher))
		{
			set_status_string(s_last_restart_error, sizeof(s_last_restart_error), "transition_failed state=LauncherCrashed event=BeginEnterLauncher");
			mister_magik_status_write();
			return;
		}
		s_spawn_pending = true;
		eventf("launcher_restart_requested", "state=LauncherCrashed");
		spawn_launcher();
		return;
	case MagikLauncherRestartAction::RestartActive:
		eventf("launcher_restart_requested", "state=LauncherActive");
		transition(MagikLauncherEvent::SuspendRequested);
		s_spawn_pending = false;
		stop_launcher_child();
		reset_launcher_tty();
		transition(MagikLauncherEvent::ChildExitedExpectedly);
		transition(MagikLauncherEvent::ResumeRequested);
		s_spawn_pending = true;
		eventf("launcher_restart_queued", "done=1");
		return;
	case MagikLauncherRestartAction::Reject:
		set_status_string(s_last_restart_error, sizeof(s_last_restart_error), "rejected state=%s", magik_launcher_state_name(s_state));
		eventf("launcher_restart_rejected", "state=%s", magik_launcher_state_name(s_state));
		return;
	}
}

static bool begin_reboot_lockdown(const char *method)
{
	if (s_state != MagikLauncherState::LauncherActive)
	{
		reboot_log("rejected", "state=%s", magik_launcher_state_name(s_state));
		eventf("launcher_reboot_rejected", "state=%s", magik_launcher_state_name(s_state));
		return false;
	}
	reboot_log("requested", "state=LauncherActive method=%s", method);
	eventf("launcher_reboot_requested", "state=LauncherActive method=%s", method);
	transition(MagikLauncherEvent::RebootRequested);
	s_spawn_pending = false;
	close_command_fifo();
	user_io_osd_key_enable(0);
	OsdDisable();
	input_switch(0);
	reboot_log("lockdown_entered", "osd=0 input=0 cmd_fifo=closed method=%s", method);
	return true;
}

static void reboot_launcher(void)
{
	if (!begin_reboot_lockdown("linux-reboot"))
		return;
	reboot_log("sync_start", "method=linux-reboot");
	sync();
	reboot_log("sync_done", "method=linux-reboot");
	eventf("launcher_reboot_now", "method=linux-reboot");

	reboot_log("linux_reboot_fork_start", "path=/sbin/reboot");
	pid_t pid = fork();
	if (pid < 0)
	{
		reboot_log("linux_reboot_fork_failed", "errno=%d", errno);
		eventf("launcher_reboot_failed", "stage=fork errno=%d", errno);
		return;
	}
	if (!pid)
	{
		exec_linux_reboot_detached();
	}
	reboot_log("linux_reboot_spawned", "pid=%d", pid);
	eventf("launcher_reboot_spawned", "pid=%d", pid);
}

static void direct_reset_launcher(bool pre_sync)
{
	if (!begin_reboot_lockdown(pre_sync ? "direct-reset" : "direct-reset-no-sync"))
		return;
	if (pre_sync)
	{
		reboot_log("sync_start", "method=direct-reset");
		sync();
		reboot_log("sync_done", "method=direct-reset");
	}
	else
	{
		reboot_log("sync_skipped", "method=direct-reset-no-sync");
	}
	eventf("launcher_reboot_now", "method=%s unsafe=1", pre_sync ? "direct-reset" : "direct-reset-no-sync");
	reboot_log("direct_reset_start", "pre_sync=%d unsafe=1", pre_sync ? 1 : 0);
	reboot(1); // unsafe direct reset experiment path only
}

static void hdmi_power_cycle(void)
{
	eventf("display_diag_hdmi_power_cycle_start", "state=%s", magik_launcher_state_name(s_state));
	video_hdmi_power(0);
	usleep(250000);
	video_hdmi_power(1);
	usleep(250000);
	eventf("display_diag_hdmi_power_cycle_done", "state=%s", magik_launcher_state_name(s_state));
}

static void video_adjust_diagnostic(void)
{
	eventf("display_diag_adjust_start", "state=%s", magik_launcher_state_name(s_state));
	video_mode_adjust(true);
	eventf("display_diag_adjust_done", "state=%s", magik_launcher_state_name(s_state));
}

static void video_reinit_diagnostic(void)
{
	eventf("display_diag_reinit_start", "state=%s", magik_launcher_state_name(s_state));
	if (s_state == MagikLauncherState::LauncherActive)
	{
		suspend_launcher();
	}
	else if (s_pid)
	{
		s_spawn_pending = false;
		stop_launcher_child();
	}
	reset_launcher_tty();
	input_switch(1);
	s_video_diagnostic_active = true;
	video_fb_enable(0);
	video_reinit();
	video_menu_bg(user_io_status_get("[3:1]"));
	s_video_diagnostic_active = false;
	eventf("display_diag_reinit_done", "state=%s", magik_launcher_state_name(s_state));
}

static void process_command_line(const char *line)
{
	MagikLauncherCommand cmd;
	if (!magik_launcher_parse_command(line, &cmd))
		return;
	if (cmd.type == MagikLauncherCommandType::Invalid)
	{
		eventf("command_invalid", "%s", cmd.error);
		return;
	}
	if (cmd.type == MagikLauncherCommandType::Launch)
	{
		complete_handoff_to_game(cmd.path);
		return;
	}
	if (cmd.type == MagikLauncherCommandType::ExternalLaunch)
	{
		complete_handoff_to_game(cmd.path, true);
		return;
	}
	if (cmd.type == MagikLauncherCommandType::LaunchPlan)
	{
		complete_handoff_to_game_plan(&cmd.plan);
		return;
	}
	if (cmd.type == MagikLauncherCommandType::ExitToMenu)
	{
		complete_handoff_to_menu();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::Suspend)
	{
		suspend_launcher();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::Resume)
	{
		resume_launcher();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::RestartLauncher)
	{
		restart_launcher();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::HdmiPowerCycle)
	{
		hdmi_power_cycle();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::VideoAdjust)
	{
		video_adjust_diagnostic();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::VideoReinit)
	{
		video_reinit_diagnostic();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::Reboot)
	{
		reboot_launcher();
		return;
	}
	if (cmd.type == MagikLauncherCommandType::DirectReset)
	{
		direct_reset_launcher(true);
		return;
	}
	if (cmd.type == MagikLauncherCommandType::DirectResetNoSync)
	{
		direct_reset_launcher(false);
		return;
	}
}

static void poll_command_fifo(void)
{
	ensure_command_fifo();
	if (s_cmd_fd < 0) return;
	char buf[8192];
	int len = read(s_cmd_fd, buf, sizeof(buf) - 1);
	if (len <= 0) return;
	buf[len] = 0;
	char *line = buf;
	while (line && *line)
	{
		char *next = strchr(line, '\n');
		if (next) *next++ = 0;
		process_command_line(line);
		line = next;
	}
}

static void clear_launcher_tty(void)
{
	int tty_fd = open(s_tty_path, O_WRONLY | O_CLOEXEC);
	if (tty_fd >= 0)
	{
		static const char blank[] = "\033[?25l\033[40m\033[30m\033[2J\033[H";
		if (write(tty_fd, blank, sizeof(blank) - 1) < 0) {}
		close(tty_fd);
	}
}

static void reset_launcher_tty(void)
{
	int tty_fd = open(s_tty_path, O_RDWR | O_NOCTTY | O_CLOEXEC);
	if (tty_fd >= 0)
	{
		ioctl(tty_fd, KDSETMODE, KD_TEXT);
		ioctl(tty_fd, KDSKBMODE, K_XLATE);

		struct vt_mode vtmode;
		memset(&vtmode, 0, sizeof(vtmode));
		vtmode.mode = VT_AUTO;
		ioctl(tty_fd, VT_SETMODE, &vtmode);

		struct termios tio;
		if (!tcgetattr(tty_fd, &tio))
		{
			tio.c_iflag |= BRKINT | ICRNL | IXON | IMAXBEL;
			tio.c_iflag &= ~(IGNBRK | INLCR | IGNCR | IXOFF);
			tio.c_oflag |= OPOST | ONLCR;
			tio.c_lflag |= ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN;
			tio.c_lflag &= ~(NOFLSH | TOSTOP);
			tio.c_cflag |= CREAD;
			tio.c_cc[VMIN] = 1;
			tio.c_cc[VTIME] = 0;
			tcsetattr(tty_fd, TCSANOW, &tio);
		}

		static const char reset[] = "\033[0m\033[?25h\033[37m\033[40m\033[2J\033[H";
		if (write(tty_fd, reset, sizeof(reset) - 1) < 0) {}
		close(tty_fd);
	}
}

static bool write_launcher_script(const char *path)
{
	FILE *f = fopen(s_script_path, "w");
	if (!f) return false;
	bool return_spawn = magik_launcher_consume_return_spawn(get_rbf_name(), get_rbf_path());
	fprintf(f,
	        "#!/bin/bash\n"
	        "export LC_ALL=en_US.UTF-8\n"
	        "export HOME=/root\n"
	        "export MISTER_MAGIK_PARENT=main-mister\n"
	        "export MISTER_MAGIK_RETURN_TO_LAUNCHER=%d\n"
	        "if [ -f \"%s\" ]; then\n"
	        "  . \"%s\"\n"
	        "fi\n"
	        ": >/tmp/mister-magik-slint.log\n"
	        "if %s && [ -f \"%s\" ] && ! grep -q '^mister_magik_scanout_slots ' /proc/modules 2>/dev/null; then\n"
	        "  insmod \"%s\" >>/tmp/mister-magik-slint.log 2>&1 || true\n"
	        "fi\n"
	        "if [ -e /dev/mister-magik-scanout-slots ]; then\n"
	        "  echo 'scanout-slots-supervisor=device-ready' >>/tmp/mister-magik-slint.log\n"
	        "else\n"
	        "  echo 'scanout-slots-supervisor=device-missing' >>/tmp/mister-magik-slint.log\n"
	        "fi\n"
	        "if ! \"$MISTER_MAGIK_PATH\" latch-readiness-report >>/tmp/mister-magik-slint.log 2>&1; then\n"
	        "  echo 'latch_startup_tsv valid=0 action=compatibility-screen reason=readiness-probe-failed' >>/tmp/mister-magik-slint.log\n"
	        "else\n"
	        "  echo 'latch_startup_tsv valid=1 action=launch-latch-ui reason=ready' >>/tmp/mister-magik-slint.log\n"
	        "fi\n"
	        "printf '\\033[0m\\033[?25l\\033[37m\\033[40m\\033[2J\\033[H'\n"
	        "exec \"$MISTER_MAGIK_PATH\" ui launcher 0 >>/tmp/mister-magik-slint.log 2>&1\n",
	        return_spawn ? 1 : 0,
	        layout_path("launcher.env"),
	        layout_path("launcher.env"),
	        artifact_verify_command(),
	        layout_path("mister_magik_scanout_slots.ko"),
	        layout_path("mister_magik_scanout_slots.ko"));
	fclose(f);
	chmod(s_script_path, 0755);
	eventf("launcher_script_written", "script=%s path=%s return_spawn=%d", s_script_path, path ? path : "", return_spawn ? 1 : 0);
	return true;
}

static void spawn_launcher(void)
{
	if (deploy_lock_active())
	{
		set_status_string(s_last_spawn_error, sizeof(s_last_spawn_error), "deploy_lock_active path=%s", layout_path("deploy.lock"));
		eventf("launcher_spawn_deferred_deploy_lock", "path=%s", layout_path("deploy.lock"));
		return;
	}

	char path[2100];
	strncpy(path, getFullPath(magik_launcher_relative_path()), sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';
	if (!FileExists(magik_launcher_relative_path(), 0))
	{
		set_status_string(s_last_spawn_error, sizeof(s_last_spawn_error), "missing path=%s", magik_launcher_relative_path());
		eventf("launcher_missing", "path=%s", magik_launcher_relative_path());
		return;
	}
	if (!write_launcher_script(path))
	{
		set_status_string(s_last_spawn_error, sizeof(s_last_spawn_error), "script_failed script=%s", s_script_path);
		eventf("launcher_script_failed", "script=%s", s_script_path);
		return;
	}

	user_io_osd_key_enable(0);
	clear_launcher_tty();
	OsdDisable();
	eventf("launcher_spawn_black_route_start", "state=%s", magik_launcher_state_name(s_state));
	if (video_magik_route_black())
		eventf("launcher_spawn_black_route_completed", "source=main format=565");
	else
		eventf("launcher_spawn_black_route_failed", "source=main");

	s_pid = fork();
	if (s_pid < 0)
	{
		set_status_string(s_last_spawn_error, sizeof(s_last_spawn_error), "fork_failed errno=%d", errno);
		eventf("launcher_fork_failed", "errno=%d", errno);
		return;
	}
	if (!s_pid)
	{
		setenv("MISTER_MAGIK_PATH", path, 1);
		setsid();
		execl("/sbin/agetty", "/sbin/agetty", "-a", "root", "-l", s_script_path, "-i", "--nohostname", "-L", s_tty, "linux", NULL);
		_exit(127);
	}

	s_last_spawn_error[0] = 0;
	log_msg("spawned pid=%d path=%s", s_pid, path);
	video_chvt(s_vt);
	input_switch(0);
	transition(MagikLauncherEvent::ChildSpawned);
}

bool mister_magik_launcher_configured(void)
{
	return FileExists(magik_launcher_relative_path(), 0) != 0;
}

bool mister_magik_launcher_active(void)
{
	if (s_video_diagnostic_active) return false;
	return magik_launcher_is_active(s_state);
}

bool mister_magik_launcher_main_framebuffer_suppressed(void)
{
	if (s_video_diagnostic_active) return false;
	return s_state == MagikLauncherState::BootingMain ||
	       magik_launcher_is_active(s_state);
}

void mister_magik_launcher_begin_boot_lockdown(void)
{
	if (!mister_magik_launcher_configured()) return;
	if (s_state == MagikLauncherState::Unconfigured)
		transition(MagikLauncherEvent::Configured);
	if (s_state != MagikLauncherState::BootingMain) return;
	user_io_osd_key_enable(0);
	OsdDisable();
	input_switch(0);
	eventf("launcher_boot_lockdown", "state=%s", magik_launcher_state_name(s_state));
	mister_magik_status_write();
}

void mister_magik_status_write(void)
{
	ensure_status_dir();
	FILE *f = fopen(s_status_path, "w");
	if (!f) return;
	char active_vt[64];
	char fb_mode[128];
	read_trimmed("/sys/class/tty/tty0/active", active_vt, sizeof(active_vt));
	read_trimmed("/sys/module/MiSTer_fb/parameters/mode", fb_mode, sizeof(fb_mode));
	fprintf(f, "{");
	fprintf(f, "\"schema\":\"mister-magik-main-status-v2\",");
	fprintf(f, "\"ts_boot_ms\":%lu,", GetTimer(0));
	fprintf(f, "\"pid\":%d,", getpid());
	if (!s_main_generation) s_main_generation = GetTimer(0);
	fprintf(f, "\"main_generation\":%lu,", s_main_generation);
	fprintf(f, "\"executable_path\":");
	json_escape(f, s_executable_path[0] ? s_executable_path : "unknown");
	bool command_ready = s_cmd_fd >= 0 || input_command_fifo_ready();
	fprintf(f, ",\"command_channel\":\"%s\",", command_ready ? "ready" : "unavailable");
	fprintf(f, "\"command_ready_ms\":%lu,", s_command_ready_ms);
	struct stat command_stat;
	unsigned long command_inode = (s_cmd_fd >= 0 && !fstat(s_cmd_fd, &command_stat)) ? (unsigned long)command_stat.st_ino : input_command_fifo_inode();
	fprintf(f, "\"command_fifo_inode\":%lu,", command_inode);
	fprintf(f, "\"launcher_pid\":%d,", s_pid);
	fprintf(f, "\"launcher_state\":");
	json_escape(f, magik_launcher_state_name(s_state));
	fprintf(f, ",\"last_operation\":");
	json_escape(f, s_last_operation);
	fprintf(f, ",\"last_operation_result\":");
	json_escape(f, s_last_operation_result);
	fprintf(f, ",\"last_operation_ms\":%lu", s_last_operation_ms);
	fprintf(f, ",\"launcher_active\":%s,", mister_magik_launcher_active() ? "true" : "false");
	fprintf(f, "\"deploy_locked\":%s,", deploy_lock_active() ? "true" : "false");
	fprintf(f, "\"crash_count\":%lu,", s_crash_count);
	fprintf(f, "\"restart_count\":%lu,", s_restart_count);
	fprintf(f, "\"last_crash_reason\":");
	json_escape(f, s_last_crash_reason);
	fprintf(f, ",\"last_crash_report\":");
	json_escape(f, s_last_crash_report);
	fprintf(f, ",\"last_crash_report_id\":");
	json_escape(f, s_last_crash_report_id);
	fprintf(f, ",\"last_crash_kind\":");
	json_escape(f, s_last_crash_kind);
	fprintf(f, ",\"last_restart_error\":");
	json_escape(f, s_last_restart_error);
	fprintf(f, ",\"last_spawn_error\":");
	json_escape(f, s_last_spawn_error);
	fprintf(f, ",");
	fprintf(f, "\"active_vt\":");
	json_escape(f, active_vt[0] ? active_vt : "unknown");
	fprintf(f, ",\"fb_mode\":");
	json_escape(f, fb_mode[0] ? fb_mode : "unknown");
	fprintf(f, ",\"scanout_slots_module_loaded\":%s", access("/sys/module/mister_magik_scanout_slots", F_OK) == 0 ? "true" : "false");
	fprintf(f, ",\"scanout_slots_device_ready\":%s", access("/dev/mister-magik-scanout-slots", R_OK | W_OK) == 0 ? "true" : "false");
	fprintf(f, ",\"invariant_count\":%lu", s_invariant_count);
	fprintf(f, ",\"last_invariant_kind\":");
	json_escape(f, s_last_invariant_kind);
	fprintf(f, ",\"last_invariant_detail\":");
	json_escape(f, s_last_invariant_detail);
	fprintf(f, "}\n");
	fclose(f);
}

void mister_magik_launcher_route_early_black(void)
{
	if (!mister_magik_launcher_configured()) return;
	if (s_state == MagikLauncherState::Unconfigured)
		transition(MagikLauncherEvent::Configured);

	eventf("early_black_main_route_start", "state=%s", magik_launcher_state_name(s_state));
	if (video_magik_route_black())
	{
		eventf("early_black_route_frame_copied", "source=main format=565");
		eventf("early_black_route_completed", "source=main format=565");
		return;
	}
	eventf("early_black_main_route_failed", "fallback=fork_exec");

	char path[2100];
	strncpy(path, getFullPath(magik_launcher_relative_path()), sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';
	eventf("early_black_spawn", "path=%s", path);

	pid_t pid = fork();
	if (pid < 0)
	{
		eventf("early_black_fork_failed", "errno=%d", errno);
		return;
	}
	if (!pid)
	{
		setenv("MISTER_MAGIK_PARENT", "main-mister", 1);
		execl(path, path, "early-black", NULL);
		_exit(127);
	}
	int status = 0;
	for (int i = 0; i < 100; i++)
	{
		if (waitpid(pid, &status, WNOHANG) == pid)
		{
			eventf("early_black_exit", "pid=%d status=%d", pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
			return;
		}
		usleep(10000);
	}
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
	eventf("early_black_timeout", "pid=%d", pid);
}

void mister_magik_launcher_enter_after_menu_init(void)
{
	if (!mister_magik_launcher_configured()) return;
	clear_input_policy_marker();
	if (s_state == MagikLauncherState::BootingMain)
		transition(MagikLauncherEvent::BeginEnterLauncher);
	s_spawn_pending = true;
	user_io_osd_key_enable(0);
	OsdDisable();
	input_switch(0);
	mister_magik_status_write();
}

void mister_magik_launcher_poll(void)
{
	if (!mister_magik_launcher_configured()) return;

	static bool scanout_slots_module_loaded;
	static bool scanout_slots_device_ready;
	bool module_loaded = access("/sys/module/mister_magik_scanout_slots", F_OK) == 0;
	bool device_ready = access("/dev/mister-magik-scanout-slots", R_OK | W_OK) == 0;
	if (module_loaded != scanout_slots_module_loaded || device_ready != scanout_slots_device_ready)
	{
		scanout_slots_module_loaded = module_loaded;
		scanout_slots_device_ready = device_ready;
		mister_magik_status_write();
	}

	if (s_pid)
	{
		int status = 0;
		if (waitpid(s_pid, &status, WNOHANG) == s_pid)
		{
			pid_t old_pid = s_pid;
			s_pid = 0;
			if (s_state == MagikLauncherState::LauncherRebooting)
			{
				transition(MagikLauncherEvent::ChildExitedExpectedly);
				eventf("launcher_exited_during_reboot", "pid=%d status=%d", old_pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
				return;
			}
			if (s_state == MagikLauncherState::LauncherSuspending)
			{
				transition(MagikLauncherEvent::ChildExitedExpectedly);
				eventf("launcher_exited_during_suspend", "pid=%d status=%d", old_pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
				return;
			}
			if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			{
				reset_launcher_tty();
				transition(MagikLauncherEvent::SuspendRequested);
				transition(MagikLauncherEvent::ChildExitedExpectedly);
				eventf("launcher_exited_cleanly", "pid=%d status=0", old_pid);
				return;
			}
			reset_launcher_tty();
			s_crash_count++;
			set_status_string(
			    s_last_crash_reason,
			    sizeof(s_last_crash_reason),
			    "pid=%d exited=%d exit_status=%d signaled=%d signal=%d",
			    old_pid,
			    WIFEXITED(status) ? 1 : 0,
			    WIFEXITED(status) ? WEXITSTATUS(status) : -1,
			    WIFSIGNALED(status) ? 1 : 0,
			    WIFSIGNALED(status) ? WTERMSIG(status) : -1);
			record_launcher_crash_report(old_pid, status, "unexpected-child-exit");
			transition(MagikLauncherEvent::ChildExitedUnexpectedly);
			eventf("launcher_exited_unexpectedly", "pid=%d status=%d", old_pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
			return;
		}
	}

	if (s_spawn_pending && s_state == MagikLauncherState::EnteringLauncher)
	{
		spawn_launcher();
	}
	if (magik_launcher_polls_commands(s_state))
	{
		poll_command_fifo();
	}
}

bool mister_magik_launcher_idle_waits(void)
{
	return mister_magik_launcher_configured() && magik_launcher_idle_waits(s_state);
}

void mister_magik_launcher_wait_for_activity(void)
{
	ensure_command_fifo();
	(void)magik_launcher_wait_for_activity(s_cmd_fd, s_maintenance_poll_ms);
}

bool mister_magik_launcher_maybe_load_latch_menu(const char *path)
{
	if (!mister_magik_launcher_configured()) return false;
	bool default_menu = !path || !path[0];
	if (!default_menu && strcasecmp(path, "menu.rbf") && strcasecmp(path, "/media/fat/menu.rbf")) return false;
	const char *latch_rbf_path = magik_latch_menu_path();
	if (access(latch_rbf_path, R_OK) != 0)
	{
		eventf("latch_menu_rbf_missing", "path=%s", latch_rbf_path);
		return false;
	}
	if (system(artifact_verify_command()) != 0)
	{
		eventf("latch_artifact_verification_failed", "fallback=stock-menu");
		return false;
	}
	eventf("latch_menu_rbf_load", "from=%s to=%s", default_menu ? "(default-menu)" : path, latch_rbf_path);
	magik_launcher_mark_latch_menu_return(path);
	fpga_load_rbf(latch_rbf_path);
	return true;
}

void mister_magik_record_invariant(const char *kind, const char *detail)
{
	MagikLauncherInvariant event;
	magik_launcher_invariant_init(&event, kind, detail);
	s_invariant_count++;
	snprintf(s_last_invariant_kind, sizeof(s_last_invariant_kind), "%s", event.kind);
	snprintf(s_last_invariant_detail, sizeof(s_last_invariant_detail), "%s", event.detail);
	eventf(event.kind, "%s", event.detail);
}
