# 编译说明

## 1. 准备 SDK

本仓库不包含移远 SDK。你需要先取得与设备匹配的 **EC800M QuecOpen C SDK**：

- 设备型号：`EC800MCN_LE`
- 固件基线：`EC800MCNNR01A01SV08_DTN`
- 平台版本：`LTE01R06A17_C_SDK_A`
- 获取方式：移远开发者网站 <https://developer.quectel.com/resource-download?pid=147&cid=16>，或联系你的方案商

SDK 内应包含：

```text
ql-sdk/
├── build.bat
├── ql-application/threadx/
├── ql-kernel/threadx/
├── ql-bootloader/
├── ql-cross-tool/win32/
├── ql-config/
└── target/
```

> 如果你拿到的 SDK 版本与上面不同，先确认芯片平台、分区和链接地址一致，不要直接混用。

## 2. 工具链

- APP：SDK 自带的 ARM GNU GCC 9.2.1 + Gnumake
- Kernel：ARMCC（ARM Compiler 5），需要匹配的 license
- 打包：`ql-cross-tool/win32/host` 和 `ql-config/quec-project/scripts` 中的工具
- 分析：同一工具链中的 `objdump`、`nm`、`readelf`、`addr2line`

APP 典型编译参数：

```text
-mcpu=cortex-r4 -mthumb -mthumb-interwork -mlittle-endian
-mfloat-abi=soft -std=c99 -g -Os
```

## 3. 放置产品代码

把本仓库的 `venus/` 放到 SDK 的：

```text
ql-sdk/ql-application/threadx/venus
```

## 4. 修改 SDK 构建文件

### 4.1 父级 Makefile

编辑 `ql-sdk/ql-application/threadx/Makefile`，把 `venus` 加入编译目录：

```makefile
COMMPILE_DIRS:= \
  venus \
  venus/third_party/lvgl \
  venus/third_party/vn_opus \
  interface/mbedtls \
  interface/log/src \
  interface/cjson
```

同时确认没有临时的 `--wrap=free`：

```makefile
# 如果有这一行，请删除
# U_LDFLAGS:=--wrap=free
```

### 4.2 链接脚本

把 `integration/app_linkscript.ld` 复制到：

```text
ql-sdk/ql-application/threadx/config/common/app_linkscript.ld
```

该脚本使用以下布局：

```text
ROM 起始地址 0x80512000，大小 0x1c2000
RAM 起始地址 0x7e420000，大小 0x100000
```

## 5. 工程配置

确认：

```text
ql-config/config/QuecCurPrj.txt: EC800MCN_LE
ql-config/config/APP_MODE.ini:   XIP
```

不要运行会重新裁剪项目配置、覆盖当前库和镜像的入口脚本（例如 `OpenEntry.bat`）。

## 6. 编译

SDK 路径较长时，建议映射为虚拟盘：

```powershell
subst Q: "<你的 SDK 路径>"
Set-Location Q:\
```

只编译应用：

```powershell
.\build.bat app clean
.\build.bat app
```

日常增量修改可以只执行 `.\build.bat app`，但发布前必须做一次 clean 全量构建。

编译内核、bootloader 和完整固件：

```powershell
.\build.bat kernel
.\build.bat bootloader
.\build.bat firmware
```

只修改 `venus` 应用时，通常不需要重新编译 kernel 和 bootloader。

## 7. 判断构建是否成功

不能只看 `.bat` 的返回码。必须同时检查：

1. 日志末尾是否出现 `GCC Compiling Finished Sucessfully`
2. 是否有 `error:`、未解析符号、ROM/RAM overflow
3. `app.elf`、`app.bin`、`app.map` 的修改时间是否为本次构建
4. `app.map` 中产品函数是否来自当前 `venus`

## 8. 编译产物

```text
ql-sdk/ql-application/threadx/build/
├── app.elf       # 带符号，用于 addr2line / objdump / map 核对
├── app.bin       # 经 crc_bin 处理的应用镜像
├── app.map       # 链接布局和符号来源
├── venus.lib
└── obj/          # 各模块目标文件
```

`app.bin` 会由 `crc_bin.exe` 添加校验信息。用于替换原包 `customer_app.bin` 的应是构建完成并经过该工具处理后的 `app.bin`，不是自行截取或另行转换的 ELF 段。

标准 `build.bat firmware` 输出在：

```text
target/<当前项目配置名>/<当前项目配置名>.zip
```

> 不同 SDK 版本的标准包名可能不同。制作烧录包前必须核对产品名、版本、分区文件和原始目标配置。

## 9. 常见编译问题

- **找不到 `armcc`**：确认 `ql-cross-tool/win32/owtoolchain` 已解压，或安装 DS-5 并设置 `ARMLMD_LICENSE_FILE`
- **路径过长**：使用 `subst` 映射虚拟盘
- **链接地址错误**：确认使用的是本仓库的 `app_linkscript.ld`
- **RAM/ROM overflow**：检查是否误加了调试代码或大资源
