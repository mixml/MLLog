#ifndef MLLOG_HPP
#define MLLOG_HPP

/**
 * @file mllog.hpp
 * @brief 多功能、跨平台、单头文件日志系统（Turbo, Anywhere-Safe Start, Pattern）
 * @author malin
 *      - Email: zcyxml@163.com  mlin2@grgbanking.com
 *      - GitHub: https://github.com/mixml
 *
 * 变更摘要（关键）：
 * @version 2.9.1
 *      - Linux下增加自愈功能. Linux 自愈同时覆盖 unlink 与 rename+create；copytruncate 不重开（符合常见策略）。
 * @version 2.9.0
 *      - 增加setPattern
 *       时间： %Y %m %d %H %M %S（strftime 语法） %e → 毫秒（000–999）
 *       级别/元信息/源码：%l 短级别（DEBUG/INFO/…）, %L （大写，等价于 %l）, %n logger 名（即 Registry 的 name）, %P 进程 id, %t 线程 id（hash 后的无符号数）, %s 文件名（当前宏里是短文件名）,%# 行号
 *       内容： %v 日志正文, 颜色标记 %^ / %$ （先占位，当前实现忽略；仍沿用你已有的整行按级别上色策略）
 *       例："%Y-%m-%d %H:%M:%S.%e [%l] %n %s:%# | %v"
 * @version 2.8.3 (2025-09-23 Default-per-DSO + Console polish)
 *      - 默认日志前缀改为：<模块目录>/log/<模块名>_MLLOG（不同 DSO 默认不冲突）
 *      - Windows 颜色：使用 STD_OUTPUT_HANDLE；仅在 isatty 且成功启用 VT 时上色（去除环境变量兜底）
 *      - 清理：移除未用成员；rollFiles_() 统一无参
 * @version 2.8.2 (2025-09-23 Linux增加hidden)
 * @version 2.8.1 (2025-09-23 增加版本命名空间与 ML_NS 宏)
 * @version 2.8.0 (2025-09-23 Multi-Instance + Anywhere-Safe Polishing)
 *      - 架构：新增 ML_LoggerRegistry（命名实例，永不析构），支持按模块/子系统/DSO 独立日志。
 *      - 启动：Light 阶段也可上屏，仍不上盘，日志入内存等待 Full 回放；Promote 增强递归/死锁防护。
 *      - 控制台：全局互斥避免并发交叉；supportsAnsiColor_ 用 call_once 缓存；Windows 优先启用 VT。
 *      - 平台：Windows 通过 GetModuleHandleExA 精确定位当前模块目录；Nix 文件遍历更健壮。
 *      - 性能/可靠性：沿用 Turbo I/O（1MB 缓冲 + 合并写）；回放失败不丢 pending；清理/日期解析更鲁棒。
 * @version 2.7.0 (2025-09-06 Anywhere-Safe Start)
 *      - 新增“阶段化初始化”机制：Off → Light → Full MLLOG_START() 仅进入 Light：只改内存状态并把“Start MLLOG”与后续日志写入内存环形缓冲，不做任何 I/O。
 *        promoteToFull() 在安全时机（非 DllMain）执行：创建目录/打开文件、一次性回放缓冲、进入 Full 正常写日志。
 *        默认在首次“非受限”日志调用时也会尝试自动转正（若失败则继续 Light 缓存，不崩溃）。
 *      - setLogFile() 不再创建目录；目录创建挪到真正 open 文件时（rollFiles_）执行。
 *      - Windows GUI 进程下 supportsAnsiColor_() 更稳健（先检查 _fileno(stdout)），避免潜在异常。
 * @version 2.6.6 (2025-09-06 Turbo-IO + DLL-Safe Init)
 *      - 健壮：将目录创建从 setLogFile() 延后到 rollFiles_()，避免 DllMain 期间触发文件系统操作。
 *      - 健壮：supportsAnsiColor_() 在 Windows GUI 进程中先检查 _fileno(stdout) 是否有效，避免潜在崩溃。
 *      - 体验：新增 MLLOG_START_LIGHT()/MLLOG_EMIT_START_BANNER() 两阶段启动宏；保留 MLLOG_START()。
 * @version 2.6.5 (2025-08-31 Turbo-IO)
 *      - 性能：替换 ofstream，Windows 使用 _sopen_s/_write + 1MB 用户缓冲；Nix 使用 FILE* + setvbuf(1MB)。
 *      - 性能：文件-only 路径将“前缀+正文+换行”合并为一次写，减少系统调用（Windows 收益显著）。
 *      - 可靠性：写/刷失败检测与 bad() 状态；可选强刷到磁盘（MLLOG_DURABLE_FLUSH）。
 * @version 2.6.4 (2025-08-31 Turbo-IO)
 *      - 性能：替换ofstream和stringstream提升写日志速度。
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
 * @subsection basic_usage 基础用法（默认实例 + Pattern）
 * @code
 * #include "mllog.hpp"
 * #include <vector>
 * #include <string>
 * using namespace ML_NS;
 * int main() {
 *     // 1) Anywhere-Safe 启动（仅 Light，不上盘；可上屏）
 *     MLLOG_START();
 *
 *     // 2) 配置（建议在 Promote 前完成）
 *     auto& logger = ML_Logger::get();           // 默认实例
 *     logger.setLogSwitch(true);
 *     logger.setLogFile("./logs/my_app", 5, 10 * 1024 * 1024); // 路径前缀/滚动数/单文件上限
 *     logger.setLevel(ML_Logger::Level::Debug);
 *     logger.setOutput(true, true);              // 文件 + 控制台
 *     logger.setScreenColor(true);               // 控制台颜色（多线程已串行化输出）
 *
 *     // 可选：自定义 Pattern（含毫秒/级别/logger名/短文件/行/函数/正文）
 *     // 说明：%e=毫秒，%l 级别，%n 实例名，%s 短文件，%g 全路径文件，%# 行号，%! 函数名，%v 正文
 *     logger.setPattern("%Y-%m-%d %H:%M:%S.%e [%l] %n %s:%# %! | %v");
 *
 *     // 3) 在安全时机转正到 Full（开始正常落盘，自动回放 Light 期间的 pending）
 *     MLLOG_PROMOTE_TO_FULL();
 *
 *     // 4) 流式日志
 *     MLLOG_INFO  << "程序启动，版本: " << 2.8;
 *     std::string user = "malin";
 *     MLLOG_DEBUG << "正在为用户 " << user << " 处理请求...";
 *     std::vector<int> data = {1, 2, 3};
 *     MLLOG_WARNING << "检测到数据异常，大小: " << data.size();
 *
 *     // 5) printf 风格
 *     MLLOG_ERRORF("处理失败，错误码: %d, 错误信息: %s", 1001, "文件未找到");
 *     MLLOG_CRITICALF("系统关键组件 '%s' 无响应!", "DatabaseConnector");
 *
 *     // 6) 可随时回退到默认前缀（清空 pattern 即可）
 *     logger.setPattern("");
 *     MLLOG_INFO << "已回退到默认前缀格式。";
 *
 *     // 7) 结束前可记录 Stop 行，并手动刷盘
 *     MLLOG_ALERT << "---------- Stop MLLOG ----------";
 *     logger.flush();
 *     return 0;
 * }
 * @endcode
 *
 * @subsection advanced_usage 进阶配置（性能/跨天/清理/滚动/并发稳定）
 * @code
 * #include "mllog.hpp"
 * #include <iostream>
 * using namespace ML_NS;
 *
 * // 自定义库内部错误处理器（可用于上报）
 * static void on_log_internal_error(const std::string& msg) {
 *     std::cerr << "!!! MLLOG INTERNAL: " << msg << std::endl;
 * }
 *
 * int main() {
 *     auto& L = ML_Logger::get();
 *
 *     // 1) 启动前设置错误回调（推荐）
 *     L.setErrorHandler(on_log_internal_error);
 *
 *     // 2) Anywhere-Safe 启动（Light）
 *     MLLOG_START();
 *
 *     // 3) 高级配置
 *     L.setLogFile("./logs/perf_test/perf", 5,  64 * 1024 * 1024);
 *     L.setLevel(ML_Logger::Level::Info);
 *     L.setAutoFlush(false);   // 高频写建议关闭自动刷盘
 *     L.setCheckDay(true);     // 跨天自动切换新日志
 *     L.setOutput(true, true); // 文件 + 控制台
 *     L.setPattern("%Y-%m-%d %H:%M:%S.%e [%l] %n %g:%# %! | %v"); // 用全路径文件名
 *
 *     // 4) 转正（Full）
 *     MLLOG_PROMOTE_TO_FULL();
 *
 *     // 5) 高频写测试（流式 + printf）
 *     for (int i = 0; i < 1000; ++i) {
 *         MLLOG_INFO << "Processing item " << i;
 *         if ((i % 200) == 0) {
 *             MLLOG_WARNINGF("milestone i=%d", i);
 *         }
 *     }
 *
 *     // 6) 手动刷盘与清理
 *     L.flush();
 *     MLLOG_INFO << "所有性能测试日志已刷新到磁盘。";
 *     L.cleanupOldLogs(7);
 *     MLLOG_INFO << "旧日志清理完成。";
 *     return 0;
 * }
 * @endcode
 *
 * @subsection named_usage 命名实例用法（每个 SO/子系统独立日志 + Pattern）
 * @code
 * #include "mllog.hpp"
 * using namespace ML_NS;
 * // 假设为某个动态库/子系统定义独立实例名
 * #define VISION_LOG "vision"
 *
 * void vision_init() {
 *     // 1) Light 启动（不上盘，可上屏）
 *     MLLOG_START_NAMED(VISION_LOG);
 *
 *     // 2) 定制 vision 实例的日志根与策略
 *     auto& LV = ML_Logger::get(VISION_LOG);
 *     LV.setLogSwitch(true);
 *     LV.setLogFile("./logs/vision/vision", 7, 50 * 1024 * 1024);
 *     LV.setOutput(true, true);
 *     LV.setScreenColor(true);
 *     LV.setPattern("%Y-%m-%d %H:%M:%S.%e [%l] %n %s:%# %! | %v"); // 短文件 + 行 + 函数
 *
 *     // 3) 安全时机转正
 *     MLLOG_PROMOTE_TO_FULL_NAMED(VISION_LOG);
 *
 *     // 4) 业务日志（流式 + printf 风格）
 *     MLLOG_INFO_NAMED(VISION_LOG) << "vision subsystem ready";
 *     MLLOG_WARNINGF_NAMED(VISION_LOG, "fps 低: %.2f", 23.7);
 *
 *     // 5) 任意时机切换 Pattern 或回退默认前缀
 *     LV.setPattern("%Y-%m-%d %H:%M:%S.%e [%l] %n %g:%# %! | %v"); // 改用全路径文件
 *     MLLOG_INFO_NAMED(VISION_LOG) << "pattern switched.";
 *     LV.setPattern(""); // 清空 => 用默认前缀
 *     MLLOG_INFO_NAMED(VISION_LOG) << "back to default prefix.";
 *
 *     // 6) 需要时手动刷盘
 *     MLLOG_FORCE_FLUSH_NAMED(VISION_LOG);
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
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread> // [NEW] 线程id散列
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#include <windows.h>
#pragma warning(disable : 4996)
#else
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
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

/* 可选强刷到磁盘（默认关） */
#ifndef MLLOG_DURABLE_FLUSH
#define MLLOG_DURABLE_FLUSH 0
#endif

