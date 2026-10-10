# MiSTer MagiK Main patch set

This document describes the current Main fork changes relative to upstream
`47221c18987e101f50caafeb3b615f53b62722ca` (`Release 20260912.`).

## Patch inventory

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
| Display transactions | Apply, confirm, cancel, roll back and persist HDMI/CRT mode selection; initialize HDMI before launcher entry | `launcher.cpp`, `video.*`, `launcher_command.*`; display / return tests |
| Main reload and Linux reboot | Keep the Dev replacement handoff and explicit reset / fault-qualification paths | `launcher.cpp`; lifecycle and reload contract checks |
| Status and incident evidence | Publish atomic state, crash, input and video diagnostics | `launcher.cpp`, `launcher_diag.*`; diagnostics tests |

## Launcher and framebuffer ownership

Main initializes video and Menu-core prerequisites, then enters native black
before platform preflight. It disables stock OSD, OSD keys, launcher input and
legacy framebuffer routing. For HDMI output, Main reasserts the initialized
transmitter and mode before launcher entry. Direct Video retains its separate
framebuffer-mux selection.

Main activates tty2 and verifies graphics mode before spawning the launcher.
Launcher restart retains graphics mode. A stock UI handoff or failed-spawn
recovery restores text mode. Console setup does not write ANSI clearing text
after graphics-mode selection.

Main transfers exclusive FPGA SPI/GPO ownership after preflight succeeds and
restores Main ownership after the supervised child is reaped. Lifecycle guards
suppress stock OSD, input and framebuffer work while MagiK owns the session.
Both upstream scheduler variants use dormant launcher waiting.

The scanout-slot application renders into a separate RGB565 composition
surface and publishes completed slots. An anonymous composition surface must
not select Main's legacy framebuffer as its source.

## Command and readiness contracts

The FIFO supports launcher lifecycle, real-path and structured core launches,
external `load_core`, Menu return, display transactions, Dev Main reload and
explicit reboot/reset diagnostics. Real and external launches share validation
and acknowledgement ordering while retaining their distinct handoff flag.
Stock core/MGL/MRA loaders remain responsible for core loading.

Ready reports use the canonical v2/v3 wire formats and bind the child PID,
startup token, Main PID/generation and FPGA owner epoch. The parser validates
capabilities, geometry, advancing sequence and route epochs, alternating slots,
receipts and nonblank source evidence. Its output contains only the identity
fields used by Main to accept the report.

Structured launch plans retain the strict 4096-byte encoded-payload bound.
The serialized argument is stored once; decoded fields retain their existing
validation and bounds. Parser helpers and status publication are private to
their translation units.

Display transactions preserve confirmation, cancellation, rollback and
persistence semantics. Legacy settings commands and direct-reset qualification
are supported. Session Main selection uses an exact executable-path match.

## Input and diagnostics

Input forwarding retains contributor counts, queued releases and bounded
journal delivery. Launcher polling temporarily uses nice -20 and restores the
caller's priority on every return. Controller policy uses one uncached marker
reader; callers retain their stock Menu exclusions and button-override rules.

State and incident reports are published atomically. They expose lifecycle,
readiness, FPGA ownership, crashes, input delivery and video diagnostics.
Readiness establishes an internal scanout boundary; physical display visibility
requires output capture.

Preflight subprocess waiting uses blocking `waitpid`, so the readiness timeout
does not bound the entire startup. The uinput journal also applies to stock
send-key delivery.

## Validation

- `bash scripts/test-magik-state.sh`
- `python3 tests/test_main_component.py`
- `bash scripts/check-magik-patch-surface.sh`
- `./build-container.sh clean all`

The host suite checks portable contracts and selected production functions.
The ARM build checks compilation and linkage. Validate visible launcher
restart, game return and display transactions using identified app/Main
revisions and direct HDMI capture.
