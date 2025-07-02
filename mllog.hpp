#ifndef MLLOG_HPP
#define MLLOG_HPP

/**
 * @file MLLog.hpp
 * @brief 多功能日志系统库，适用于C++应用程序。
 *
 * @author malin
 *      - Email: zcyxml@163.com  mlin2@grgbanking.com
 *      - GitHub: https://github.com/mixml
 * @version 2.2 (2025-06-17) - (Gemini 2.5 pro)
 *      - 功能: 增加了 getLogSwitch() 函数，用于查询日志系统是否已启用。
 * @version 2.1 (2025-06-17) - (Gemini 2.5 pro)
 *      - 健壮性: 增加了可配置的内部错误处理回调机制(setErrorHandler)，避免库直接写入std::cerr。
 *      - 代码风格: 为不修改对象状态的成员函数增加了const修饰符(getAddNewLine)。
 * @version 2.0 (2025-06-17) - (Gemini 2.5 pro)
 *      - 修复: 修复长日志无法正确输出的问题.
 *      - 优化: 对太长的日志进行截断.
 *      - 健壮性: 优化了 `cleanupOldLogs` 内部的日期解析函数，使其更加可靠。
 * @version 1.9 (2025-06-17 Gemini 2.5 pro)
 *      - 修复: 修复了长日志消息可能被截断的bug。
 *      - 优化: 优化了跨天检查逻辑、文件大小计算和目录创建函数。
 *      - 优化: 使用 `constexpr` 函数实现 `MLSHORT_FILE`。
 * @version 1.8 (2025-06-17) - (基于用户反馈的健壮性修复 Gemini 2.5 pro)
 *      - 修复: log() 函数中文件未打开时的低效回退逻辑，避免性能陷阱和状态污染。
 *      - 修复: 移除析构函数中的日志调用，以解决静态对象析构顺序问题，防止程序在退出时崩溃。
 *      - 清理: 移除内部调试用的 std::cout 输出。
 * @version 1.7 (2025年4月9日)
 *      - 增加日期判断, 跨天会新生成日志.
 * @version 1.6 (2025年4月5日)
 *      - 输出控制台颜色WIN平台的修改.
 * @version 1.5 (2025年3月18日)
 *      - 增加控制台颜色输出. ML_Logger::getInstance().setScreenColor(true)进行设置.默认打开颜色,打开输出到控制台.
 *      - 增加宏MLLOG_START()进行日志开启.
 * @version 1.4 (2025年2月17日)
 *      - 修复只输出到屏幕仍然生成文件的问题.
 * @version 1.3 (2024年10月18日)
 *      - 去掉setLogOff函数, 增加setLogSwitch开关函数. 默认是关闭,如果要启动日志,需要setLogSwitch(true).
 * @version 1.2 (2024年9月8日)
 *      - 去掉宏MLLOG_MESSAGE_ONLY,MLLOG_ADD_NEWLINE,变成可以设置,避免在不同项目中使用不同的hpp源码不好维护.
 *      - 增加setAddNewLine,getAddNewLine,setLogOff,setDefaultLogFormat设置默认是否增加换行符和日志默认名称.
 * @version 1.1 (2024年4月19日)
 *      - 增加两个宏MLLOG_MESSAGE_ONLY,MLLOG_ADD_NEWLINE
 * @version 1.0 (2024年4月16日)
 *      - 初始版本。
 *
 * 使用说明:
 *      - **初始化**: 在 `main` 函数开始时调用 `MLLOG_START();`。
 *      - **配置**: 调用 `ML_Logger::getInstance().setLogFile(...)` 等函数进行配置。
 *      - **自定义错误处理 (可选)**: 调用 `ML_Logger::getInstance().setErrorHandler(...)` 来处理库内部错误。
 *      - **清理旧日志 (可选)**: 调用 `ML_Logger::getInstance().cleanupOldLogs(天数);` 来删除过期日志。
 *      - **退出**: 在 `main` 结束前，建议记录一条关闭日志。
 *
 * @section usage 使用示例 (Usage)
 *
 * @subsection basic_usage 基础用法
 * @code
 * #include "MLLog.hpp"
 * #include <iostream>
 * #include <string>
 * #include <vector>
 *
 * int main() {
 *     // 1. 启动日志系统 (必须)
 *     MLLOG_START();
 *
 *     // 2. (可选) 配置日志
 *     auto& logger = ML_Logger::getInstance();
 *     logger.setLogFile("./logs/my_app", 5, 10 * 1024 * 1024); // 日志路径: ./logs/my_app, 保留5个滚动文件, 每个最大10MB
 *     logger.setLevel(ML_Logger::DEBUG);                       // 设置日志级别为DEBUG，所有级别都将输出
 *     logger.setOutput(true, true);                            // 同时输出到文件和控制台
 *     logger.setScreenColor(true);                             // 开启控制台颜色
 *
 *     // 3. 使用流式宏记录日志
 *     MLLOG_INFO << "程序启动，版本: " << 1.0;
 *     std::string user = "malin";
 *     MLLOG_DEBUG << "正在为用户 " << user << " 处理请求...";
 *     std::vector<int> data = {1, 2, 3};
 *     MLLOG_WARNING << "检测到数据异常，大小: " << data.size();
 *
 *     // 4. 使用格式化宏记录日志 (printf-style)
 *     MLLOG_ERRORF("处理失败，错误码: %d, 错误信息: %s", 1001, "文件未找到");
 *     MLLOG_CRITICALF("系统关键组件'%s'无响应!", "DatabaseConnector");
 *
 *     // 5. (可选) 记录一条程序结束日志
 *     MLLOG_ALERT << "---------- Stop MLLOG ----------";
 *
 *     return 0;
 * }
 * @endcode
 *
 * @subsection advanced_usage 进阶用法
 * @code
 * #include "MLLog.hpp"
 * #include <iostream>
 *
 * // 自定义日志库内部错误处理器
 * void handle_log_system_error(const std::string& error_message) {
 *     // 在生产环境中，这里可以发送警报或写入紧急日志
 *     std::cerr << "!!! MLLog internal error: " << error_message << std::endl;
 * }
 *
 * int main() {
 *     auto& logger = ML_Logger::getInstance();
 *
 *     // 1. (推荐) 在启动前设置错误处理器
 *     logger.setErrorHandler(handle_log_system_error);
 *
 *     // 2. 启动日志系统
 *     MLLOG_START();
 *
 *     // 3. 更多高级配置
 *     logger.setLogFile("./logs/perf_test");
 *     logger.setLevel(ML_Logger::INFO);
 *     logger.setAutoFlush(false); // 关闭自动刷新以提升高频写入性能
 *     logger.setCheckDay(true);   // 开启跨天自动创建新日期日志文件功能
 *
 *     // 4. 性能测试 (高频写入)
 *     for (int i = 0; i < 1000; ++i) {
 *         MLLOG_INFO << "Processing item " << i;
 *     }
 *
 *     // 5. 手动刷新缓冲区，确保所有日志都已写入磁盘
 *     logger.flush();
 *     MLLOG_INFO << "所有性能测试日志已刷新到磁盘。";
 *
 *     // 6. 清理7天前的旧日志文件
 *     logger.cleanupOldLogs(7);
 *     MLLOG_INFO << "旧日志清理完成。";
 *
 *     return 0;
 * }
 * @endcode
 */

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional> // <-- 【新增】为 std::function 增加头文件
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <direct.h>
#include <io.h>
#include <windows.h>