#if defined(_WIN32)
#ifdef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define MLLOG_VT_ENABLE ENABLE_VIRTUAL_TERMINAL_PROCESSING
#else
#define MLLOG_VT_ENABLE 0x0004u
#endif
#endif

/* Linux/Clang/GCC：默认隐藏符号（避免跨 DSO 符号冲突）；MSVC 无影响 */
#if !defined(_WIN32)
#pragma GCC visibility push(hidden)
#endif

/* ========================= 命名空间：当前版本 ========================= */
namespace mllog_v291
{
    inline namespace v2_9_1
    {
        /* ---------- constexpr 工具 ---------- */
        constexpr const char* mllog_find_last_slash_helper(const char* s, const char* last)
        {
            return (*s == '\0') ? (last ? last : s)
                                : mllog_find_last_slash_helper(
                                      s + 1, ((*s == '/' || *s == '\\') && s[1] != '\0') ? (s + 1) : last);
        }
        constexpr const char* mllog_file_name_from_path(const char* path) { return mllog_find_last_slash_helper(path, path); }

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

#ifndef ML_DEPRECATED
#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(deprecated)
#define ML_DEPRECATED(msg) [[deprecated(msg)]]
#else
#if defined(_MSC_VER)
#define ML_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__) || defined(__clang__)
#define ML_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#define ML_DEPRECATED(msg)
#endif
#endif
#else
#if defined(_MSC_VER)
#define ML_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__) || defined(__clang__)
#define ML_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#define ML_DEPRECATED(msg)
#endif
#endif
#endif

        template <class T>
        ML_NODISCARD ML_ALWAYS_INLINE constexpr const T& ml_max(const T& a, const T& b) noexcept { return (a < b) ? b : a; }

        /* ============= 轻量 ofstream 替代（略同你现有实现） ============= */
        class ML_FastOFStream
        {
        public:
            ML_FastOFStream()
#if defined(_WIN32)
                : fd_(-1), len_(0), failed_(false), buf_(1 << 20)
#else
                : fp_(nullptr), failed_(false)
#endif
            {
            }
            ~ML_FastOFStream() { close(); }

            ML_FastOFStream(const ML_FastOFStream&) = delete;
            ML_FastOFStream& operator=(const ML_FastOFStream&) = delete;

            void open(const std::string& path, std::ios::openmode mode)
            {
                close();
                failed_ = false;
#if defined(_WIN32)
                int flags = _O_WRONLY | _O_BINARY | _O_CREAT;
                if (mode & std::ios::trunc)
                    flags |= _O_TRUNC;
                else if (mode & std::ios::app)
                    flags |= _O_APPEND;
                int pmode = _S_IREAD | _S_IWRITE;
                if (_sopen_s(&fd_, path.c_str(), flags, _SH_DENYNO, pmode) != 0)
                {
                    fd_ = -1;
                    failed_ = true;
                    return;
                }
                len_ = 0;
#else
                const bool trunc = (mode & std::ios::trunc) != 0;
                const bool app = (mode & std::ios::app) != 0;
                const char* m = trunc ? "wb" : (app ? "ab" : "wb");
                fp_ = ::fopen(path.c_str(), m);
                if (fp_)
                    (void)::setvbuf(fp_, nullptr, _IOFBF, 1 << 20);
                failed_ = (fp_ == nullptr);
#endif
            }

            bool is_open() const
            {
#if defined(_WIN32)
                return fd_ != -1;
#else
                return fp_ != nullptr;
#endif
            }

            void write(const char* data, size_t n)
            {
                if (n == 0)
                    return;
#if defined(_WIN32)
                if (fd_ == -1)
                {
                    failed_ = true;
                    return;
                }
                if (n < (buf_.size() >> 1))
                {
                    if (len_ + n > buf_.size())
                        flush_buffer_();
                    if (n > buf_.size())
                    {
                        size_t off = 0;
                        while (off < n)
                        {
                            unsigned chunk = (unsigned)std::min<size_t>(n - off, 1 << 20);
                            int w = _write(fd_, data + off, chunk);
                            if (w != (int)chunk)
                            {
                                failed_ = true;
                                break;
                            }
                            off += (size_t)w;
                        }
                        return;
                    }
                    std::memcpy(buf_.data() + len_, data, n);
                    len_ += n;
                }
                else
                {
                    flush_buffer_();
                    size_t off = 0;
                    while (off < n)
                    {
                        unsigned chunk = (unsigned)std::min<size_t>(n - off, 1 << 20);
                        int w = _write(fd_, data + off, chunk);
                        if (w != (int)chunk)
                        {
                            failed_ = true;
                            break;
                        }
                        off += (size_t)w;
                    }
                }
#else
                if (!fp_)
                {
                    failed_ = true;
                    return;
                }
                size_t w = ::fwrite(data, 1, n, fp_);
                if (w != n)
                    failed_ = true;
#endif
            }

            void put(char c)
            {
#if defined(_WIN32)
                if (fd_ == -1)
                {
                    failed_ = true;
                    return;
                }
                if (len_ == buf_.size())
                    flush_buffer_();
                buf_[len_] = c;
                ++len_;
#else
                if (!fp_)
                {
                    failed_ = true;
                    return;
                }
                if (::fputc((unsigned char)c, fp_) == EOF)
                    failed_ = true;
#endif
            }

            void flush()
            {
#if defined(_WIN32)
                if (fd_ == -1)
                    return;
                flush_buffer_();
#if MLLOG_DURABLE_FLUSH
                if (_commit(fd_) != 0)
                    failed_ = true;
#endif
#else
                if (!fp_)
                    return;
                if (::fflush(fp_) != 0)
                    failed_ = true;
#if MLLOG_DURABLE_FLUSH
                if (::fsync(::fileno(fp_)) != 0)
                    failed_ = true;
#endif
#endif
            }

            void close()
            {
#if defined(_WIN32)
                if (fd_ != -1)
                {
                    flush_buffer_();
                    _close(fd_);
                    fd_ = -1;
                }
#else
                if (fp_)
                {
                    ::fclose(fp_);
                    fp_ = nullptr;
                }
#endif
            }

            void seekp(long long off, std::ios_base::seekdir dir)
            {
#if defined(_WIN32)
                if (fd_ == -1)
                    return;
                flush_buffer_();
                int whence = (dir == std::ios_base::beg) ? SEEK_SET : (dir == std::ios_base::cur) ? SEEK_CUR
                                                                                                  : SEEK_END;
                (void)_lseeki64(fd_, off, whence);
#else
                if (!fp_)
                    return;
                int whence = (dir == std::ios_base::beg) ? SEEK_SET : (dir == std::ios_base::cur) ? SEEK_CUR
                                                                                                  : SEEK_END;
                ::fseeko(fp_, (off_t)off, whence);
#endif
            }

            std::streampos tellp()
            {
#if defined(_WIN32)
                if (fd_ == -1)
                    return std::streampos(-1);
                __int64 pos = _telli64(fd_);
                return (pos >= 0) ? std::streampos(pos) : std::streampos(-1);
#else
                if (!fp_)
                    return std::streampos(-1);
                off_t pos = ::ftello(fp_);
                return (pos >= 0) ? std::streampos(pos) : std::streampos(-1);
#endif
            }

            bool bad() const { return failed_; }
            void clear_bad() { failed_ = false; }
#ifndef _WIN32
            int native_fileno() const { return fp_ ? ::fileno(fp_) : -1; }
#endif
        private:
#if defined(_WIN32)
            void flush_buffer_()
            {
                size_t off = 0;
                while (off < len_)
                {
                    unsigned chunk = (unsigned)std::min<size_t>(len_ - off, 1 << 20);
                    int w = _write(fd_, buf_.data() + off, chunk);
                    if (w != (int)chunk)
                    {
                        failed_ = true;
                        break;
                    }
                    off += (size_t)w;
                }
                len_ = 0;
            }
            int fd_;
            size_t len_;
            bool failed_;
            std::vector<char> buf_;
#else
            FILE* fp_;
            bool failed_;
#endif
        };

        /* ======================= Registry 前向声明 ======================= */
        class ML_Logger;

        class ML_LoggerRegistry
        {
        public:
            static ML_LoggerRegistry& getInstance();
            ML_Logger& get(const std::string& name);
            ~ML_LoggerRegistry();

        private:
            ML_LoggerRegistry() = default;
            ML_LoggerRegistry(const ML_LoggerRegistry&) = delete;
            ML_LoggerRegistry& operator=(const ML_LoggerRegistry&) = delete;

            std::mutex _mutex;
            std::map<std::string, std::unique_ptr<ML_Logger>> _loggers;
        };

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
            using ErrorHandler = std::function<void(const std::string&)>;

            static const size_t MAX_LOG_MESSAGE_SIZE = 1024u * 1024u * 5u;
            static constexpr const char* TRUNCATED_MESSAGE = "\n... [Message Truncated]";
            enum class Phase : int
            {
                Off = 0,
                Light = 1,
                Full = 2
            };
            static ML_Logger& get(const std::string& name = "default")
            {
                return ML_LoggerRegistry::getInstance().get(name);
            }
            static ML_Logger& getInstance() { return get("default"); }
            // [CHG]：带 name 的构造；Registry 会传入
            ML_Logger(const std::string& name = "default")
                : _name(name),
                  _isRoll(false), _logLevel(Level::Debug),
                  _outputToFile(true), _outputToScreen(true),
                  _currentRollIndex(0), _maxRolls(5),
                  _maxSizeInBytes(100u * 1024u * 1024u), _currentSize(0),
                  _initialized(false), _message_only(false), _add_newline(true),
                  _log_enabled(false), _log_screen_color(true),
                  _default_file_name_day(true), _isCheckDay(false),
                  _start_timestamp(), _last_log_ymd(0), _auto_flush(true),
                  _need_day_switch(false), _error_handler(nullptr),
                  _pending_bytes(0), _phase((int)Phase::Off),
                  _has_pattern(false)
            {
                std::string def = get_module_path() + "/log/" + platform_getModuleBasename_() + "_MLLOG";
                setLogFile(def, _maxRolls, _maxSizeInBytes);
            }

            ~ML_Logger()
            {
                try
                {
                    flush();
                }
                catch (...)
                {
                }
                try
                {
                    if (_file.is_open())
                        _file.close();
                }
                catch (...)
                {
                }
            }

            ML_Logger(const ML_Logger&) = delete;
            ML_Logger& operator=(const ML_Logger&) = delete;

            /* 配置接口 */
            void setLevel(Level lv) { _logLevel = lv; }
            void setCheckDay(bool on) { _isCheckDay = on; }
            void setOutput(bool toFile, bool toScreen)
            {
                _outputToFile = toFile;
                _outputToScreen = toScreen;
            }
            void setAddNewLine(bool add) { _add_newline = add; }
            bool getAddNewLine() const { return _add_newline; }
            void setDefaultLogFormat(bool dateOnly) { _default_file_name_day = dateOnly; }
            void setMessageOnly(bool msgOnly) { _message_only = msgOnly; }
            void setScreenColor(bool on) { _log_screen_color = on; }
            void setAutoFlush(bool enabled) { _auto_flush = enabled; }
            void setErrorHandler(ErrorHandler h) { _error_handler = std::move(h); }

            void setLogSwitch(bool on)
            {
                _log_enabled = on;
                if (on && phase() == Phase::Off)
                    setPhase_(Phase::Light);
            }
            bool getLogSwitch() const { return _log_enabled; }

            // ---------- Anywhere-Safe 启停 ----------
            void startAnywhere(bool emit_banner = true)
            {
                std::lock_guard<std::mutex> lk(_mutex);
                _log_enabled = true;
                if (phase() == Phase::Off)
                    setPhase_(Phase::Light);
                if (emit_banner)
                    enqueueStartBanner_NoIO_UnsafeLocked_();
            }

            void promoteToFull()
            {
                struct InLogGuard
                {
                    InLogGuard() { ML_Logger::in_logging_flag_() = true; }
                    ~InLogGuard() { ML_Logger::in_logging_flag_() = false; }
                } guard;
                std::lock_guard<std::mutex> lk(_mutex);
                if (phase() == Phase::Full)
                    return;

                if (!_outputToFile)
                {
                    setPhase_(Phase::Full);
                    _pending_bytes = 0;
                    _pending.clear();
                    return;
                }

                if (!_initialized)
                {
                    rollFiles_();
                    _initialized = true;
                }
                else if (!_file.is_open())
                {
                    rollFiles_();
                }

                if (!_file.is_open())
                {
                    reportError_("promoteToFull(): open log file failed, stay in Light.");
                    return;
                }

                for (const auto& line : _pending)
                {
                    const size_t sz = line.size();
                    if (_currentSize > 0 && (_currentSize + sz > _maxSizeInBytes))
                        rollFiles_();
                    _file.write(line.data(), sz);
                    if (_auto_flush)
                        _file.flush();
                    if (_file.bad())
                    {
                        reportError_("promoteToFull(): flush pending failed; keeping pending.");
                        _file.close();
                        _initialized = false;
                        return;
                    }
                    _currentSize += sz;
                    if (_currentSize >= _maxSizeInBytes)
                        rollFiles_();
                }
                _pending_bytes = 0;
                _pending.clear();
                setPhase_(Phase::Full);
            }

            void setLogFile(const std::string& baseName, int maxRolls = 5, size_t maxSizeInBytes = 100u * 1024u * 1024u)
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

                _start_timestamp = currentTimestamp_();
                _baseFullNameWithDateAndTime = _baseName + "_" + _start_timestamp;
                if (_file.is_open())
                    _file.close();
                _curFilePath.clear();
                _heal_counter = 0;
            }

