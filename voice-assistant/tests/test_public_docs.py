from __future__ import annotations

import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VOICE = ROOT / "voice-assistant"


class PublicDocsTest(unittest.TestCase):
    def test_repository_homepage_has_voice_assistant_entry(self) -> None:
        homepage = (ROOT / "README.md").read_text(encoding="utf-8")

        self.assertIn("voice-assistant/", homepage)
        self.assertIn("完整语音助手", homepage)

    def test_quickstart_links_every_required_stage(self) -> None:
        quickstart = (VOICE / "README.md").read_text(encoding="utf-8")

        self.assertIn("tools/bootstrap.py doctor", quickstart)
        self.assertIn("tools/bootstrap.py setup", quickstart)
        self.assertIn("AGENT_PROMPT.md", quickstart)
        self.assertIn("docs/provider-adaptation.md", quickstart)
        self.assertIn("docs/troubleshooting.md", quickstart)
        self.assertIn("candidate", quickstart)

    def test_agent_prompt_keeps_secrets_out_of_chat_and_qh(self) -> None:
        prompt = (VOICE / "AGENT_PROMPT.md").read_text(encoding="utf-8")

        self.assertIn("不要让我在聊天", prompt)
        self.assertIn("本地中文网页", prompt)
        self.assertIn("QH 平台", prompt)

    def test_lock_pins_public_repositories_to_full_commits(self) -> None:
        lock = json.loads(
            (VOICE / "integration-lock.json").read_text(encoding="utf-8")
        )

        for name in ("qh-voice-skill", "qh-voice-kit"):
            entry = lock["repositories"][name]
            self.assertEqual(
                entry["url"], f"https://github.com/qihaokaigong/{name}.git"
            )
            self.assertRegex(entry["commit"], r"^[0-9a-f]{40}$")

    def test_generated_workspace_is_ignored(self) -> None:
        gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8")

        self.assertIn(".qh-voice-workspace/", gitignore)

    def test_local_markdown_links_resolve(self) -> None:
        markdown_files = [ROOT / "README.md", *VOICE.rglob("*.md")]
        for document in markdown_files:
            content = document.read_text(encoding="utf-8")
            for raw_target in re.findall(r"\[[^\]]+\]\(([^)]+)\)", content):
                if raw_target.startswith(("http://", "https://", "#")):
                    continue
                target = raw_target.split("#", 1)[0]
                if not target:
                    continue
                resolved = (document.parent / target).resolve()
                self.assertTrue(
                    resolved.exists(),
                    f"{document.relative_to(ROOT)} -> {raw_target}",
                )


if __name__ == "__main__":
    unittest.main()
