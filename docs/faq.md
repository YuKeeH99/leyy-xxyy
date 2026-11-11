# 常见问题

## 1. 本仓库能直接编译吗？

不能。本仓库只包含产品层 `venus`，还需要匹配版本的移远 EC800M QuecOpen C SDK、ARMCC/GCC 工具链和 license。详见 [编译说明](build.md)。

## 2. Wi-Fi 模组固件在哪里？

FGM840R 是独立模组，有自己的固件和 OTA 流程。本仓库只包含 EC800M 侧的 UART AT 控制逻辑，不包含 FGM840R 的完整固件。

## 3. Wi-Fi 能和控制板一起烧录吗？

不能直接合并进 EC800M 的 18 文件 QFlash ZIP。两颗模组有独立处理器、Flash 和升级状态机。量产建议「QFlash 烧 EC800M + AT+WIFIOTATEST 升级 Wi-Fi」两步走。详见 [协议说明](protocol.md)。

## 4. A01 和 A02 能混用吗？

不能。本工程以 `EC800MCNNR01A01SV08_DTN` 为行为基线。A02 只能参考，不能当作当前设备的逐字节一致版本。不要混用不同版本或不同尾缀的固件。

## 5. 烧录时报 `download.json: Inconsistent information`

说明恢复 ZIP 的结构、顺序或元数据与原包不兼容。检查：

- 是否保持 18 个根文件
- `customer_app.bin` 的位置是否正确
- 是否直接删除再新建 ZIP 条目（这会改变元数据和顺序）

详见 [烧录说明](flashing.md)。

## 6. 烧录时 `DL Time Out` 或 USB 掉线

- 确认端口选择正确（Download Port）
- 确认 `AT+QDOWNLOAD=1` 后设备已重新枚举
- 换 USB 线、换供电、换电脑 USB 口
- QFlash 先点 Start 再给设备上电

## 7. 编译成功但烧录后没变化

- 确认替换的是 `customer_app.bin`
- 确认 `app.bin` 是本次构建产物
- 确认烧录包版本和后台登记版本一致
- 确认设备真的进入了下载模式，而不是只重启

## 8. 设备反复重启

- 先抓 dump，确认是应用断言还是看门狗
- 检查最近修改的音频、显示、网络代码
- 确认 RAM / ROM 没有 overflow
- 确认链接脚本和 SDK 版本匹配

## 9. 音频 Codec 型号不确定

当前只确认 I2C0 和驱动路径，具体 Codec 型号应以原理图 / BOM 为准，不要猜测。

## 10. 可以用在商业产品里吗？

本仓库产品层代码采用「个人非商业使用许可」，不得用于商业用途。第三方组件（LVGL、Opus、EasyFlash、Mongoose 等）遵循各自许可；其中 Mongoose 是 GPLv2 或商业双许可，商业使用需要单独取得授权。详见 [LICENSE](../LICENSE) 和 [THIRD_PARTY_LICENSES.md](../THIRD_PARTY_LICENSES.md)。
