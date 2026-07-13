# MiSTer MagiK Main_MiSTer Fork

This repository is the MiSTer MagiK fork of upstream `MiSTer-devel/Main_MiSTer`.
It is intentionally kept separate from the Slint/Rust application repository so
Main-specific changes can be reviewed, tested, and rebuilt from upstream without
being mixed into the product UI codebase.

## Upstream Baseline

- Upstream: `MiSTer-devel/Main_MiSTer`
- Baseline commit: `93d13fb690db4581768389450fb639822ae88333`
- Baseline subject: `Release 20260707.`
- Baseline policy: use upstream release-marker commits named `Release YYYYMMDD.`
- Device binary name: `/media/fat/MiSTer_MagiK`

This is not an official MiSTer-devel build. Published binaries must be labelled
as MiSTer MagiK builds.

## Fork Policy

The fork exists only to let stock Main initialize the MiSTer hardware and then
act as a dormant parent for the MiSTer MagiK Slint launcher. Main must not own
visible UI or framebuffer routing while the launcher is active. Main regains
those powers only through explicit handoff commands.

Keep the patch surface small. The living inventory is `MAGIK_PATCHSET.md`; update
that file with every feature, invariant, and test added to this fork.

## Build

Use the Apple-container wrapper on Apple Silicon so the host does not need an
ARM GCC toolchain:

```bash
./build-container.sh
```

The arm64 Linux image is a toolchain image only. Source is bind-mounted into the
container so source edits do not invalidate image layers.
