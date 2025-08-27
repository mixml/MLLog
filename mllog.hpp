#ifndef MLLOG_HPP
#define MLLOG_HPP

/**
 * @file mllog.hpp
 * @brief 多功能、跨平台、单头文件日志系统（SRP 重构 + 平台代码隔离）。
 *
 * @author malin
 *      - Email: zcyxml@163.com  mlin2@grgbanking.com
 *      - GitHub: https://github.com/mixml
 * 变更摘要：
 * @version 2.5 (2025-08-27)
 *      - 性能: formatMessage_ 使用 _scprintf/_snprintf（Windows）与 snprintf（Unix, 移除 ostringstream 组装，提升性能。
 * @version 2.4 (2025-08-25 重构)
 *      - 设计: 按单一职责原则（SRP）拆分 log() 为多个私有函数：
 *      updateAndGetTimeCache_() / maybeTruncate_() / formatMessage_() /
 *      writeToTargets_() / writeToFile_() / writeToScreen_()
 *      - 平台隔离: 新增 platform_ 前缀函数。
 *      - 可靠性: 彻底修复跨天切换的潜在死锁与崩溃；跨天检测改为 YYYYMMDD。
 *      - 细节: 补齐 <cctype>/<cerrno>，修复 ANSI RED 宏，降低 log() 体积、可读性更强。
 * @version 2.3.1 (2025-08-25)
 *      - 修复: 跨天写日志崩溃的问题
 * @version 2.3 (2025-07-08)
 *      - 优化: 修改默认日志的名称.
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
 * #include "mllog.hpp"
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
 * #include "mllog.hpp"
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
#include <cctype> // std::isdigit
#include <cerrno> // errno
#include <chrono>
#include <cstdarg>
#include <cstdio> // snprintf, _scprintf, _snprintf
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

 /* ========================= 平台头文件 ========================= */
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
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

/* ========================= ANSI 颜色 ========================= */
#define MLLOG_COLOR_NORMAL "\x1B[0m"
#define MLLOG_COLOR_RED "\x1B[31m"
#define MLLOG_COLOR_GREEN "\x1B[32m"
#define MLLOG_COLOR_YELLOW "\x1B[33m"
#define MLLOG_COLOR_BLUE "\x1B[34m"
#define MLLOG_COLOR_MAGENTA "\x1B[35m"
#define MLLOG_COLOR_CYAN "\x1B[36m"
#define MLLOG_COLOR_WHITE "\x1B[37m"
#define MLLOG_COLOR_RESET "\x1B[0m"
#define MLLOG_EMPTY ""

