//----------------------------------------------------------------------------
//-- CLog - log class implementation
//----------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
//#include <mutex>
#include <fstream>
#include <cstdarg>

enum eLogLevel {
    INFO = 0,
    WARNING,
    ERR, 
    DEBUG
};

class CLog {
public:
    struct LogEntry {
        std::string message;
        std::string timestamp;
        eLogLevel type = eLogLevel::INFO;
    };

    CLog();
    ~CLog();

    static CLog& Get() {
        static CLog Log;
        return Log;
    }

    CLog(const CLog&) = delete;
    CLog& operator=(const CLog&) = delete;

    void AddLog(eLogLevel type, const char* fmt, va_list args);
    void AddLog(eLogLevel type, const char* fmt, ...);

    void Clear();
    const std::vector<LogEntry>& GetEntries() const { return m_LogEntries; }

    std::string GetTimestamp() const;
    std::string GetFileTimestamp() const;
    const char* LevelToString(eLogLevel type) const;

private:
    void CreateNewLogFile(const std::string& directory);

    std::vector<LogEntry> m_LogEntries;
    std::ofstream m_FileStream;
    // std::mutex m_Mutex;
};

//-- global adding log entityes
inline void LogMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    CLog::Get().AddLog(INFO, fmt, args);
    va_end(args);
}

inline void LogMsg(eLogLevel type, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    CLog::Get().AddLog(type, fmt, args);
    va_end(args);
}