#undef min
#undef max
#undef ERROR
#pragma warning(disable : 4996)
#else
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif

/* Supported colors */
#define MLLOG_COLOR_NORMAL "\x1B[0m"
#define MLLOG_COLOR_RED "\x1B[31m"
#define MLLOG_COLOR_GREEN "\x1B[32m"
#define MLLOG_COLOR_YELLOW "\x1B[33m"
#define MLLOG_COLOR_BLUE "\x1B[34m"
#define MLLOG_COLOR_MAGENTA "\x1B[35m"
#define MLLOG_COLOR_CYAN "\x1B[36m"
#define MLLOG_COLOR_WHITE "\x1B[37m"
#define MLLOG_COLOR_RESET "\033[0m"
#define MLLOG_EMPTY ""

// C++11 compatible constexpr function to get short filename at compile time
constexpr const char *find_last_slash_helper(const char *s, const char *last_slash)
{
    return *s == '\0' ? last_slash : find_last_slash_helper(s + 1, (*s == '/' || *s == '\\') ? s + 1 : last_slash);
}

constexpr const char *file_name_from_path(const char *path)
{
    return find_last_slash_helper(path, path);
}

class ML_Logger
{
public:
    enum Level
    {
        DEBUG = 0,
        INFO,
        NOTICE,
        WARNING,
        ERROR,
        CRITICAL,
        ALERT
    };
    // 【新增】错误处理回调函数类型
    using ErrorHandler = std::function<void(const std::string &)>;

