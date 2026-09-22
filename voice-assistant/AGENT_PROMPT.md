# 交给 Agent 的提示词

## 首次接入

在 `python voice-assistant/tools/bootstrap.py setup` 执行成功后，把下面一段话发送给能够访问本地
文件和 USB 串口的 Agent：

```text
请先完整读取 .qh-voice-workspace/qh-voice-skill/SKILL.md，然后按该 Skill 的流程帮我接入
QH Voice Kit。先只检查电脑、USB 串口和硬件 Profile，不要烧录。把每个实测结果和未验证建议
分开说明；只有在你展示了精确 Release ID、硬件 Profile ID、串口、写入文件、地址和保留区域，
并收到我的精确确认后，才可以烧录。

不要让我在聊天、命令行、Issue、截图或 QH 平台中提供 API Key、Wi-Fi 密码或其他密钥。
需要配置时，必须使用 Skill 打开的 127.0.0.1 本地中文网页，并通过 USB 直接写入设备。
配置完成后，请带我完成屏幕、录音、识别、回答播放和断电重启的真实验收。
```

如果自定义了工作目录，请把第一行路径替换为实际的 `qh-voice-skill/SKILL.md` 绝对路径。也可以
执行以下命令，让工具生成已经包含绝对路径的版本：

```bash
python voice-assistant/tools/bootstrap.py prompt --workspace path/to/workspace
```

## 使用不同 Provider

先准备对方的官方、版本化 API 文档，但不要提供真实密钥，然后发送：

```text
请先读取当前工作目录中的 qh-voice-skill/SKILL.md、qh-voice-kit/README.md、
qh-voice-kit/docs/protocol-sources.md，以及本仓库 voice-assistant/docs/provider-adaptation.md。
我要把默认实时语音 Provider 适配为我提供的接口。请先对照官方文档列出协议差异、不能从文档
确定的字段、需要修改的最小文件集合和验收计划，不要先写代码，不要索要或记录真实密钥，
不要引入 Voice Gateway。方案确认后，只修改 Provider 适配层、配置 Schema/Wire 和对应测试；
完成主机测试与固件编译后，再创建 candidate 并等待精确烧录确认。
```
