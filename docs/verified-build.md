# 已验证环境与构建记录

## 1. 最近一次成功构建

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

恢复记录中，A01SV08 的 286 个产品函数全部链接存在；恢复包已记录 QFlash `100% PASS`。

## 2. 2026-11-11 静态核对

在整理发布仓库时做了以下核对：

- `venus/` 产品层完整复制，包含 app、bsp、component、xiaozhi、resource、third_party。
- 移除临时诊断代码：
  - `AT+NULLFREE`
  - `__wrap_free` / `__real_free`
  - `Vn_NullFreeCaller`
- `VN_AT_CMD_COUNT` 从 8 改为 7。
- 添加 `integration/` 链接脚本和 `setup_venus` 脚本。
- 扫描确认没有真实 Wi-Fi 密码、`nvs_dump.bin`、内部构建路径。
- 生成源码 zip 和固件校验值。

## 3. 尚未验证

- 没有在本仓库基础上，用一套全新的 SDK 做一次干净编译。
- 没有对 `wakeabort` 修复包做最终烧录和长期稳定性验证。
- 没有做完整功能、OTA/FOTA 和长期联网测试。
- FGM840R Wi-Fi 模组的完整固件源码尚未恢复；当前只恢复 EC800M 侧的 UART AT 控制逻辑。

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

## 5. 建议

正式发给买家前，建议：

1. 用一套干净的 SDK 完整编译一次。
2. 用 QFlash 烧录 `recovered.zip`，确认 100% PASS。
3. 测试语音、联网、OTA、雷达、显示和音频。
4. 如需 `wakeabort.zip`，先做同样的完整验证，再作为量产包。
5. 把结果补充到本文件。
