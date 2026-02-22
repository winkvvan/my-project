#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {

std::string currentTimestamp() {
    using std::chrono::system_clock;
    const auto now = system_clock::now();
    const std::time_t nowTime = system_clock::to_time_t(now);

    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    return std::string(buffer);
}

std::optional<std::string> runCommand(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }

    std::string output;
    char chunk[256];
    while (fgets(chunk, sizeof(chunk), pipe) != nullptr) {
        output += chunk;
    }

    const int status = pclose(pipe);
    if (status != 0) {
        return std::nullopt;
    }

    return output;
}

#ifndef _WIN32
std::optional<std::string> readClipboardLinux() {
    // Wayland first.
    if (const auto value = runCommand("wl-paste --no-newline 2>/dev/null"); value && !value->empty()) {
        return value;
    }

    // Then X11 helpers.
    if (const auto value = runCommand("xclip -selection clipboard -o 2>/dev/null"); value && !value->empty()) {
        return value;
    }

    if (const auto value = runCommand("xsel --clipboard --output 2>/dev/null"); value && !value->empty()) {
        return value;
    }

    return std::nullopt;
}
#else
std::optional<std::string> readClipboardWindows() {
    if (!OpenClipboard(nullptr)) {
        return std::nullopt;
    }

    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (!handle) {
        CloseClipboard();
        return std::nullopt;
    }

    LPCWSTR text = static_cast<LPCWSTR>(GlobalLock(handle));
    if (!text) {
        CloseClipboard();
        return std::nullopt;
    }

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        GlobalUnlock(handle);
        CloseClipboard();
        return std::nullopt;
    }

    std::string utf8(sizeNeeded - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data(), sizeNeeded, nullptr, nullptr);

    GlobalUnlock(handle);
    CloseClipboard();

    return utf8;
}
#endif

void appendLog(const std::filesystem::path& filePath, const std::string& text) {
    std::ofstream out(filePath, std::ios::app);
    if (!out) {
        throw std::runtime_error("无法打开日志文件: " + filePath.string());
    }

    out << "[" << currentTimestamp() << "]\n";
    out << text << "\n";
    out << "----------------------------------------\n";
}

} // namespace

int main(int argc, char* argv[]) {
    std::filesystem::path output = "clipboard_log.txt";
    int pollMs = 500;

    if (argc >= 2) {
        output = argv[1];
    }

    if (argc >= 3) {
        pollMs = std::max(100, std::atoi(argv[2]));
    }

    std::cout << "剪贴板自动记录器已启动。\n";
    std::cout << "日志文件: " << output << "\n";
    std::cout << "轮询间隔: " << pollMs << "ms\n";
    std::cout << "按 Ctrl+C 停止。\n\n";

    std::optional<std::string> lastText;

    while (true) {
#ifdef _WIN32
        const auto current = readClipboardWindows();
#else
        const auto current = readClipboardLinux();
#endif

        if (current && (!lastText || *current != *lastText)) {
            try {
                appendLog(output, *current);
                std::cout << "已记录一条新内容，时间: " << currentTimestamp() << "\n";
            } catch (const std::exception& ex) {
                std::cerr << "写入失败: " << ex.what() << "\n";
            }

            lastText = current;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(pollMs));
    }

    return 0;
}
