# MLLog

[![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/Version-2.3-blue.svg)](https://github.com/mixml/MLLog)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

**English | [中文](./README.md)**

`MLLog` is a powerful, lightweight, thread-safe, header-only C++ logging library. It is designed for easy integration and use, while providing a rich set of features to meet various logging needs, from simple command-line tools to complex server applications.

## Core Features

*   🚀 **Header-Only**: Just include `mllog.hpp` in your project. No compilation or linking required.
*   🔒 **Thread-Safe**: All logging operations are protected by a mutex, making it safe to use in multi-threaded environments.
*   ✨ **Dual API Styles**:
    *   **Stream API**: `MLLOG_INFO << "User " << user_id << " logged in.";`
    *   **printf-style API**: `MLLOG_ERRORF("Failed with code %d", error_code);`
*   🌈 **Console Colors**: Supports colored output in the console to easily distinguish between log levels.
*   📄 **Flexible Output**: Log to a file, the console, or both simultaneously.
*   🔄 **Log Rotation**:
    *   **By Size**: Automatically creates a new log file when the current one reaches a specified size.
    *   **By File Count**: Keeps a specified number of log files, automatically overwriting the oldest one.
*   📅 **Daily Logs**: Automatically creates a new log file named with the current date when the day changes.
*   🧹 **Auto Cleanup**: Automatically cleans up old log files based on a specified number of days to keep.
*   ⚙️ **Highly Configurable**: Customize log levels, file paths, rotation policies, output formats, and more.
*   ⚡ **Performance-Optimized**: Includes features like time string caching and optional auto-flushing to boost performance.
*   🛡️ **Robust Error Handling**: Set a custom error handler to prevent internal library issues from affecting your main application.

## Quick Start

### 1. Prerequisites

*   A C++11 compliant compiler (e.g., g++, clang, MSVC).

### 2. Installation

`MLLog` is a header-only library, so integration is trivial:

1.  Download the `mllog.hpp` file.
2.  Copy it into your project's source directory.
3.  `#include "mllog.hpp"` in your code.

## How to Use

### Basic Usage

```cpp
#include "mllog.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    // 1. Start the logging system (required)
    MLLOG_START();

    // 2. (Optional) Configure the logger
    auto& logger = ML_Logger::getInstance();
    logger.setLogFile("./logs/my_app", 5, 10 * 1024 * 1024); // Path: ./logs/my_app, 5 rolling files, 10MB max size each
    logger.setLevel(ML_Logger::DEBUG);                       // Set level to DEBUG, all levels will be logged
    logger.setOutput(true, true);                            // Output to both file and console
    logger.setScreenColor(true);                             // Enable console colors

    // 3. Log messages using stream-style macros
    MLLOG_INFO << "Application started, version: " << 1.0;
    std::string user = "malin";
    MLLOG_DEBUG << "Processing request for user " << user << "...";
    std::vector<int> data = {1, 2, 3};
    MLLOG_WARNING << "Abnormal data detected, size: " << data.size();

    // 4. Log messages using printf-style macros
    MLLOG_ERRORF("Processing failed, error code: %d, message: %s", 1001, "File not found");
    MLLOG_CRITICALF("Critical component '%s' is not responding!", "DatabaseConnector");

    // 5. (Optional) Log an application stop message
    MLLOG_ALERT << "---------- Stop MLLOG ----------";

    return 0;
}
```

### Advanced Usage

```cpp
#include "mllog.hpp"
#include <iostream>

// Custom handler for MLLog's internal errors
void handle_log_system_error(const std::string& error_message) {
    // In a production environment, you might send an alert or write to an emergency log
    std::cerr << "!!! MLLog internal error: " << error_message << std::endl;
}

int main() {
    auto& logger = ML_Logger::getInstance();

    // 1. (Recommended) Set the error handler before starting
    logger.setErrorHandler(handle_log_system_error);

    // 2. Start the logging system
    MLLOG_START();

    // 3. More advanced configurations
    logger.setLogFile("./logs/perf_test");
    logger.setLevel(ML_Logger::INFO);
    logger.setAutoFlush(false); // Disable auto-flushing for high-frequency logging performance
    logger.setCheckDay(true);   // Enable auto-creation of new log files when the date changes

    // 4. Performance test (high-frequency writing)
    for (int i = 0; i < 1000; ++i) {
        MLLOG_INFO << "Processing item " << i;
    }

    // 5. Manually flush the buffer to ensure all logs are written to disk
    logger.flush();
    MLLOG_INFO << "All performance test logs have been flushed to disk.";

    // 6. Clean up log files older than 7 days
    logger.cleanupOldLogs(7);
    MLLOG_INFO << "Old logs cleanup complete.";

    return 0;
}
```

## API Reference (Main Configuration Functions)

After getting the singleton instance via `ML_Logger::getInstance()`, you can call the following functions to configure it:

| Function | Description |
| --- | --- |
| `setLogSwitch(bool)` | **(Required)** Global switch. `true` enables the logging system. The `MLLOG_START()` macro includes this. |
| `setLogFile(baseName, maxRolls, maxSize)` | Sets the log file base name, max number of rolling files, and max size per file. |
| `setLevel(Level)` | Sets the logging level. Messages below this level will be ignored. |
| `setOutput(toFile, toScreen)` | Sets the log output destination (file, console). |
| `setScreenColor(bool)` | Enables or disables colored console output. |
| `setCheckDay(bool)` | Enables or disables creating a new log file automatically when the day changes. |
| `setAutoFlush(bool)` | Sets whether to automatically flush the file buffer after each log message. Can be disabled for performance. |
| `setErrorHandler(handler)` | Sets a custom function to handle errors that occur within the logging library itself. |
| `cleanupOldLogs(daysToKeep)` | Cleans up old log files older than the specified number of days. |
| `flush()` | Manually flushes the log buffer contents to the file. |

## Performance Tip

For scenarios involving high-frequency logging (e.g., logging inside a tight loop), it is recommended to disable auto-flushing for the best performance and to call `flush()` manually at appropriate times.

```cpp
auto& logger = ML_Logger::getInstance();
logger.setAutoFlush(false); // Disable auto-flush

for (int i = 0; i < 100000; ++i) {
    MLLOG_DEBUG << "Looping fast... " << i;
}

logger.flush(); // Manually flush after the loop
```

## License

This project is licensed under the [MIT License](LICENSE).

## Author

*   **malin**
*   **Email**: `zcyxml@163.com` / `mlin2@grgbanking.com`
*   **GitHub**: <https://github.com/mixml>