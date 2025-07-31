#include "writelog.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <thread>
#include <Windows.h>   //2024年9月14日 为了调用WinSDK获取当前程序路径，需要包含Windows.h头文件，后期若考虑其他平台再说
#include <queue>
#include <condition_variable>

namespace wlhsb {

// 获取程序的文件名和路径
std::string GetProgramNameAndPath()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    return path;
}

// 获取当前日期，用于日志文件命名
std::string GetCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now_time);
#else
    localtime_r(&tm, &now_time); // POSIX
#endif
    localtime_s(&tm, &now_time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

// 获取当前时间的格式化字符串
std::string GetCurrentTimestamp() {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);

    // 使用 std::tm 结构来存储当地时间
    std::tm local_tm = {};
#ifdef _WIN32
    localtime_s(&local_tm, &time_t_now); // Windows
#else
    localtime_r(&time_t_now, &local_tm); // POSIX
#endif

    // 使用 std::ostringstream 格式化时间
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S"); // 自定义格式
    return oss.str();
}

// 获取当前线程ID
std::string GetCurrentThreadId()
{
    std::ostringstream ss;
    auto threadId = std::this_thread::get_id();
    ss << std::hex << std::setw(8) << std::setfill('0') << threadId;
    return ss.str();
}



// 日志级别
enum class LogLevel {
    INFO,
    WARNING,
    //ERROR,    //2024年9月14日 这个会有重复宏定义，反正不用，注掉算了
    EXCEPTION
};

// 日志项
struct LogEntry {
    std::string message;
    std::string timestamp;
    std::string threadId;
    LogLevel level;
};

class MyLogger {
public:
    MyLogger() : stop_flag_(false) {

        // 获取当前程序文件名和路径
        std::string fullPath = GetProgramNameAndPath();
        std::filesystem::path logPath = fullPath;
        logPath.replace_extension(".log");

        // 日志文件的名称与当前程序名称相同
        output_dir_ = logPath;

        StartLoggingThread();
    }

    ~MyLogger() {
        StopLoggingThread();
    }

    // 记录日志
    void Log(const std::string& message, const std::string& timestamp, const std::string& threadId) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        log_queue_.emplace(LogEntry{ message, timestamp, threadId, LogLevel::INFO/*2024年9月14日 这个不再使用，后期可以考虑换个简便的方式来用*/});
        condition_.notify_one();  // 通知日志线程有新日志
    }

private:
    std::filesystem::path output_dir_;        // 包含日志名称的目录
    std::queue<LogEntry> log_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_flag_;
    std::thread logging_thread_;

    // 启动日志线程
    void StartLoggingThread() {
        logging_thread_ = std::thread([this]() {
            while (!stop_flag_ || !log_queue_.empty()) {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this]() {
                    return !log_queue_.empty() || stop_flag_;
                    });

                while (!log_queue_.empty()) {
                    LogEntry entry = log_queue_.front();
                    log_queue_.pop();
                    lock.unlock();
                    WriteLogToFile(entry);
                    lock.lock();
                }
            }
            });
    }

    // 停止日志线程
    void StopLoggingThread() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_flag_ = true;
        }
        condition_.notify_one();
        if (logging_thread_.joinable()) {
            logging_thread_.join();
        }
    }

    // 写入日志到文件
    void WriteLogToFile(const LogEntry& entry) {
        std::ofstream log_file(output_dir_, std::ios_base::app);
        if (log_file.is_open()) {
            log_file 
                << entry.timestamp << " "
                << entry.threadId << " "
                << entry.message 
                << std::endl;
        }
    }
};





// 日志写入函数
void TKWriteLog(const char* format, ...) 
{
    // 局部静态变量，保证线程安全
    static MyLogger logger;  

    // 获取当前时间和线程ID
    std::string timeStr = GetCurrentTimestamp();
    std::string threadIdStr = GetCurrentThreadId();

    // 格式化日志消息到一个字符串流中
    va_list args;
    va_start(args, format);

    // 用于存储格式化后的字符串
    std::vector<char> buffer(2048);       // 用vector代替char[]，可以保证当字符串中存在'\0'时依旧能完整打印
    vsnprintf(buffer.data(), buffer.size(), format, args);

    // 创建一个字符串流以处理包含空字符的字符串
    std::ostringstream oss;
    oss.write(buffer.data(), buffer.size());
    std::string message = oss.str();

    va_end(args);


    // 写入日志文件
    logger.Log(message,
        GetCurrentTimestamp(), 
        GetCurrentThreadId());

}


} // namespace wlhsb
