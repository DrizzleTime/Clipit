# Clipit

一个以 **Linux Wayland** 为主要使用场景，同时支持 Linux X11、Windows 和 macOS 的轻量截图与标注工具。

Clipit 在 Wayland 下通过标准的 [XDG Desktop Portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Screenshot.html) 获取画面，再在应用内完成区域选择、标注、保存和复制。Wayland 路径不依赖 X11 抓屏，也不绑定 GNOME Shell 私有接口。

![Clipit 程序截图](https://img.cdn1.vip/i/6a52698f6b18b_1783785871.webp)

> 项目目前处于早期开发阶段，核心截图与标注流程已可用，但安装包、多显示器联合框选等功能仍待完善。

## 为什么做 Clipit

Wayland 默认禁止普通应用直接读取其他窗口或桌面像素。这是正确的安全设计，但也改变了截图工具的工作方式：

- 依赖 `QScreen::grabWindow()`、X11 API 或全局按键监听的传统工具，在 Wayland 下可能无法抓屏；
- 依赖特定合成器接口的工具，通常无法同时适配 GNOME、KDE 等桌面环境；
- GNOME 自带截图功能可以完成基础截图，但标注、模糊、马赛克等后续编辑能力有限；
- “先用系统截图，再打开图片编辑器标注”的流程步骤多，也容易打断当前工作。

Clipit 要解决的是这段缺失的工作流：**在遵守 Wayland 安全模型的前提下，把截图、框选和标注放回同一个流程中。**

## 功能

- 区域截图、窗口截图、全屏截图和延时截图
- 矩形、箭头、自由画笔和单行文字
- 模糊与马赛克隐私处理
- 预设色板与屏幕像素取色
- 撤销标注、重新框选
- 自动保存 PNG 并复制到系统剪贴板
- 最近一次截图预览和另存副本
- 可配置默认保存目录
- 深色与浅色界面
- 命令行截图，可绑定桌面环境的全局快捷键

默认保存目录是系统图片目录下的 `Clipit`。

## 平台支持

| 平台 | 截图方式 | 当前状态 |
| --- | --- | --- |
| GNOME / KDE Wayland | XDG Desktop Portal | 主要支持目标 |
| Linux X11 | X11 活动窗口查询 + Qt `QScreen::grabWindow()` | 可用 |
| Windows | Win32 活动窗口查询 + Qt `QScreen::grabWindow()` | 基础支持 |
| macOS | Qt + Core Graphics | 基础支持，需要录屏权限 |

Wayland 下，portal 负责向 Clipit 交付截图。区域截图仍先获取完整屏幕，再由 Clipit 完成框选和标注。窗口截图优先请求 portal v3 的 `Active Window` 目标；如果系统只提供窗口选择或旧版交互接口，会显示系统选择界面。首次使用或受沙箱策略影响时，系统也可能显示授权界面；这是 Wayland 安全模型的一部分，应用无法绕过。

### 跨平台设计

Clipit 使用 C++、Qt 6 和 QML 开发。主界面、区域选择、标注、PNG 保存与剪贴板等核心逻辑由各平台共用，平台差异集中在“如何安全地取得屏幕画面”这一层：

- **Linux Wayland**：通过标准 XDG Desktop Portal 请求屏幕或窗口画面，兼容采用 portal 的 GNOME、KDE 等桌面环境；
- **Linux X11**：通过 X11 查询活动窗口，再由 Qt 获取窗口画面；
- **Windows**：通过 Win32 查询活动窗口，再由 Qt 获取窗口画面；
- **macOS**：通过 Core Graphics 查询并获取最前方窗口画面，沿用系统录屏权限机制，用户需要在首次使用时授权。

这种方式不要求为每个平台重新实现整套编辑器，也避免把 Wayland 支持绑定到某个桌面环境的私有接口。相同的截图一旦交给 Clipit，后续框选、标注、保存和复制行为保持一致。

这里的“跨平台”表示项目具备统一代码和对应的抓屏路径，不代表当前版本在所有平台上的完成度完全相同。现阶段主要开发和验证环境是 GNOME Wayland；Windows、macOS 和 X11 已有基础实现，但仍需要更多实际设备测试，多显示器联合框选也尚未实现。

## 运行依赖

- Qt 6.8 或更高版本
- Qt Quick
- Linux：Qt DBus、`xdg-desktop-portal` 和当前桌面对应的 portal 后端
- Linux X11 窗口截图：构建时需要 X11 开发库；未找到时仍可构建，但不包含 X11 窗口截图
- Wayland 命令行模式：`wl-clipboard`，用于在 Clipit 退出后继续保留剪贴板内容

GNOME 通常使用 `xdg-desktop-portal-gnome`，KDE Plasma 通常使用 `xdg-desktop-portal-kde`。

## 从源码构建

项目使用 CMake，推荐使用 Ninja：

```bash
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build/release --parallel
./build/release/appClipit
```

如果 Qt 已安装到 CMake 能自动发现的位置，可以省略 `CMAKE_PREFIX_PATH`。如果本机 `ccache` 目录不可写，可以在构建命令前设置 `CCACHE_DISABLE=1`。

运行测试：

```bash
ctest --test-dir build/release --output-on-failure
```

## 命令行与全局快捷键

不带参数时启动完整界面：

```bash
./build/release/appClipit
```

也可以直接发起截图：

```bash
# 区域截图
./build/release/appClipit --region

# 窗口截图
./build/release/appClipit --window

# 全屏截图
./build/release/appClipit --fullscreen

# 延时 3 秒后全屏截图
./build/release/appClipit --fullscreen --delay 3
```

短参数分别为 `-r`、`-w`、`-f` 和 `-d`。`--region`、`--window` 与 `--fullscreen` 互斥，三种截图模式都可以与 `--delay` 组合，延时范围为 0–3600 秒。

`--window` 在 X11、Windows 和 macOS 下截取当前活动窗口。Wayland 下优先截取活动窗口；portal 不支持该目标时会打开系统界面，由用户选择窗口。

Clipit 暂不自行注册全局快捷键。可以在 GNOME 的“设置 → 键盘 → 查看及自定义快捷键 → 自定义快捷键”中绑定以下命令，路径需要替换为可执行文件的实际绝对路径：

```text
/home/user/apps/Clipit/appClipit --region
/home/user/apps/Clipit/appClipit --window
/home/user/apps/Clipit/appClipit --fullscreen
```

命令执行成功后，标准输出只打印最终 PNG 的绝对路径。退出码如下：

| 退出码 | 含义 |
| --- | --- |
| `0` | 截图并保存成功 |
| `1` | 抓屏、portal 调用或文件保存失败 |
| `2` | 命令参数错误 |
| `3` | 用户取消截图 |
| `4` | PNG 已保存，但持久化剪贴板失败 |

## 实现方式

Clipit 针对不同环境选择不同的抓屏后端，但复用同一套区域选择和标注流程：

1. `PortalScreenCaptureBackend` 在 Wayland 下通过 `org.freedesktop.portal.Screenshot` 请求屏幕或窗口画面；`QtScreenCaptureBackend` 在其他平台查询活动窗口并通过平台接口获取画面。
2. `CaptureOverlay.qml` 在静态画面上完成区域选择与标注。
3. `ScreenshotService` 管理截图状态，`AnnotationRenderer` 合成最终图片，`ScreenshotStorage` 以原子方式保存 PNG。

程序有四种启动模式：完整 GUI、仅区域截图、仅窗口截图和仅全屏截图。窗口与全屏命令模式不会加载 QML 界面，适合快捷键或脚本调用。

项目将可独立测试的图片逻辑放在 `src/core/`，将 portal、Qt 抓屏和剪贴板实现放在 `src/platform/`。详细模块边界和扩展方式见 [架构说明](docs/architecture.md)。提交代码前请阅读 [贡献指南](CONTRIBUTING.md)。

## 当前限制

- 非 Wayland 平台的全屏与区域模式目前只捕获主屏幕，尚不支持多显示器联合框选。
- 尚未接入 Wayland `GlobalShortcuts` portal，需要通过桌面设置绑定快捷键。
- 文字标注目前只支持单行输入。
- 模糊工具的编辑预览是视觉近似，最终 PNG 会执行实际模糊处理。
- 尚无 OCR、托盘和持久化截图历史。
- macOS 首次截图需要在系统设置中授予录屏权限。

## 参与开发

欢迎提交 Issue 报告可复现的问题。请同时提供桌面环境、显示协议（Wayland 或 X11）、Qt 版本，以及相关终端输出。

提交代码前请阅读 [贡献指南](CONTRIBUTING.md) 和 [架构说明](docs/architecture.md)。

## 许可证

Clipit 以 [GNU General Public License v3.0 or later](LICENSE) 发布。发布修改版或衍生作品时，需要遵守 GPL 的源代码公开和许可证保留要求。Qt 本身由 Qt 项目按其各自许可证提供，不属于 Clipit 仓库内容。