            void flush()
            {
                std::lock_guard<std::mutex> lk(_mutex);
                if (_file.is_open())
                    _file.flush();
            }

            // [CHG]：log / logformat 现在额外携带 fullpath 与 func；pattern 可用 %g / %!
            void log(const char* file_short, const char* file_full, const char* func, int line,
                     Level lv, const std::string& original, bool isNewLine = true)
            {
                if (!_log_enabled || lv < _logLevel)
                    return;

                int ms_count = 0;
                std::tm cached_tm{};
                const char* time_c = nullptr;
                updateAndGetTimeCache_(cached_tm, ms_count, time_c);

                const std::string msg = maybeTruncate_(original);
                // 计算有效结尾（零分配）
                size_t end = msg.size();
                while (end > 0)
                {
                    char c = msg[end - 1];
                    if (c == '\n' || c == '\r')
                        --end;
                    else
                        break;
                }
                bool needNewLine = isNewLine;
                // -------------------------------------------------------------------
                // Light 阶段：上屏 + 入 pending
                if (phase() != Phase::Full)
                {
                    std::string linebuf;
                    if (!_message_only)
                    {
                        if (_has_pattern.load(std::memory_order_acquire))
                        {
                            std::lock_guard<std::mutex> lk(_mutex); // 复用已有互斥量
                            if (_has_pattern.load(std::memory_order_relaxed) && !_pat_ops.empty())
                            {
                                renderPattern_(cached_tm, ms_count, lv, file_short, file_full, func, line, msg, linebuf);
                            }
                            else
                            {
                                char prefix[192];
                                int plen = buildPrefix_(prefix, sizeof(prefix), lv, file_short, line, time_c, ms_count);
                                if (plen > 0)
                                    linebuf.append(prefix, (size_t)plen);
                                linebuf.append(msg);
                            }
                        }
                        else
                        {
                            char prefix[192];
                            int plen = buildPrefix_(prefix, sizeof(prefix), lv, file_short, line, time_c, ms_count);
                            if (plen > 0)
                                linebuf.append(prefix, (size_t)plen);
                            linebuf.append(msg);
                        }
                    }
                    else
                    {
                        linebuf.append(msg);
                    }
                    if (isNewLine)
                        linebuf.push_back('\n');

                    {
                        std::lock_guard<std::mutex> lk(_mutex);
                        if (_outputToScreen)
                        {
                            const bool colorize = _log_screen_color && supportsAnsiColor_();
                            std::lock_guard<std::mutex> cg(console_mutex_());
                            if (colorize)
                                std::cout << levelColorAnsi_(lv);
                            std::cout.write(linebuf.data(), (std::streamsize)linebuf.size());
                            if (colorize)
                                std::cout << MLLOG_COLOR_RESET;
                        }
                        enqueuePendingLine_NoIO_UnsafeLocked_(std::move(linebuf));
                    }

                    tryAutoPromoteToFull_NoThrow_();
                    return;
                }

                // Full 阶段
                auto& formatted = tls_buf_();
                if (_message_only)
                {
                    formatted.assign(msg);
                }
                else if (_has_pattern.load(std::memory_order_acquire))
                {
                    std::lock_guard<std::mutex> lk(_mutex); // 防止与 setPattern() 并发
                    if (_has_pattern.load(std::memory_order_relaxed) && !_pat_ops.empty())
                    {
                        renderPattern_(cached_tm, ms_count, lv, file_short, file_full, func, line, msg, formatted);
                    }
                    else
                    {
                        formatMessageFast_DefaultPrefix_(lv, file_short, line, time_c, ms_count, msg, formatted);
                    }
                }
                else
                {
                    formatMessageFast_DefaultPrefix_(lv, file_short, line, time_c, ms_count, msg, formatted);
                }
                writeToTargets_(formatted, isNewLine, lv);
            }