/* ========================= 工具 constexpr ========================= */
// 返回 path 中最后一个分隔符后的基名指针；若无分隔符，返回 path 本身
constexpr const char* mllog_find_last_slash_helper(const char* s, const char* last)
{
    return (*s == '\0')
        ? (last ? last : s)
        : mllog_find_last_slash_helper(
            s + 1,
            // 只有当当前字符为分隔符且后面不是字符串终止符时，才更新 last
            ((*s == '/' || *s == '\\') && s[1] != '\0') ? (s + 1) : last);
}
constexpr const char* mllog_file_name_from_path(const char* path) { return mllog_find_last_slash_helper(path, path); }

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
    using ErrorHandler = std::function<void(const std::string&)>;

    static const size_t MAX_LOG_MESSAGE_SIZE = 1024 * 1024 * 5; // 5 MB
    static constexpr const char* TRUNCATED_MESSAGE = "\n... [Message Truncated]";

    static ML_Logger& getInstance()
    {
        static ML_Logger inst;
        return inst;
    }

    ML_Logger(const ML_Logger&) = delete;
    ML_Logger& operator=(const ML_Logger&) = delete;
    ML_Logger(ML_Logger&&) = delete;
    ML_Logger& operator=(ML_Logger&&) = delete;

    /* ========================= 配置接口 ========================= */
    void setLevel(Level lv) { _logLevel = lv; }
    void setCheckDay(bool on) { _isCheckDay = on; }

    void setLogFile(const std::string& baseName, int maxRolls = 5, size_t maxSizeInBytes = 100 * 1024 * 1024)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _baseName = baseName;
        _maxRolls = maxRolls > 0 ? maxRolls : 1;
        _maxSizeInBytes = maxSizeInBytes;

        _currentSize = 0;
        _currentRollIndex = 0;
        _initialized = false;
        _isRoll = false;

        size_t p = _baseName.find_last_of("\\/");
        _baseNameWithoutPath = (p != std::string::npos) ? _baseName.substr(p + 1) : _baseName;
        if (_outputToFile)
        {
            std::string dir = (p != std::string::npos) ? _baseName.substr(0, p) : std::string(".");
            platform_createDirectories_(dir);
        }

        _start_timestamp = currentTimestamp_();
        _baseFullNameWithDateAndTime = _baseName + "_" + _start_timestamp;
        if (_file.is_open())
            _file.close();
    }

    void setOutput(bool toFile, bool toScreen)
    {
        _outputToFile = toFile;
        _outputToScreen = toScreen;
    }
    void setAddNewLine(bool add) { _add_newline = add; }
    bool getAddNewLine() const { return _add_newline; }

    void setLogSwitch(bool on) { _log_enabled = on; }
    bool getLogSwitch() const { return _log_enabled; }

    void setDefaultLogFormat(bool dateOnly) { _default_file_name_day = dateOnly; }
    void setMessageOnly(bool msgOnly) { _message_only = msgOnly; }
    void setScreenColor(bool on) { _log_screen_color = on; }
    void setAutoFlush(bool enabled) { _auto_flush = enabled; }

    void setErrorHandler(ErrorHandler h) { _error_handler = std::move(h); }

    void flush()
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_file.is_open())
            _file.flush();
    }

    /* ========================= 写日志接口 ========================= */
    void log(const char* file, int line, Level lv, const std::string& original, bool isNewLine = true)
    {
        if (!_log_enabled || lv < _logLevel)
            return;

        int ms_count = 0;
        std::tm cached_tm = {};
        updateAndGetTimeCache_(cached_tm, ms_count);

        const std::string msg = maybeTruncate_(original);
        const std::string formatted = formatMessage_(lv, file, line, msg, ms_count);
        writeToTargets_(formatted, isNewLine, lv);
    }

    void logformat(const char* file, int line, Level lv, const char* fmt, ...)
    {
        if (!_log_enabled || lv < _logLevel)
            return;
        va_list args;
        va_start(args, fmt);
        std::vector<char> buf(1024);
        while (true)
        {
            va_list cpy;
            va_copy(cpy, args);
            int need = std::vsnprintf(buf.data(), buf.size(), fmt, cpy);
            va_end(cpy);
            if (need < 0)
            {
                va_end(args);
                return;
            }
            if ((size_t)need >= buf.size())
                buf.resize(need + 1);
            else
                break;
        }
        va_end(args);
        log(file, line, lv, std::string(buf.data()), _add_newline);
    }

    /* ========================= 工具接口 ========================= */
    static std::string get_module_path() { return platform_getModulePath_(); }
    std::string get_process_name() { return platform_getProcessName_(); }

    void cleanupOldLogs(int daysToKeep = 5)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (!_outputToFile)
            return;
        size_t p = _baseName.find_last_of("\\/");
        std::string dir = (p != std::string::npos) ? _baseName.substr(0, p + 1) : platform_currentDirWithSlash_();
        std::string stem = (p != std::string::npos) ? _baseName.substr(p + 1) : _baseName;
        std::vector<std::string> files = platform_listLogFiles_(dir, stem);
        for (const auto& name : files)
        {
            if (shouldDeleteLog_(name, daysToKeep))
                platform_deleteFile_(dir + name);
        }
    }

