#pragma once

#include <Windows.h>
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
#include <format>
#include <chrono>


enum class eLogLevel : uint32_t
{
	Verbose,
    Hint,
    Info,
    Warning,
    Error,
    Fatal
};

#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.

enum class eConsoleForeground : uint32_t
{
    BLACK = 0,
    DARKBLUE = FOREGROUND_BLUE,
    DARKGREEN = FOREGROUND_GREEN,
    DARKCYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
    DARKRED = FOREGROUND_RED,
    DARKMAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
    DARKYELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
    DARKGRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    GRAY = FOREGROUND_INTENSITY,
    BLUE = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
    GREEN = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
    CYAN = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED = FOREGROUND_INTENSITY | FOREGROUND_RED,
    MAGENTA = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
    WHITE = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};


#define LOG_VERBOSE(fmt,...) { \
    Logger::GetInstance().Log(eLogLevel::Verbose, eConsoleForeground::GRAY, __FILE__,__LINE__,__func__,fmt,##__VA_ARGS__); \
}

#define LOG_HINT(fmt,...) { \
    Logger::GetInstance().Log(eLogLevel::Hint, eConsoleForeground::GREEN, __FILE__,__LINE__,__func__,fmt,##__VA_ARGS__); \
}

#define LOG_INFO(fmt,...) { \
    Logger::GetInstance().Log(eLogLevel::Info, eConsoleForeground::WHITE, __FILE__,__LINE__,__func__,fmt,##__VA_ARGS__); \
}

#define LOG_WARN(fmt,...) { \
    Logger::GetInstance().Log(eLogLevel::Warning, eConsoleForeground::YELLOW, __FILE__,__LINE__,__func__,fmt,##__VA_ARGS__); \
}

#define LOG_ERROR(fmt,...) { \
    Logger::GetInstance().Log(eLogLevel::Error, eConsoleForeground::RED, __FILE__,__LINE__,__func__,fmt,##__VA_ARGS__); \
}
#define LOG_FATAL(fmt,...) { \
    Logger::GetInstance().Log(eLogLevel::Fatal, eConsoleForeground::RED, __FILE__,__LINE__,__func__,fmt,##__VA_ARGS__); \
}


class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    void SetFileOutput(const std::string& filePath) {
        fileOut.open(filePath, std::ios::out | std::ios::app);
    }

    template<typename... Args>
    void Log(eLogLevel level, eConsoleForeground color, const char* _file, int line, const char* _func, std::format_string<Args...> fmt, Args&&... args) {
        if ((uint32_t)level < (uint32_t)m_logLevel)
            return;

        std::string f(_file);
        // file is constexpr so always valid and always will have at least one '\'
        f = f.substr(f.rfind('\\') + 1);
        
        auto t = std::time(nullptr);
        tm time = {};
        localtime_s(&time, &t);
        auto tid = std::this_thread::get_id();
        std::string formatted = std::format(fmt, std::forward<Args>(args)...);

        {
            std::lock_guard<std::mutex> lock(mtx);
            SetConsoleColor(color);
            std::cout << std::put_time(&time, "[%m-%d %H:%M]")
                << FormatPrefix(level)
                << "[tid:" << tid <<"]"
                << "[" << getPrettyTimestamp() << "]"
                << f << ":" << line << "[" << _func << "] "
                << formatted << std::endl;

            if (fileOut.is_open()) {
                fileOut << line << std::endl;
            }
        }
    }

private:
    std::mutex mtx;
    std::ofstream fileOut;
	eLogLevel m_logLevel = eLogLevel::Info; // Default log level
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    std::string FormatPrefix(eLogLevel level) {
        std::string lvl;
        switch (level) {
		case eLogLevel::Verbose: lvl = "[verb]"; break;
		case eLogLevel::Hint: lvl = "[hint]"; break;
        case eLogLevel::Info: lvl = "[info]"; break;
        case eLogLevel::Warning: lvl = "[warn]"; break;
        case eLogLevel::Error: lvl = "[error]"; break;
        case eLogLevel::Fatal: lvl = "[fatal]"; break;
        }
        return lvl;
    }

    static inline std::chrono::high_resolution_clock::time_point s_timeSinceBegin = std::chrono::high_resolution_clock::now();

    // Returns a microseconds string as seconds:mseconds:useconds
    inline std::string prettifyMicrosecondsString(const uint64_t microseconds)
    {
        auto tmp = microseconds;

        // Calculate seconds, milliseconds, and remaining microseconds
        uint64_t seconds = tmp / 1000000;
        tmp %= 1000000;
        uint64_t milliseconds = tmp / 1000;
        tmp %= 1000;

        // Format the result
        std::ostringstream formattedTime;
        formattedTime << seconds << "s:" << std::setw(3) << std::setfill('0') << milliseconds << "ms:" << std::setw(3) << std::setfill('0') << tmp << "us";
        return formattedTime.str();
    }

    // Records a timestamp and returns it as a seconds:mseconds:useconds string
    inline std::string getPrettyTimestamp()
    {
        std::chrono::duration<uint64_t, std::micro> sinceInit = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::micro>>
            (std::chrono::high_resolution_clock::now() - s_timeSinceBegin);
        return prettifyMicrosecondsString(sinceInit.count());
    }

    void SetConsoleColor(eConsoleForeground color) {
        SetConsoleTextAttribute(hConsole, (WORD)color);
    }

    void ResetConsoleColor() {
        SetConsoleTextAttribute(hConsole, (WORD)eConsoleForeground::WHITE);
    }
};