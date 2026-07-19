#!/usr/bin/env python3

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "support/mister_magik/main_component.py"
SPEC = importlib.util.spec_from_file_location("main_component", SCRIPT)
assert SPEC and SPEC.loader
component = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(component)


class MainComponentTests(unittest.TestCase):
    revision = "1" * 40
    toolchain = component.TOOLCHAIN

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="main-component-")
        self.root = Path(self.temp.name)
        self.binary = self.root / "MiSTer"
        self.binary.write_bytes(b"main-binary\n")
        self.output = self.root / "artifact"

    def tearDown(self):
        self.temp.cleanup()

    def test_identity_is_deterministic_and_authority_bound(self):
        expected = "a9ead23864010064528bde4fa70a84567058ea6009026089af7f6783a7fad36d"
        self.assertEqual(component.component_id(self.revision, self.toolchain), expected)
        self.assertNotEqual(component.component_id("2" * 40, self.toolchain), expected)
        with self.assertRaisesRegex(component.ComponentError, "unsupported toolchain"):
            component.component_id(self.revision, "other")

    def test_create_and_verify_round_trip(self):
        created = component.create(self.binary, self.output, self.revision, self.toolchain)
        verified = component.verify(self.output, self.revision, self.toolchain)
        self.assertEqual(created, verified)
        self.assertEqual((self.output / "MiSTer_MagiK").read_bytes(), self.binary.read_bytes())

    def test_verify_rejects_wrong_revision_toolchain_and_binary(self):
        component.create(self.binary, self.output, self.revision, self.toolchain)
        with self.assertRaisesRegex(component.ComponentError, "source revision mismatch"):
            component.verify(self.output, "2" * 40, self.toolchain)
        with self.assertRaisesRegex(component.ComponentError, "unsupported toolchain"):
            component.verify(self.output, self.revision, "other")
        (self.output / "MiSTer_MagiK").write_bytes(b"corrupt")
        with self.assertRaisesRegex(component.ComponentError, "binary identity mismatch"):
            component.verify(self.output)

    def test_verify_rejects_malformed_receipt_and_checksums(self):
        component.create(self.binary, self.output, self.revision, self.toolchain)
        receipt = self.output / "main-component-v0.1.json"
        payload = json.loads(receipt.read_text())
        payload["component_id"] = "0" * 64
        receipt.write_text(json.dumps(payload))
        with self.assertRaisesRegex(component.ComponentError, "component identity mismatch"):
            component.verify(self.output)

    def test_verify_rejects_invalid_json_root_and_field_types(self):
        component.create(self.binary, self.output, self.revision, self.toolchain)
        receipt = self.output / "main-component-v0.1.json"
        receipt.write_text("not-json")
        with self.assertRaises(json.JSONDecodeError):
            component.verify(self.output)
        receipt.write_text("[]")
        with self.assertRaisesRegex(component.ComponentError, "must be an object"):
            component.verify(self.output)
        payload = component.create(self.binary, self.output, self.revision, self.toolchain)
        payload["source_revision"] = 1
        receipt.write_text(json.dumps(payload))
        with self.assertRaisesRegex(component.ComponentError, "invalid component receipt fields"):
            component.verify(self.output)

    def test_verify_rejects_wrong_typed_binary_metadata(self):
        payload = component.create(self.binary, self.output, self.revision, self.toolchain)
        receipt = self.output / "main-component-v0.1.json"
        payload["binary"]["size"] = True
        receipt.write_text(json.dumps(payload))
        with self.assertRaisesRegex(component.ComponentError, "invalid binary size"):
            component.verify(self.output)


if __name__ == "__main__":
    unittest.main()