    static const size_t MAX_LOG_MESSAGE_SIZE = 1024 * 1024 * 5; // 最大单条5M进行截断
    static constexpr const char *TRUNCATED_MESSAGE = "\n... [Message Truncated]";
    static ML_Logger &getInstance()
    {
        static ML_Logger instance;
        return instance;
    }

    ML_Logger(const ML_Logger &) = delete;
    ML_Logger &operator=(const ML_Logger &) = delete;
    ML_Logger(ML_Logger &&) = delete;
    ML_Logger &operator=(ML_Logger &&) = delete;

    void setLevel(Level level)
    {
        _logLevel = level;
    }

    void setCheckDay(bool isCheck)
    {
        _isCheckDay = isCheck;
    }

    void setLogFile(const std::string &baseName, int maxRolls = 5, size_t maxSizeInBytes = 100 * 1024 * 1024)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _initialized = false;
        _isRoll = false;
        _baseName = baseName;
        _start_timestamp = getCurrentTimestamp();

        size_t lastSlashPos = _baseName.find_last_of("\\/");
        if (_outputToFile)
        {
            std::string directoryPath = (lastSlashPos != std::string::npos) ? _baseName.substr(0, lastSlashPos) : ".";
            create_directories(directoryPath);
        }

        _baseNameWithoutPath = (lastSlashPos != std::string::npos) ? _baseName.substr(lastSlashPos + 1) : _baseName;
        _baseFullNameWithDateAndTime = _baseName + "_" + _start_timestamp;

        _maxRolls = maxRolls > 0 ? maxRolls : 1;
        _maxSizeInBytes = maxSizeInBytes;
        _currentSize = 0;
        _currentRollIndex = 0;
        _last_log_day = 0;

