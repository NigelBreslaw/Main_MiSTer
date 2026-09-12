#!/usr/bin/env python3

import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = ROOT / "support/mister_magik/launcher.cpp"
WRITER_SILENCE = ROOT / "support/mister_magik/writer_silence.cpp"


def source_function(source: str, start: str, end: str) -> str:
    offset = source.index(start)
    return source[offset : source.index(end, offset)]


class LauncherContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = LAUNCHER.read_text()

    def compile(self, directory: Path, name: str, source: str, *extra: Path) -> Path:
        cpp = directory / f"{name}.cpp"
        cpp.write_text(source)
        binary = directory / name
        subprocess.run(
            [
                "c++",
                "-std=c++14",
                "-Wall",
                "-Wextra",
                "-I",
                str(ROOT),
                str(cpp),
                *(str(path) for path in extra),
                "-o",
                str(binary),
            ],
            check=True,
        )
        return binary

    def test_writer_silence_requires_active_graphics_vt(self):
        spawn = source_function(
            self.source,
            "static MagikLauncherSpawnResult spawn_launcher(void)",
            "void mister_magik_launcher_route_early_black(void)",
        )
        self.assertLess(
            spawn.index("acquire_writer_silence()"),
            spawn.index("run_launcher_readiness_preflight(path)"),
        )
        self.assertNotIn("video_chvt(s_vt)", spawn)

        activate = source_function(
            self.source,
            "static bool activate_and_verify_launcher_vt(void)",
            "static bool set_and_verify_launcher_graphics(void)",
        )
        graphics = source_function(
            self.source,
            "static bool set_and_verify_launcher_graphics(void)",
            "static bool acquire_writer_silence(void)",
        )
        acquire = source_function(
            self.source,
            "static bool acquire_writer_silence(void)",
            "static void release_writer_silence(void)\n{",
        )
        harness = r'''
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "support/mister_magik/writer_silence.h"

struct vt_stat { unsigned short v_active, v_signal, v_state; };
static const int O_RDONLY=1, O_RDWR=2, O_NOCTTY=4, O_CLOEXEC=8;
static const unsigned long VT_ACTIVATE=10, VT_WAITACTIVE=11, VT_GETSTATE=12;
static const unsigned long KDSETMODE=20, KDGETMODE=21;
static const int KD_TEXT=0, KD_GRAPHICS=1;
static const int s_vt=2;
static const char *s_tty_path="/dev/tty2";
static MagikWriterSilence s_writer_silence;
static int fault, active_vt=1, graphics_mode=KD_TEXT, resets;
static bool s_module_preflight_started, s_module_preflight_passed;
static std::vector<std::string> calls;

static int fake_open(const char *path, int) {
    if (!strcmp(path,"/dev/tty0")) return fault==1 ? -1 : 10;
    if (!strcmp(path,"/dev/tty2")) return fault==6 ? -1 : 20;
    return -1;
}
static int fake_ioctl(int, unsigned long request, int argument) {
    if (request==VT_ACTIVATE) {
        calls.push_back("activate");
        return fault==2 ? -1 : 0;
    }
    if (request==VT_WAITACTIVE) {
        calls.push_back("wait");
        if (fault==3) return -1;
        active_vt=fault==5 ? 1 : argument;
        return 0;
    }
    if (request==KDSETMODE) {
        calls.push_back("kdset");
        if (fault==7) return -1;
        graphics_mode=argument;
        return 0;
    }
    return -1;
}
static int fake_ioctl(int, unsigned long request, struct vt_stat *state) {
    calls.push_back("getstate");
    if (request!=VT_GETSTATE || fault==4) return -1;
    state->v_active=active_vt;
    return 0;
}
static int fake_ioctl(int, unsigned long request, int *mode) {
    calls.push_back("kdget");
    if (request!=KDGETMODE || fault==8) return -1;
    *mode=fault==9 ? KD_TEXT : graphics_mode;
    return 0;
}
static int fake_close(int) { return 0; }
static void eventf(const char *, const char *, ...) {}
static void reset_launcher_tty(void) {
    resets++;
    s_writer_silence.release();
    s_module_preflight_started=false;
    s_module_preflight_passed=false;
}
#define open fake_open
#define ioctl fake_ioctl
#define close fake_close
'''
        main = r'''
int main(int argc, char **argv) {
    fault=argc > 1 ? atoi(argv[1]) : 0;
    bool ready=acquire_writer_silence();
    printf("ready=%d module=%d active=%d graphics=%d resets=%d calls=", ready,
        s_writer_silence.ready_for_module(), active_vt, graphics_mode, resets);
    for (size_t i=0; i<calls.size(); ++i) printf("%s%s", i ? "," : "", calls[i].c_str());
    puts("");
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="main-writer-silence-") as temporary:
            binary = self.compile(
                Path(temporary),
                "writer-silence",
                harness + activate + graphics + acquire + main,
                WRITER_SILENCE,
            )
            success = subprocess.run([binary, "0"], check=True, text=True, capture_output=True).stdout
            self.assertIn("ready=1 module=1 active=2 graphics=1 resets=0", success)
            self.assertIn("calls=activate,wait,getstate,kdset,kdget", success)
            for fault in range(1, 10):
                with self.subTest(fault=fault):
                    failed = subprocess.run(
                        [binary, str(fault)], check=True, text=True, capture_output=True
                    ).stdout
                    self.assertIn("ready=0 module=0", failed)
                    self.assertIn("resets=1", failed)

    def test_disarmed_script_clears_stale_qualification_environment(self):
        writer = source_function(
            self.source,
            "static bool write_launcher_script(const char *path)",
            "static MagikLauncherSpawnResult spawn_launcher(void)\n{",
        )
        harness = r'''
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
static const char *s_script_path, *env_path;
static bool armed;
static bool magik_launcher_consume_return_spawn(const char *, const char *) { return false; }
static const char *get_rbf_name() { return ""; }
static const char *get_rbf_path() { return ""; }
static bool latch_reuse_qualification_armed() { return armed; }
static const char *layout_path(const char *) { return env_path; }
static struct { const char *token="test"; unsigned long main_pid=1; unsigned long long main_generation=1,owner_epoch=1; } s_ready;
static const char *s_ready_fifo_path="unused";
static const char *resolved_runtime_output() { return "auto"; }
static const char *configured_display_mode() { return "auto"; }
static struct { bool pending=false, confirm_ui=false; } s_display_transaction;
static void eventf(const char *, const char *, ...) {}
'''
        main = r'''
