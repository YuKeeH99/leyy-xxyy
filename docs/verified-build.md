# 构建环境与发货说明

## 1. 构建环境

| 项目 | 值 |
| --- | --- |
| 主控 | Quectel EC800MCN_LE |
| SDK | QuecOpen C SDK `LTE01R06A17_C_SDK_A` |
| 固件基线 | `EC800MCNNR01A01SV08_DTN` |
| 运行模式 | XIP |
| 构建主机 | Windows |
| 构建命令 | `build.bat app clean` + `build.bat app` |
| 构建结果 | `GCC Compiling Finished Sucessfully` |
| 应用镜像 | `app.bin`，约 1.18 MB |

A01SV08 的 286 个产品函数全部链接存在；恢复包已记录 QFlash `100% PASS`。

## 2. 源码与集成核对

- `venus/` 产品层完整，包含 app、bsp、component、xiaozhi、resource、third_party。
- 临时诊断代码已清理：`AT+NULLFREE`、`__wrap_free` / `__real_free`、`Vn_NullFreeCaller`。
- `VN_AT_CMD_COUNT` 已调整为 7。
- `integration/` 包含链接脚本和 `setup_venus` 脚本。
- 已确认没有真实 Wi-Fi 密码、`nvs_dump.bin` 和内部构建路径。
- 源码 zip 和固件校验值已生成。

## 3. 发货内容

买家资料包中提供：

- `venus/` 产品层源码和 `integration/` 集成脚本。
- `EC800MCNNR01A01SV08_DTN_recovered.zip`：推荐恢复包。
- `EC800MCNNR01A01SV08_DTN_wakeabort.zip`：唤醒中止修复包，按需使用。
- 原厂回滚包、QFlash、Quectel USB 驱动。
- `SHA256SUMS.txt` / `MD5SUMS.txt`：固件校验值。

## 4. 复现构建

1. 准备 Windows 电脑和匹配版本的 EC800M QuecOpen SDK。
2. 把 `venus/` 放到 `ql-sdk/ql-application/threadx/venus`。
3. 按 `integration/README.md` 修改父级 Makefile 和链接脚本。
4. 在 SDK 根目录执行：

   ```bat
   build.bat app clean
   build.bat app
   ```

5. 检查 `ql-application/threadx/build/` 下的 `app.elf`、`app.bin`、`app.map`。

## 5. 卖家已确认

- 本页与仓库当前版本一致，固件基线为 `A01SV08`。
- 产品层源码、集成脚本、文档、恢复包、原厂回滚包、QFlash 和 USB 驱动已随买家资料包核对。
- 买家按本页或在线路书操作即可；遇到环境问题，把报错和操作步骤交给 AI 编程工具按本文命令排查。
