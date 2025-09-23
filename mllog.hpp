#ifndef MLLOG_HPP
#define MLLOG_HPP

/**
 * @file mllog.hpp
 * @brief 多功能、跨平台、单头文件日志系统（Turbo, Anywhere-Safe Start）
 * @author malin
 *      - Email: zcyxml@163.com  mlin2@grgbanking.com
 *      - GitHub: https://github.com/mixml
 * 变更摘要（关键）：
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
 * @section usage 使用示例 (Usage)
 *
 * @subsection basic_usage 基础用法（默认实例）
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
 *     // 3) 在安全时机转正到 Full（开始正常落盘）
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
 *     // 6) 结束前可记录 Stop 行，并手动刷盘
 *     MLLOG_ALERT << "---------- Stop MLLOG ----------";
 *     logger.flush();
 *     return 0;
 * }
 * @endcode
 *
 * @subsection advanced_usage 进阶配置（性能/跨天/清理）
 * @code
 * #include "mllog.hpp"
 * #include <iostream>
 *
 * // 自定义库内部错误处理器（可用于上报）
 * static void on_log_internal_error(const std::string& msg) {
 *     std::cerr << "!!! MLLOG INTERNAL: " << msg << std::endl;
 * }
 * using namespace ML_NS;
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
 *     L.setLogFile("./logs/perf_test");
 *     L.setLevel(ML_Logger::Level::Info);
 *     L.setAutoFlush(false);   // 高频写建议关闭自动刷盘
 *     L.setCheckDay(true);     // 跨天自动切换新日志
 *     L.setOutput(true, true); // 文件 + 控制台
 *
 *     // 4) 转正（Full）
 *     MLLOG_PROMOTE_TO_FULL();
 *
 *     // 5) 高频写测试
 *     for (int i = 0; i < 1000; ++i) {
 *         MLLOG_INFO << "Processing item " << i;
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
 * @subsection named_usage 命名实例用法（每个 SO/子系统独立日志）
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
 *
 *     // 3) 安全时机转正
 *     MLLOG_PROMOTE_TO_FULL_NAMED(VISION_LOG);
 *
 *     // 4) 业务日志（流式 + printf 风格）
 *     MLLOG_INFO_NAMED(VISION_LOG) << "vision subsystem ready";
 *     MLLOG_WARNINGF_NAMED(VISION_LOG, "fps 低: %.2f", 23.7);
 *
 *     // 5) 需要时手动刷盘
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
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <fcntl.h> // _O_*
#include <io.h>
#include <share.h> // _sopen_s
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

/* ========================= ANSI 颜色（保持全局宏，不产生命名冲突） ========================= */
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

#if !defined(_WIN32)
#pragma GCC visibility push(hidden)
#endif

/* ========================= 命名空间：当前版本 ========================= */
namespace mllog_v282
{
    inline namespace v2_8_2
    {

        /* ========================= constexpr 工具（移入命名空间） ========================= */
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

        template <class T>
        ML_NODISCARD ML_ALWAYS_INLINE constexpr const T& ml_max(const T& a, const T& b) noexcept { return (a < b) ? b : a; }

        /* ============= 轻量 ofstream 替代（Win: _open/_write + 1MB；Nix: FILE* + 1MB） ============= */
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
            ML_FastOFStream(ML_FastOFStream&&) = delete;
            ML_FastOFStream& operator=(ML_FastOFStream&&) = delete;

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
            static ML_LoggerRegistry& getInstance(); // 永不析构
            ML_Logger& get(const std::string& name); // 定义在 ML_Logger 定义之后
            ~ML_LoggerRegistry();                    // 不会被调用（永不析构）

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

            ML_Logger()
                : _isRoll(false), _logLevel(Level::Debug),
                  _outputToFile(true), _outputToScreen(true),
                  _currentRollIndex(0), _maxRolls(5),
                  _maxSizeInBytes(100u * 1024u * 1024u), _currentSize(0),
                  _initialized(false), _message_only(false), _add_newline(true),
                  _log_enabled(false), _log_screen_color(true),
                  _default_file_name_day(true), _isCheckDay(false),
                  _start_timestamp(), _last_log_ymd(0), _auto_flush(true),
                  _cached_time_sec(0), _need_day_switch(false), _error_handler(nullptr),
                  _phase((int)Phase::Off), _pending_bytes(0)
            {
                std::string def = get_module_path() + "/log/" + get_process_name() + "_MLLOG";
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
            ML_Logger(ML_Logger&&) = delete;
            ML_Logger& operator=(ML_Logger&&) = delete;

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

            ML_Logger::Level LevelFromInt_Clamped(int v)
            {
                using UT = std::underlying_type<ML_Logger::Level>::type;
                UT lo = static_cast<UT>(ML_Logger::Level::Debug);
                UT hi = static_cast<UT>(ML_Logger::Level::Alert);
                if (v < static_cast<int>(lo))
                    v = static_cast<int>(lo);
                if (v > static_cast<int>(hi))
                    v = static_cast<int>(hi);
                return static_cast<ML_Logger::Level>(static_cast<UT>(v));
            }

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
                    rollFiles_(true);
                    _initialized = true;
                }
                else if (!_file.is_open())
                {
                    rollFiles_(true);
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
                        return; // 不转 Full
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
            }

            void flush()
            {
                std::lock_guard<std::mutex> lk(_mutex);
                if (_file.is_open())
                    _file.flush();
            }

            void log(const char* file, int line, Level lv, const std::string& original, bool isNewLine = true)
            {
                if (!_log_enabled || lv < _logLevel)
                    return;

                int ms_count = 0;
                std::tm cached_tm{};
                const char* time_c = nullptr;
                updateAndGetTimeCache_(cached_tm, ms_count, time_c);
                const std::string msg = maybeTruncate_(original);

                char prefix[192];
                int plen = 0;
                if (!_message_only)
                {
                    plen = buildPrefix_(prefix, sizeof(prefix), lv, file, line, time_c, ms_count);
                    if (plen < 0)
                        plen = 0;
                }

                Phase ph = phase();
                if (ph != Phase::Full)
                {
                    std::string linebuf;
                    if (!_message_only && plen > 0)
                        linebuf.append(prefix, (size_t)plen);
                    linebuf.append(msg);
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

                if (_outputToFile && !_outputToScreen && !_message_only)
                {
                    if (plen > 0)
                    {
                        writeToTargetsFileOnly_(prefix, (size_t)plen, msg, isNewLine, lv);
                        return;
                    }
                }

                auto& formatted = tls_buf_();
                formatMessageFast_(lv, file, line, time_c, ms_count, msg, formatted);
                writeToTargets_(formatted, isNewLine, lv);
            }

            void logformat(const char* file, int line, Level lv, const char* fmt, ...)
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
                log(file, line, lv, s, _add_newline);
            }

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
                for (const auto& name : files)
                    if (shouldDeleteLog_(name, daysToKeep))
                        platform_deleteFile_(dir + name);
            }

