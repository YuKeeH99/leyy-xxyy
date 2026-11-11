# SDK 集成说明

本目录放的是把 `venus/` 放回移远 SDK 所需的两个小文件。

## 1. 复制文件

把仓库里的 `venus/` 复制到：

```text
<SDK>/ql-application/threadx/venus
```

把 `integration/app_linkscript.ld` 复制到：

```text
<SDK>/ql-application/threadx/config/common/app_linkscript.ld
```

也可以直接运行：

```bat
integration\setup_venus.bat <SDK 路径>
```

或：

```bash
./integration/setup_venus.sh <SDK 路径>
```

## 2. 修改父级 Makefile

编辑 `<SDK>/ql-application/threadx/Makefile`，确认 `COMMPILE_DIRS` 包含：

```makefile
COMMPILE_DIRS:= \
  venus \
  venus/third_party/lvgl \
  venus/third_party/vn_opus \
  interface/mbedtls \
  interface/log/src \
  interface/cjson
```

## 3. 移除临时链接参数

如果父级 Makefile 里有：

```makefile
U_LDFLAGS:=--wrap=free
```

请删除这一行。这个参数只用于定位空指针释放问题，正式版本不需要。

## 4. 链接脚本

`app_linkscript.ld` 的布局：

```text
ROM 起始地址 0x80512000，大小 0x1c2000
RAM 起始地址 0x7e420000，大小 0x100000
```

如果换硬件或改分区，必须同步修改这个文件，并确认不会 ROM/RAM overflow。