            void logformat(const char* file_short, const char* file_full, const char* func, int line,
                           Level lv, const char* fmt, ...)
            {
                if (!_log_enabled || lv < _logLevel)
                    return;
                std::string s;
                va_list args;
                va_start(args, fmt);
                std::vector<char> buf(256);
                while (true)
                {
                    va_list cpy;
                    va_copy(cpy, args);
#if defined(_WIN32)
                    int need = _vsnprintf(buf.data(), buf.size(), fmt, cpy);
#else
                    int need = vsnprintf(buf.data(), buf.size(), fmt, cpy);
#endif
                    va_end(cpy);
                    if (need < 0)
                    {
                        buf.resize(buf.size() * 2);
                        continue;
                    }
                    if ((size_t)need >= buf.size())
                    {
                        buf.resize((size_t)need + 1);
                        continue;
                    }
                    s.assign(buf.data(), (size_t)need);
                    break;
                }
                va_end(args);
                log(file_short, file_full, func, line, lv, s, _add_newline);
            }

            // 工具：路径/进程名
            static std::string get_module_path() { return platform_getModulePath_(); }
            static std::string get_module_basename() { return platform_getModuleBasename_(); }
            static std::string process_name() { return platform_getProcessName_(); }
            ML_DEPRECATED("Use ML_Logger::process_name() (static) instead")
            std::string get_process_name() const { return ML_Logger::process_name(); }