        private:
            enum class Phase_AtomicTag
            {
            }; // 只为表意
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
                char prefix[192];
                int plen = buildPrefix_(prefix, sizeof(prefix), Level::Alert, "mllog.hpp", 0, tc, ms);
                std::string line;
                if (plen > 0)
                    line.append(prefix, (size_t)plen);
                line.append("---------- Start MLLOG ----------");
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
                    rollFiles_(true);
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

            static int buildPrefix_(char* buf, size_t cap, Level lv, const char* file, int line, const char* time_c, int ms)
            {
#if defined(_WIN32)
                return _snprintf(buf, (unsigned)cap, "%s.%03d %s [%s:%d] ", time_c, ms, levelToStringC_(lv), file, line);
#else
                return std::snprintf(buf, cap, "%s.%03d %s [%s:%d] ", time_c, ms, levelToStringC_(lv), file, line);
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

            void formatMessageFast_(Level lv, const char* file, int line,
                                    const char* time_c, int ms_count,
                                    const std::string& msg, std::string& out) const
            {
                if (_message_only)
                {
                    out.assign(msg);
                    return;
                }
                char prefix[192];
                int plen = buildPrefix_(prefix, sizeof(prefix), lv, file, line, time_c, ms_count);
                if (plen < 0)
                {
                    out.assign(msg);
                    return;
                }
                out.reserve((size_t)plen + msg.size());
                out.append(prefix, (size_t)plen);
                out.append(msg);
            }

            void writeToTargetsFileOnly_(const char* prefix, size_t plen,
                                         const std::string& msg, bool isNewLine, Level /*lv*/)
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
                if (!_outputToFile)
                    return;

                if (!_file.is_open())
                {
                    rollFiles_(true);
                    _initialized = true;
                    if (!_file.is_open())
                    {
                        reportError_("Failed to open file for writing. Prefix lost");
                        return;
                    }
                }

                const size_t msg_size = plen + msg.size() + (isNewLine ? 1u : 0u);
                if (_currentSize > 0 && (_currentSize + msg_size > _maxSizeInBytes))
                    rollFiles_();

                const size_t total = msg_size;
                thread_local std::vector<char> oneline;
                if (oneline.size() < total)
                    oneline.resize(total);

                size_t off = 0;
                std::memcpy(oneline.data() + off, prefix, plen);
                off += plen;
                std::memcpy(oneline.data() + off, msg.data(), msg.size());
                off += msg.size();
                if (isNewLine)
                    oneline[off++] = '\n';

                _file.write(oneline.data(), off);
                if (_auto_flush)
                    _file.flush();
                if (_file.bad())
                {
                    reportError_("Log write/flush failed (file-only).");
                    _file.close();
                    _initialized = false;
                    return;
                }
                _currentSize += msg_size;
                if (_currentSize >= _maxSizeInBytes)
                    rollFiles_();
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
                {
                    rollFiles_(true);
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

                _file.write(s.data(), s.size());
                if (isNewLine)
                    _file.put('\n');
                if (_auto_flush)
                    _file.flush();
                if (_file.bad())
                {
                    reportError_("Log write/flush failed.");
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
                                   {
                                       HANDLE h = GetStdHandle((DWORD)-11); // STD_OUTPUT_HANDLE
                                       DWORD mode = 0;
                                       if (h && GetConsoleMode(h, &mode))
                                           if (SetConsoleMode(h, mode | MLLOG_VT_ENABLE))
                                           {
                                               cached = true;
                                               return;
                                           }
                                   }
                                   const char* wt = std::getenv("WT_SESSION");
                                   const char* term = std::getenv("TERM");
                                   cached = (wt != nullptr) || (term != nullptr);
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
            }

            void rollFiles_(bool /*first*/ = false)
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
                _file.open(fn.str(), std::ios::out | (_isRoll ? std::ios::trunc : std::ios::app) | std::ios::binary);
                if (!_file.is_open())
                {
                    reportError_(std::string("Failed to open new log file: ") + fn.str());
                    _currentSize = 0;
                    return;
                }
                _file.seekp(0, std::ios::end);
                std::streampos pos = _file.tellp();
                _currentSize = (pos >= 0) ? (size_t)pos : 0u;
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
                if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                       reinterpret_cast<LPCSTR>(&platform_getModulePath_),
                                       &h))
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