        if (_file.is_open())
        {
            _file.close();
        }
    }

    void setOutput(bool toFile, bool toScreen)
    {
        _outputToFile = toFile;
        _outputToScreen = toScreen;
    }
    void setAddNewLine(bool isAddNewLine) { _add_newline = isAddNewLine; }
    // 【修改】增加了 const 修饰符
    bool getAddNewLine() const { return _add_newline; }
    void setLogSwitch(bool enabled) { _log_enabled = enabled; }
    // 【新增】返回日志系统是否启用的函数
    bool getLogSwitch() const { return _log_enabled; }
    void setDefaultLogFormat(bool isDefault) { _default_file_name_day = isDefault; }
    void setMessageOnly(bool isMessageOnly) { _message_only = isMessageOnly; }
    void setScreenColor(bool isColor) { _log_screen_color = isColor; }

    /**
     * @brief 【新增】设置日志库内部的错误处理器。
     * @param handler 一个函数对象，接收一个string作为错误信息。
     * @note 例如: ML_Logger::getInstance().setErrorHandler([](const std::string& err){ std::cerr << "My handler: " << err << std::endl; });
     *       如果不设置，默认行为是输出到 std::cerr。
     */
    void setErrorHandler(ErrorHandler handler)
    {
        _error_handler = std::move(handler);
    }

    void setAutoFlush(bool enabled)
    {
        _auto_flush = enabled;
    }

    void flush()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_file.is_open())
        {
            _file.flush();
        }
    }
    void log(const char *file, int line, Level level, const std::string &original_message, bool isNewLine = true)
    {
        if (!_log_enabled || level < _logLevel)
            return;

        std::string message = original_message;
        if (message.length() > MAX_LOG_MESSAGE_SIZE)
        {
            message.resize(MAX_LOG_MESSAGE_SIZE);
            message.append(TRUNCATED_MESSAGE);
        }

        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        auto time_t_now = std::chrono::system_clock::to_time_t(now);

        if (time_t_now != _cached_time_sec)
        {
            _cached_time_sec = time_t_now;
            std::tm local_tm;

#if defined(_WIN32)
            localtime_s(&local_tm, &time_t_now);
#else
            localtime_r(&time_t_now, &local_tm);
#endif

            if (_isCheckDay)
            {
                if (_last_log_day != 0 && _last_log_day != local_tm.tm_mday)
                {
                    std::lock_guard<std::mutex> lock(_mutex);
                    setLogFile(_baseName, _maxRolls, _maxSizeInBytes);
                }
                if (_last_log_day == 0)
                {
                    _last_log_day = local_tm.tm_mday;
                }
            }

            char time_buf[64];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &local_tm);
            _cached_time_str = time_buf;
        }

        std::vector<char> buffer(1024);
        int written_len = 0;

        while (true)
        {
            if (_message_only)
            {
                if (message.length() >= buffer.size())
                {
                    buffer.resize(message.length() + 1);
                }
                memcpy(&buffer[0], message.c_str(), message.length() + 1);
                break;
            }
            else
            {
#if defined(_WIN32)
                int needed = _scprintf("%s.%03lld %s [%s:%d] %s",
                                       _cached_time_str.c_str(),
                                       (long long)ms.count(), levelToString(level).c_str(), file, line, message.c_str());

                if (needed < 0)
                {
                    buffer.assign(1, '\0');
                    break;
                }
                if (static_cast<size_t>(needed) >= buffer.size())
                {
                    buffer.resize(needed + 1);
                }
                written_len = _snprintf(buffer.data(), buffer.size(), "%s.%03lld %s [%s:%d] %s",
                                        _cached_time_str.c_str(),
                                        (long long)ms.count(), levelToString(level).c_str(), file, line, message.c_str());
                break;

#else
                written_len = snprintf(buffer.data(), buffer.size(), "%s.%03lld %s [%s:%d] %s",
                                       _cached_time_str.c_str(),
                                       (long long)ms.count(), levelToString(level).c_str(), file, line, message.c_str());

                if (written_len < 0)
                {
                    buffer.assign(1, '\0');
                    break;
                }
                if (static_cast<size_t>(written_len) >= buffer.size())
                {
                    buffer.resize(written_len + 1);
                }
                else
                {
                    break;
                }
#endif
            }
        }

        std::string output = buffer.data();
        std::string outputScreen = output;

        if (_log_screen_color && _outputToScreen)
        {
#ifndef _WIN32
            outputScreen = logGetColor(level) + outputScreen + MLLOG_COLOR_RESET;
#endif
        }

        std::lock_guard<std::mutex> lock(_mutex);

        if (_outputToFile && !_initialized)
        {
            rollFiles(true);
            _initialized = true;
        }

        if (_outputToFile)
        {
            if (!_file.is_open())
                reopenFile();

            if (_file.is_open())
            {
                size_t newline_size = 0;
                if (isNewLine)
                {
#if defined(_WIN32)
                    newline_size = 2;
#else
                    newline_size = 1;
#endif
                }
                size_t message_size = output.length() + newline_size;
                if (_currentSize > 0 && (_currentSize + message_size > _maxSizeInBytes))
                {
                    rollFiles();
                }

                _file << output << (isNewLine ? "\n" : "");
                if (_auto_flush)
                    _file.flush();

                _currentSize += message_size;

                if (_currentSize >= _maxSizeInBytes)
                {
                    rollFiles();
                }
            }
            else
            {
                // 【修改】使用新的错误处理机制
                reportError("Failed to write to log file. Message: " + output);
            }
        }

        if (_outputToScreen)
        {
#ifdef _WIN32
            mllogSetColor(level);
#endif
            std::cout << outputScreen << (isNewLine ? "\n" : "");
#ifdef _WIN32
            resetColor();
#endif
        }
    }

    void logformat(const char *file, int line, Level level, const char *format, ...)
    {
        if (!_log_enabled || level < _logLevel)
            return;

        va_list args;
        va_start(args, format);

        std::vector<char> buffer(1024);
        while (true)
        {
            va_list args_copy;
            va_copy(args_copy, args);
            int needed_length = vsnprintf(buffer.data(), buffer.size(), format, args_copy);
            va_end(args_copy);

            if (needed_length < 0)
            {
                va_end(args);
                return;
            }
            if (static_cast<size_t>(needed_length) >= buffer.size())
                buffer.resize(needed_length + 1);
            else
                break;
        }
        va_end(args);
        log(file, line, level, std::string(buffer.data()), _add_newline);
    }

    void cleanupOldLogs(int daysToKeep = 5)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        size_t lastSlashPos = _baseName.find_last_of("\\/");
        std::string directoryPath = (lastSlashPos != std::string::npos) ? _baseName.substr(0, lastSlashPos + 1) : getCurrentDirectory();
        std::string fileNamePattern = (lastSlashPos != std::string::npos) ? _baseName.substr(lastSlashPos + 1) : _baseName;
