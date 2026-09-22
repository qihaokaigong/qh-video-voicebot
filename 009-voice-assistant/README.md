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

接下来由 Skill 和 AI 完成环境检查、设备识别、固件校验、烧录预览、烧录并打开中文配置页。
用户只需要：

1. 断电完成下方接线，并确认实物与 Hardware Profile 一致；
2. 在烧录前确认 AI 展示的设备、Release ID 和 Hardware Profile ID；
3. 在 AI 打开的本地中文网页中填写 Wi-Fi 和豆包实时语音配置。

## 完整接线表

本表只适用于当前固件的 `qh.voice-kit.breadboard.n16r8.v1` Profile。接线前拔掉 USB 和其他电源，
按照模块 PCB 丝印确认针脚，不要根据杜邦线颜色猜测。所有模块必须与 ESP32 共地。

| 模块 | 模块针脚 | 连接位置 | 说明 |
| --- | --- | --- | --- |
| INMP441 | `VDD` | `3V3` | 只用 3.3 V，不接 5 V |
| INMP441 | `GND` | `GND` | 共地 |
| INMP441 | `SCK` | `GPIO4` | I²S 位时钟 |
| INMP441 | `WS` | `GPIO5` | I²S 左右时钟 |
| INMP441 | `SD` | `GPIO6` | 麦克风数据 |
| INMP441 | `L/R` | `GND` | 选择左声道；固件读取左 slot |
| 三针按钮模块 | `VCC` | `3V3` | 按钮模块供电 |
| 三针按钮模块 | `OUT` | `GPIO8` | 按住说话；当前模块松开 LOW、按下 HIGH |
| 三针按钮模块 | `GND` | `GND` | 共地 |
| GMT130-V1.0 ST7789 | `GND` | `GND` | 共地 |
| GMT130-V1.0 ST7789 | `VCC` | `3V3` | 屏幕供电 |
| GMT130-V1.0 ST7789 | `SCK` | `GPIO9` | SPI 时钟 |
| GMT130-V1.0 ST7789 | `SDA` | `GPIO10` | SPI MOSI，不是 I²C SDA |
| GMT130-V1.0 ST7789 | `RES` | `GPIO11` | 屏幕复位 |
| GMT130-V1.0 ST7789 | `DC` | `GPIO12` | 命令／数据选择 |
| GMT130-V1.0 ST7789 | `BLK` | `3V3` | 背光常亮 |
| GMT130-V1.0 ST7789 | 无 `CS` 针脚 | 不连接 | 固件使用 CS=-1、硬件 SPI Mode 3 |
| MAX98357A | `VIN` | `3V3` | 当前实测装配的功放供电 |
| MAX98357A | `GND` | `GND` | 共地 |
| MAX98357A | `BCLK` | `GPIO16` | I²S 位时钟 |
| MAX98357A | `LRC` | `GPIO17` | I²S 左右时钟 |
| MAX98357A | `DIN` | `GPIO18` | ESP32 输出的 PCM 数据 |
| MAX98357A | `GAIN` | 不连接 | 使用模块默认增益 |
| MAX98357A | `SD` | 不连接 | 当前参考模块保持悬空 |
| MAX98357A | `SPK+` | 喇叭正端 | 差分输出 |
| MAX98357A | `SPK-` | 喇叭负端 | 差分输出；喇叭任一端都不能接 GND |

当前参考喇叭为无源 `4 Ω / 3 W`。本机使用的红线接 `SPK+`、黑线接 `SPK-` 只对应这只喇叭，
更换喇叭时应按端子标识确认，不能套用线色。

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
系统没有 Voice Gateway、本地 Python 语音服务，也不需要分别配置 ASR、LLM 和 TTS。设备确认写入并
自动重启后，安装与配置流程到此结束，不额外执行健康检查；直接开始使用即可。

## 当前支持范围

- 目标电脑：macOS Intel（x64）、macOS Apple Silicon（arm64）、Windows x64；
- 硬件 Profile：`qh.voice-kit.breadboard.n16r8.v1`；
- 主控：ESP32-S3 N16R8；
- 麦克风：INMP441；
- 功放：MAX98357A；
- 屏幕：GMT130-V1.0 / ST7789 / 240×240；
- 操作方式：按住按钮说话，松开后等待回答。

只有芯片名称同为 ESP32-S3 不能证明硬件兼容。具体接线和数据边界见
[架构与硬件边界](docs/architecture.md)。

## 开始使用与遇到问题

设备进入“按住说话”后即可直接使用：按住按钮说话，松开后等待回答。Skill 不会为了完成安装而强制
执行测试对话、断电重启或完整健康检查。

如果后续出现黑屏、无法联网、回答报错、没有声音或杂音，再把实际现象告诉 AI，让它使用 Skill 做针对性
诊断并记录问题，也可参考[故障排查](docs/troubleshooting.md)。

## 用户接口与当前实现不同怎么办

把新 Provider 的官方、版本化接口文档发给 AI，然后说：

```text
请使用 qh-voice-skill 评估并适配这个实时语音接口。先输出协议差异、未知项、最小修改范围和验收计划，
不要索取真实密钥，不要引入 Voice Gateway；方案确认后再修改代码。
```

具体边界见 [Provider 适配指南](docs/provider-adaptation.md)。适配工作仍由 Skill 和 AI 执行，用户不需要
先查找源码目录或准备开发命令。