        private:
            ML_FastOFStream _file;
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
            std::atomic<int> _last_log_ymd;
            bool _auto_flush;
            std::time_t _cached_time_sec;
            std::atomic<bool> _need_day_switch;
            ErrorHandler _error_handler;
            static constexpr size_t PENDING_MAX_BYTES = 4u * 1024u * 1024u;
            static constexpr size_t PENDING_MAX_COUNT = 2000u;
            std::deque<std::string> _pending;
            size_t _pending_bytes;
            std::atomic<int> _phase;

            static bool& in_logging_flag_()
            {
                thread_local bool flag = false;
                return flag;
            }
        };

        /* =================== Registry 方法定义（放在 ML_Logger 之后） =================== */
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
                it = _loggers.emplace(name, std::unique_ptr<ML_Logger>(new ML_Logger())).first;
            return *it->second;
        }
        inline ML_LoggerRegistry::~ML_LoggerRegistry() = default;

        /* ========================= 日志流 ========================= */
        class LoggerStream
        {
        public:
            LoggerStream(ML_Logger& logger, ML_Logger::Level lv, const char* file, int line)
                : _logger(logger), _lv(lv), _file(file), _line(line)
            {
                if (_buf.capacity() < 256)
                    _buf.reserve(256);
                _buf.clear();
            }

            ~LoggerStream() { _logger.log(_file, _line, _lv, _buf, _logger.getAddNewLine()); }

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
            const char* _file;
            int _line;
            std::string _buf;
        };

    } // inline namespace v2_8_2
} // namespace mllog_v282
#if !defined(_WIN32)
#pragma GCC visibility pop
#endif
/* ========================= 版本选择宏（全局定义） ========================= */
#ifndef ML_NS
#define ML_NS ::mllog_v282::v2_8_2
#endif

/* ========================= 宏接口（引用命名空间内符号） ========================= */
#define MLSHORT_FILE ML_NS::mllog_file_name_from_path(__FILE__)
#define MLLOG_STREAM(logger, level) ML_NS::LoggerStream(logger, level, MLSHORT_FILE, __LINE__)

#define MLLOGF_FORMAT(logger, level, fmt, ...)                                 \
    do                                                                         \
    {                                                                          \
        (logger).logformat(MLSHORT_FILE, __LINE__, level, fmt, ##__VA_ARGS__); \
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

#define MLLOGF_NAMED(name, level, fmt, ...) MLLOGF_FORMAT(ML_NS::ML_Logger::get(name), level, fmt, ##__VA_ARGS__)
#define MLLOG_DEBUGF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Debug, fmt, ##__VA_ARGS__)
#define MLLOG_INFOF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Info, fmt, ##__VA_ARGS__)
#define MLLOG_NOTICEF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Notice, fmt, ##__VA_ARGS__)
#define MLLOG_WARNINGF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Warning, fmt, ##__VA_ARGS__)
#define MLLOG_ERRORF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Error, fmt, ##__VA_ARGS__)
#define MLLOG_CRITICALF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Critical, fmt, ##__VA_ARGS__)
#define MLLOG_ALERTF_NAMED(name, fmt, ...) MLLOGF_NAMED(name, ML_NS::ML_Logger::Level::Alert, fmt, ##__VA_ARGS__)

/* ===== 可选：在不并存多版本的工程里，把类名引回全局（默认关闭） =====
   使用方式：对单一版本、且确认不会与其他版本同进程并存时，在编译指令里加
   -DMLLOG_LEGACY_GLOBALS=1
*/
#if defined(MLLOG_LEGACY_GLOBALS) && MLLOG_LEGACY_GLOBALS
using ML_Logger = ML_NS::ML_Logger;
using ML_LoggerRegistry = ML_NS::ML_LoggerRegistry;
using ML_FastOFStream = ML_NS::ML_FastOFStream;
using LoggerStream = ML_NS::LoggerStream;
#endif

#endif // MLLOG_HPP
