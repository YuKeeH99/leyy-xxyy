# 调试说明

## 1. 应用日志

`bsp/vn_log/vn_log.c` 中：

- `ql_log_mask_set(QL_LOG_APP_MASK, QL_LOG_PORT_USB)` 将应用日志输出到 USB
- `printf()` 和 `_ql_app_log()` 双路输出
- `VN_LOG_D/I/W/E` 经队列由独立日志任务发送

普通 USB 串口可辅助观察应用输出；更完整的模组和网络日志使用官方 Catlog / CATStudio、DIAG 口及官方工具。

## 2. dump 抓取

当模组发生断言或异常重启时：

1. 使用官方 dump log 工具和 `Quectel USB Dump Debug Port`
2. 等待转储文件完整导出
3. 重点查看 `com_errlog.bin`、任务名、PC、LR、SP、内存和任务信息
4. dump 导出后如果模组不再枚举，断电重启再进入正常模式或下载模式

## 3. 地址还原到源码

构建时保留 `-g` 和 `app.elf`。发生异常时，用当前固件对应的同一份 ELF 解析地址：

```powershell
& <SDK>\ql-cross-tool\win32\owtoolchain\gcc-arm-none-eabi\bin\arm-none-eabi-addr2line.exe `
  -e <SDK>\ql-application\threadx\build\app.elf `
  -f -C 0x80516338
```

Thumb 函数返回地址的 bit0 可能为 1。若日志返回地址为 `0x80516339`，符号解析通常使用对齐后的 `0x80516338`，并配合 objdump 确认真实调用点。

不能用另一版本、另一次链接布局不同的 `app.elf` 解析地址。

## 4. 日志和调试点

| 内容 | 文件 |
| --- | --- |
| 应用日志 | `bsp/vn_log/vn_log.c` |
| 主循环 | `app/src/vn_main_app.c` |
| 小智状态机 | `app/src/vn_xiaozhi_app.c` |
| 网络 | `component/vn_net`、`component/vn_http`、`component/vn_websocket` |
| OTA | `component/vn_ota`、`xiaozhi/src/vn_xiaozhi_ota.c` |
| 雷达 | `bsp/vn_radar` |
| 显示 | `bsp/vn_lcd`、`bsp/vn_oled`、`app/src/vn_display.c` |

## 5. 板上调试建议

1. 先确认 AT 口能执行 `ATI`，排除 USB / 驱动问题
2. 再确认 `AT+CPIN?`、`AT+CSQ`、`AT+CREG?`，排除 SIM 卡和网络注册问题
3. 再确认 Wi-Fi 模组版本 `AT+QGMR` 和状态 `AT+WLSTATE`
4. 最后看应用日志和 WebSocket 连接状态

不要同时修改网络、音频和显示三条链路；一次只改一处，便于定位。