            // 清理旧日志
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
                    if (shouldDeleteLog_(name, daysToKeep))
                        platform_deleteFile_(dir + name);
            }

            // --------- Pattern API（新增）---------
            void setPattern(const std::string& pattern)
            {
                std::lock_guard<std::mutex> lk(_mutex);
                _pattern_raw = pattern;
                _pat_ops.clear();
                _has_pattern.store(compilePattern_(pattern, _pat_ops), std::memory_order_release);
            }
            std::string getPattern()
            {
                std::lock_guard<std::mutex> lk(_mutex);
                return _pattern_raw;
            }
            std::string name() const { return _name; }

            void setHealCheckEvery(int n)
            {
                std::lock_guard<std::mutex> lk(_mutex);
                _heal_every = (n > 0 ? n : 0);
                _heal_counter = 0;
            }

        private:
            /* -------- 运行时状态 & 内部函数（保留原有结构，略去未改动的注释） -------- */
            enum class Phase_AtomicTag
            {
            };
            Phase phase() const { return (Phase)_phase.load(std::memory_order_acquire); }
            void setPhase_(Phase p) { _phase.store((int)p, std::memory_order_release); }

            void enqueuePendingLine_NoIO_UnsafeLocked_(std::string&& line)
            {
                _pending_bytes += line.size();
                _pending.emplace_back(std::move(line));
                while (_pending_bytes > PENDING_MAX_BYTES || _pending.size() > PENDING_MAX_COUNT)
                {
                    _pending_bytes -= _pending.front().size();
                    _pending.pop_front();
                }
            }

            void enqueueStartBanner_NoIO_UnsafeLocked_()
            {
                int ms = 0;
                std::tm tm{};
                const char* tc = nullptr;
                updateAndGetTimeCache_(tm, ms, tc);
                std::string line;

                if (_has_pattern)
                {
                    renderPattern_(tm, ms, Level::Alert, "mllog.hpp", "mllog.hpp", "?", 0, "---------- Start MLLOG ----------", line);
                }
                else
                {
                    char prefix[192];
                    int plen = buildPrefix_(prefix, sizeof(prefix), Level::Alert, "mllog.hpp", 0, tc, ms);
                    if (plen > 0)
                        line.append(prefix, (size_t)plen);
                    line.append("---------- Start MLLOG ----------");
                }
                if (_add_newline)
                    line.push_back('\n');
                enqueuePendingLine_NoIO_UnsafeLocked_(std::move(line));
            }

            void tryAutoPromoteToFull_NoThrow_()
            {
                if (phase() != Phase::Light)
                    return;
                std::lock_guard<std::mutex> lk(_mutex);
                if (phase() != Phase::Light)
                    return;

                if (!_outputToFile)
                {
                    setPhase_(Phase::Full);
                    _pending_bytes = 0;
                    _pending.clear();
                    return;
                }
                if (!_initialized || !_file.is_open())
                {
                    rollFiles_();
                    _initialized = true;
                }
                if (!_file.is_open())
                    return;

                for (const auto& line : _pending)
                {
                    const size_t sz = line.size();
                    if (_currentSize > 0 && (_currentSize + sz > _maxSizeInBytes))
                        rollFiles_();
                    _file.write(line.data(), sz);
                    if (_auto_flush)
                        _file.flush();
                    if (_file.bad())
                    {
                        _file.close();
                        _initialized = false;
                        return;
                    }
                    _currentSize += sz;
                    if (_currentSize >= _maxSizeInBytes)
                        rollFiles_();
                }
                _pending_bytes = 0;
                _pending.clear();
                setPhase_(Phase::Full);
            }

            void updateAndGetTimeCache_(std::tm& out_tm, int& out_ms, const char*& out_time_c)
            {
                using clock = std::chrono::system_clock;
                auto now = clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % std::chrono::seconds(1);
                out_ms = (int)ms.count();
                const std::time_t t = clock::to_time_t(now);
                struct TLS
                {
                    std::time_t sec;
                    std::tm tm;
                    char time_buf[32];
                    TLS() : sec((time_t)-1), tm{} { time_buf[0] = '\0'; }
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
                            _last_log_ymd.store(ymd, std::memory_order_relaxed);
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

            static int buildPrefix_(char* buf, size_t cap, Level lv, const char* file_short, int line, const char* time_c, int ms)
            {
#if defined(_WIN32)
                return _snprintf(buf, (unsigned)cap, "%s.%03d %s [%s:%d] ", time_c, ms, levelToStringC_(lv), file_short, line);
#else
                return std::snprintf(buf, cap, "%s.%03d %s [%s:%d] ", time_c, ms, levelToStringC_(lv), file_short, line);
#endif
            }

            std::string maybeTruncate_(const std::string& s) const
            {
                if (s.size() <= MAX_LOG_MESSAGE_SIZE)
                    return s;
                std::string r = s.substr(0, MAX_LOG_MESSAGE_SIZE);
                r.append(TRUNCATED_MESSAGE);
                return r;
            }

            static std::string& tls_buf_()
            {
                thread_local std::string buf;
                if (buf.capacity() < 256)
                    buf.reserve(256);
                buf.clear();
                return buf;
            }

            static const char* levelToStringC_(Level lv)
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

            // 默认前缀路径（未设置 pattern 时）
            void formatMessageFast_DefaultPrefix_(Level lv, const char* file_short, int line,
                                                  const char* time_c, int ms_count,
                                                  const std::string& msg, std::string& out) const
            {
                char prefix[192];
                int plen = buildPrefix_(prefix, sizeof(prefix), lv, file_short, line, time_c, ms_count);
                if (plen < 0)
                {
                    out.assign(msg);
                    return;
                }
                out.reserve((size_t)plen + msg.size());
                out.append(prefix, (size_t)plen);
                out.append(msg);
            }

            void writeToTargets_(const std::string& formatted, bool isNewLine, Level lv)
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
                    rollFiles_();
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

                // NEW: 周期性自愈（POSIX unlink 检测）
                maybeHealUnlinked_(); // <== 新增

                if (!_file.is_open())
                {
                    rollFiles_();
                    _initialized = true;
                    if (!_file.is_open())
                    {
                        reportError_(std::string("Failed to open file. Message: ") + s);
                        return;
                    }
                }

                const size_t msg_size = s.size() + (isNewLine ? 1u : 0u);
                if (_currentSize > 0 && (_currentSize + msg_size > _maxSizeInBytes))
                    rollFiles_();

                // 写入（一次 append）
                _file.write(s.data(), s.size());
                if (isNewLine)
                    _file.put('\n');

                // NEW: 写失败 → 尝试重开同一路径并重试一次
                if (_file.bad())
                {
                    // 保存要重试的数据
                    const char* p1 = s.data();
                    size_t len1 = s.size();
                    char nl = '\n';

                    _file.close();
                    if (_curFilePath.empty())
                    {
                        _initialized = false;
                        reportError_("Log write failed and no current path.");
                        return;
                    }

                    _file.open(_curFilePath, std::ios::out | std::ios::app | std::ios::binary);
                    if (_file.is_open())
                    {
                        _file.write(p1, len1);
                        if (isNewLine)
                            _file.put(nl);
                    }

                    if (_file.bad())
                    {
                        reportError_("Log write retry failed.");
                        _file.close();
                        _initialized = false; // 让下一次触发 rollFiles_()
                        return;
                    }
                }

                if (_auto_flush)
                    _file.flush();
                if (_file.bad())
                {
                    reportError_("Log flush failed.");
                    _file.close();
                    _initialized = false;
                    return;
                }

                _currentSize += msg_size;
                if (_currentSize >= _maxSizeInBytes)
                    rollFiles_();
            }

            static bool supportsAnsiColor_()
            {
                static std::once_flag flag;
                static bool cached = false;
                std::call_once(flag, []
                               {
#if defined(_WIN32)
                                   int fd = _fileno(stdout);
                                   if (fd < 0 || !_isatty(fd))
                                   {
                                       cached = false;
                                       return;
                                   }
                                   HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
                                   DWORD mode = 0;
                                   if (h && GetConsoleMode(h, &mode) && SetConsoleMode(h, mode | MLLOG_VT_ENABLE))
                                       cached = true;
                                   else
                                       cached = false;
#else
                                   cached = ::isatty(STDOUT_FILENO);
#endif
                               });
                return cached;
            }

            static std::mutex& console_mutex_()
            {
                static std::mutex m;
                return m;
            }

            void writeToScreen_(const std::string& s, bool isNewLine, Level lv)
            {
                const bool colorize = _log_screen_color && supportsAnsiColor_();
                std::lock_guard<std::mutex> g(console_mutex_());
                if (colorize)
                    std::cout << levelColorAnsi_(lv);
                std::cout << s;
                if (isNewLine)
                    std::cout << '\n';
                if (colorize)
                    std::cout << MLLOG_COLOR_RESET;
            }

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
                _curFilePath.clear();
                _heal_counter = 0;
            }

            void rollFiles_()
            {
                if (_baseName.empty())
                    return;
                size_t p = _baseName.find_last_of("\\/");
                if (p != std::string::npos)
                {
                    std::string dir = _baseName.substr(0, p);
                    platform_createDirectories_(dir);
                }
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
                _curFilePath = fn.str(); // <== 记录当前文件路径
                _file.open(_curFilePath, std::ios::out | (_isRoll ? std::ios::trunc : std::ios::app) | std::ios::binary);
                if (!_file.is_open())
                {
                    reportError_(std::string("Failed to open new log file: ") + _curFilePath);
                    _currentSize = 0;
                    return;
                }

                _file.seekp(0, std::ios::end);
                std::streampos pos = _file.tellp();
                _currentSize = (pos >= 0) ? (size_t)pos : 0u;
                _heal_counter = 0;
            }

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

            const char* levelColorAnsi_(Level lv) const
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

            void reportError_(const std::string& m)
            {
                if (in_logging_flag_())
                {
                    try
                    {
                        std::cerr << "MLLOG CRITICAL: " << m << std::endl;
                    }
                    catch (...)
                    {
                    }
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
                    try
                    {
                        std::cerr << "MLLOG CRITICAL: " << m << std::endl;
                    }
                    catch (...)
                    {
                    }
                }
            }

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

            static std::string platform_getModulePath_()
            {
                char buf[1024] = {0};
#if defined(_WIN32)
                HMODULE h = nullptr;
                if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                       reinterpret_cast<LPCSTR>(&platform_getModulePath_), &h))
                {
                    char path[MAX_PATH] = {0};
                    DWORD n = GetModuleFileNameA(h, path, MAX_PATH);
                    if (n > 0)
                    {
                        std::string p(path, n);
                        size_t pos = p.find_last_of("\\/");
                        return (pos != std::string::npos) ? p.substr(0, pos) : ".";
                    }
                }
                char* exe = nullptr;
                if (_get_pgmptr(&exe) == 0 && exe)
                {
                    std::string p(exe);
                    size_t pos = p.find_last_of("\\/");
                    return (pos != std::string::npos) ? p.substr(0, pos) : ".";
                }
                if (_getcwd(buf, (int)sizeof(buf)))
                    return std::string(buf);
                return ".";
