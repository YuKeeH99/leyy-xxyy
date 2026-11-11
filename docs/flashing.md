# 烧录说明

> 本仓库不包含烧录包。恢复烧录包请向卖家获取，并确认包名、版本和校验值。

## 1. 工具和驱动

- Quectel QFlash（建议使用与你手上的包匹配的版本）
- Quectel Windows USB 驱动
- 原厂回滚包（必须保留）

安装驱动后，设备会枚举出多个串口，名称和 COM 号因电脑而异：

| 端口名称 | 用途 |
| --- | --- |
| Quectel USB AT Port | AT 命令，115200 8N1 |
| Quectel USB DIAG Port | DIAG / 日志工具 |
| USB 串行设备（MI05） | 普通 USB 串口 / 应用日志 |
| Quectel Download Port | ROM / aboot 下载 |
| Quectel USB Dump Debug Port | 崩溃 dump 导出 |

不要把这些 COM 号写死到其他电脑。

## 2. 烧录流程

1. 安装匹配的 Quectel Windows USB 驱动。
2. 确认识别到 EC800M，AT 口能执行 `ATI`。
3. 准备原始回滚包和恢复包，核对 ZIP 条目及版本。
4. 打开 QFlash，加载恢复 ZIP，波特率选择 **460800**。
5. 如果设备已经在 `Quectel Download Port`，直接选择该端口并 Start。
6. 如果设备处于正常 AT 模式，可先在 AT 口执行：

   ```text
   AT+QDOWNLOAD=1
   ```

   AT 口随后会断开，设备重新枚举为 `Quectel Download Port`。
7. 等 Download Port 稳定出现后，在 QFlash 选择该端口并 Start。
8. 如果使用「Start 后重新上电」的方式，应先确认 QFlash 已在扫描，再重新上电。
9. 必须看到 **100% PASS** 才认为烧录完成；进度条、绿色状态或端口出现都不等于成功。
10. 烧录后设备重新枚举为 AT / DIAG / 日志口，再进行 AT 和功能验证。

## 3. 烧录包结构

恢复包沿用原厂 18 文件结构：

```text
ReliableData.bin
bootloader.ubi
cp.bin
customer_app.bin
customer_fs.bin
download.json
dsp.bin
firmware.bin
flasher.img
flashinfo.bin
fota.json
fwcerts.bin
logo.bin
partition.bin
preboot.img
rf.bin
system.img
updater.bin
```

其中 `download.json` 描述了 preboot / flasher 加载、分区表、CP、DSP、RF、bootloader、customer_app、customer_fs 等写入过程。

### 替换原则

- 使用原始 A01 ZIP 或解包目录作为模板
- 只替换 `customer_app.bin`
- 保持 18 个文件的根目录结构、原始顺序和 ZIP 元数据
- 保留原始 ZIP 作为回滚包
- 生成后检查条目数量、`customer_app.bin` 的位置、应用长度和包哈希

不要直接删除 ZIP 内的 `customer_app.bin` 再新建条目。该操作曾把应用移到包尾并改变元数据，导致 QFlash 报：

```text
failed to extract 'download.json': Inconsistent information
aboot download engine started failed: boot engine startup failed
```

## 4. 常见失败

| 现象 | 可能原因 |
| --- | --- |
| `DL Time Out` | 端口选择、下载模式或 Start / 上电时序不正确 |
| `FAILED status malformed (0 bytes)`、Windows 错误 31 | 下载过程中 USB 掉线，等待端口稳定后重试，检查供电和线缆 |
| `download.json: Inconsistent information` | 恢复 ZIP 的结构、顺序或元数据与原包不兼容 |
| 下载引擎启动失败 | 先检查包解析日志，不要反复要求用户断电 |
| QFlash 内容空白或状态异常 | 关闭并重启工具后重新加载包 |

QFlash 调试日志位于其安装目录的 `log/Debug_Log_File.log`，是判断「包错误」还是「USB / 端口错误」的关键依据。

## 5. 回滚

- 保留原始 A01 ZIP
- 不要混用 A01 / A02
- 不要直接烧录不同版本或不同尾缀的固件
- 烧录前确认硬件版本、分区、回滚策略和升级兼容性

## 6. 串口下载和 AT 的关系

`AT+QDOWNLOAD=1` 只是让正常运行的 EC800M 重启并进入下载模式：

```text
正常模式                           下载模式
EC800M 固件运行中  --AT+QDOWNLOAD-->  ROM/aboot 等待 QFlash
USB AT Port                         USB Download Port
```

进入下载模式后，QFlash 发送的是 preboot、flasher、分区和镜像数据，使用平台下载协议，不是继续发送一长串 AT 命令。

EC800M 与 FGM840R 之间也是 UART AT 通讯，但那是板内两颗模组之间的链路，和电脑通过 USB AT 口控制 EC800M 是不同层次。