#if defined(_WIN32)
        WIN32_FIND_DATAA findFileData;
        HANDLE hFind;
        std::string searchPath = directoryPath + fileNamePattern + "_*";
        hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    if (shouldDeleteLog(findFileData.cFileName, daysToKeep))
                    {
                        std::string fullFilePath = directoryPath + findFileData.cFileName;
                        if (!DeleteFileA(fullFilePath.c_str()))
                        {
                            // 【修改】使用新的错误处理机制
                            reportError("Failed to delete old log file: " + fullFilePath);
                        }
                    }
                }
            } while (FindNextFileA(hFind, &findFileData));
            FindClose(hFind);
        }
#else
        DIR *dir;
        dirent *ent;
        if ((dir = opendir(directoryPath.c_str())) != NULL)
        {
            while ((ent = readdir(dir)) != NULL)
            {
                if (ent->d_type == DT_REG)
                {
                    std::string filename = ent->d_name;
                    if (filename.rfind(fileNamePattern + "_", 0) == 0)
                    {
                        if (shouldDeleteLog(filename, daysToKeep))
                        {
                            std::string fullFilePath = directoryPath + filename;
                            if (remove(fullFilePath.c_str()) != 0)
                            {
                                // 【修改】使用新的错误处理机制
                                reportError("Failed to delete old log file: " + fullFilePath + ", Error: " + strerror(errno));
                            }
                        }
                    }
                }
            }
            closedir(dir);
        }
#endif
    }

