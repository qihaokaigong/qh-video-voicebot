# QH 完整语音助手接入指南

这条路径把麦克风、实时语音模型、扬声器和屏幕组成一个可独立运行的 ESP32-S3 语音助手：

```text
按住按钮说话
  -> ESP32-S3 通过一个 WebSocket 连接豆包实时语音模型 3.0（Seeduplex）
  -> 屏幕显示状态和文本，扬声器播放回答
```

配置完成后，电脑和 Agent 都不需要持续运行。设备重新上电会自动连接 Wi-Fi 和模型；不存在
Voice Gateway、本地 Python 语音服务，也不需要分别配置 ASR、LLM 和 TTS。

## 当前公开状态

本目录现在可以完成锁定源码、安装接入工具和进入 Agent 工作流。当前固件版本仍是
`candidate`：它已经在一块参考设备上完成屏幕、按住说话、回答播放和主要杂音修复的人工验证，
但 macOS arm64、Windows x64 的干净电脑验收和正式 `allowed` Release 尚未完成。

因此：

- 开发者可以按本文完成源码接入和候选版验收；
- 普通用户的稳定烧录必须等待 `allowed` Release，Skill 会在缺少正式 Release 时停止；
- 不要绕过 Release 状态、手填 Flash 地址或把候选版描述成正式版。

## 你需要准备什么

- macOS arm64 或 Windows x64 电脑；
- Python 3.11 或更高版本、Git；
- USB 数据线；
- 与 `qh.voice-kit.breadboard.n16r8.v1` 完全一致的 ESP32-S3 N16R8、INMP441、
  MAX98357A、ST7789 和按钮接线；
- 2.4 GHz Wi-Fi；
- 已开通豆包实时语音模型 3.0（Seeduplex）的独立、可撤销、有限额 API Key。

只有芯片同为 ESP32-S3 并不能证明硬件兼容。具体引脚和模块见
[架构与硬件边界](docs/architecture.md)。Linux、Intel Mac 和其他接线当前只检查并报告，
不进入烧录。

## 最快接入

### 1. 获取入口仓库

```bash
git clone https://github.com/qihaokaigong/qh-video-voicebot.git
cd qh-video-voicebot
```

### 2. 检查电脑

macOS 和 Windows 在仓库根目录执行同一条 Python 命令：

```bash
python voice-assistant/tools/bootstrap.py doctor
```

如果 Windows 没有 `python` 命令，可改用 `py -3.11`。工具只检查环境和版本锁，不烧录、
不访问串口、不收集密钥。

### 3. 准备锁定源码和本地工具

```bash
python voice-assistant/tools/bootstrap.py setup
```

该命令会在 `.qh-voice-workspace/` 中：

1. 按 [`integration-lock.json`](integration-lock.json) 获取精确版本的 `qh-voice-skill`；
2. 获取精确版本的 `qh-voice-kit` 固件源码；
3. 创建隔离的 Python 虚拟环境并安装本地接入命令；
4. 输出下一步可以直接发送给 Agent 的提示词。

工具不会修改已存在且有改动的仓库，也不会下载任意固件或接受 Flash 地址。需要换目录时使用：

```bash
python voice-assistant/tools/bootstrap.py setup --workspace path/to/empty-workspace
```

### 4. 把工作交给 Agent

复制终端最后输出的提示词发送给 Agent。也可以直接复制
[`AGENT_PROMPT.md`](AGENT_PROMPT.md) 中的“首次接入”提示词。

Agent 应按以下顺序工作：

1. 完整读取 `qh-voice-skill/SKILL.md`；
2. 只读检查电脑、USB 串口和硬件 Profile；
3. 验证 Release Manifest、固件大小、哈希和 Flash 区间；
4. 向你展示精确设备、Release、Profile、写入范围和恢复影响；
5. 只有收到精确确认后才烧录；
6. 打开临时的 `127.0.0.1` 中文配置页；
7. 配置写入 ESP32 后关闭页面，设备自动重启并开始运行；
8. 按屏幕状态完成第一次真实对话验收。

## 配置页要填写什么

当前正式路径只需要：

- Wi-Fi 名称与密码；
- 豆包实时语音模型 API Key；
- 该服务授权的音色 ID；
- 可选的设备名称、提示词和 QH 结构化数据同步配置。

先在[豆包语音控制台](https://console.volcengine.com/speech/)的“开通管理”中开通
“豆包实时语音模型 3.0（Seeduplex）”，再进入
[API Key 管理](https://console.volcengine.com/speech/new/setting/apikeys?projectName=default.)
创建或复制项目可用的 Key。若当前页面只显示 APP ID，请离开应用详情页，进入 API Key 管理；
不要把 APP ID、Access Token、账号 AK/SK 或其他模型的 Key 填入本流程。

API Key 和 Wi-Fi 密码只能填在 Skill 临时打开的本地中文页面中。不要把它们粘贴到 Agent 对话、
命令行、Issue、截图或 QH 平台。页面经 USB 直接写入设备 NVS，QH 平台不持有或转发 Key，
原始音频也不经过 QH。

## 第一次对话如何算成功

完成配置后，按顺序核对：

- 屏幕从“等待配置”进入联网、连接模型，最终显示“按住说话”；
- 按住按钮时显示录音状态，松开后显示等待回答；
- 屏幕出现识别或回答文本；
- 扬声器连续播放完整回答，没有明显周期性断音或爆裂杂音；
- 断电重启后无需电脑端程序，设备能够再次进入“按住说话”。

编译成功、能识别串口或屏幕亮起，都不能替代一次真实的语音往返验收。出现问题时按
[故障排查](docs/troubleshooting.md)逐层定位。

## 用户接口与当前实现不同怎么办

不要在安装现场临时拼接 ASR、LLM 和 TTS，也不要引入 Voice Gateway。先阅读
[Provider 适配指南](docs/provider-adaptation.md)，让 Agent 根据对方的官方、版本化接口文档，
只改 `qh-voice-kit` 中的 Provider 适配层和对应测试，再走编译、候选版烧录和实机验收。

## 三个仓库分别负责什么

| 仓库 | 职责 |
| --- | --- |
| `qh-video-voicebot` | 用户入口、版本锁、接入说明、引导工具和视频资料 |
| `qh-voice-skill` | Agent 工作流、主机检查、Release 验证、安全烧录、本地中文配置页 |
| `qh-voice-kit` | ESP32 固件、硬件 Profile、Provider 协议、屏幕与音频运行时、Release 契约 |

详细的数据流和硬件引脚见[架构与硬件边界](docs/architecture.md)。本目录不复制另外两个仓库的
实现代码，避免入口文档与真实固件分叉。

## 历史示例说明

[`008-voice-input`](../008-voice-input/) 是视频对应的“录音后上传到本地 HTTP 服务”示例，
用于理解录音、WAV 和上传流程。它不是当前完整语音助手的安装入口，也不应和本流程混用。
