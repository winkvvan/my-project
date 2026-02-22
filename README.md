# 剪贴板自动记录器（C++）

这是一个简单的 C++ 小工具，会持续轮询系统剪贴板，把新复制的文本自动追加到日志文件中。

## 功能
- 自动监控剪贴板文本变化
- 只记录“变化后的新内容”（避免重复刷屏）
- 每条记录带时间戳
- 可指定日志文件路径和轮询间隔
- 跨平台支持：
  - Windows：使用 Win32 Clipboard API
  - Linux：优先 `wl-paste`（Wayland），其次 `xclip` / `xsel`（X11）

## 1) 先编译
在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build
```

编译成功后会生成可执行文件：
- Linux/macOS: `build/clipboard_logger`
- Windows: `build/clipboard_logger.exe`

## 2) 再运行
### Linux/macOS
```bash
./build/clipboard_logger
```

### Windows（PowerShell 或 CMD）
```powershell
.\build\clipboard_logger.exe
```

## 3) 可选参数
```bash
./build/clipboard_logger [日志文件路径] [轮询间隔毫秒]
```

例如：
```bash
./build/clipboard_logger my_clipboard_log.txt 300
```

如果不传参数：
- 日志文件默认为 `clipboard_log.txt`
- 轮询间隔默认为 `500ms`（最小 100ms）

## 4) 停止程序
- 前台运行时按 `Ctrl+C`。

## Linux 依赖（很重要）
请确保至少安装一个命令：
- Wayland: `wl-paste`（通常来自 `wl-clipboard` 包）
- X11: `xclip` 或 `xsel`

可用下面命令检查：
```bash
which wl-paste || which xclip || which xsel
```

如果没有输出，请先安装其中任意一个，否则程序无法读取剪贴板文本。
