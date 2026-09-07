# MiSTer MagiK Main_MiSTer Fork

This repository is the MiSTer MagiK fork of upstream `MiSTer-devel/Main_MiSTer`.
It is intentionally kept separate from the Slint/Rust application repository so
Main-specific changes can be reviewed, tested, and rebuilt from upstream without
being mixed into the product UI codebase.

## Upstream Baseline

- Upstream: `MiSTer-devel/Main_MiSTer`
- Baseline commit: `f8dc68e3dcf4694f5593e6552aea56cd852982af`
- Baseline subject: `Release 20260907.`
- Baseline policy: use upstream release-marker commits named `Release YYYYMMDD.`
- Device binary name: `/media/fat/MiSTer_MagiK`

This is not an official MiSTer-devel build. Published binaries must be labelled
as MiSTer MagiK builds.

This baseline includes all nine commits since `Release 20260823.`: CD32/CDTV
raw CUE/BIN support, Amiga quick-start ROM lookup through `HomeDir()`, MacPlus
CD-ROM support, and Minimig CPU selector, preset, and configuration repairs,
plus the CD-fix merge and release marker. The release is merged into the
existing MagiK patch stack without rewriting published history.

CD32 and A1200 presets now select the approximately 14 MHz 68020 mode. Validate
them with the matching Minimig `20260907` core or a verified later descendant;
older cores can apply the wrong throttle. Existing device results in
`MAGIK_PATCHSET.md` are historical, not qualification of this baseline.

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
