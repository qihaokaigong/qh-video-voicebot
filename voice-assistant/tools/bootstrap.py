#!/usr/bin/env python3

"""Prepare the pinned QH voice-assistant source and local Agent tool."""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Sequence


REQUIRED_REPOSITORIES = ("qh-voice-skill", "qh-voice-kit")
COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LOCK = ROOT / "integration-lock.json"
DEFAULT_WORKSPACE = Path.cwd() / ".qh-voice-workspace"


def _run(command: Sequence[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess:
    completed = subprocess.run(
        list(command),
        cwd=cwd,
        text=True,
        capture_output=True,
    )
    if completed.returncode != 0:
        detail = completed.stderr.strip() or completed.stdout.strip() or "未知错误"
        raise ValueError(f"命令执行失败：{detail}")
    return completed


def load_lock(path: Path) -> dict:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"无法读取版本锁文件：{error}") from error
    if not isinstance(payload, dict) or payload.get("schemaVersion") != 1:
        raise ValueError("版本锁文件 schemaVersion 必须为 1")
    repositories = payload.get("repositories")
    if not isinstance(repositories, dict):
        raise ValueError("版本锁文件缺少 repositories")
    for name in REQUIRED_REPOSITORIES:
        entry = repositories.get(name)
        if not isinstance(entry, dict):
            raise ValueError(f"版本锁文件缺少 {name}")
        url = entry.get("url")
        commit = entry.get("commit")
        if not isinstance(url, str) or not url.strip():
            raise ValueError(f"{name} 缺少仓库地址")
        if not isinstance(commit, str) or not COMMIT_PATTERN.fullmatch(commit):
            raise ValueError(f"{name} 的 commit 必须是 40 位小写 SHA")
    return payload


def inspect_host(
    *,
    system: str | None = None,
    machine: str | None = None,
    version: tuple[int, int] | None = None,
) -> dict:
    actual_system = system or platform.system()
    actual_machine = machine or platform.machine()
    actual_version = version or (sys.version_info.major, sys.version_info.minor)
    system_key = actual_system.lower()
    machine_key = actual_machine.lower()
    if system_key == "darwin" and machine_key in {"arm64", "aarch64"}:
        host = "macos-arm64"
        target_host = True
    elif system_key == "windows" and machine_key in {"amd64", "x86_64"}:
        host = "windows-x64"
        target_host = True
    else:
        label = "macos" if system_key == "darwin" else system_key
        host = f"{label}-{machine_key}"
        target_host = False
    blockers: list[str] = []
    if not target_host:
        blockers.append("当前仅支持 macOS arm64 和 Windows x64")
    if actual_version < (3, 11):
        blockers.append("Python 3.11")
    if shutil.which("git") is None:
        blockers.append("未找到 Git")
    return {
        "host": host,
        "python": f"{actual_version[0]}.{actual_version[1]}",
        "supported": not blockers,
        "blockers": blockers,
    }


def sync_repository(*, name: str, url: str, commit: str, target: Path) -> dict:
    target = target.resolve()
    if target.exists():
        if not (target / ".git").is_dir():
            raise ValueError(f"{target} 已存在，但不是 Git 仓库")
        status = _run(["git", "status", "--porcelain"], cwd=target).stdout.strip()
        if status:
            raise ValueError(f"{name} 含有未提交改动，未执行覆盖")
        origin = _run(["git", "remote", "get-url", "origin"], cwd=target).stdout.strip()
        if origin != url:
            raise ValueError(f"{name} 的 origin 与版本锁不一致")
        head = _run(["git", "rev-parse", "HEAD"], cwd=target).stdout.strip()
        if head != commit:
            raise ValueError(f"{name} 不是锁定版本；请使用新的空工作目录")
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        _run(["git", "clone", "--no-checkout", url, str(target)])
        _run(["git", "checkout", "--detach", commit], cwd=target)
        head = _run(["git", "rev-parse", "HEAD"], cwd=target).stdout.strip()
        if head != commit:
            raise ValueError(f"{name} 未能检出锁定版本")
    return {"name": name, "status": "ready", "commit": commit, "path": str(target)}


def _venv_python(venv: Path) -> Path:
    if os.name == "nt":
        return venv / "Scripts" / "python.exe"
    return venv / "bin" / "python"


def setup(lock_path: Path, workspace: Path) -> dict:
    host = inspect_host()
    if not host["supported"]:
        raise ValueError("；".join(host["blockers"]))
    lock = load_lock(lock_path)
    workspace = workspace.resolve()
    workspace.mkdir(parents=True, exist_ok=True)
    repositories = []
    for name in REQUIRED_REPOSITORIES:
        entry = lock["repositories"][name]
        repositories.append(
            sync_repository(
                name=name,
                url=entry["url"],
                commit=entry["commit"],
                target=workspace / name,
            )
        )
    venv = workspace / ".venv"
    python = _venv_python(venv)
    if not python.is_file():
        _run([sys.executable, "-m", "venv", str(venv)])
    _run(
        [
            str(python),
            "-m",
            "pip",
            "install",
            "--disable-pip-version-check",
            "--no-input",
            str(workspace / "qh-voice-skill"),
        ]
    )
    return {
        **host,
        "status": "ready",
        "workspace": str(workspace),
        "repositories": repositories,
        "agentPrompt": agent_prompt(workspace),
    }


def agent_prompt(workspace: Path) -> str:
    skill = (workspace.resolve() / "qh-voice-skill" / "SKILL.md").as_posix()
    return (
        f"请先完整读取 {skill}，然后按该 Skill 的流程帮我接入 QH Voice Kit。"
        "先只检查电脑、USB 串口和硬件 Profile，不要烧录。"
        "不要让我在聊天、命令行或 QH 平台中提供 API Key 或 Wi-Fi 密码；"
        "配置必须使用 Skill 打开的本地中文网页。"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true", help="输出机器可读 JSON")
    commands = parser.add_subparsers(dest="command", required=True)
    doctor = commands.add_parser("doctor", help="检查电脑是否满足接入前提")
    doctor.add_argument("--lock", type=Path, default=DEFAULT_LOCK)
    setup_command = commands.add_parser("setup", help="准备锁定源码和本地 Skill 工具")
    setup_command.add_argument("--lock", type=Path, default=DEFAULT_LOCK)
    setup_command.add_argument("--workspace", type=Path, default=DEFAULT_WORKSPACE)
    prompt = commands.add_parser("prompt", help="生成交给 Agent 的下一步提示词")
    prompt.add_argument("--workspace", type=Path, default=DEFAULT_WORKSPACE)
    return parser


def _emit(payload: dict | str, as_json: bool) -> None:
    if as_json:
        if isinstance(payload, str):
            payload = {"message": payload}
        print(json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True))
        return
    if isinstance(payload, str):
        print(payload)
        return
    print(f"状态：{payload.get('status', 'unknown')}")
    if payload.get("host") and payload.get("python"):
        print(f"电脑：{payload['host']} / Python {payload['python']}")
    if payload.get("blockers"):
        for blocker in payload["blockers"]:
            print(f"阻塞：{blocker}")
    if payload.get("workspace"):
        print(f"工作目录：{payload['workspace']}")
    if payload.get("agentPrompt"):
        print("\n把下面这段话发送给你的 Agent：\n")
        print(payload["agentPrompt"])


def main(argv: list[str] | None = None) -> int:
    arguments = build_parser().parse_args(argv)
    try:
        if arguments.command == "doctor":
            load_lock(arguments.lock)
            result = inspect_host()
            result["status"] = "ready" if result["supported"] else "blocked"
            _emit(result, arguments.json)
            return 0 if result["supported"] else 2
        if arguments.command == "setup":
            result = setup(arguments.lock, arguments.workspace)
            _emit(result, arguments.json)
            return 0
        if arguments.command == "prompt":
            _emit(agent_prompt(arguments.workspace), arguments.json)
            return 0
    except ValueError as error:
        _emit({"status": "blocked", "message": str(error)}, arguments.json)
        if not arguments.json:
            print(f"阻塞：{error}")
        return 2
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
