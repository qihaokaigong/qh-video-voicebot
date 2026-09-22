# 009｜ESP32 完整语音助手：安装与接入

[返回视频索引](../README.md) · [官方网站](https://qihao.dev/)

## 最简单的使用方式

你不需要下载固件源码，不需要配置 Python，也不需要自己执行烧录命令。只需要使用一个支持安装
Skill、访问本机 USB 串口的 AI Agent。

### 第一步：让 AI 安装 Skill

把下面这句话发送给你的 AI：

```text
请安装这个 Skill：https://github.com/qihaokaigong/qh-voice-skill
```

AI 完成安装后，开始一个新对话。如果你的 AI 会显示已安装 Skill 列表，应当能看到
`qh-voice-skill` 或 “QH Voice Installer”。

### 第二步：让 Skill 完成接入

把开发板通过 USB 数据线连接电脑，然后发送：

```text
请使用 qh-voice-skill 帮我安装并配置 QH Voice Kit。
```

接下来由 Skill 和 AI 完成环境检查、设备识别、固件校验、烧录预览、烧录、打开中文配置页和验收。
用户只需要：

1. 确认实物接线与 AI 识别出的硬件 Profile 一致；
2. 在烧录前确认 AI 展示的设备、Release ID 和 Hardware Profile ID；
3. 在 AI 打开的本地中文网页中填写 Wi-Fi 和豆包实时语音配置；
4. 按屏幕提示完成一次真实对话。

源码位置、安装工具依赖和内部执行命令由 Skill 自己管理。AI 不应要求用户克隆
`qh-voice-skill`、`qh-voice-kit`，也不应让用户手动准备 Python 环境或复制内部命令。

## 配置前需要准备的信息

当前路径只需要：

- 2.4 GHz Wi-Fi 名称和密码；
- 豆包实时语音模型 3.0（Seeduplex）API Key；
- 该服务授权的音色 ID；
- 可选的设备名称、提示词和 QH 结构化数据同步配置。

先在[豆包语音控制台](https://console.volcengine.com/speech/)的“开通管理”中开通
“豆包实时语音模型 3.0（Seeduplex）”，再进入
[API Key 管理](https://console.volcengine.com/speech/new/setting/apikeys?projectName=default.)
创建或复制项目可用的 Key。

如果应用详情页只显示 APP ID，请进入 API Key 管理。不要把 APP ID、Access Token、账号 AK/SK、
单独的 ASR Key、LLM Key 或 TTS Key 填入当前流程。

API Key 和 Wi-Fi 密码只能填在 Skill 打开的 `127.0.0.1` 本地中文页面中，不要发到 AI 对话、
Issue、截图或 QH 平台。配置通过 USB 直接写入 ESP32；页面完成后会关闭，不需要常驻电脑程序。

## 使用流程

```text
在 AI 中安装 qh-voice-skill
  -> 对 AI 说“帮我安装并配置 QH Voice Kit”
  -> AI 检查电脑、USB 串口和硬件 Profile
  -> AI 展示经过校验的烧录计划
  -> 用户精确确认后烧录
  -> Skill 打开本地中文配置页
  -> 配置通过 USB 写入 ESP32
  -> 设备自动重启并进入语音对话
```

配置完成后，电脑和 AI 都不需要持续运行。设备重新上电会自动连接 Wi-Fi 和豆包实时语音模型。
系统没有 Voice Gateway、本地 Python 语音服务，也不需要分别配置 ASR、LLM 和 TTS。

## 当前支持范围

- 目标电脑：macOS arm64、Windows x64；
- 硬件 Profile：`qh.voice-kit.breadboard.n16r8.v1`；
- 主控：ESP32-S3 N16R8；
- 麦克风：INMP441；
- 功放：MAX98357A；
- 屏幕：GMT130-V1.0 / ST7789 / 240×240；
- 操作方式：按住按钮说话，松开后等待回答。

只有芯片名称同为 ESP32-S3 不能证明硬件兼容。具体接线和数据边界见
[架构与硬件边界](docs/architecture.md)。

## 当前 Release 状态

当前固件仍是 `candidate`：参考设备已经完成人工语音回合验证，但正式 `allowed` Release 和
macOS arm64／Windows x64 干净电脑验收尚未完成。

Skill 必须遵守 Release 门禁：没有可用的正式 Release 时，应明确告诉用户当前阻塞，而不是让用户手填
Flash 地址或把候选版当作稳定版烧录。开发者候选验收也必须由 Skill 展示精确计划并取得确认。

## 第一次对话如何算成功

- 屏幕从“等待配置”进入联网、连接模型，最终显示“按住说话”；
- 按住按钮时显示录音状态，松开后显示等待回答；
- 屏幕出现识别或回答文本；
- 扬声器连续播放完整回答，没有明显周期性断音或爆裂杂音；
- 断电重启后不启动任何电脑端程序，也能再次进入“按住说话”。

编译成功、识别到串口或屏幕亮起，都不能替代一次真实语音往返。出现问题时让 AI 使用 Skill 诊断，
也可参考[故障排查](docs/troubleshooting.md)。

## 用户接口与当前实现不同怎么办

把新 Provider 的官方、版本化接口文档发给 AI，然后说：

```text
请使用 qh-voice-skill 评估并适配这个实时语音接口。先输出协议差异、未知项、最小修改范围和验收计划，
不要索取真实密钥，不要引入 Voice Gateway；方案确认后再修改代码。
```

具体边界见 [Provider 适配指南](docs/provider-adaptation.md)。适配工作仍由 Skill 和 AI 执行，用户不需要
先查找源码目录或准备开发命令。

## 代码职责

| 仓库 | 职责 |
| --- | --- |
| [`qh-voice-skill`](https://github.com/qihaokaigong/qh-voice-skill) | 用户安装入口；驱动 AI 完成检查、烧录、配置、诊断和接口适配 |
| [`qh-voice-kit`](https://github.com/qihaokaigong/qh-voice-kit) | ESP32 固件、硬件 Profile、实时语音协议、屏幕与音频运行时、Release 契约 |
| `qh-video-voicebot` | 面向用户的视频资料与使用说明 |
