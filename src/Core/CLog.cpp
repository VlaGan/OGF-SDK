//----------------------------------------------------------------------------
//-- CLog - log class implementation
//----------------------------------------------------------------------------
#include "CLog.h"

#include <cstdio>
#include <ctime>
#include <chrono>
#include <filesystem>

CLog::CLog() {
    CreateNewLogFile("userdata/logs");
}

CLog::~CLog() {
    if (m_FileStream.is_open())
        m_FileStream.close();
}

void CLog::CreateNewLogFile(const std::string& directory) {
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {}

    std::string filename = "ogf_sdk_" + GetFileTimestamp() + ".txt";
    std::filesystem::path fullPath = ec ? std::filesystem::path(filename)
        : std::filesystem::path(directory) / filename;

    if (m_FileStream.is_open())
        m_FileStream.close();

    m_FileStream.open(fullPath, std::ios::out);
}

std::string CLog::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
        tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
    return buf;
}

std::string CLog::GetFileTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif

    // Example: 11-01-26_15-22-02  (d-m-y_hour-min-sec)
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d-%02d-%02d_%02d-%02d-%02d",
        tmBuf.tm_mday, tmBuf.tm_mon + 1, (tmBuf.tm_year + 1900) % 100,
        tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec);
    return buf;
}

const char* CLog::LevelToString(eLogLevel type) const {
    switch (type) {
    case INFO:    return "INFO";
    case WARNING: return "WARN";
    case ERR:     return "ERROR";
    case DEBUG:   return "DEBUG";
    }
    return "UNKNOWN";
}

void CLog::AddLog(eLogLevel type, const char* fmt, va_list args) {
    char buf[1024];
    std::vsnprintf(buf, sizeof(buf), fmt, args);

    LogEntry entry;
    entry.message = buf;
    entry.timestamp = GetTimestamp();
    entry.type = type;

    m_LogEntries.push_back(entry);

    if (m_FileStream.is_open()) {
        m_FileStream << "[" << entry.timestamp << "] ["
            << LevelToString(type) << "] "
            << entry.message << "\n";
        m_FileStream.flush();
    }
}

void CLog::AddLog(eLogLevel type, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    AddLog(type, fmt, args);
    va_end(args);
}

void CLog::Clear() {
    m_LogEntries.clear();
}
