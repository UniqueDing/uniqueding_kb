# Charybdis：Kanata 与 Linux 原生 Colemak

此分支基于 HeeTuic `36_charybdis_left_trackball`（`af73431dfe32f036ad51f24dd93eacb3df8369b5`），保留右侧 central、PMW3610 与 Studio 硬件设置。键位依据用户当前 Kanata 配置迁移。Unicode 依赖固定为 `urob/zmk-unicode` v0.3 对应提交 `3e8ca744e5c3b6f430b3713f3e45b806ce89be7b`；ZMK v0.3 和 PMW3610 driver 引用不变。

## 图层与两种基底模式

固件共 10 个有序图层：KANATA=0、LINUX=1、SYM=2、NUM=3、NAV_LINUX=4、FN=5、MSE=6、SCR=7、EMO_LINUX=8、LAT=9。开机默认 KANATA；固件不自动检测主机操作系统，不持久化模式选择。

- **KANATA** 是普通 QWERTY，唯一的基底双功能键是右上反斜杠：轻点 `\`，按住进入 FN。Tab、Caps、Shift、引号、空格和实际 RAlt 都是普通键。
- **LINUX** 使用标准 Colemak（不是 Colemak-DH），保留 Caps→Esc/Ctrl、Tab/NAV、分号/EMO、引号/SYM、斜杠/LAT、反斜杠/FN 双功能行为。
- 在 FN 层，右手上排按物理左至右：第 1 键返回 KANATA，第 2 键留空（此前的 macOS 模式入口已移除），第 3 键切换到 LINUX，第 4 键 momentary 进入 MSE。MSE 中按住中键进入 SCR；退出时先释放中键，再释放 FN 中的 MSE 键。

## 拇指键

左右手物理分组不互换；仅将每只手内部从左至右的顺序反转。

| 基底模式 | 左手：物理左至右 | 右手：物理左至右 |
| --- | --- | --- |
| KANATA | Space、LGUI（Command/Super）、LAlt（macOS Option） | 实际 RAlt、Enter |
| LINUX | Space（按住 MSE）、LGUI（Command/Super）、LAlt（macOS Option） | Backspace（按住 NUM）、Enter |

FN 层的 Studio 解锁键随 LAlt 移至左手第三个拇指位；其余功能层拇指透明。KANATA 的右手左侧拇指输出真实 RAlt；LINUX 模式的对应位置轻点 Backspace、按住 NUM。右手右侧拇指在两种模式下均为 Enter。相关 tap-hold 与 hold-tap 时限维持 200 ms；ZMK 的重按、打断边界可能和 Kanata 不完全一致，必要时可按手感调整。

## 平台输入

固件现有原生模式是 LINUX。NAV_LINUX 截图键为 PrintScreen；SYM/LAT 及 EMO_LINUX 通过 Linux Unicode 行为发送字符。Linux 主机需启用可用的 Fcitx5 Unicode 插件，通常以 Ctrl+Shift+U 输入码点并按空格确认；具体应用兼容性取决于桌面输入法与应用。

如果主机运行 Kanata，只应让它对键盘服务于 KANATA 模式；切换到 LINUX 固件布局时，须绕过/停用对该设备的主机端重复改键。固件不会更改主机设置。macOS 可由主机端 Kanata 配合 KANATA 键位使用；本固件没有独立的 macOS 模式，也不依赖 Unicode Hex Input。Windows 原生 Unicode 路径不在此配置的支持或验证范围内。

## 验证状态与实机测试

实现期间进行了本地静态键位、hold-tap、Unicode、层切换和指针层配置核对；静态检查不等于固件构建或运行验证。当前没有可用的 west/Zephyr SDK，固件尚未构建，键盘和主机运行时行为也未实测。

刷写后建议验证两种模式间的转换、切换后释放 FN、FN→MSE→SCR 的进入/释放顺序、模式切换后再输出 Unicode、PrintScreen、LAT 大小写、emoji 和 Linux 输入法配置。当前没有固件体积或构建成功的证据。
