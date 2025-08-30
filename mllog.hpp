#ifndef MLLOG_HPP
#define MLLOG_HPP

/**
 * @file mllog.hpp
 * @brief 多功能、跨平台、单头文件日志系统（Turbo）。
 *
 * @author malin
 *      - Email: zcyxml@163.com  mlin2@grgbanking.com
 *      - GitHub: https://github.com/mixml
 *
 * 变更摘要：
 * @version 2.6.3 (2025-08-30 Turbo)
 *      - 性能：秒级时间串使用 TLS 缓存（localtime/strftime 仅在秒变化时执行），避免每条日志反复格式化。
 *      - 性能：二进制直写 _file.write() + put('\n')，减少 iostream 格式化开销。
 *      - 性能：每线程复用 TLS std::string 作为格式化缓冲，降低分配/拷贝。
 *      - 并发：跨天检测采用原子标记（锁外置位、锁内切换）；全程线程安全。
 *      - 健壮：错误回调在日志临界区使用线程哨兵避免自递归死锁；VS2015 兼容 [[nodiscard]] 与 inline 变量。
 * @version 2.6.2 (2025-08-30)
 *      - 修复数据竞争、Windows printf 越界、错误回调潜在死锁；去除 ctor 打印；TTY 颜色检测缓存。
 * @version 2.6.1 (2025-08-29)
 *      - 跨天切换在锁内执行（锁外仅打标记），避免回拨时间导致的潜在卡死。
 *      - 文件以二进制模式打开，精确计量大小；打开后用 tellp() 得到真实写入位置。
 *      - *nix 仅在 TTY 下输出 ANSI 颜色；Windows 维持可选 VT（仅当工程自行包含 <windows.h> 时）。
 * @version 2.6 (2025-08-29)
 *      - 去除 <windows.h> 强依赖；ANSI 颜色 + 可选 VT。
 *      - 更换 Level 定义。
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
 *     logger.setLevel(ML_Logger::Level::Debug);                // 设置日志级别为DEBUG，所有级别都将输出
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
 *     logger.setLevel(ML_Logger::Level::Info);
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
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

#if defined(_WIN32)
#include <direct.h> // _mkdir, _getcwd
#include <io.h>     // _findfirst, _findnext, _finddata_t, _isatty, _fileno
#pragma warning(disable : 4996)
#else
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h> // isatty, readlink, getcwd
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

/* Windows: 如果项目外部引入 <windows.h>，可用 VT 模式；本头不强制依赖 */
#if defined(_WIN32)
#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define MLLOG_VT_ENABLE ENABLE_VIRTUAL_TERMINAL_PROCESSING
#else
#define MLLOG_VT_ENABLE 0x0004u
#endif
#endif

/* ========================= constexpr 工具 ========================= */
constexpr const char *mllog_find_last_slash_helper(const char *s, const char *last)
{
    return (*s == '\0')
               ? (last ? last : s)
               : mllog_find_last_slash_helper(
                     s + 1,
                     ((*s == '/' || *s == '\\') && s[1] != '\0') ? (s + 1) : last);
}
constexpr const char *mllog_file_name_from_path(const char *path)
{
    return mllog_find_last_slash_helper(path, path);
}

#if defined(_MSC_VER)
#define ML_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define ML_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define ML_ALWAYS_INLINE inline
#endif

#ifndef ML_NODISCARD
#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(nodiscard)
#define ML_NODISCARD [[nodiscard]]
#else
#define ML_NODISCARD
#endif
#else
#define ML_NODISCARD
#endif
#endif

template <class T>
ML_NODISCARD ML_ALWAYS_INLINE constexpr const T &ml_max(const T &a, const T &b) noexcept { return (a < b) ? b : a; }

/* ========================= 核心类 ========================= */
class ML_Logger
{
public:
    enum class Level
    {
        Debug = 0,
        Info,
        Notice,
        Warning,
        Error,
        Critical,
        Alert
    };
    using ErrorHandler = std::function<void(const std::string &)>;

    static const size_t MAX_LOG_MESSAGE_SIZE = 1024u * 1024u * 5u; // 5 MB
    static constexpr const char *TRUNCATED_MESSAGE = "\n... [Message Truncated]";

    static ML_Logger &getInstance()
    {
        static ML_Logger inst;
        return inst;
    }

