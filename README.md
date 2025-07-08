# MLLog

[![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/Version-2.2-blue.svg)](https://github.com/mixml/MLLog)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

**[English](./README_EN.md) | 中文**

`MLLog` 是一个功能强大、轻量级、线程安全的 C++ 单头文件日志库。它被设计为易于集成和使用，同时提供了丰富的功能，以满足从简单命令行工具到复杂服务器应用程序的各种日志记录需求。

## 核心特性

*   🚀 **单头文件**: 只需将 `mllog.hpp` 包含到你的项目中即可，无需编译或链接。
*   🔒 **线程安全**: 所有日志操作都由互斥锁保护，可在多线程环境中安全使用。
*   ✨ **两种API风格**:
    *   **流式API**: `MLLOG_INFO << "User " << user_id << " logged in.";`
    *   **printf风格API**: `MLLOG_ERRORF("Failed with code %d", error_code);`
*   🌈 **控制台颜色**: 支持在控制台输出带颜色的日志，便于区分不同级别的日志。
*   📄 **灵活的输出**: 可同时将日志输出到文件和控制台，或只输出到其中之一。
*   🔄 **日志滚动**:
    *   **按大小滚动**: 当日志文件达到设定大小时，自动创建新文件。
    *   **按文件数滚动**: 保留指定数量的日志文件，自动覆盖最旧的文件。
*   📅 **每日日志**: 支持跨天时自动创建以当天日期命名的新日志文件。
*   🧹 **自动清理**: 可根据设定的天数自动清理旧的日志文件。
*   ⚙️ **高度可配置**: 可自定义日志级别、文件路径、滚动策略、输出格式等。
*   ⚡ **性能优化**: 包含时间字符串缓存、可选的自动刷新等机制以提升性能。
*   🛡️ **健壮的错误处理**: 可设置自定义错误处理器，避免日志库内部问题影响主程序。

## 快速开始

### 1. 依赖

*   C++11 或更高版本的编译器 (例如 g++, clang, MSVC)。

### 2. 安装

`MLLog` 是一个纯头文件库，集成非常简单：

1.  下载 `mllog.hpp` 文件。
2.  将其复制到你的项目源码目录中。
3.  在你的代码中 `#include "mllog.hpp"`。

## 使用方法

### 基础用法

```cpp
#include "mllog.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // 1. 启动日志系统 (必须)
    MLLOG_START();

    // 2. (可选) 配置日志
    auto& logger = ML_Logger::getInstance();
    logger.setLogFile("./logs/my_app", 5, 10 * 1024 * 1024); // 日志路径: ./logs/my_app, 保留5个滚动文件, 每个最大10MB
    logger.setLevel(ML_Logger::DEBUG);                       // 设置日志级别为DEBUG，所有级别都将输出
    logger.setOutput(true, true);                            // 同时输出到文件和控制台
    logger.setScreenColor(true);                             // 开启控制台颜色

    // 3. 使用流式宏记录日志
    MLLOG_INFO << "程序启动，版本: " << 1.0;
    std::string user = "malin";
    MLLOG_DEBUG << "正在为用户 " << user << " 处理请求...";
    std::vector<int> data = {1, 2, 3};
    MLLOG_WARNING << "检测到数据异常，大小: " << data.size();

    // 4. 使用格式化宏记录日志 (printf-style)
    MLLOG_ERRORF("处理失败，错误码: %d, 错误信息: %s", 1001, "文件未找到");
    MLLOG_CRITICALF("系统关键组件'%s'无响应!", "DatabaseConnector");

    // 5. (可选) 记录一条程序结束日志
    MLLOG_ALERT << "---------- Stop MLLOG ----------";

    return 0;
}
```

### 进阶用法

```cpp
#include "mllog.hpp"
#include <iostream>

// 自定义日志库内部错误处理器
void handle_log_system_error(const std::string& error_message) {
    // 在生产环境中，这里可以发送警报或写入紧急日志
    std::cerr << "!!! MLLog internal error: " << error_message << std::endl;
}

int main() {
    auto& logger = ML_Logger::getInstance();

    // 1. (推荐) 在启动前设置错误处理器
    logger.setErrorHandler(handle_log_system_error);

    // 2. 启动日志系统
    MLLOG_START();

    // 3. 更多高级配置
    logger.setLogFile("./logs/perf_test");
    logger.setLevel(ML_Logger::INFO);
    logger.setAutoFlush(false); // 关闭自动刷新以提升高频写入性能
    logger.setCheckDay(true);   // 开启跨天自动创建新日期日志文件功能

    // 4. 性能测试 (高频写入)
    for (int i = 0; i < 1000; ++i) {
        MLLOG_INFO << "Processing item " << i;
    }

    // 5. 手动刷新缓冲区，确保所有日志都已写入磁盘
    logger.flush();
    MLLOG_INFO << "所有性能测试日志已刷新到磁盘。";

    // 6. 清理7天前的旧日志文件
    logger.cleanupOldLogs(7);
    MLLOG_INFO << "旧日志清理完成。";

    return 0;
}
```

## API 参考 (主要配置函数)

通过 `ML_Logger::getInstance()` 获取单例对象后，可以调用以下函数进行配置：

| 函数 | 描述 |
| --- | --- |
| `setLogSwitch(bool)` | **(必需)** 全局开关，`true` 启用日志系统。`MLLOG_START()` 宏已包含此调用。 |
| `setLogFile(baseName, maxRolls, maxSize)` | 设置日志文件基础名、最大滚动文件数和单个文件最大尺寸。 |
| `setLevel(Level)` | 设置日志级别，低于此级别的日志将不会被记录。 |
| `setOutput(toFile, toScreen)` | 设置日志输出目标 (文件、控制台)。 |
| `setScreenColor(bool)` | 启用或禁用控制台颜色输出。 |
| `setCheckDay(bool)` | 启用或禁用跨天自动创建新日志文件。 |
| `setAutoFlush(bool)` | 设置是否在每条日志后自动刷新文件缓冲区。为性能考虑可关闭。 |
| `setErrorHandler(handler)` | 设置一个自定义函数来处理日志库内部发生的错误。 |
| `cleanupOldLogs(daysToKeep)` | 清理指定天数之前的旧日志文件。 |
| `flush()` | 手动将日志缓冲区内容刷新到文件。 |

## 性能提示

对于需要高频写入日志的场景 (例如循环中大量记录)，建议关闭自动刷新以获得最佳性能，并在适当的时机手动调用 `flush()`。

```cpp
auto& logger = ML_Logger::getInstance();
logger.setAutoFlush(false); // 关闭自动刷新

for (int i = 0; i < 100000; ++i) {
    MLLOG_DEBUG << "Looping fast... " << i;
}

logger.flush(); // 在循环结束后手动刷新
```

## 许可证

本项目使用 [MIT 许可证](LICENSE)。

## 作者

*   **malin**
*   **Email**: `zcyxml@163.com` / `mlin2@grgbanking.com`
*   **GitHub**: <https://github.com/mixml>