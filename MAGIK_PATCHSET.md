# MiSTer MagiK Main patch set

Upstream baseline: `47221c18987e101f50caafeb3b615f53b62722ca`
(`Release 20260912.`). This branch is rebuilt from that release, rather than
replaying the development history. The previous published state is preserved
at `mister-magik-20261010` (`e88a82c32220656e15160a9a034c332501e34f7e`).
Historical qualification and implementation notes remain in that archive.
The separate colour/monochrome OSD and Spectrum experiments are excluded.

## Retained patches

| Area | Why Main needs it | Owning code / validation |
| --- | --- | --- |
| Build and provenance | Reproducible ARM builds with an explicit upstream identity | `build-container.sh`, workflow, `main_component.py`; component tests |
| Public / Dev layout | Select the correct app, module, RBF, logs and launcher from Main's executable name | `layout.*`; layout tests |
| Session Main | Preserve an explicitly selected no-reboot session executable across core restarts | `session_main.*`, `user_io.cpp`; exact-match tests |
| Supervised lifecycle | Start, suspend, resume, restart and reap the launcher through Main | `launcher.*`, `launcher_state.*`, `launcher_wait.*`; state and production-function tests |
| Bootstrap black and VT | Enter native black before preflight and spawn; acquire and verify graphics on tty2 | `bootstrap_sequence.*`, `launcher.cpp`, `video.cpp`, `osd.cpp`; ordering and VT-failure tests |
| FPGA ownership | Fence SPI/GPO writes and transfer ownership only after preflight and child reaping | `fpga_ownership.*`, `fpga_io.*`; ownership tests |
| Platform preflight | Verify the matching module/RBF and latch capabilities before transferring ownership | `launcher.cpp`; bootstrap / preflight contract checks |
| Ready reports | Accept only canonical v2/v3 reports bound to child, Main generation and owner epoch | `launcher_ready.*`; malformed, sequence, receipt and identity tests |
| FIFO commands and replies | Preserve command association, partial delivery and external load_core compatibility | `launcher_command.*`, `launcher_reply.*`; parser and reply tests |
| Real and structured launches | Use stock core/MGL/MRA loaders and validate SDRAM / structured payloads | `launcher.cpp`, `mra_loader.*`, `sdram_config.h`; launch / SDRAM checks |
| Menu return | Keep stock Exit and add Back to MagiK; retain load_core handoff | `menu.cpp`, `launcher_return.*`, `menu_path.*`; return and path tests |
| Dormancy | Block while MagiK owns the UI and preserve both upstream scheduler variants | `main.cpp`, `scheduler.cpp`, `launcher_wait.*`; wait tests and integration checks |
| Input forwarding and priority | Forward launcher input with contributor counts, queued releases and scoped nice restoration | `input.cpp`, `input_proxy.*`, `input_priority.*`; proxy / priority tests |
| Controller policy | Honor simple input and button overrides without changing stock Menu mapping | `input.cpp`, `joymapping.cpp`, `button_overrides.*`; mapping tests |
| Display transactions | Preserve apply, confirm, cancel, rollback, persistence and HDMI/CRT mode selection | `launcher.cpp`, `video.*`, `launcher_command.*`; display / return tests |
| Main reload and Linux reboot | Keep the Dev replacement handoff and explicit reset / fault-qualification paths | `launcher.cpp`; lifecycle and reload contract checks |
| Status and incident evidence | Publish atomic state, crash, input and video diagnostics | `launcher.cpp`, `launcher_diag.*`; diagnostics tests |

## Restart and buffer contract

Main remains dormant while MagiK owns the launcher. Existing lifecycle guards
suppress stock OSD, input and framebuffer work during startup and ownership.
Main enters native black before preflight, then transfers FPGA ownership.
For an HDMI route, Main reasserts its initialized transmitter and mode before
entering the launcher, including a fresh Main start rather than only game return.
MagiK declares ready only after two completed advancing latch posts on
alternating scanout slots. This proves the internal boundary; USB video is
needed to establish physical visibility.

Restarting the launcher retains graphics mode; genuine stock UI handoff and
failed-spawn recovery restore text mode. Actual VT activation and graphics
verification remain required. Redundant ANSI clearing after graphics-mode
selection and the writer-silence bookkeeping class are removed. Its claimed
drain did not synchronously drain the asynchronous module-parameter writer.
Existing lifecycle guards continue to suppress stock framebuffer/OSD work.

A fresh Main start must reassert its HDMI transmitter/mode before launcher
entry. Source-valid scanout slots and ready reports alone did not guarantee
visible HDMI during qualification. The existing game-return reassertion routine
is now also used for fresh Main starts. The Direct Video path remains excluded.
The proposed VT cleanup was initially blamed for black output; testing with
that cleanup reverted disproved the attribution. HDMI initialization restored
visible output and the combined cleanup is qualified separately below.

The application's anonymous RGB565 composition surface is separate from
Main's legacy framebuffer. Its initial, periodic and recovery paths must not
select Main's buffer as a legacy RGB565 source. The app correction lives in
the MiSTer MagiK repository, rather than adding another Main workaround.

## Deletion and simplification audit

Removed six helpers without production callers, an unused tty name, unused
ready-report output fields and a duplicate encoded launch-plan buffer. The
ready parser retains every wire, geometry, sequence, receipt and nonblank
validation, including v2 compatibility. The encoded payload still has the
same strict 4096-byte bound.

Real / external launch dispatch now shares validation and acknowledgement
ordering. Both input consumers use one uncached policy reader with their
existing Menu exclusions. The display rollback uses its existing boolean
rather than an identity helper. Command parser helpers and status publication
remain private to their translation units.

Broader handoff/scheduler/ownership rewrites are deferred. Legacy settings
commands, HDMI return reassertion and direct-reset qualification remain until
there is evidence to retire them. Preflight's blocking child wait and global
uinput journal scope remain separate follow-ups; this rebuild does not claim
all startup subprocesses are time bounded.

## Validation

Run `bash scripts/test-magik-state.sh`, `python3 tests/test_main_component.py`,
`bash scripts/check-magik-patch-surface.sh` and `./build-container.sh clean all`.
The host suite tests portable contracts and selected production functions;
the ARM build checks linkage. Physical restart, game return and display
validation must identify the installed app/Main revisions and direct HDMI
capture. A passing build is not device qualification.

No FPGA, kernel, stock core or MiSTer.ini changes are part of this rebuild.