#else
                Dl_info info{};
                if (dladdr((void*)(&platform_getModulePath_), &info) && info.dli_fname)
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

            static std::string platform_getModuleBasename_()
            {
#if defined(_WIN32)
                HMODULE h = nullptr;
                if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                       reinterpret_cast<LPCSTR>(&platform_getModuleBasename_), &h))
                {
                    char path[MAX_PATH] = {0};
                    DWORD n = GetModuleFileNameA(h, path, MAX_PATH);
                    if (n)
                    {
                        std::string p(path, n);
                        size_t pos = p.find_last_of("\\/");
                        std::string b = (pos == std::string::npos) ? p : p.substr(pos + 1);
                        size_t dot = b.rfind('.');
                        if (dot != std::string::npos)
                            b.resize(dot);
                        return b;
                    }
                }
                return platform_getProcessName_();
#else
                Dl_info info{};
                if (dladdr((void*)(&platform_getModuleBasename_), &info) && info.dli_fname)
                {
                    std::string p(info.dli_fname);
                    size_t pos = p.find_last_of('/');
                    std::string b = (pos == std::string::npos) ? p : p.substr(pos + 1);
                    size_t dot = b.rfind('.');
                    if (dot != std::string::npos)
                        b.resize(dot);
                    return b;
                }
                return platform_getProcessName_();
#endif
            }

            static std::string platform_getProcessName_()
            {
                char buf[1024] = {0};
#if defined(_WIN32)
                char* exe = nullptr;
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
                ssize_t r = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
                if (r > 0)
                    buf[r] = '\0';
                if (r > 0)
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
                if (_getcwd(buf, (int)sizeof(buf)) != nullptr)
                    return std::string(buf) + "\\";
#else
                if (getcwd(buf, sizeof(buf)) != nullptr)
                    return std::string(buf) + "/";
#endif
                return std::string();
            }

            static std::vector<std::string> platform_listLogFiles_(const std::string& dir, const std::string& stem)
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
                if (DIR* d = opendir(dir.c_str()))
                {
                    while (dirent* e = readdir(d))
                    {
                        if (e->d_type == DT_REG || e->d_type == DT_LNK)
                        {
                            std::string n = e->d_name;
                            if (n.rfind(stem + "_", 0) == 0)
                                out.emplace_back(n);
                        }
                        else if (e->d_type == DT_UNKNOWN)
                        {
                            std::string full = dir;
                            if (!full.empty() && full.back() != '/')
                                full.push_back('/');
                            full += e->d_name;
                            struct stat st{};
                            if (::stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode))
                            {
                                std::string n = e->d_name;
                                if (n.rfind(stem + "_", 0) == 0)
                                    out.emplace_back(n);
                            }
                        }
                    }
                    closedir(d);
                }
