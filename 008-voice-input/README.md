# 008｜ESP32 语音输入：INMP441 录音、中文波形与 WAV 上传

[观看 B 站视频](https://www.bilibili.com/video/BV1MHeP65EFo/) · [返回视频索引](../README.md) · [官方网站](https://qihao.dev/)

## 这份代码能做什么

[`esp32/`](esp32/) 是可以直接编译的 Arduino Sketch，收录了本期写入 ESP32-S3 的语音输入程序。
它实现以下功能：

- 按住 GPIO8 按钮开始录音，松开保存，最长录制 5 秒；
- 从 INMP441 左声道以 16 kHz、32-bit I²S 格式连续采样；
- 在 ST7789 屏幕上显示录音状态、计时和约 25 FPS 的相对声量波形；
- 把采样封装为单声道、16-bit PCM WAV；
- 通过 Wi-Fi 上传到本地语音服务，屏幕显示“正在上传”“保存成功”或中文错误状态；
- 向服务端发送设备心跳，使管理页面能够区分待机、录音、上传与错误状态。

公开目录已经把开发项目中的多级相对引用整理成一个独立 Arduino Sketch。算法和引脚配置与本期
实机版本一致；真实 Wi-Fi 密码和设备令牌未公开。

## 目录结构

```text
008-voice-input/
├── README.md
└── esp32/
    ├── esp32.ino                 # Arduino 主程序
    ├── secrets.example.h         # 公开配置模板
    ├── chinese_display.h         # 中文点阵与 UTF-8 绘制
    ├── audio_envelope.h          # 声量包络
    ├── recording_*.h             # 录音状态与上传逻辑
    ├── wav_builder.h             # WAV 封装
    └── NotoSansCJK-OFL-1.1.txt   # 中文字形来源许可证
```

Arduino 要求主 `.ino` 文件与 Sketch 文件夹同名，因此主程序使用 [`esp32.ino`](esp32/esp32.ino)。
视频主题和源码职责由上级目录 `008-voice-input` 以及文件内的模块名表达。

## 已验证硬件与接线

该接线只对应本期使用的 ESP32-S3 N16R8、预焊 INMP441、GPIO8 三针按钮和 ST7789 屏幕：

| 模块端点 | ESP32-S3 端点 | 作用 |
| --- | --- | --- |
| INMP441 `VDD` | `3V3` | 麦克风供电，不能接 5V |
| INMP441 `GND` | `GND` | 公共地 |
| INMP441 `SCK` | `GPIO4` | I²S 位时钟 |
| INMP441 `WS` | `GPIO5` | I²S 左右声道选择时钟 |
| INMP441 `SD` | `GPIO6` | 麦克风数据输入 |
| INMP441 `L/R` | `GND` | 数据进入左声道 slot |
| 按钮 `VCC` | `3V3` | 按钮模块供电 |
| 按钮 `OUT` | `GPIO8` | 松开 LOW、按下 HIGH |
| 按钮 `GND` | `GND` | 公共地 |
| ST7789 `SCL/SCK` | `GPIO9` | SPI 时钟 |
| ST7789 `SDA/MOSI` | `GPIO10` | SPI 数据 |
| ST7789 `RES/RST` | `GPIO11` | 屏幕复位 |
| ST7789 `DC` | `GPIO12` | 命令／数据选择 |

本期屏幕实测配置是 `SPI_MODE3`、1 MHz、240×240、旋转方向 2。不同屏幕批次可能需要不同
初始化参数，不能只按外观推断。

## 编译和配置

1. 安装 ESP32 Arduino Core `3.3.11`。
2. 在 Arduino Library Manager 安装 `Adafruit GFX Library` 和 `Adafruit ST7735 and ST7789 Library`。
3. 复制 [`secrets.example.h`](esp32/secrets.example.h) 为同目录下的 `secrets.h`。
4. 在 `secrets.h` 填写 Wi-Fi 名称、密码、本地服务地址和设备令牌。服务地址必须使用电脑的局域网 IP，
   不能写 `localhost`，末尾不要添加 `/`。
5. 打开 [`esp32.ino`](esp32/esp32.ino)，开发板选择
   `ESP32S3 Dev Module`，Flash 选择 16 MB，PSRAM 选择 OPI。

`secrets.h` 已由[仓库根目录的 `.gitignore`](../.gitignore) 统一排除，该规则对所有视频源码目录生效。
不要把真实密码或令牌提交到仓库、截图或视频素材中。

## 运行和检查

1. 先启动兼容的本地服务端，使 `POST /api/v1/device/recordings` 可以接收带鉴权的 WAV。
2. 编译并上传 Sketch，保持 USB 连接。
3. 屏幕显示“按住说话／按键已就绪”后，按住按钮说话，至少保持 0.3 秒。
4. 松开按钮后等待“正在上传 → 保存成功 → 按住说话”。

屏幕使用内置的 16×16 简体中文字形子集，不需要额外安装中文显示库。录音界面显示“录音中”和
“松开后保存”，中间的声量柱继续来自同一批录音 PCM。串口中的 `STATUS`、`RECORDING`、`UPLOAD`
和 `ERROR` 等机器协议字段仍为英文，便于脚本解析。

服务端保存的“原始采集”是设备上传原件，不应被覆盖。当前实机原始录音电平很低，可能无法直接听清；
经 80 Hz 高通、24 dB 增益和峰值限制生成的独立清晰版，才是已经通过听测的日常试听版本。

## 已验证范围与当前边界

- 已实机验证录音按钮、连续采样、屏幕波形、WAV 上传和服务端保存。
- 已实听验证清晰处理版可闻；原始上传文件完整但电平很低。
- 中文屏幕版已经完成编译、写入和 Flash 哈希校验；具体中文字形、方向与对齐仍以当前实物屏幕的
  用户目视确认为最终验收。
- 该目录不包含服务端和网页代码。
- 当前固件只负责录音、上传和心跳，不在 ESP32 上执行语音转文字；STT、简体转写展示及后续处理属于
  独立服务端／网页代码，不包含在本目录中。
- 固件中的网络上传使用普通 HTTP，只适合当前可信局域网实验环境。

## 许可说明

本目录中的原创代码和文字遵循仓库根目录的 [LICENSE](../LICENSE)。Arduino Core、Adafruit 库及其
依赖遵循各自项目的许可。内置中文点阵由 Noto Sans CJK SC 2.004 生成，仅保留本界面所需字形；
该字形子集遵循 [SIL Open Font License 1.1](esp32/NotoSansCJK-OFL-1.1.txt)。
