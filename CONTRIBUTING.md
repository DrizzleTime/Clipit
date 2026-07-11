# 参与 Clipit 开发

## 开始之前

提交代码前请先搜索现有 Issue 和 Pull Request。修复明确的问题可以直接提交；新增较大的功能应先通过 Issue 明确使用场景、平台差异和实现边界，避免完成后才发现方向不一致。

开发环境需要：

- CMake 3.21 或更高版本
- Ninja
- 支持 C++17 的编译器
- Qt 6.8 或更高版本，包含 Qt Quick 和 Qt Test
- Linux 还需要 Qt DBus

## 构建与测试

如果 CMake 可以自动找到 Qt，可以使用项目预设：

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
cmake --build build/debug --target appClipit_qmllint
```

如果 Qt 安装在自定义目录，在配置时传入路径：

```bash
cmake --preset debug -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

如果 `ccache` 目录不可写，在命令前设置 `CCACHE_DISABLE=1`。

## 代码边界

- `src/core/` 不依赖桌面协议，放置标注模型、图片处理和文件保存等可独立测试的逻辑。
- `src/platform/` 封装不同平台的抓屏和剪贴板实现。
- `ScreenshotService` 只编排截图流程并向 QML 暴露状态。
- `components/` 放置有独立状态或行为的 QML 组件。
- `tests/` 中的测试应能在无真实屏幕和无 portal 的环境运行。

新增平台抓屏实现时，请实现 `ScreenCaptureBackend`，不要在 `ScreenshotService` 中增加新的平台条件分支。新增标注类型时，需要同时更新标注解析、最终渲染、QML 预览和对应测试。

详细关系见 [架构说明](docs/architecture.md)。

## 代码风格

C++ 使用仓库中的 `.clang-format`。只格式化本次修改涉及的文件，避免在功能 PR 中混入全仓库格式变更。

QML 保持四空格缩进。组件只有在拥有独立职责、状态或复用价值时才拆分，不为单个静态控件创建文件。

用户可见文本使用 `tr()` 或 `qsTr()`。错误消息应说明实际失败原因，不要只返回“操作失败”。

## Pull Request 要求

- 一个 PR 只解决一个问题。
- 说明问题、实现方式和受影响的平台。
- 行为变化需要测试；无法自动测试时，写明手动验证环境和步骤。
- 提交前确保构建、`ctest` 和 `appClipit_qmllint` 通过。
- 不提交构建目录、IDE 用户配置或本机 Qt 路径。

除非另有明确说明，向本项目提交的代码将按项目的 `GPL-3.0-or-later` 许可证发布。提交者必须有权按该许可证提供相关代码。
