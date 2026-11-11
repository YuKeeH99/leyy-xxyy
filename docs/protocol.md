# 协议说明

## 1. AT 测试命令

产品工程注册了以下测试命令：

```text
AT+VNOTATEST
AT+VOLUMETEST
AT+AUDIOTEST
AT+SSIDDELTEST
AT+WIFIOTATEST
AT+WIFIENABLETEST
AT+WIFIVERTEST
```

基础检查：

```text
AT
ATI
AT+CGMR
AT+CPIN?
AT+CSQ
AT+CREG?
AT+CGREG?
AT+QENG="servingcell"
```

## 2. 网络链路

设备支持两条网络链路：

1. **4G**：EC800M 自身的 LTE Cat.1
2. **Wi-Fi**：EC800M 通过 UART AT 控制 FGM840R

Wi-Fi 相关 AT 命令：

```text
AT+QGMR                 查询 Wi-Fi 版本
AT+CSUB                 查询 Wi-Fi 子版本
AT+WLCONN               连接路由器
AT+WLRSSI               查询 Wi-Fi 信号
AT+WLSTATE              查询 Wi-Fi 状态
AT+QAPNETOPEN/CLOSE     打开/关闭配网 AP
AT+QWSCFG/QWSOPEN       配置并连接 WebSocket
AT+QWSWRITE             发送 WebSocket 数据
AT+HTTPCLIENT           Wi-Fi HTTP 请求
AT+OTA=<地址>,<端口>    触发 Wi-Fi 模组自行 OTA
```

## 3. 小智协议

产品层通过 HTTP / WebSocket 与后台通信：

- OTA 检查：获取固件版本、下载地址和 WebSocket 地址
- WebSocket：语音数据、控制指令、状态上报
- 设备注册：获取设备编号和绑定信息

OTA 地址和注册地址在：

```text
app/src/vn_xiaozhi_app.c
xiaozhi/src/vn_xiaozhi_ota.c
```

部署自己的后台时，需要同步修改这些地址。

## 4. OTA / FOTA

| 方式 | 目标 | 传输链路 |
| --- | --- | --- |
| QFlash 整包 | EC800M 主控及平台分区 | USB Download Port + aboot |
| Cell FOTA | EC800M 应用/支持的分区 | 4G 网络下载 |
| 小智 OTA 检查 | 获取版本和升级信息 | Wi-Fi 或 4G HTTP |
| Wi-Fi 工厂 OTA | FGM840R 独立固件 | EC800M 发 AT 参数，Wi-Fi 自行联网下载 |
| ESP32 固件升级 | 另一块 ESP32 板 | 不属于本工程 |

不要把小智服务器 OTA 地址、Cell FOTA 地址和 Wi-Fi 工厂 OTA 服务器参数混为一项配置。

## 5. Wi-Fi 模组 OTA

当前主控代码提供的是工厂 Wi-Fi OTA 流程：

1. 电脑通过 EC800M 的外部 AT 命令设置参数：

   ```text
   AT+WIFIOTATEST=<ssid>,<pwd>,<ota_wifi_ip>,<port>
   ```

2. `Vn_AT_WifiOta_Cmd()` 调用 `Vn_Factory_SetOtaWifi()` 保存一次性升级参数。
3. 下次启动时，`Vn_Main_App()` 检测 `Vn_Factory_IsOtaWifiMode()`。
4. EC800M 通过 UART AT 命令让 FGM840R 连接指定 AP。
5. EC800M 再向 FGM840R 发送：

   ```text
   AT+OTA=<ota_wifi_ip>,<port>
   ```

6. FGM840R 自己连接服务器、下载并升级自己的固件。
7. EC800M 等待后通过 `AT+QGMR` 查询 Wi-Fi 新版本。

所以流程不是「电脑 → EC800M → UART 透传 Wi-Fi BIN」，而是「电脑设置参数 → EC800M 转发 OTA 参数 → FGM840R 联网自行下载升级」。

## 6. Wi-Fi 镜像能否与 Cat.1 镜像一起下载

按当前实现，不能直接合并进 EC800M 的 18 文件 QFlash ZIP：

- 两颗模组有各自独立的处理器、Flash、固件格式和升级状态机
- Cat.1 的 `download.json` 没有 FGM840R 镜像分区或升级命令
- EC800M 的 `customer_app.bin` 只含 Wi-Fi 主控驱动，不含 Wi-Fi 固件
- 当前代码只实现了 Wi-Fi 网络 OTA，没有实现「从 Cat.1 镜像取 Wi-Fi BIN，再通过 UART 写 Wi-Fi Flash」的下载器

量产时可以采用「同一工位、两步升级」：

```text
步骤 1：QFlash 烧录 EC800M 完整包
步骤 2：通过 AT+WIFIOTATEST 触发 FGM840R OTA
步骤 3：读取 ATI 和 AT+WIFIVERTEST，分别核对两个版本
```