    ML_Logger(const ML_Logger &) = delete;
    ML_Logger &operator=(const ML_Logger &) = delete;
    ML_Logger(ML_Logger &&) = delete;
    ML_Logger &operator=(ML_Logger &&) = delete;

    /* ========== 配置接口 ========== */
    void setLevel(Level lv) { _logLevel = lv; }
    void setCheckDay(bool on) { _isCheckDay = on; }
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

    void setLogFile(const std::string &baseName,
                    int maxRolls = 5,
                    size_t maxSizeInBytes = 100u * 1024u * 1024u)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _baseName = baseName;
        _maxRolls = ml_max(1, maxRolls);
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

    void flush()
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_file.is_open())
            _file.flush();
    }

    /* ========== 写日志接口 ========== */
    void log(const char *file, int line, Level lv, const std::string &original, bool isNewLine = true)
    {
        if (!_log_enabled || lv < _logLevel)
            return;

        // 锁外完成：时间缓存获取 & 格式化字符串拼装到 TLS 缓冲
        int ms_count = 0;
        std::tm cached_tm{};
        const char *time_c = nullptr;
        updateAndGetTimeCache_(cached_tm, ms_count, time_c);

        const std::string msg = maybeTruncate_(original);
        auto &formatted = tls_buf_();
        formatMessageFast_(lv, file, line, time_c, ms_count, msg, formatted);

        // 锁内：跨天切换 + 写出
        writeToTargets_(formatted, isNewLine, lv);
    }

    void logformat(const char *file, int line, Level lv, const char *fmt, ...)
    {
        if (!_log_enabled || lv < _logLevel)
            return;

        std::string s;
        va_list args;
        va_start(args, fmt);
#if defined(_WIN32)
        va_list cpy;
        va_copy(cpy, args);
        int need = _vscprintf(fmt, cpy); // 不含'\0'
        va_end(cpy);
        if (need < 0)
        {
            va_end(args);
            return;
        }
        std::vector<char> tmp(static_cast<size_t>(need) + 1u);
        int written = _vsnprintf_s(tmp.data(), tmp.size(), _TRUNCATE, fmt, args);
        va_end(args);
        if (written < 0)
            return;
        s.assign(tmp.data(), static_cast<size_t>(written));
#else
        std::vector<char> buf(1024);
        while (true)
        {
            va_list cpy;
            va_copy(cpy, args);
            int need = vsnprintf(buf.data(), buf.size(), fmt, cpy);
            va_end(cpy);
            if (need < 0)
            {
                va_end(args);
                return;
            }
            if ((size_t)need >= buf.size())
            {
                buf.resize(static_cast<size_t>(need) + 1);
                continue;
            }
            s.assign(buf.data(), static_cast<size_t>(need));
            break;
        }
        va_end(args);
#endif
        log(file, line, lv, s, _add_newline);
    }

    /* ========== 工具接口 ========== */
    static std::string get_module_path() { return platform_getModulePath_(); }
    std::string get_process_name() const { return platform_getProcessName_(); }

    void cleanupOldLogs(int daysToKeep = 5)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (!_outputToFile)
            return;

        size_t p = _baseName.find_last_of("\\/");
        std::string dir = (p != std::string::npos) ? _baseName.substr(0, p + 1) : platform_currentDirWithSlash_();
        std::string stem = (p != std::string::npos) ? _baseName.substr(p + 1) : _baseName;

        std::vector<std::string> files = platform_listLogFiles_(dir, stem);
        for (const auto &name : files)
        {
            if (shouldDeleteLog_(name, daysToKeep))
            {
                platform_deleteFile_(dir + name);
            }
        }
    }