#endif
                std::sort(out.begin(), out.end());
                return out;
            }

            static void platform_deleteFile_(const std::string& full)
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

            bool shouldDeleteLog_(const std::string& filename, int days) const
            {
                const std::time_t ft = parseDateFromFilename_(filename);
                if (ft == (time_t)-1)
                    return false;
                const std::time_t now = std::time(nullptr);
                return std::difftime(now, ft) > double(days) * 24.0 * 3600.0;
            }

            std::time_t parseDateFromFilename_(const std::string& filename) const
            {
                if (filename.rfind(_baseNameWithoutPath + "_", 0) != 0)
                    return (time_t)-1;
                std::string rest = filename.substr(_baseNameWithoutPath.length() + 1);
                if (rest.size() < 8)
                    return (time_t)-1;
                std::string ymd = rest.substr(0, 8);
                for (char c : ymd)
                    if (!std::isdigit(static_cast<unsigned char>(c)))
                        return (time_t)-1;
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

            // ---------- Pattern 引擎（NEW） ----------
            enum class PatType
            {
                Lit,
                DateChunk,
                Ms,
                LevelShort,
                LevelLong,
                LoggerName,
                PID,
                TID,
                FileShort,
                FileFull,
                Line,
                Func,
                Message,
                ColorStart,
                ColorStop
            };

            struct PatOp
            {
                PatType type;
                std::string text;
            };

            // 编译 pattern 为 token 序列；把时间片段（含多种 %X 与字面）聚合成单个 DateChunk
            static bool is_time_spec_char_(char c)
            {
                return std::isalpha(static_cast<unsigned char>(c)) ? true : false;
            }

            bool compilePattern_(const std::string& p, std::vector<PatOp>& out)
            {
                if (p.empty())
                    return false;
                out.clear();
                std::string lit, datechunk;
                auto flush_lit = [&]()
                { if (!lit.empty()) { out.push_back({ PatType::Lit, lit }); lit.clear(); } };
                auto flush_date = [&]()
                { if (!datechunk.empty()) { out.push_back({ PatType::DateChunk, datechunk }); datechunk.clear(); } };

                for (size_t i = 0; i < p.size(); ++i)
                {
                    if (p[i] != '%')
                    {
                        if (!datechunk.empty())
                            datechunk.push_back(p[i]);
                        else
                            lit.push_back(p[i]);
                        continue;
                    }
                    if (i + 1 >= p.size())
                    {
                        (datechunk.empty() ? lit : datechunk).push_back('%');
                        break;
                    }
                    char c = p[i + 1];
                    i++;
                    switch (c)
                    {
                    case 'v':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::Message, {}});
                        break;
                    case 'l':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::LevelShort, {}});
                        break;
                    case 'L':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::LevelLong, {}});
                        break;
                    case 'n':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::LoggerName, {}});
                        break;
                    case 'P':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::PID, {}});
                        break;
                    case 't':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::TID, {}});
                        break;
                    case 's':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::FileShort, {}});
                        break;
                    case 'g':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::FileFull, {}});
                        break; // [NEW] 全路径
                    case '#':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::Line, {}});
                        break;
                    case '!':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::Func, {}});
                        break; // [NEW] 函数名
                    case '^':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::ColorStart, {}});
                        break;
                    case '$':
                        flush_date();
                        flush_lit();
                        out.push_back({PatType::ColorStop, {}});
                        break;
                    case 'e': // 毫秒
                        if (!datechunk.empty())
                        {
                            datechunk += "%e";
                        }
                        else
                        {
                            flush_lit();
                            out.push_back({PatType::Ms, {}});
                        }
                        break;
                    default:
                        if (datechunk.empty())
                            flush_lit();
                        datechunk.push_back('%');
                        datechunk.push_back(c);
                        if (!is_time_spec_char_(c))
                        {
                        }
                        break;
                    }
                }
                flush_date();
                flush_lit();
                return !out.empty();
            }

            void renderPattern_(const std::tm& tmv, int ms, Level lv,
                                const char* file_short, const char* file_full, const char* func, int line,
                                const std::string& msg, std::string& out) const
            {
                const char* level_str = levelToStringC_(lv);
#if defined(_WIN32)
                const unsigned pid = GetCurrentProcessId();
#else
                const unsigned pid = (unsigned)getpid();
#endif
                const unsigned tid = (unsigned)std::hash<std::thread::id>{}(std::this_thread::get_id());

                for (const auto& op : _pat_ops)
                {
                    switch (op.type)
                    {
                    case PatType::Lit:
                        out.append(op.text);
                        break;
                    case PatType::LevelShort:
                    case PatType::LevelLong:
                        out.append(level_str);
                        break;
                    case PatType::LoggerName:
                        out.append(_name);
                        break;
                    case PatType::PID:
                    {
                        char b[32];
                        int n = std::snprintf(b, sizeof(b), "%u", pid);
                        if (n > 0)
                            out.append(b, n);
                    }
                    break;
                    case PatType::TID:
                    {
                        char b[32];
                        int n = std::snprintf(b, sizeof(b), "%u", tid);
                        if (n > 0)
                            out.append(b, n);
                    }
                    break;
                    case PatType::FileShort:
                        out.append(file_short ? file_short : "?");
                        break;
                    case PatType::FileFull:
                        out.append(file_full ? file_full : "?");
                        break;
                    case PatType::Line:
                    {
                        char b[16];
                        int n = std::snprintf(b, sizeof(b), "%d", line);
                        if (n > 0)
                            out.append(b, n);
                    }
                    break;
                    case PatType::Func:
                        out.append(func ? func : "?");
                        break;
                    case PatType::Message:
                        out.append(msg);
                        break;
                    case PatType::Ms:
                    {
                        char b[8];
                        int n = std::snprintf(b, sizeof(b), "%03d", ms);
                        if (n > 0)
                            out.append(b, n);
                    }
                    break;
                    case PatType::DateChunk:
                    {
                        // 处理 %e（毫秒）占位：先把 %e 替换为 @@@ ，strftime 后再二次替换为真正数值
                        std::string tmp;
                        tmp.reserve(op.text.size() + 8);
                        for (size_t i = 0; i < op.text.size(); ++i)
                        {
                            if (op.text[i] == '%' && i + 1 < op.text.size() && op.text[i + 1] == 'e')
                            {
                                tmp += "@@@";
                                ++i;
                            }
                            else
                                tmp.push_back(op.text[i]);
                        }
                        size_t cap = 128;
                        std::string buf(cap, '\0');
                        size_t n = 0;
                        for (;;)
                        {
                            n = std::strftime(&buf[0], buf.size(), tmp.c_str(), &tmv);
                            if (n > 0)
                                break;
                            cap <<= 1;
                            if (cap > 4096)
                                break; // 安全上限
                            buf.assign(cap, '\0');
                        }
                        if (n > 0)
                        {
                            std::string t2;
                            t2.reserve(n + 4);
                            for (size_t j = 0; j < n;)
                            {
                                if (j + 3 <= n && std::memcmp(buf.data() + j, "@@@", 3) == 0)
                                {
                                    char b[8];
                                    int k = std::snprintf(b, sizeof(b), "%03d", ms);
                                    if (k > 0)
                                        t2.append(b, k);
                                    j += 3;
                                }
                                else
                                {
                                    t2.push_back(buf[j++]);
                                }
                            }
                            out.append(t2);
                        }
                    }
                    break;
                    case PatType::ColorStart:
                    case PatType::ColorStop:
                        // 忽略（仍采用整行按级别上色，不污染文件）
                        break;
                    }
                }
            }
            void maybeHealUnlinked_()
            {
#if defined(_WIN32)
                (void)0;
#else
                if (_heal_every <= 0 || !_outputToFile || !_file.is_open() || _curFilePath.empty())
                    return;
                if (++_heal_counter < _heal_every)
                    return;
                _heal_counter = 0;

                // 先取“路径是否存在 + inode 是否匹配”
                bool need_reopen = false;
                struct stat st_path{};
                if (::stat(_curFilePath.c_str(), &st_path) != 0)
                {
                    // 路径不存在（被 unlink）
                    need_reopen = true;
                }
                else
                {
                    int fd = _file.native_fileno(); // <== 需要补丁 A
                    if (fd >= 0)
                    {
                        struct stat st_fd{};
                        if (::fstat(fd, &st_fd) == 0)
                        {
                            // dev/inode 变化 => 我们拿的是“旧文件”，路径上是“新文件”
                            if (st_fd.st_dev != st_path.st_dev || st_fd.st_ino != st_path.st_ino)
                                need_reopen = true;
                        }
                    }
                }

                if (need_reopen)
                {
                    _file.close();
                    _file.open(_curFilePath, std::ios::out | std::ios::app | std::ios::binary);
                    if (_file.is_open())
                    {
                        _file.seekp(0, std::ios::end);
                        std::streampos pos = _file.tellp();
                        _currentSize = (pos >= 0) ? (size_t)pos : 0u;
                    }
                    else
                    {
                        reportError_(std::string("Heal reopen failed: ") + _curFilePath);
                        _initialized = false; // 让后续写入触发 rollFiles_()
                    }
                }
#endif
            }

        private:
            ML_FastOFStream _file;
            std::mutex _mutex;
            std::string _name; // [NEW] 实例名（%n）
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
            std::atomic<int> _last_log_ymd;
            bool _auto_flush;
            std::atomic<bool> _need_day_switch;
            ErrorHandler _error_handler;
            static constexpr size_t PENDING_MAX_BYTES = 4u * 1024u * 1024u;
            static constexpr size_t PENDING_MAX_COUNT = 2000u;
            std::deque<std::string> _pending;
            size_t _pending_bytes;
            std::atomic<int> _phase;

            // Pattern 状态
            std::string _pattern_raw;    // [NEW]
            std::vector<PatOp> _pat_ops; // [NEW]
            std::atomic<bool> _has_pattern{false};

            std::string _curFilePath; // 当前打开并写入的文件完整路径
            int _heal_every = 256;    // 每写多少行做一次自愈检查（0=关闭）
            int _heal_counter = 0;    // 计数器

            static bool& in_logging_flag_()
            {
                thread_local bool flag = false;
                return flag;
            }
        };

        /* =================== Registry 方法定义 =================== */
        inline ML_LoggerRegistry& ML_LoggerRegistry::getInstance()
        {
            static ML_LoggerRegistry* p = new ML_LoggerRegistry(); // 永不析构
            return *p;
        }
        inline ML_Logger& ML_LoggerRegistry::get(const std::string& name)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _loggers.find(name);
            if (it == _loggers.end())
                it = _loggers.emplace(name, std::unique_ptr<ML_Logger>(new ML_Logger(name))).first; // [CHG]
            return *it->second;
        }
        inline ML_LoggerRegistry::~ML_LoggerRegistry() = default;

        /* ========================= 日志流（携带 短/全文件+函数） ========================= */
        class LoggerStream
        {
        public:
            LoggerStream(ML_Logger& logger, ML_Logger::Level lv,
                         const char* file_short, const char* file_full, const char* func, int line)
                : _logger(logger), _lv(lv), _file_short(file_short), _file_full(file_full), _func(func), _line(line)
            {
                if (_buf.capacity() < 256)
                    _buf.reserve(256);
                _buf.clear();
            }

            ~LoggerStream() { _logger.log(_file_short, _file_full, _func, _line, _lv, _buf, _logger.getAddNewLine()); }

            LoggerStream& operator<<(const std::string& s)
            {
                _buf.append(s);
                return *this;
            }
            LoggerStream& operator<<(const char* s)
            {
                _buf.append(s ? s : "nullptr");
                return *this;
            }
            LoggerStream& operator<<(char c)
            {
                _buf.push_back(c);
                return *this;
            }

            template <class T>
            LoggerStream& operator<<(T* p)
            {
                if (!p)
                {
                    _buf.append("nullptr");
                    return *this;
                }
                char tmp[32];
#if defined(_WIN32)
                int n = _snprintf(tmp, (unsigned)sizeof(tmp), "%p", (const void*)p);
#else
                int n = std::snprintf(tmp, sizeof(tmp), "%p", (const void*)p);
#endif
                if (n > 0)
                    _buf.append(tmp, (size_t)n);
                return *this;
            }
            LoggerStream& operator<<(bool v)
            {
                _buf.push_back(v ? '1' : '0');
                return *this;
            }
            LoggerStream& operator<<(short v)
            {
                append_int((long long)v);
                return *this;
            }
            LoggerStream& operator<<(unsigned short v)
            {
                append_uint((unsigned long long)v);
                return *this;
            }
            LoggerStream& operator<<(int v)
            {
                append_int((long long)v);
                return *this;
            }
            LoggerStream& operator<<(unsigned int v)
            {
                append_uint((unsigned long long)v);
                return *this;
            }
            LoggerStream& operator<<(long v)
            {
                append_int((long long)v);
                return *this;
            }
            LoggerStream& operator<<(unsigned long v)
            {
                append_uint((unsigned long long)v);
                return *this;
            }
            LoggerStream& operator<<(long long v)
            {
                append_int(v);
                return *this;
            }
            LoggerStream& operator<<(unsigned long long v)
            {
                append_uint(v);
                return *this;
            }
            LoggerStream& operator<<(float v)
            {
                append_float((double)v);
                return *this;
            }
            LoggerStream& operator<<(double v)
            {
                append_float(v);
                return *this;
            }
            LoggerStream& operator<<(long double v)
            {
                append_float((double)v);
                return *this;
            }
            template <class T>
            LoggerStream& operator<<(const T& v)
            {
                std::ostringstream oss;
                oss << v;
                _buf.append(oss.str());
                return *this;
            }

        private:
            static inline void append_int_to(std::string& out, long long v)
            {
                char tmp[32];
#if defined(_WIN32)
                int n = _snprintf(tmp, (unsigned)sizeof(tmp), "%lld", v);
#else
                int n = std::snprintf(tmp, sizeof(tmp), "%lld", (long long)v);
#endif
                if (n > 0)
                    out.append(tmp, (size_t)n);
            }
            static inline void append_uint_to(std::string& out, unsigned long long v)
            {
                char tmp[32];
#if defined(_WIN32)
                int n = _snprintf(tmp, (unsigned)sizeof(tmp), "%llu", v);
#else
                int n = std::snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)v);
