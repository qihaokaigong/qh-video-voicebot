# 001｜一个新手点亮屏幕，经历了什么

[观看 B 站视频](https://www.bilibili.com/video/BV14E8n61EtV) · [返回视频索引](../README.md) · [官方网站](https://qihao.dev/)

本目录收录视频中最终使用的 ST7735 屏幕与按键表情程序。按键松开时显示普通脸；按住按键时显示笑脸。

## 源码文件

| 文件 | 说明 |
| --- | --- |
| [`st7735-button-face-test.ino`](st7735-button-face-test/st7735-button-face-test.ino) | Arduino 主程序，负责初始化屏幕、读取按键并绘制表情 |
| [`face_logic.h`](st7735-button-face-test/face_logic.h) | 将按键状态转换为普通脸或笑脸的独立逻辑 |

两个文件必须保留在同一个 `st7735-button-face-test` 文件夹中。

## 适用硬件

- ESP32-S3 N16R8 开发板
- 1.8 英寸、128 × 160、ST7735 SPI 屏幕
- 三针按键模块，按下输出 `HIGH`、松开输出 `LOW`
- 面包板、杜邦线和 USB-C 数据线

代码和接线来自视频中的实物组合，不保证直接适用于其他尺寸、批次或初始化类型的 ST7735 屏幕。

## 接线

接线前先断开 USB 电源。

### ST7735 屏幕

| ST7735 引脚 | ESP32-S3 | 代码中的名称 |
| --- | --- | --- |
| `GND` | `GND` | — |
| `VCC` | `3V3` | — |
| `SCL` / `SCK` | `GPIO12` | `TFT_SCLK` |
| `SDA` / `MOSI` | `GPIO11` | `TFT_MOSI` |
| `RES` / `RST` | `GPIO14` | `TFT_RST` |
| `DC` | `GPIO13` | `TFT_DC` |
| `CS` | `GPIO10` | `TFT_CS` |
| `BL` | `3V3` | — |

### 三针按键模块

| 按键引脚 | ESP32-S3 |
| --- | --- |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `OUT` | `GPIO8` |

请以模块上的实际丝印为准，不要只按照线的颜色判断引脚。这里使用的是带稳定输出的三针按键模块；普通两针裸按键不能直接照抄当前 `INPUT` 配置。

## 软件依赖

该版本曾在以下环境中完成编译：

- Arduino IDE
- ESP32 Arduino Core `3.3.10-cn`
- Adafruit GFX Library `1.12.6`
- Adafruit ST7735 and ST7789 Library `1.11.0`
- Adafruit BusIO `1.17.4`

其他版本可能也能工作，但尚未在本项目中验证。

## 使用说明

1. 在 Arduino IDE 的库管理器中安装上述 Adafruit 显示库及其依赖。
2. 打开 [`st7735-button-face-test.ino`](st7735-button-face-test/st7735-button-face-test.ino)。
3. 开发板选择 `ESP32S3 Dev Module`，然后选择实际连接的串口。
4. 断电完成接线并逐项复核，确认没有短路后再连接 USB。
5. 编译并上传程序。
6. 如需查看状态信息，将串口监视器设置为 `115200 baud`。

## 预期结果

- 程序启动且按键处于松开状态时，屏幕显示深色背景和黄色普通脸。
- 按住按键时，屏幕切换为带红色脸颊的笑脸。
- 松开按键后，屏幕恢复普通脸。
- 串口输出 `Face: NEUTRAL` 或 `Face: SMILE`。

## 验证状态

- 源码已使用 `ESP32S3 Dev Module` 完成编译。
- 普通脸已在视频使用的 128 × 160 ST7735 屏幕上显示成功。
- `INITR_GREENTAB` 已单次实机验证，可消除该屏幕右侧和底部的彩色花线。
- 按键与表情切换逻辑测试通过。
- 按住显示笑脸、松开恢复普通脸的最终实机复测记录尚未归档。

## 已知问题

- 本项目屏幕需要使用 `INITR_GREENTAB`。如果其他 ST7735 屏幕出现偏移、边缘花线或颜色异常，应先核对屏幕型号和初始化类型，不要同时修改接线与绘图代码。
- 当前按键代码按三针模块的 `HIGH / LOW` 输出设计。如果使用裸按键，需要另外设计上拉或下拉电路，并同步调整输入模式。
- 当前代码中的 GPIO 只对应本项目已经验证的 ESP32-S3 N16R8 接线，其他开发板应先核对针脚能力和占用情况。

## 许可说明

本目录中的原创源码和文字说明遵循仓库根目录的 [LICENSE](../LICENSE)。Adafruit 库不包含在本仓库中，使用时遵循其各自的许可条款。