private:
    ML_Logger()
        : _isRoll(false), _logLevel(DEBUG), _outputToFile(true), _outputToScreen(true),
        _currentRollIndex(0), _maxRolls(5), _maxSizeInBytes(100 * 1024 * 1024), _currentSize(0),
        _initialized(false), _message_only(false), _add_newline(true), _log_enabled(false),
        _log_screen_color(true), _default_file_name_day(true), _isCheckDay(false),
        _last_log_ymd(0), _auto_flush(true), _cached_time_sec(0)
    {
        std::string def = get_module_path() + "/log/" + get_process_name() + "_MLLOG";
        setLogFile(def, _maxRolls, _maxSizeInBytes);
    }

    ~ML_Logger()
    {
        flush();
        if (_file.is_open())
            _file.close();
    }

    /* ========================= 时间缓存与跨天 ========================= */
    void updateAndGetTimeCache_(std::tm& out_tm, int& out_ms)
    {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        out_ms = static_cast<int>(ms.count());
        const std::time_t t = clock::to_time_t(now);
        if (t != _cached_time_sec)
        {
            _cached_time_sec = t;
#if defined(_WIN32)
            localtime_s(&out_tm, &t);
#else
            localtime_r(&t, &out_tm);
#endif
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &out_tm);
            _cached_time_str = buf;
            if (_isCheckDay)
            {
                const int ymd = (out_tm.tm_year + 1900) * 10000 + (out_tm.tm_mon + 1) * 100 + out_tm.tm_mday;
                if (_last_log_ymd == 0)
                {
                    _last_log_ymd = ymd;
                }
                else if (_last_log_ymd != ymd)
                {
                    onDayChange_();
                    _last_log_ymd = ymd;
                }
            }
        }
        else
        {
#if defined(_WIN32)
            localtime_s(&out_tm, &_cached_time_sec);
#else
            localtime_r(&_cached_time_sec, &out_tm);
#endif
        }
    }

    /* ========================= 截断 ========================= */
    std::string maybeTruncate_(const std::string& s) const
    {
        if (s.size() <= MAX_LOG_MESSAGE_SIZE)
            return s;
        std::string r = s.substr(0, MAX_LOG_MESSAGE_SIZE);
        r.append(TRUNCATED_MESSAGE);
        return r;
    }

    /* ========================= 快速格式化 ========================= */
    static const char* levelToStringC_(Level lv)
    {
        switch (lv)
        {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case NOTICE:
            return "NOTICE";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case CRITICAL:
            return "CRITICAL";
        case ALERT:
            return "ALERT";
        default:
            return "";
        }
    }

    std::string formatMessage_(Level lv, const char* file, int line, const std::string& msg, int ms_count) const
    {
        if (_message_only)
            return msg;
        const char* level_c = levelToStringC_(lv);
        const char* time_c = _cached_time_str.c_str();
#if defined(_WIN32)
        int need = _scprintf("%s.%03d %s [%s:%d] %s", time_c, ms_count, level_c, file, line, msg.c_str());
        if (need < 0)
            return std::string();
        std::vector<char> buf(static_cast<size_t>(need) + 1u);
        _snprintf(buf.data(), buf.size(), "%s.%03d %s [%s:%d] %s", time_c, ms_count, level_c, file, line, msg.c_str());
        return std::string(buf.data(), static_cast<size_t>(need));
#else
        int need = std::snprintf(nullptr, 0, "%s.%03d %s [%s:%d] %s", time_c, ms_count, level_c, file, line, msg.c_str());
        if (need < 0)
            return std::string();
        std::vector<char> buf(static_cast<size_t>(need) + 1u);
        std::snprintf(buf.data(), buf.size(), "%s.%03d %s [%s:%d] %s", time_c, ms_count, level_c, file, line, msg.c_str());
        return std::string(buf.data(), static_cast<size_t>(need));
#endif
    }

    /* ========================= 写入目标（持锁） ========================= */
    void writeToTargets_(const std::string& formatted, bool isNewLine, Level lv)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_outputToFile && !_initialized)
        {
            rollFiles_(true);
            _initialized = true;
        }
        if (_outputToFile)
            writeToFile_(formatted, isNewLine);
        if (_outputToScreen)
            writeToScreen_(formatted, isNewLine, lv);
    }

    void writeToFile_(const std::string& s, bool isNewLine)
    {
        if (!_outputToFile)
            return;
        if (!_file.is_open())
            reopenFile_();
        if (!_file.is_open())
        {
            reportError_(std::string("Failed to open file for writing. Message: ") + s);
            return;
        }
        const size_t newline_size = isNewLine ? (
#if defined(_WIN32)
            2u
#else
            1u
#endif
            )
            : 0u;
        const size_t msg_size = s.size() + newline_size;
        if (_currentSize > 0 && (_currentSize + msg_size > _maxSizeInBytes))
            rollFiles_();
        _file << s << (isNewLine ? "\n" : "");
        if (_auto_flush)
            _file.flush();
        _currentSize += msg_size;
        if (_currentSize >= _maxSizeInBytes)
            rollFiles_();
    }

    void writeToScreen_(const std::string& s, bool isNewLine, Level lv)
    {
        platform_setConsoleColor_(_log_screen_color ? lv : DEBUG);
#if !defined(_WIN32)
        if (_log_screen_color)
            std::cout << levelColorAnsi_(lv);
#endif
        std::cout << s << (isNewLine ? "\n" : "");
        platform_resetConsoleColor_();
    }

    /* ========================= 跨天切换（内部自锁） ========================= */
    void onDayChange_()
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_file.is_open())
            _file.close();
        _initialized = false;
        _isRoll = false;
        _currentRollIndex = 0;
        _currentSize = 0;
        _start_timestamp = currentTimestamp_();
        _baseFullNameWithDateAndTime = _baseName + "_" + _start_timestamp;
    }

    /* ========================= 文件轮转与打开（需持锁） ========================= */
    void reopenFile_()
    {
        if (_baseName.empty() || _currentRollIndex == 0)
            return;
        if (_file.is_open())
            _file.close();
        std::ostringstream fn;
        fn << _baseFullNameWithDateAndTime << '_' << _currentRollIndex << ".log";
        _file.open(fn.str(), std::ofstream::out | std::ofstream::app);
        if (!_file.is_open())
            reportError_("Failed to reopen log file: " + fn.str());
    }

    void rollFiles_(bool /*first*/ = false)
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
        std::ostringstream fn;
        fn << _baseFullNameWithDateAndTime << '_' << _currentRollIndex << ".log";
        _file.open(fn.str(), std::ofstream::out | (_isRoll ? std::ofstream::trunc : std::ofstream::app));
        if (!_file.is_open())
            reportError_("Failed to open new log file: " + fn.str());
        _currentSize = 0;
    }

    /* ========================= 时间/级别/颜色帮助 ========================= */
    std::string currentTimestamp_() const
    {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm lt;
#if defined(_WIN32)
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        char buf[64];
        if (_default_file_name_day)
            std::strftime(buf, sizeof(buf), "%Y%m%d", &lt);
        else
            std::strftime(buf, sizeof(buf), "%Y%m%d%H%M", &lt);
        return std::string(buf);
    }

    const char* levelColorAnsi_(Level lv) const
    {
        switch (lv)
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

    /* ========================= 错误报告 ========================= */
    void reportError_(const std::string& m)
    {
        if (_error_handler)
        {
            try
            {
                _error_handler("MLLOG INTERNAL: " + m);
            }
            catch (...)
            {
            }
        }
        else
        {
            std::cerr << "MLLOG CRITICAL: " << m << std::endl;
        }
    }

    /* ========================= 平台封装 ========================= */
    static bool platform_createDirectories_(const std::string& path)
    {
        std::string p = path;
        std::replace(p.begin(), p.end(), '\\', '/');
        if (p.empty())
            return true;
        std::string cur = (p[0] == '/') ? std::string("/") : std::string();
        std::stringstream ss(p);
        std::string seg;
        while (std::getline(ss, seg, '/'))
        {
            if (seg.empty() || seg == ".")
                continue;
#if defined(_WIN32)
            if (cur.empty() && seg.find(':') != std::string::npos)
            {
                cur += seg;
            }
            else
            {
                if (!cur.empty() && cur.back() != '/')
                    cur.push_back('/');
                cur += seg;
            }
            if (_mkdir(cur.c_str()) != 0 && errno != EEXIST)
                return false;
#else
            if (!cur.empty() && cur.back() != '/')
                cur.push_back('/');
            cur += seg;
            if (::mkdir(cur.c_str(), 0755) != 0 && errno != EEXIST)
                return false;
#endif
        }
        return true;
    }

    static void platform_setConsoleColor_(Level lv)
    {
#if defined(_WIN32)
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD fg = 7, bg = 0;
        switch (lv)
        {
        case DEBUG:
            fg = 2;
            break;
        case INFO:
            fg = 11;
            break;
        case NOTICE:
            fg = 9;
            break;
        case WARNING:
            fg = 14;
            break;
        case ERROR:
            fg = 12;
            break;
        case CRITICAL:
            fg = 15;
            bg = 4;
            break;
        case ALERT:
            fg = 15;
            bg = 4;
            break;
        default:
            fg = 7;
            break;
        }
        SetConsoleTextAttribute(h, (bg << 4) | fg);
#else
        (void)lv; // 非 Windows 使用 ANSI，在 writeToScreen_ 中处理
#endif
    }

    static void platform_resetConsoleColor_()
    {
#if defined(_WIN32)
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, 7);
#else
        std::cout << MLLOG_COLOR_RESET;
#endif
    }

    static std::string platform_getModulePath_()
    {
        char buf[1024] = { 0 };
#if defined(_WIN32)
        HMODULE h = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&platform_getModulePath_, &h);
        GetModuleFileNameA(h, buf, sizeof(buf));
        std::string p(buf);
        size_t pos = p.find_last_of("\\/");
        return (pos != std::string::npos) ? p.substr(0, pos) : ".";