#endif
                if (n > 0)
                    out.append(tmp, (size_t)n);
            }
            static inline void append_float_to(std::string& out, double v)
            {
                char tmp[64];
#if defined(_WIN32)
                int n = _snprintf(tmp, (unsigned)sizeof(tmp), "%.6g", v);
#else
                int n = std::snprintf(tmp, sizeof(tmp), "%.6g", v);
#endif
                if (n > 0)
                    out.append(tmp, (size_t)n);
            }
            inline void append_int(long long v) { append_int_to(_buf, v); }
            inline void append_uint(unsigned long long v) { append_uint_to(_buf, v); }
            inline void append_float(double v) { append_float_to(_buf, v); }

        private:
            ML_Logger& _logger;
            ML_Logger::Level _lv;
            const char* _file_short;
            const char* _file_full; // [NEW]
            const char* _func;      // [NEW]
            int _line;
            std::string _buf;
        };
    } // inline namespace v2_9_1
} // namespace mllog_v291

#if !defined(_WIN32)
#pragma GCC visibility pop
#endif

/* ========================= 版本选择宏（全局定义） ========================= */
#ifndef ML_NS
#define ML_NS ::mllog_v291::v2_9_1
#endif

/* ========================= 宏接口 ========================= */
// [NEW] 同时提供短/全路径与函数名
#define MLFILE_SHORT ML_NS::mllog_file_name_from_path(__FILE__)
#define MLFILE_FULL __FILE__
#define MLFUNC __func__

#define MLLOG_STREAM(logger, level) ML_NS::LoggerStream(logger, level, MLFILE_SHORT, MLFILE_FULL, MLFUNC, __LINE__)

#define MLLOGF_FORMAT(logger, level, fmt, ...)                                                      \
    do                                                                                              \
    {                                                                                               \
        (logger).logformat(MLFILE_SHORT, MLFILE_FULL, MLFUNC, __LINE__, level, fmt, ##__VA_ARGS__); \
    } while (0)

/* 默认 logger ("default") */
#define MLLOG_START()                                \
    do                                               \
    {                                                \
        ML_NS::ML_Logger::get().startAnywhere(true); \
    } while (0)
#define MLLOG_PROMOTE_TO_FULL()                  \
    do                                           \
    {                                            \
        ML_NS::ML_Logger::get().promoteToFull(); \
    } while (0)
#define MLLOG_FORCE_FLUSH()              \
    do                                   \
    {                                    \
        ML_NS::ML_Logger::get().flush(); \
    } while (0)

#define MLLOG(level) MLLOG_STREAM(ML_NS::ML_Logger::get(), level)
#define MLLOG_DEBUG MLLOG(ML_NS::ML_Logger::Level::Debug)
#define MLLOG_INFO MLLOG(ML_NS::ML_Logger::Level::Info)
#define MLLOG_NOTICE MLLOG(ML_NS::ML_Logger::Level::Notice)
#define MLLOG_WARNING MLLOG(ML_NS::ML_Logger::Level::Warning)
#define MLLOG_ERROR MLLOG(ML_NS::ML_Logger::Level::Error)
#define MLLOG_CRITICAL MLLOG(ML_NS::ML_Logger::Level::Critical)
#define MLLOG_ALERT MLLOG(ML_NS::ML_Logger::Level::Alert)
#define MLLOGD MLLOG_DEBUG
#define MLLOGI MLLOG_INFO
#define MLLOGN MLLOG_NOTICE
#define MLLOGW MLLOG_WARNING
#define MLLOGE MLLOG_ERROR
#define MLLOGC MLLOG_CRITICAL
#define MLLOGA MLLOG_ALERT
#define mllogd MLLOG_DEBUG
#define mllogi MLLOG_INFO
#define mllogn MLLOG_NOTICE
#define mllogw MLLOG_WARNING
#define mlloge MLLOG_ERROR
#define mllogc MLLOG_CRITICAL
#define mlloga MLLOG_ALERT

#define MLLOGF(level, fmt, ...) MLLOGF_FORMAT(ML_NS::ML_Logger::get(), level, fmt, ##__VA_ARGS__)
#define MLLOG_DEBUGF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Debug, fmt, ##__VA_ARGS__)
#define MLLOG_INFOF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Info, fmt, ##__VA_ARGS__)
#define MLLOG_NOTICEF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Notice, fmt, ##__VA_ARGS__)
#define MLLOG_WARNINGF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Warning, fmt, ##__VA_ARGS__)
#define MLLOG_ERRORF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Error, fmt, ##__VA_ARGS__)
#define MLLOG_CRITICALF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Critical, fmt, ##__VA_ARGS__)
#define MLLOG_ALERTF(fmt, ...) MLLOGF(ML_NS::ML_Logger::Level::Alert, fmt, ##__VA_ARGS__)

/* 命名实例 */
#define MLLOG_START_NAMED(name)                          \
    do                                                   \
    {                                                    \
        ML_NS::ML_Logger::get(name).startAnywhere(true); \
    } while (0)
#define MLLOG_PROMOTE_TO_FULL_NAMED(name)            \
    do                                               \
    {                                                \
        ML_NS::ML_Logger::get(name).promoteToFull(); \
    } while (0)
#define MLLOG_FORCE_FLUSH_NAMED(name)        \
    do                                       \
    {                                        \
        ML_NS::ML_Logger::get(name).flush(); \
    } while (0)

#define MLLOG_NAMED(name, level) MLLOG_STREAM(ML_NS::ML_Logger::get(name), level)
#define MLLOG_DEBUG_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Debug)
#define MLLOG_INFO_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Info)
#define MLLOG_NOTICE_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Notice)
#define MLLOG_WARNING_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Warning)
#define MLLOG_ERROR_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Error)
#define MLLOG_CRITICAL_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Critical)
#define MLLOG_ALERT_NAMED(name) MLLOG_NAMED(name, ML_NS::ML_Logger::Level::Alert)

#define MLLOGF_NAMED(name, level, fmt, ...)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        ML_NS::ML_Logger::get(name).logformat(MLFILE_SHORT, MLFILE_FULL, MLFUNC, __LINE__, level, fmt, ##__VA_ARGS__); \
    } while (0)

#define MLLOG_DEBUGF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Debug, fmt, ##__VA_ARGS__)
#define MLLOG_INFOF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Info, fmt, ##__VA_ARGS__)
#define MLLOG_NOTICEF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Notice, fmt, ##__VA_ARGS__)
#define MLLOG_WARNINGF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Warning, fmt, ##__VA_ARGS__)
#define MLLOG_ERRORF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Error, fmt, ##__VA_ARGS__)
#define MLLOG_CRITICALF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Critical, fmt, ##__VA_ARGS__)
#define MLLOG_ALERTF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Alert, fmt, ##__VA_ARGS__)

/* ===== 可选：旧名导出（仅在不并存多版本时启用） ===== */
#if defined(MLLOG_LEGACY_GLOBALS) && MLLOG_LEGACY_GLOBALS
using ML_Logger = ML_NS::ML_Logger;
using ML_LoggerRegistry = ML_NS::ML_LoggerRegistry;
using ML_FastOFStream = ML_NS::ML_FastOFStream;
using LoggerStream = ML_NS::LoggerStream;
#endif

#endif // MLLOG_HPP