#!/usr/bin/env python3
"""Create and verify content-addressed MiSTer MagiK Main artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path

FORMAT = "mister-magik-main-component-v0.1"
REPOSITORY = "NigelBreslaw/Main_MiSTer"
BRANCH = "mister-magik"
TOOLCHAIN = "gcc-arm-10.2-2020.11-x86_64-arm-none-linux-gnueabihf"
HEX40 = 40
HEX64 = 64


class ComponentError(ValueError):
    pass


def require_hex(name: str, value: str, length: int) -> None:
    if len(value) != length or any(char not in "0123456789abcdef" for char in value):
        raise ComponentError(f"invalid {name}")


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def component_id(revision: str, toolchain: str = TOOLCHAIN) -> str:
    require_hex("source_revision", revision, HEX40)
    if toolchain != TOOLCHAIN:
        raise ComponentError("unsupported toolchain")
    material = (
        f"format={FORMAT}\nrepository={REPOSITORY}\nbranch={BRANCH}\n"
        f"source_revision={revision}\ntoolchain={toolchain}\n"
    )
    return hashlib.sha256(material.encode()).hexdigest()


def create(binary: Path, output: Path, revision: str, toolchain: str = TOOLCHAIN) -> dict[str, object]:
    if not binary.is_file() or binary.is_symlink():
        raise ComponentError("Main binary is missing or invalid")
    identity = component_id(revision, toolchain)
    output.mkdir(parents=True, exist_ok=True)
    target = output / "MiSTer_MagiK"
    shutil.copyfile(binary, target)
    target.chmod(0o755)
    payload: dict[str, object] = {
        "format": FORMAT,
        "component_id": identity,
        "repository": REPOSITORY,
        "branch": BRANCH,
        "source_revision": revision,
        "toolchain": toolchain,
        "binary": {
            "path": "MiSTer_MagiK",
            "size": target.stat().st_size,
            "sha256": digest(target),
        },
    }
    receipt = output / "main-component-v0.1.json"
    receipt.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    (output / "SHA256SUMS").write_text(
        f"{digest(target)}  MiSTer_MagiK\n{digest(receipt)}  main-component-v0.1.json\n"
    )
    verify(output, revision, toolchain)
    return payload


def verify(root: Path, revision: str | None = None, toolchain: str | None = None) -> dict[str, object]:
    receipt = root / "main-component-v0.1.json"
    binary = root / "MiSTer_MagiK"
    checksums = root / "SHA256SUMS"
    if not receipt.is_file() or not binary.is_file() or not checksums.is_file():
        raise ComponentError("component artifact is incomplete")
    payload = json.loads(receipt.read_text())
    if not isinstance(payload, dict):
        raise ComponentError("component receipt must be an object")
    if payload.get("format") != FORMAT or payload.get("repository") != REPOSITORY or payload.get("branch") != BRANCH:
        raise ComponentError("unsupported component authority")
    stored_revision = payload.get("source_revision")
    stored_toolchain = payload.get("toolchain")
    if not isinstance(stored_revision, str) or not isinstance(stored_toolchain, str):
        raise ComponentError("invalid component receipt fields")
    expected_id = component_id(stored_revision, stored_toolchain)
    if payload.get("component_id") != expected_id:
        raise ComponentError("component identity mismatch")
    if revision is not None and stored_revision != revision:
        raise ComponentError("source revision mismatch")
    if toolchain is not None and toolchain != TOOLCHAIN:
        raise ComponentError("unsupported toolchain")
    if stored_toolchain != TOOLCHAIN:
        raise ComponentError("toolchain mismatch")
    binary_meta = payload.get("binary")
    if not isinstance(binary_meta, dict) or binary_meta.get("path") != "MiSTer_MagiK":
        raise ComponentError("invalid binary metadata")
    size = binary_meta.get("size")
    sha256 = binary_meta.get("sha256")
    if isinstance(size, bool) or not isinstance(size, int) or size < 0:
        raise ComponentError("invalid binary size")
    if not isinstance(sha256, str):
        raise ComponentError("invalid binary checksum")
    require_hex("binary sha256", sha256, HEX64)
    if size != binary.stat().st_size or sha256 != digest(binary):
        raise ComponentError("binary identity mismatch")
    expected_sums = f"{digest(binary)}  MiSTer_MagiK\n{digest(receipt)}  main-component-v0.1.json\n"
    if checksums.read_text() != expected_sums:
        raise ComponentError("component checksums mismatch")
    return payload


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    identity = commands.add_parser("identity")
    identity.add_argument("--revision", required=True)
    identity.add_argument("--toolchain", required=True)
    create_parser = commands.add_parser("create")
    create_parser.add_argument("--binary", type=Path, required=True)
    create_parser.add_argument("--output", type=Path, required=True)
    create_parser.add_argument("--revision", required=True)
    create_parser.add_argument("--toolchain", required=True)
    verify_parser = commands.add_parser("verify")
    verify_parser.add_argument("--artifact", type=Path, required=True)
    verify_parser.add_argument("--revision")
    verify_parser.add_argument("--toolchain")
    try:
        args = parser.parse_args()
        if args.command == "identity":
            print(component_id(args.revision, args.toolchain))
        elif args.command == "create":
            print(json.dumps(create(args.binary, args.output, args.revision, args.toolchain), sort_keys=True))
        else:
            print(json.dumps(verify(args.artifact, args.revision, args.toolchain), sort_keys=True))
    except (ComponentError, json.JSONDecodeError, OSError) as error:
        print(f"Main component error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