#else
        Dl_info info;
        if (dladdr((void*)(&platform_getModulePath_), &info) && info.dli_fname)
        {
            std::string p(info.dli_fname);
            size_t pos = p.find_last_of('/');
            return (pos != std::string::npos) ? p.substr(0, pos) : ".";
        }
        return ".";
#endif
    }

    static std::string platform_getProcessName_()
    {
        char buf[1024] = { 0 };
#if defined(_WIN32)
        if (GetModuleFileNameA(NULL, buf, sizeof(buf) - 1) > 0)
        {
            std::string n = buf;
            size_t p = n.find_last_of("\\/");
            if (p != std::string::npos)
                n = n.substr(p + 1);
            p = n.rfind('.');
            if (p != std::string::npos)
                n = n.substr(0, p);
            return n;
        }
#else
        if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) > 0)
        {
            std::string n = buf;
            size_t p = n.find_last_of('/');
            if (p != std::string::npos)
                n = n.substr(p + 1);
            return n;
        }
#endif
        return "unknown_process";
    }

    static std::string platform_currentDirWithSlash_()
    {
        char buf[1024];
#if defined(_WIN32)
        if (GetCurrentDirectoryA(sizeof(buf), buf))
            return std::string(buf) + "\\";
#else
        if (getcwd(buf, sizeof(buf)) != NULL)
            return std::string(buf) + "/";
#endif
        return std::string();
    }

    static std::vector<std::string> platform_listLogFiles_(const std::string& dir, const std::string& stem)
    {
        std::vector<std::string> out;
#if defined(_WIN32)
        WIN32_FIND_DATAA f;
        HANDLE h;
        std::string search = dir + stem + "_*";
        h = FindFirstFileA(search.c_str(), &f);
        if (h != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    out.emplace_back(f.cFileName);
            } while (FindNextFileA(h, &f));
            FindClose(h);
        }
