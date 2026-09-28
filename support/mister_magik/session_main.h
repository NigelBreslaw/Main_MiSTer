#pragma once

// A no-reboot MagiK session starts its Main with this absolute path in the
// environment. Main's exec restarts inherit it, so core launches and returns
// keep the session Main; a reboot clears it without touching MiSTer.ini.
// The guard overrides every effective `main=`, including core-specific INI
// sections, and child processes inherit the variable by design.
bool magik_session_keeps_main(const char *current_exe);