private:
    ML_Logger() : _logLevel(DEBUG), _outputToFile(true), _outputToScreen(true),
                  _maxRolls(5), _maxSizeInBytes(100 * 1024 * 1024), _initialized(false),
                  _isRoll(false), _currentRollIndex(0), _currentSize(0),
                  _message_only(false), _add_newline(true), _log_enabled(false),
                  _log_screen_color(true), _default_file_name_day(true),
                  _isCheckDay(false), _last_log_day(0),
                  _auto_flush(true),
                  _cached_time_sec(0),
                  _error_handler{} // 【新增】初始化错误处理器
    {
        setLogFile("mllog", _maxRolls, _maxSizeInBytes);
    }

    ~ML_Logger()
    {
        flush();
        if (_file.is_open())
        {
            _file.close();
        }
    }

    // 【新增】私有辅助函数，用于报告内部错误
    void reportError(const std::string &message)
    {
        if (_error_handler)
        {
            try
            {
                _error_handler("MLLOG INTERNAL: " + message);
            }
            catch (...)
            {
                // 防止用户的错误处理器本身抛出异常
            }
        }
        else
        {
            // 默认行为
            std::cerr << "MLLOG CRITICAL: " << message << std::endl;
        }
    }

    bool create_directories(const std::string &path)
    {
        std::string current_path;
        std::string p = path;
        std::replace(p.begin(), p.end(), '\\', '/');

        if (p.find_first_of('/') == 0)
        {
            current_path = "/";
        }

        std::stringstream ss(p);
        std::string dir;

        while (std::getline(ss, dir, '/'))
        {
            if (dir.empty() || dir == ".")
                continue;
#ifdef _WIN32
            if (current_path.empty() && dir.find(':') != std::string::npos)
            {
                current_path += dir;
            }
            else
            {
                if (!current_path.empty() && current_path.back() != '/')
                    current_path += '/';
                current_path += dir;
            }
#else
            if (!current_path.empty() && current_path.back() != '/')
                current_path += '/';
            current_path += dir;
#endif

#ifdef _WIN32
            if (_mkdir(current_path.c_str()) != 0 && errno != EEXIST)
                return false;
#else
            if (mkdir(current_path.c_str(), 0755) != 0 && errno != EEXIST)
                return false;
#endif
        }
        return true;
    }

    void reopenFile()
    {
        if (_baseName.empty() || _currentRollIndex == 0)
            return;
        if (_file.is_open())
            _file.close();

        std::ostringstream filename;
        filename << _baseFullNameWithDateAndTime << "_" << _currentRollIndex << ".log";
        _file.open(filename.str(), std::ofstream::out | std::ofstream::app);
        if (!_file.is_open())
        {
            // 【修改】使用新的错误处理机制
            reportError("Failed to reopen log file: " + filename.str());
        }
    }

    void rollFiles(bool forceNewFile = false)
    {
        if (_baseName.empty())
            return;
        if (_file.is_open())
            _file.close();

        _currentRollIndex++;
        if (_currentRollIndex > _maxRolls)
        {
            _isRoll = true;
            _currentRollIndex = 1;
        }

        std::ostringstream filename;
        filename << _baseFullNameWithDateAndTime << "_" << _currentRollIndex << ".log";

        _file.open(filename.str(), std::ofstream::out | (_isRoll ? std::ofstream::trunc : std::ofstream::app));
        if (!_file.is_open())
        {
            // 【修改】使用新的错误处理机制
            reportError("Failed to open new log file: " + filename.str());
        }
        _currentSize = 0;
    }

    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm local_tm;
#if defined(_WIN32)
        localtime_s(&local_tm, &time_t_now);
#else
        localtime_r(&time_t_now, &local_tm);
#endif
        char buffer[64];
        if (_default_file_name_day)
            strftime(buffer, sizeof(buffer), "%Y%m%d", &local_tm);
        else
            strftime(buffer, sizeof(buffer), "%Y%m%d%H%M", &local_tm);
        return std::string(buffer);
    }

    const char *logGetColor(Level eFlag)
    {
        switch (eFlag)
        {
        case DEBUG:
            return MLLOG_COLOR_GREEN;
        case INFO:
            return MLLOG_COLOR_CYAN;
        case NOTICE:
            return MLLOG_COLOR_BLUE;
        case WARNING:
            return MLLOG_COLOR_YELLOW;
        case ERROR:
            return MLLOG_COLOR_RED;
        case CRITICAL:
            return MLLOG_COLOR_MAGENTA;
        case ALERT:
            return MLLOG_COLOR_WHITE;
        default:
            return MLLOG_EMPTY;
        }
    }