#else
        if (DIR* d = opendir(dir.c_str()))
        {
            while (dirent* e = readdir(d))
            {
                if (e->d_type == DT_REG)
                {
                    std::string n = e->d_name;
                    if (n.rfind(stem + "_", 0) == 0)
                        out.emplace_back(n);
                }
            }
            closedir(d);
        }
#endif
        return out;
    }

    static void platform_deleteFile_(const std::string& full)
    {
#if defined(_WIN32)
        if (!DeleteFileA(full.c_str()))
        {
            std::cerr << "MLLOG CRITICAL: Failed to delete file: " << full << std::endl;
        }
#else
        if (::remove(full.c_str()) != 0)
        {
            std::cerr << "MLLOG CRITICAL: Failed to delete file: " << full << ", errno=" << errno << std::endl;
        }
#endif
    }

    /* ========================= 旧文件日期解析/保留策略 ========================= */
    bool shouldDeleteLog_(const std::string& filename, int days) const
    {
        const std::time_t ft = parseDateFromFilename_(filename);
        if (ft == (time_t)-1)
            return false;
        const std::time_t now = std::time(nullptr);
        return std::difftime(now, ft) > days * 24 * 3600;
    }

    std::time_t parseDateFromFilename_(const std::string& filename) const
    {
        if (filename.rfind(_baseNameWithoutPath + "_", 0) != 0)
            return -1;
        std::string rest = filename.substr(_baseNameWithoutPath.length() + 1);
        if (rest.size() < 8)
            return -1;
        std::string ymd = rest.substr(0, 8);
        for (char c : ymd)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return -1;
        }
        std::tm tmv = {};
        try
        {
            tmv.tm_year = std::stoi(ymd.substr(0, 4)) - 1900;
            tmv.tm_mon = std::stoi(ymd.substr(4, 2)) - 1;
            tmv.tm_mday = std::stoi(ymd.substr(6, 2));
            tmv.tm_isdst = -1;
        }
        catch (...)
        {
            return -1;
        }
        return std::mktime(&tmv);
    }

