from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "voice-assistant" / "tools" / "bootstrap.py"


def load_module():
    spec = importlib.util.spec_from_file_location("qh_voice_bootstrap", MODULE_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load bootstrap module")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class BootstrapTest(unittest.TestCase):
    def test_load_lock_requires_the_two_owned_repositories(self) -> None:
        bootstrap = load_module()
        with tempfile.TemporaryDirectory() as directory:
            lock_path = Path(directory) / "integration-lock.json"
            lock_path.write_text(
                json.dumps(
                    {
                        "schemaVersion": 1,
                        "repositories": {
                            "qh-voice-skill": {
                                "url": "https://github.com/qihaokaigong/qh-voice-skill.git",
                                "commit": "d" * 40,
                            }
                        },
                    }
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "qh-voice-kit"):
                bootstrap.load_lock(lock_path)

    def test_inspect_host_accepts_only_documented_release_targets(self) -> None:
        bootstrap = load_module()

        mac = bootstrap.inspect_host(system="Darwin", machine="arm64", version=(3, 11))
        windows = bootstrap.inspect_host(
            system="Windows", machine="AMD64", version=(3, 12)
        )
        intel_mac = bootstrap.inspect_host(
            system="Darwin", machine="x86_64", version=(3, 12)
        )
        old_python = bootstrap.inspect_host(
            system="Windows", machine="AMD64", version=(3, 10)
        )

        self.assertTrue(mac["supported"])
        self.assertEqual(mac["host"], "macos-arm64")
        self.assertTrue(windows["supported"])
        self.assertEqual(windows["host"], "windows-x64")
        self.assertFalse(intel_mac["supported"])
        self.assertFalse(old_python["supported"])
        self.assertIn("Python 3.11", old_python["blockers"])

    def test_sync_repository_clones_and_checks_out_exact_commit(self) -> None:
        bootstrap = load_module()
        with tempfile.TemporaryDirectory() as directory:
            temp_root = Path(directory)
            source = temp_root / "source"
            source.mkdir()
            subprocess.run(["git", "init", "-q"], cwd=source, check=True)
            subprocess.run(
                ["git", "config", "user.email", "test@example.com"],
                cwd=source,
                check=True,
            )
            subprocess.run(
                ["git", "config", "user.name", "Test"], cwd=source, check=True
            )
            (source / "SKILL.md").write_text("# test\n", encoding="utf-8")
            subprocess.run(["git", "add", "SKILL.md"], cwd=source, check=True)
            subprocess.run(["git", "commit", "-qm", "initial"], cwd=source, check=True)
            commit = subprocess.run(
                ["git", "rev-parse", "HEAD"],
                cwd=source,
                check=True,
                text=True,
                capture_output=True,
            ).stdout.strip()

            target = temp_root / "workspace" / "qh-voice-skill"
            result = bootstrap.sync_repository(
                name="qh-voice-skill",
                url=str(source),
                commit=commit,
                target=target,
            )

            self.assertEqual(result["status"], "ready")
            self.assertEqual(result["commit"], commit)
            self.assertEqual(
                subprocess.run(
                    ["git", "rev-parse", "HEAD"],
                    cwd=target,
                    check=True,
                    text=True,
                    capture_output=True,
                ).stdout.strip(),
                commit,
            )

    def test_existing_repository_with_changes_is_never_overwritten(self) -> None:
        bootstrap = load_module()
        with tempfile.TemporaryDirectory() as directory:
            temp_root = Path(directory)
            source = temp_root / "source"
            source.mkdir()
            subprocess.run(["git", "init", "-q"], cwd=source, check=True)
            subprocess.run(
                ["git", "config", "user.email", "test@example.com"],
                cwd=source,
                check=True,
            )
            subprocess.run(
                ["git", "config", "user.name", "Test"], cwd=source, check=True
            )
            tracked = source / "SKILL.md"
            tracked.write_text("# test\n", encoding="utf-8")
            subprocess.run(["git", "add", "SKILL.md"], cwd=source, check=True)
            subprocess.run(["git", "commit", "-qm", "initial"], cwd=source, check=True)
            commit = subprocess.run(
                ["git", "rev-parse", "HEAD"],
                cwd=source,
                check=True,
                text=True,
                capture_output=True,
            ).stdout.strip()
            target = temp_root / "workspace" / "qh-voice-skill"
            subprocess.run(["git", "clone", "-q", str(source), str(target)], check=True)
            (target / "SKILL.md").write_text("user change\n", encoding="utf-8")

            with self.assertRaisesRegex(ValueError, "未提交改动"):
                bootstrap.sync_repository(
                    name="qh-voice-skill",
                    url=str(source),
                    commit=commit,
                    target=target,
                )

            self.assertEqual(
                (target / "SKILL.md").read_text(encoding="utf-8"), "user change\n"
            )

    def test_parser_never_accepts_provider_or_wifi_secrets(self) -> None:
        bootstrap = load_module()
        parser = bootstrap.build_parser()
        option_names = {
            option
            for action in parser._actions
            for option in action.option_strings
        }

        self.assertNotIn("--api-key", option_names)
        self.assertNotIn("--wifi-password", option_names)
        self.assertNotIn("--access-token", option_names)

    def test_doctor_prints_flat_host_result_without_traceback(self) -> None:
        bootstrap = load_module()
        with tempfile.TemporaryDirectory() as directory:
            lock_path = Path(directory) / "integration-lock.json"
            lock_path.write_text(
                json.dumps(
                    {
                        "schemaVersion": 1,
                        "repositories": {
                            name: {
                                "url": f"https://github.com/qihaokaigong/{name}.git",
                                "commit": "a" * 40,
                            }
                            for name in ("qh-voice-skill", "qh-voice-kit")
                        },
                    }
                ),
                encoding="utf-8",
            )
            output = StringIO()
            with patch.object(
                bootstrap,
                "inspect_host",
                return_value={
                    "host": "macos-x86_64",
                    "python": "3.13",
                    "supported": False,
                    "blockers": ["当前仅支持 macOS arm64 和 Windows x64"],
                },
            ), redirect_stdout(output):
                result = bootstrap.main(["doctor", "--lock", str(lock_path)])

            self.assertEqual(result, 2)
            self.assertIn("电脑：macos-x86_64 / Python 3.13", output.getvalue())
            self.assertIn("当前仅支持", output.getvalue())


if __name__ == "__main__":
    unittest.main()