#ifdef _WIN32
    void SetColor(WORD foreground, WORD background = 0)
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, (background << 4) | foreground);
    }
    void mllogSetColor(Level level)
    {
        switch (level)
        {
        case DEBUG:
            SetColor(2);
            break;
        case INFO:
            SetColor(11);
            break;
        case NOTICE:
            SetColor(9);
            break;
        case WARNING:
            SetColor(14);
            break;
        case ERROR:
            SetColor(12);
            break;
        case CRITICAL:
            SetColor(15, 4);
            break;
        case ALERT:
            SetColor(15, 4);
            break;
        default:
            SetColor(7);
            break;
        }
    }
    void resetColor() { SetColor(7); }
#endif

    std::string levelToString(Level level)
    {
        static const char *const buffer[] = {"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "CRITICAL", "ALERT"};
        return buffer[level];
    }

    std::string getCurrentDirectory()
    {
        char buffer[1024];
#ifdef _WIN32
        if (GetCurrentDirectoryA(sizeof(buffer), buffer))
            return std::string(buffer) + "\\";
#else
        if (getcwd(buffer, sizeof(buffer)) != NULL)
            return std::string(buffer) + "/";
#endif
        return std::string();
    }

    bool shouldDeleteLog(const std::string &filename, int daysToKeep)
    {
        std::time_t fileTime = parseDateFromFilename(filename);
        if (fileTime == static_cast<time_t>(-1))
            return false;

        std::time_t now = std::time(nullptr);
        return std::difftime(now, fileTime) > daysToKeep * 24 * 3600;
    }

    time_t parseDateFromFilename(const std::string &filename)
    {
        if (filename.rfind(_baseNameWithoutPath + "_", 0) != 0)
            return -1;

        std::string datePart = filename.substr(_baseNameWithoutPath.length() + 1);
        if (datePart.length() < 8)
            return -1;

        std::string dateStr = datePart.substr(0, 8);
        for (char c : dateStr)
        {
            if (!isdigit(c))
                return -1;
        }

        struct tm tm = {};
        try
        {
            tm.tm_year = std::stoi(dateStr.substr(0, 4)) - 1900;
            tm.tm_mon = std::stoi(dateStr.substr(4, 2)) - 1;
            tm.tm_mday = std::stoi(dateStr.substr(6, 2));
            tm.tm_isdst = -1;
        }
        catch (const std::exception &)
        {
            return -1;
        }
        return mktime(&tm);
    }

private:
    std::ofstream _file;
    std::mutex _mutex;
    std::string _baseName;
    std::string _baseNameWithoutPath;
    std::string _baseFullNameWithDateAndTime;
    bool _isRoll;
    Level _logLevel;
    bool _outputToFile;
    bool _outputToScreen;
    int _currentRollIndex;
    int _maxRolls;
    size_t _maxSizeInBytes;
    size_t _currentSize;
    bool _initialized;
    bool _message_only;
    bool _add_newline;
    bool _log_enabled;
    bool _log_screen_color;
    bool _default_file_name_day;
    bool _isCheckDay;
    std::string _start_timestamp;
    int _last_log_day;
    bool _auto_flush;
    time_t _cached_time_sec;
    std::string _cached_time_str;
    ErrorHandler _error_handler; // 【新增】错误处理器成员
};

class LoggerStream
{
public:
    LoggerStream(ML_Logger::Level level, const char *file, int line)
        : _level(level), _file(file), _line(line)
    {
    }