private:
    // 文件/锁
    std::ofstream _file;
    std::mutex _mutex;

    // 配置
    std::string _baseName;
    std::string _baseNameWithoutPath;
    std::string _baseFullNameWithDateAndTime;
    bool _isRoll;
    Level _logLevel;
    bool _outputToFile;
    bool _outputToScreen;

    // 滚动
    int _currentRollIndex;
    int _maxRolls;
    size_t _maxSizeInBytes;
    size_t _currentSize;
    bool _initialized;

    // 行为
    bool _message_only;
    bool _add_newline;
    bool _log_enabled;
    bool _log_screen_color;
    bool _default_file_name_day;
    bool _isCheckDay;

    // 时间缓存
    std::string _start_timestamp;
    int _last_log_ymd;
    bool _auto_flush;
    std::time_t _cached_time_sec;
    std::string _cached_time_str;

    // 错误回调
    ErrorHandler _error_handler;
};

/* ========================= 日志流（保持原宏兼容） ========================= */
class LoggerStream
{
public:
    LoggerStream(ML_Logger::Level lv, const char* file, int line) : _lv(lv), _file(file), _line(line) {}
    template <typename T>
    LoggerStream& operator<<(const T& v)
    {
        _ss << v;
        return *this;
    }
    template <typename T>
    LoggerStream& operator<<(T* p)
    {
        if (p)
            _ss << static_cast<const void*>(p);
        else
            _ss << "nullptr";
        return *this;
    }
    LoggerStream& operator<<(const char* s)
    {
        if (s)
            _ss << s;
        else
            _ss << "nullptr";
        return *this;
    }
    ~LoggerStream() { ML_Logger::getInstance().log(_file, _line, _lv, _ss.str(), ML_Logger::getInstance().getAddNewLine()); }

private:
    ML_Logger::Level _lv;
    const char* _file;
    int _line;
    std::stringstream _ss;
};

/* ========================= 宏接口（兼容） ========================= */
#define MLSHORT_FILE mllog_file_name_from_path(__FILE__)

#define MLLOG(level) LoggerStream(level, MLSHORT_FILE, __LINE__)
#define MLLOGF(level, fmt, ...)                                                                \
    do                                                                                         \
    {                                                                                          \
        ML_Logger::getInstance().logformat(MLSHORT_FILE, __LINE__, level, fmt, ##__VA_ARGS__); \
    } while (0)

#define MLLOG_START()                                                   \
    do                                                                  \
    {                                                                   \
        ML_Logger::getInstance().setLogSwitch(true);                    \
        MLLOG(ML_Logger::ALERT) << "---------- Start MLLOG ----------"; \
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

#define MLLOG_DEBUGF(fmt, ...) MLLOGF(ML_Logger::DEBUG, fmt, ##__VA_ARGS__)
#define MLLOG_INFOF(fmt, ...) MLLOGF(ML_Logger::INFO, fmt, ##__VA_ARGS__)
#define MLLOG_NOTICEF(fmt, ...) MLLOGF(ML_Logger::NOTICE, fmt, ##__VA_ARGS__)
#define MLLOG_WARNINGF(fmt, ...) MLLOGF(ML_Logger::WARNING, fmt, ##__VA_ARGS__)
#define MLLOG_ERRORF(fmt, ...) MLLOGF(ML_Logger::ERROR, fmt, ##__VA_ARGS__)
#define MLLOG_CRITICALF(fmt, ...) MLLOGF(ML_Logger::CRITICAL, fmt, ##__VA_ARGS__)
#define MLLOG_ALERTF(fmt, ...) MLLOGF(ML_Logger::ALERT, fmt, ##__VA_ARGS__)

#endif // MLLOG_HPP