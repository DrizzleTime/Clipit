# Clipit 架构

## 目标

Clipit 的核心流程是获取屏幕画面、选择区域、应用标注、保存图片和写入剪贴板。平台差异主要存在于屏幕画面的获取方式，因此架构只对抓屏后端和可独立测试的图片处理建立边界。

## 模块关系

```text
QML 界面
   │
   ▼
ScreenshotService（流程与状态）
   ├── ScreenCaptureBackend
   │   ├── PortalScreenCaptureBackend
   │   └── QtScreenCaptureBackend
   ├── AnnotationRenderer
   ├── ScreenshotStorage
   └── ClipboardService
```

### `ScreenshotService`

`ScreenshotService` 是 QML 与 C++ 的边界，负责：

- 接收区域、全屏、延时和取消命令；
- 驱动抓屏后端；
- 保存待编辑图片并通知 QML 打开选区窗口；
- 将标注数据交给解析器和渲染器；
- 组织保存与剪贴板操作；
- 发出完成、取消和失败信号。

它不包含 D-Bus 协议实现、标注绘制算法或文件覆盖逻辑。

截图状态只有以下几种：

```text
Idle → Delaying → Capturing → Selecting → Saving → Idle
                     │                         │
                     └──── Fullscreen ─────────┘
```

取消或失败会清理临时选区图片并回到 `Idle`。

### 抓屏后端

`ScreenCaptureBackend` 只负责产生一张完整的原始屏幕图片：

- `PortalScreenCaptureBackend` 实现 XDG Desktop Portal 的异步请求、取消、超时和服务可用性监听；
- `QtScreenCaptureBackend` 使用 `QScreen::grabWindow()`，用于 X11、Windows 和 macOS。

区域选择不是抓屏后端的职责。所有后端返回原始画面后，都复用同一个选区和标注流程。

### 标注模型与渲染

QML 将编辑数据作为 `QVariantList` 传入。`parseAnnotations()` 是唯一解析入口，它将动态数据校验并转换成强类型 `Annotation`。未知标注类型和无效坐标会直接返回错误，不会被静默忽略。

`AnnotationRenderer` 是无界面的纯图片处理模块，负责：

- 视图坐标到原图坐标的换算；
- 选区裁剪；
- 矩形、箭头、画笔和文字渲染；
- 模糊和马赛克处理；
- 屏幕像素取色。

QML Canvas 只提供编辑预览，最终 PNG 以 `AnnotationRenderer` 的结果为准。

### 文件与剪贴板

`ScreenshotStorage` 负责保存目录、文件名、临时选区图片和另存副本。正式图片和副本通过 `QSaveFile` 提交，避免覆盖失败时破坏已有文件。

`ClipboardService` 封装 Qt 剪贴板。Wayland 命令行模式退出后需要继续持有剪贴板，因此额外调用 `wl-copy`。

## 新增平台后端

新增抓屏方式时：

1. 实现 `ScreenCaptureBackend` 的 `available()`、`capture()` 和 `cancel()`。
2. 正确发出 `captured`、`canceled` 或 `failed` 终态信号。
3. 在组合入口中按平台选择后端。
4. 使用假后端测试控制器状态，不要求 CI 连接真实桌面服务。

后端不得负责选区、标注、保存或剪贴板。

## 测试边界

- `tst_annotationrenderer` 验证数据解析、坐标换算和图片输出。
- `tst_screenshotstorage` 验证 PNG 保存与原子覆盖。
- `tst_screenshotservice` 使用假抓屏后端验证状态和终态信号。
- `tst_applicationoptions` 验证命令行契约。

真实 GNOME Wayland、KDE Wayland、X11、Windows 和 macOS 的截图行为仍需要平台集成测试。CI 的跨平台构建只能证明代码可编译和纯逻辑测试通过，不能代替真实桌面验证。