private:
    ML_Logger()
        : _isRoll(false), _logLevel(Level::Debug), _outputToFile(true), _outputToScreen(true), _currentRollIndex(0), _maxRolls(5), _maxSizeInBytes(100u * 1024u * 1024u), _currentSize(0), _initialized(false), _message_only(false), _add_newline(true), _log_enabled(false), _log_screen_color(true), _default_file_name_day(true), _isCheckDay(false), _start_timestamp(), _last_log_ymd(0), _auto_flush(true), _cached_time_sec(0) // 兼容旧接口（当前不做跨线程共享）
          ,
          _need_day_switch(false), _error_handler(nullptr)
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

    /* ========== 时间缓存与跨天（锁外：TLS 缓存时间串；跨天仅置原子标记） ========== */
    void updateAndGetTimeCache_(std::tm &out_tm, int &out_ms, const char *&out_time_c)
    {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % std::chrono::seconds(1);
        out_ms = static_cast<int>(ms.count());

        const std::time_t t = clock::to_time_t(now);
        struct TLS
        {
            std::time_t sec;
            std::tm tm;
            char time_buf[32];
            TLS() : sec((std::time_t)-1), tm{} { time_buf[0] = '\0'; }
        };
        thread_local TLS tls;

        if (t != tls.sec)
        {
            tls.sec = t;
#if defined(_WIN32)
            localtime_s(&tls.tm, &t);
#else
            localtime_r(&t, &tls.tm);
#endif
            std::strftime(tls.time_buf, sizeof(tls.time_buf), "%Y-%m-%d %H:%M:%S", &tls.tm);

            if (_isCheckDay)
            {
                const int ymd = (tls.tm.tm_year + 1900) * 10000 + (tls.tm.tm_mon + 1) * 100 + tls.tm.tm_mday;
                int last = _last_log_ymd.load(std::memory_order_relaxed);
                if (last == 0)
                {
                    _last_log_ymd.store(ymd, std::memory_order_relaxed);
                }
                else if (last != ymd)
                {
                    _need_day_switch.store(true, std::memory_order_relaxed);
                    _last_log_ymd.store(ymd, std::memory_order_relaxed);
                }
            }
        }
        out_tm = tls.tm;
        out_time_c = tls.time_buf;
    }

    /* ========== 截断 ========== */
    std::string maybeTruncate_(const std::string &s) const
    {
        if (s.size() <= MAX_LOG_MESSAGE_SIZE)
            return s;
        std::string r = s.substr(0, MAX_LOG_MESSAGE_SIZE);
        r.append(TRUNCATED_MESSAGE);
        return r;
    }

    // 线程局部可复用缓冲，减少分配/拷贝
    static std::string &tls_buf_()
    {
        thread_local std::string buf;
        if (buf.capacity() < 1024)
            buf.reserve(1024);
        buf.clear();
        return buf;
    }

    /* ========== 级别字符串 ========== */
    static const char *levelToStringC_(Level lv)
    {
        switch (lv)
        {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Notice:
            return "NOTICE";
        case Level::Warning:
            return "WARNING";
        case Level::Error:
            return "ERROR";
        case Level::Critical:
            return "CRITICAL";
        case Level::Alert:
            return "ALERT";
        default:
            return "";
        }
    }

    /* ========== 快速格式化到 TLS 缓冲 ========== */
    void formatMessageFast_(Level lv, const char *file, int line,
                            const char *time_c, int ms_count,
                            const std::string &msg, std::string &out) const
    {
        if (_message_only)
        {
            out.assign(msg);
            return;
        }
#if defined(_WIN32)
        int need = _scprintf("%s.%03d %s [%s:%d] %s",
                             time_c, ms_count, levelToStringC_(lv), file, line, msg.c_str());
        if (need < 0)
        {
            out.clear();
            return;
        }
        out.resize(static_cast<size_t>(need));
        _snprintf(&out[0], out.size() + 1, "%s.%03d %s [%s:%d] %s",
                  time_c, ms_count, levelToStringC_(lv), file, line, msg.c_str());
#else
        int need = std::snprintf(nullptr, 0, "%s.%03d %s [%s:%d] %s",
                                 time_c, ms_count, levelToStringC_(lv), file, line, msg.c_str());
        if (need < 0)
        {
            out.clear();
            return;
        }
        out.resize(static_cast<size_t>(need));
        std::snprintf(&out[0], out.size() + 1, "%s.%03d %s [%s:%d] %s",
                      time_c, ms_count, levelToStringC_(lv), file, line, msg.c_str());
#endif
    }

    /* ========== 写目标（持锁；此处执行跨天） ========== */
    void writeToTargets_(const std::string &formatted, bool isNewLine, Level lv)
    {
        struct InLogGuard
        {
            InLogGuard() { ML_Logger::in_logging_flag_() = true; }
            ~InLogGuard() { ML_Logger::in_logging_flag_() = false; }
        } guard;
        std::lock_guard<std::mutex> lk(_mutex);

        if (_need_day_switch.exchange(false, std::memory_order_relaxed))
            onDayChangeLocked_();

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

    void writeToFile_(const std::string &s, bool isNewLine)
    {
        if (!_outputToFile)
            return;

        if (!_file.is_open())
        {
            rollFiles_(true);
            _initialized = true;
            if (!_file.is_open())
            {
                reportError_(std::string("Failed to open file for writing. Message: ") + s);
                return;
            }
        }

        const size_t msg_size = s.size() + (isNewLine ? 1u : 0u); // '\n' 1 字节
        if (_currentSize > 0 && (_currentSize + msg_size > _maxSizeInBytes))
            rollFiles_();

        _file.write(s.data(), s.size());
        if (isNewLine)
            _file.put('\n');
        if (_auto_flush)
            _file.flush();

        _currentSize += msg_size;
        if (_currentSize >= _maxSizeInBytes)
            rollFiles_();
    }

    /* ========== 控制台着色支持 ========== */
    static bool supportsAnsiColor_()
    {
        static bool inited = false;
        static bool cached = false;
        if (inited)
            return cached;
        inited = true;
#if defined(_WIN32)
        if (!_isatty(_fileno(stdout)))
        {
            cached = false;
            return cached;
        }
#if defined(_INC_WINDOWS) || defined(_WINDOWS_)
        {
            static bool initialized = false;
            static bool vt_enabled = false;
            if (!initialized)
            {
                initialized = true;
                HANDLE h = GetStdHandle((DWORD)-11);
                DWORD mode = 0;
                if (h && GetConsoleMode(h, &mode))
                {
                    if (SetConsoleMode(h, mode | MLLOG_VT_ENABLE))
                        vt_enabled = true;
                }
            }
            if (vt_enabled)
            {
                cached = true;
                return cached;
            }
        }
#endif
        {
            const char *wt = std::getenv("WT_SESSION");
            const char *term = std::getenv("TERM");
            cached = (wt != nullptr) || (term != nullptr);
            return cached;
        }
#else
        cached = ::isatty(STDOUT_FILENO);
        return cached;
#endif
    }

    void writeToScreen_(const std::string &s, bool isNewLine, Level lv)
    {
        const bool colorize = _log_screen_color && supportsAnsiColor_();
        if (colorize)
            std::cout << levelColorAnsi_(lv);
        std::cout << s;
        if (isNewLine)
            std::cout << '\n';
        if (colorize)
            std::cout << MLLOG_COLOR_RESET;
    }

    /* ========== 跨天切换（已持锁版本） ========== */
    void onDayChangeLocked_()
    {
        if (_file.is_open())
            _file.close();
        _initialized = false;
        _isRoll = false;
        _currentRollIndex = 0;
        _currentSize = 0;
        _start_timestamp = currentTimestamp_();
        _baseFullNameWithDateAndTime = _baseName + "_" + _start_timestamp;
    }

    /* ========== 文件滚动/打开（需持锁） ========== */
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
        _file.open(fn.str(), std::ofstream::out | (_isRoll ? std::ofstream::trunc : std::ofstream::app) | std::ofstream::binary);
        if (!_file.is_open())
        {
            reportError_(std::string("Failed to open new log file: ") + fn.str());
            _currentSize = 0;
            return;
        }
        _file.seekp(0, std::ios::end);
        std::streampos pos = _file.tellp();
        _currentSize = (pos >= 0) ? static_cast<size_t>(pos) : 0u;
    }

    /* ========== 时间/颜色帮助 ========== */
    std::string currentTimestamp_() const
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm lt{};
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

    const char *levelColorAnsi_(Level lv) const
    {
        switch (lv)
        {
        case Level::Debug:
            return MLLOG_COLOR_GREEN;
        case Level::Info:
            return MLLOG_COLOR_CYAN;
        case Level::Notice:
            return MLLOG_COLOR_BLUE;
        case Level::Warning:
            return MLLOG_COLOR_YELLOW;
        case Level::Error:
            return MLLOG_COLOR_RED;
        case Level::Critical:
            return MLLOG_COLOR_MAGENTA;
        case Level::Alert:
            return MLLOG_COLOR_WHITE;
        default:
            return MLLOG_EMPTY;
        }
    }

    /* ========== 错误报告 ========== */
    void reportError_(const std::string &m)
    {
        if (in_logging_flag_())
        {
            std::cerr << "MLLOG CRITICAL: " << m << std::endl;
            return;
        }
        auto h = _error_handler;
        if (h)
        {
            try
            {
                h(std::string("MLLOG INTERNAL: ") + m);
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

    /* ========== 平台封装 ========== */
    static bool platform_createDirectories_(const std::string &path)
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

    static std::string platform_getModulePath_()
    {
        char buf[1024] = {0};
#if defined(_WIN32)
        char *exe = nullptr;
        if (_get_pgmptr(&exe) == 0 && exe)
        {
            std::string p(exe);
            size_t pos = p.find_last_of("\\/");
            return (pos != std::string::npos) ? p.substr(0, pos) : ".";
        }
        if (_getcwd(buf, sizeof(buf)))
            return std::string(buf);
        return ".";
#else
        Dl_info info{};
        if (dladdr((void *)(&platform_getModulePath_), &info) && info.dli_fname)
        {
            std::string p(info.dli_fname);
            size_t pos = p.find_last_of('/');
            return (pos != std::string::npos) ? p.substr(0, pos) : ".";
        }
        if (getcwd(buf, sizeof(buf)) != nullptr)
            return std::string(buf);
        return ".";
#endif
    }

    static std::string platform_getProcessName_()
    {
        char buf[1024] = {0};
#if defined(_WIN32)
        char *exe = nullptr;
        if (_get_pgmptr(&exe) == 0 && exe)
        {
            std::string n(exe);
            size_t p = n.find_last_of("\\/");
            if (p != std::string::npos)
                n = n.substr(p + 1);
            p = n.rfind('.');
            if (p != std::string::npos)
                n = n.substr(0, p);
            return n;
        }
        return "unknown_process";
#else
        if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) > 0)
        {
            std::string n(buf);
            size_t p = n.find_last_of('/');
            if (p != std::string::npos)
                n = n.substr(p + 1);
            return n;
        }
        return "unknown_process";
#endif
    }

    static std::string platform_currentDirWithSlash_()
    {
        char buf[1024] = {0};
#if defined(_WIN32)
        if (_getcwd(buf, sizeof(buf)) != nullptr)
            return std::string(buf) + "\\";
#else
        if (getcwd(buf, sizeof(buf)) != nullptr)
            return std::string(buf) + "/";
#endif
        return std::string();
    }

    static std::vector<std::string> platform_listLogFiles_(const std::string &dir, const std::string &stem)
    {
        std::vector<std::string> out;
#if defined(_WIN32)
        std::string search = dir;
        if (!search.empty())
        {
            char back = search.back();
            if (back != '\\' && back != '/')
                search.push_back('\\');
        }
        search += stem + "_*";
        struct _finddata_t f;
        intptr_t h = _findfirst(search.c_str(), &f);
        if (h != -1)
        {
            do
            {
                if (!(f.attrib & _A_SUBDIR))
                    out.emplace_back(f.name);
            } while (_findnext(h, &f) == 0);
            _findclose(h);
        }
#else
        if (DIR *d = opendir(dir.c_str()))
        {
            while (dirent *e = readdir(d))
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
        std::sort(out.begin(), out.end());
        return out;
    }

    static void platform_deleteFile_(const std::string &full)
    {
        if (std::remove(full.c_str()) != 0)
        {
#if defined(_WIN32)
            std::cerr << "MLLOG CRITICAL: Failed to delete file: " << full << std::endl;
#else
            std::cerr << "MLLOG CRITICAL: Failed to delete file: " << full << ", errno=" << errno << std::endl;
#endif
        }
    }

    /* ========== 旧文件日期解析/保留策略 ========== */
    bool shouldDeleteLog_(const std::string &filename, int days) const
    {
        const std::time_t ft = parseDateFromFilename_(filename);
        if (ft == (time_t)-1)
            return false;
        const std::time_t now = std::time(nullptr);
        return std::difftime(now, ft) > double(days) * 24.0 * 3600.0;
    }

    std::time_t parseDateFromFilename_(const std::string &filename) const
    {
        if (filename.rfind(_baseNameWithoutPath + "_", 0) != 0)
            return (time_t)-1;
        std::string rest = filename.substr(_baseNameWithoutPath.length() + 1);
        if (rest.size() < 8)
            return (time_t)-1;
        std::string ymd = rest.substr(0, 8);
        for (char c : ymd)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return (time_t)-1;
        }
        std::tm tmv{};
        try
        {
            tmv.tm_year = std::stoi(ymd.substr(0, 4)) - 1900;
            tmv.tm_mon = std::stoi(ymd.substr(4, 2)) - 1;
            tmv.tm_mday = std::stoi(ymd.substr(6, 2));
            tmv.tm_isdst = -1;
        }
        catch (...)
        {
            return (time_t)-1;
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

    // 滚动状态
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

    // 时间缓存/跨天
    std::string _start_timestamp;
    std::atomic<int> _last_log_ymd;
    bool _auto_flush;
    std::time_t _cached_time_sec;
    std::atomic<bool> _need_day_switch;

    // 错误回调
    ErrorHandler _error_handler;

    // 递归/自锁防护（线程局部，兼容 C++14）
    static bool &in_logging_flag_()
    {
        thread_local bool flag = false;
        return flag;
    }
};

/* ========================= 日志流（宏兼容） ========================= */
class LoggerStream
{
public:
    LoggerStream(ML_Logger::Level lv, const char *file, int line) : _lv(lv), _file(file), _line(line) {}
    template <typename T>
    LoggerStream &operator<<(const T &v)
    {
        _ss << v;
        return *this;
    }
    template <typename T>
    LoggerStream &operator<<(T *p)
    {
        if (p)
            _ss << static_cast<const void *>(p);
        else
            _ss << "nullptr";
        return *this;
    }
    LoggerStream &operator<<(const char *s)
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
    const char *_file;
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

#define MLLOG_START()                                                          \
    do                                                                         \
    {                                                                          \
        ML_Logger::getInstance().setLogSwitch(true);                           \
        MLLOG(ML_Logger::Level::Alert) << "---------- Start MLLOG ----------"; \
    } while (0)

#define MLLOG_DEBUG MLLOG(ML_Logger::Level::Debug)
#define MLLOG_INFO MLLOG(ML_Logger::Level::Info)
#define MLLOG_NOTICE MLLOG(ML_Logger::Level::Notice)
#define MLLOG_WARNING MLLOG(ML_Logger::Level::Warning)
#define MLLOG_ERROR MLLOG(ML_Logger::Level::Error)
#define MLLOG_CRITICAL MLLOG(ML_Logger::Level::Critical)
#define MLLOG_ALERT MLLOG(ML_Logger::Level::Alert)

#define MLLOGD MLLOG(ML_Logger::Level::Debug)
#define MLLOGI MLLOG(ML_Logger::Level::Info)
#define MLLOGN MLLOG(ML_Logger::Level::Notice)
#define MLLOGW MLLOG(ML_Logger::Level::Warning)
#define MLLOGE MLLOG(ML_Logger::Level::Error)
#define MLLOGC MLLOG(ML_Logger::Level::Critical)
#define MLLOGA MLLOG(ML_Logger::Level::Alert)

#define mllogd MLLOG(ML_Logger::Level::Debug)
#define mllogi MLLOG(ML_Logger::Level::Info)
#define mllogn MLLOG(ML_Logger::Level::Notice)
#define mllogw MLLOG(ML_Logger::Level::Warning)
#define mlloge MLLOG(ML_Logger::Level::Error)
#define mllogc MLLOG(ML_Logger::Level::Critical)
#define mlloga MLLOG(ML_Logger::Level::Alert)

#define MLLOG_DEBUGF(fmt, ...) MLLOGF(ML_Logger::Level::Debug, fmt, ##__VA_ARGS__)
#define MLLOG_INFOF(fmt, ...) MLLOGF(ML_Logger::Level::Info, fmt, ##__VA_ARGS__)
#define MLLOG_NOTICEF(fmt, ...) MLLOGF(ML_Logger::Level::Notice, fmt, ##__VA_ARGS__)
#define MLLOG_WARNINGF(fmt, ...) MLLOGF(ML_Logger::Level::Warning, fmt, ##__VA_ARGS__)
#define MLLOG_ERRORF(fmt, ...) MLLOGF(ML_Logger::Level::Error, fmt, ##__VA_ARGS__)
#define MLLOG_CRITICALF(fmt, ...) MLLOGF(ML_Logger::Level::Critical, fmt, ##__VA_ARGS__)
#define MLLOG_ALERTF(fmt, ...) MLLOGF(ML_Logger::Level::Alert, fmt, ##__VA_ARGS__)

#endif // MLLOG_HPP