    template <typename T>
    LoggerStream &operator<<(const T &value)
    {
        _buffer << value;
        return *this;
    }
    template <typename T>
    LoggerStream &operator<<(T *ptr)
    {
        if (ptr)
            _buffer << static_cast<const void *>(ptr);
        else
            _buffer << "nullptr";
        return *this;
    }
    LoggerStream &operator<<(const char *str)
    {
        if (str)
            _buffer << str;
        else
            _buffer << "nullptr";
        return *this;
    }
    ~LoggerStream() { ML_Logger::getInstance().log(_file, _line, _level, _buffer.str(), ML_Logger::getInstance().getAddNewLine()); }

private:
    ML_Logger::Level _level;
    const char *_file;
    int _line;
    std::stringstream _buffer;
};

#define MLSHORT_FILE file_name_from_path(__FILE__)

#define MLLOG(level) LoggerStream(level, MLSHORT_FILE, __LINE__)
#define MLLOGF(level, format, ...)                                                                \
    do                                                                                            \
    {                                                                                             \
        ML_Logger::getInstance().logformat(MLSHORT_FILE, __LINE__, level, format, ##__VA_ARGS__); \
    } while (0)

#define MLLOG_START()                                       \
    do                                                      \
    {                                                       \
        ML_Logger::getInstance().setLogSwitch(true);        \
        MLLOG_ALERT << "---------- Start MLLOG ----------"; \
    } while (0)

#define MLLOG_DEBUG MLLOG(ML_Logger::DEBUG)
#define MLLOG_INFO MLLOG(ML_Logger::INFO)
#define MLLOG_NOTICE MLLOG(ML_Logger::NOTICE)
#define MLLOG_WARNING MLLOG(ML_Logger::WARNING)
#define MLLOG_ERROR MLLOG(ML_Logger::ERROR)
#define MLLOG_CRITICAL MLLOG(ML_Logger::CRITICAL)
#define MLLOG_ALERT MLLOG(ML_Logger::ALERT)

#define MLLOGD MLLOG(ML_Logger::DEBUG)
#define MLLOGI MLLOG(ML_Logger::INFO)
#define MLLOGN MLLOG(ML_Logger::NOTICE)
#define MLLOGW MLLOG(ML_Logger::WARNING)
#define MLLOGE MLLOG(ML_Logger::ERROR)
#define MLLOGC MLLOG(ML_Logger::CRITICAL)
#define MLLOGA MLLOG(ML_Logger::ALERT)

#define mllogd MLLOG(ML_Logger::DEBUG)
#define mllogi MLLOG(ML_Logger::INFO)
#define mllogn MLLOG(ML_Logger::NOTICE)
#define mllogw MLLOG(ML_Logger::WARNING)
#define mlloge MLLOG(ML_Logger::ERROR)
#define mllogc MLLOG(ML_Logger::CRITICAL)
#define mlloga MLLOG(ML_Logger::ALERT)

#define MLLOG_DEBUGF(format, ...) MLLOGF(ML_Logger::DEBUG, format, ##__VA_ARGS__)
#define MLLOG_INFOF(format, ...) MLLOGF(ML_Logger::INFO, format, ##__VA_ARGS__)
#define MLLOG_NOTICEF(format, ...) MLLOGF(ML_Logger::NOTICE, format, ##__VA_ARGS__)
#define MLLOG_WARNINGF(format, ...) MLLOGF(ML_Logger::WARNING, format, ##__VA_ARGS__)
#define MLLOG_ERRORF(format, ...) MLLOGF(ML_Logger::ERROR, format, ##__VA_ARGS__)
#define MLLOG_CRITICALF(format, ...) MLLOGF(ML_Logger::CRITICAL, format, ##__VA_ARGS__)
#define MLLOG_ALERTF(format, ...) MLLOGF(ML_Logger::ALERT, format, ##__VA_ARGS__)

#endif // MLLOG_HPP