int main(int argc, char **argv) {
    (void)argc;
    s_script_path=argv[1]; env_path=argv[2]; armed=atoi(argv[3]) != 0;
    return write_launcher_script("unused") ? 0 : 1;
}
'''
        with tempfile.TemporaryDirectory(prefix="main-launcher-script-") as temporary:
            root = Path(temporary)
            binary = self.compile(root, "script-writer", harness + writer + main)
            env_file = root / "launcher.env"
            env_file.write_text(
                "export MISTER_LATCH_V5_QUALIFICATION=stale\n"
                "export MISTER_MAGIK_DEV_LATCH_REUSE_QUARANTINE_VBLANKS=99\n"
            )
            for marker, expected in (("0", "unset/unset"), ("1", "1/8")):
                generated = root / f"launcher-{marker}.sh"
                subprocess.run([binary, generated, env_file, marker], check=True)
                prefix = generated.read_text().split(
                    "if [ -e /dev/mister-magik-scanout-slots ]; then", 1
                )[0]
                probe = prefix + r'''
printf '%s/%s\n' "${MISTER_LATCH_V5_QUALIFICATION-unset}" "${MISTER_MAGIK_DEV_LATCH_REUSE_QUARANTINE_VBLANKS-unset}"
'''
                environment = {
                    "PATH": "/usr/bin:/bin",
                    "MISTER_LATCH_V5_QUALIFICATION": "inherited",
                    "MISTER_MAGIK_DEV_LATCH_REUSE_QUARANTINE_VBLANKS": "77",
                }
                result = subprocess.run(
                    ["bash", "-c", probe],
                    check=True,
                    text=True,
                    capture_output=True,
                    env=environment,
                )
                self.assertEqual(result.stdout.strip(), expected)

    def test_readiness_child_explicitly_clears_inherited_qualification(self):
        child = source_function(
            self.source,
            "\tif (!readiness_pid)\n\t{",
            "\tif (!wait_for_preflight_child(readiness_pid, \"latch-readiness\"))",
        )
        qualification_unset = child.index('unsetenv("MISTER_LATCH_V5_QUALIFICATION")')
        quarantine_unset = child.index(
            'unsetenv("MISTER_MAGIK_DEV_LATCH_REUSE_QUARANTINE_VBLANKS")'
        )
        armed = child.index("if (latch_reuse_qualification_armed())")
        qualification_set = child.index('\t\t\tsetenv("MISTER_LATCH_V5_QUALIFICATION"')
        quarantine_set = child.index(
            '\t\t\tsetenv("MISTER_MAGIK_DEV_LATCH_REUSE_QUARANTINE_VBLANKS"'
        )
        self.assertLess(qualification_unset, armed)
        self.assertLess(quarantine_unset, armed)
        self.assertGreater(qualification_set, armed)
        self.assertGreater(quarantine_set, armed)


if __name__ == "__main__":
    unittest.main()
