# 硬件说明

## 1. 平台概述

这块板子不是单芯片设备，而是由 Cat.1 主控、独立 Wi-Fi 模组和多组外设组成：

```text
                         服务器
                     HTTP / WebSocket / OTA
                           ^
                           |
              +------------+-------------+
              |                          |
       LTE Cat.1 4G                 Wi-Fi 网络
              |                          |
+-------------+------------+     UART AT |
| Quectel EC800MCN_LE      |     460800  |
| ASR1606 / QuecOpen C SDK +------------>+------------------+
| ThreadX + 产品 venus APP |             | FGM840R Wi-Fi    |
+--------------------------+             | 独立固件/独立 OTA |
 |  |  |  |  |  |  |  |                  +------------------+
 |  |  |  |  |  |  +-- GPIO/ADC ---- 充电、电池、触摸、LDO、PA
 |  |  |  |  |  +----- UART -------- XEND101H 雷达
 |  |  |  |  +-------- UART -------- 唤醒词芯片
 |  |  |  +----------- SPI2 -------- 外置 SPI NOR Flash
 |  |  +-------------- I2C0 -------- 音频 Codec
 |  +----------------- I2C2 -------- SSD1306 OLED
 +-------------------- SPI1 -------- GC9107 LCD
 +----------------------- USB --------- AT / DIAG / LOG / DOWNLOAD
```

## 2. 主控

```text
Model: EC800MCN_LE
Revision: LTE01R06A17_C_SDK_A
CustRevision: EC800MCNNR01A01SV08_DTN
```

平台为 **ASR1606 + ARM Cortex-R 系列 + QuecOpen C SDK RTOS**。应用 GCC 参数使用 `-mcpu=cortex-r4`，该参数描述编译指令集目标，不代表芯片型号。

## 3. Wi-Fi 模组

板级信息代码写的是 **FGM840R**：

- `venus/bsp/vn_board_info/vn_board_info.c`：`VN_BOARD_WIFI_NAME "FGM840R"`
- `venus/bsp/vn_wifi/vn_wifi.c`：主 UART，波特率 460800，8N1，无流控
- EC800M 通过 GPIO27 控制 Wi-Fi CHIP_EN，GPIO20 控制 Wi-Fi LDO

Wi-Fi 模组有自己的固件、协议栈和 OTA 流程。EC800M 的 `customer_app.bin` 只包含主控侧的 Wi-Fi AT 驱动和业务逻辑，不包含 FGM840R 的完整固件。

## 4. 外设与代码对应

| 外设/功能 | 接口或引脚 | 主要代码 |
| --- | --- | --- |
| Wi-Fi | `QL_MAIN_UART_PORT`，460800；CHIP_EN GPIO27；LDO GPIO20 | `bsp/vn_wifi`、`app/src/vn_wifi_app.c` |
| 雷达 | `QL_BT_UART_PORT`；OUT GPIO22；LDO GPIO23 | `bsp/vn_radar/vn_xend101h.c` |
| 唤醒词芯片 | `QL_U4_UART_PORT`；INT GPIO26；LDO GPIO21 | `bsp/vn_wakeup_chip` |
| LCD | SPI1；DC GPIO35；RST GPIO32；TE GPIO8 | `bsp/vn_lcd/vn_gc9107.c` |
| OLED | I2C2；LCD/OLED LDO GPIO25 | `bsp/vn_oled/vn_ssd1306.c` |
| 外置 SPI Flash | SPI2；CS GPIO5 | `bsp/vn_spi_flash` |
| 音频 Codec | I2C0；Codec LDO GPIO24 | `bsp/vn_audio` |
| PA | GPIO124 | `bsp/vn_audio` |
| 电池电压/温度 | ADC0 / ADC1 | `bsp/vn_charge` |
| 充电状态/使能 | CHRG GPIO14；STDBY GPIO15；EN GPIO1 | `bsp/vn_charge` |
| 触摸 | GPIO31 | `bsp/vn_gpio` |
| 状态输出 | GPIO70 | 板级 GPIO 代码 |
| 调试串口 | `QL_DEBUG_UART_PORT` | `bsp/vn_log` |

引脚定义集中在 `venus/vn_plat_pin_def.h`。修改板子时先核对原理图和该文件，不要只改驱动名称。

## 5. 软件分层

```text
产品应用层
  Vn_Main_App / Vn_At_Cmd_App
  小智状态机、音频、显示、Wi-Fi 应用、AT 测试命令
        |
产品组件层
  HTTP / WebSocket / OTA / 网络抽象 / 参数 / 队列
        |
板级 BSP
  GPIO / LCD / OLED / 雷达 / 音频 / Wi-Fi UART / 充电 / Flash
        |
第三方库
  LVGL / Opus / Mongoose / EasyFlash / AT framework / mbedTLS / cJSON
        |
QuecOpen API 与 api_map
  ql_rtos / ql_uart / ql_gpio / ql_nw / ql_log / ql_dev ...
        |
ThreadX 内核、芯片厂 HAL、闭源平台库、bootloader、CP/DSP/RF
```

## 6. 应用启动

主入口 `app/src/vn_main_app.c` 通过：

```c
application_init(Vn_Main_App, "Vn_Main_App", 20, 110);
```

注册应用。主要启动顺序：

1. 日志、LDO、参数和显示初始化
2. 检查启动电压
3. 初始化雷达、触摸、唤醒芯片、充电和 Wi-Fi
4. 创建 `Audio_App` 任务
5. 如果设置了 Wi-Fi 工厂 OTA 模式，执行 `Vn_Factory_RunOtaWifi()`
6. 创建 `Xiaozhi_App` 任务
7. 主循环处理触摸、网络信号、电量、雷达和唤醒芯片周期复位

AT 命令通过 `user_boot_init(Vn_Set_AT_Cmds, 1)` 注册。
