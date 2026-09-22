# 架构与硬件边界

## 运行时数据流

```text
INMP441 麦克风
  -> ESP32-S3：16 kHz / 16-bit / 单声道 PCM，20 ms 一帧
  -> 豆包实时语音模型 3.0（Seeduplex），单个 TLS WebSocket
  -> 24 kHz PCM 回答
  -> ESP32-S3 缓冲播放
  -> MAX98357A + 扬声器

                         ESP32-S3 -> ST7789 状态与文本
                         ESP32-S3 -> 可选 QH 结构化事件
```

QH 平台只承接用户授权的结构化设备和对话数据。Provider API Key、Wi-Fi 密码和原始音频不进入
QH 平台。QH 不可用时，也不应阻断当前语音对话。

电脑与 Agent 只参与安装、配置变更、诊断、恢复和 Provider 适配。运行时没有 Voice Gateway，
也没有需要用户每次手动启动的电脑程序。

## 唯一支持的硬件 Profile

当前 Profile ID：`qh.voice-kit.breadboard.n16r8.v1`。

| 模块端点 | ESP32-S3 端点 | 说明 |
| --- | --- | --- |
| INMP441 `SCK` | GPIO4 | 麦克风位时钟 |
| INMP441 `WS` | GPIO5 | 左右声道时钟 |
| INMP441 `SD` | GPIO6 | 麦克风数据 |
| 按钮 `OUT` | GPIO8 | 按住说话 |
| ST7789 `SCK` | GPIO9 | 屏幕 SPI 时钟 |
| ST7789 `MOSI` | GPIO10 | 屏幕 SPI 数据 |
| ST7789 `RST` | GPIO11 | 屏幕复位 |
| ST7789 `DC` | GPIO12 | 屏幕命令/数据 |
| MAX98357A `BCLK` | GPIO16 | 扬声器位时钟 |
| MAX98357A `LRC` | GPIO17 | 扬声器声道时钟 |
| MAX98357A `DIN` | GPIO18 | 扬声器 PCM 数据 |

记录的屏幕模块为 GMT130-V1.0、ST7789、240×240、SPI mode 3；主控为 ESP32-S3 N16R8，
Flash 16 MB，OPI PSRAM。INMP441 使用 3.3 V，所有模块必须共地。完整机器可读事实以
`qh-voice-kit/hardware-profiles/qh.voice-kit.breadboard.n16r8.v1.json` 为准。

## 屏幕不是装饰功能

固件必须显示启动、等待配置、联网、校时、连接模型、待机、录音、等待回答、播放、完成和可恢复
错误。黑屏是 Release 阻塞问题，即使串口日志或声音正常也不能视为通过。

## 配置与商务数据分离

- 固件镜像不内置用户 API Key、Wi-Fi 密码或 QH 登录信息；
- Skill 临时启动的配置页只监听 `127.0.0.1`，不加载第三方资源；
- 配置通过 USB 串口事务写入设备 NVS，成功后页面和进程退出；
- 业务提示词、设备名和可选 QH 同步配置属于运行时配置，不和固件源码绑定；
- 当前是开发板方案，没有安全芯片。物理接触设备并使用专业工具的人可能提取设备配置，因此应使用
  独立、可撤销、有限额的 Provider Key。

## Skill 与 Release

用户在 AI 中安装 `qh-voice-skill` 后，由 Skill 自己定位内部工具和固件 Release，用户不需要管理源码
目录或版本锁。只有 Release 包中的 `release-manifest.json` 可以声明 Flash 文件、地址、大小和
SHA-256；稳定烧录还必须满足 `acceptance.status=allowed`。文档示例、文件名和 Agent 记忆都不能代替
Manifest 成为烧录地址来源。
