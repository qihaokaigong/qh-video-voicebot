from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GUIDE = ROOT / "009-voice-assistant"


class PublicDocsTest(unittest.TestCase):
    def test_repository_index_uses_numbered_009_entry(self) -> None:
        homepage = (ROOT / "README.md").read_text(encoding="utf-8")

        self.assertIn("| 009 |", homepage)
        self.assertIn("009-voice-assistant/", homepage)
        self.assertNotIn("[语音助手接入指南](voice-assistant/)", homepage)

    def test_user_flow_starts_with_installing_the_skill_in_ai(self) -> None:
        guide = (GUIDE / "README.md").read_text(encoding="utf-8")

        self.assertIn("https://github.com/qihaokaigong/qh-voice-skill", guide)
        self.assertIn("请安装这个 Skill", guide)
        self.assertNotIn("git clone", guide)
        self.assertNotIn("bootstrap.py", guide)
        self.assertNotIn("```bash", guide)
        self.assertNotIn("python voice-assistant", guide.lower())
        self.assertNotIn("python3 voice-assistant", guide.lower())

    def test_009_guide_does_not_explain_its_relationship_to_008(self) -> None:
        guide = (GUIDE / "README.md").read_text(encoding="utf-8")

        self.assertNotIn("008", guide)
        self.assertNotIn("历史示例", guide)

    def test_local_markdown_links_resolve(self) -> None:
        markdown_files = [ROOT / "README.md", *GUIDE.rglob("*.md")]
        for document in markdown_files:
            content = document.read_text(encoding="utf-8")
            for raw_target in re.findall(r"\[[^\]]+\]\(([^)]+)\)", content):
                if raw_target.startswith(("http://", "https://", "#")):
                    continue
                target = raw_target.split("#", 1)[0]
                if target:
                    self.assertTrue(
                        (document.parent / target).resolve().exists(),
                        f"{document.relative_to(ROOT)} -> {raw_target}",
                    )


if __name__ == "__main__":
    unittest.main()
