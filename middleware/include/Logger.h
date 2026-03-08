#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <ctime>

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

class Logger {
public:
    class LogEntry {
    public:
        LogEntry(LogLevel lvl, const std::string& msg, const std::string& component)
            : m_lvl(lvl), m_msg(msg), m_component(component) {}

        LogEntry& field(const std::string& key, const std::string& value) {
            m_extra += ",\"" + escape(key) + "\":\"" + escape(value) + "\"";
            return *this;
        }
        LogEntry& field(const std::string& key, int value) {
            m_extra += ",\"" + escape(key) + "\":" + std::to_string(value);
            return *this;
        }

        void log() const {
            if (m_lvl < Logger::s_minLevel) return;
            std::ostringstream oss;
            oss << "{"
                << "\"level\":\"" << lvlStr() << "\""
                << ",\"ts\":\""   << now()    << "\""
                << ",\"component\":\"" << escape(m_component) << "\""
                << ",\"msg\":\""  << escape(m_msg) << "\""
                << m_extra << "}";
            std::lock_guard<std::mutex> lk(Logger::s_mutex);
            if (m_lvl >= LogLevel::ERROR) std::cerr << oss.str() << "\n";
            else                          std::cout << oss.str() << "\n";
        }

    private:
        LogLevel    m_lvl;
        std::string m_msg, m_component, m_extra;

        std::string lvlStr() const {
            switch (m_lvl) {
                case LogLevel::DEBUG: return "debug";
                case LogLevel::INFO:  return "info";
                case LogLevel::WARN:  return "warn";
                case LogLevel::ERROR: return "error";
            }
            return "unknown";
        }
        static std::string now() {
            auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm tm{};
            #ifdef _WIN32
                gmtime_s(&tm, &t);
            #else
                gmtime_r(&t, &tm);
            char buf[32]; std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
            return buf;
        }
        static std::string escape(const std::string& s) {
            std::ostringstream o;
            for (char c : s) {
                if      (c=='"')  o<<"\\\"";
                else if (c=='\\') o<<"\\\\";
                else if (c=='\n') o<<"\\n";
                else              o<<c;
            }
            return o.str();
        }
    }; // end LogEntry

    static void setLevel(const std::string& l) {
        if      (l=="debug") s_minLevel=LogLevel::DEBUG;
        else if (l=="info")  s_minLevel=LogLevel::INFO;
        else if (l=="warn")  s_minLevel=LogLevel::WARN;
        else if (l=="error") s_minLevel=LogLevel::ERROR;
    }
    static LogEntry debug(const std::string& m, const std::string& c="middleware") { return {LogLevel::DEBUG,m,c}; }
    static LogEntry info (const std::string& m, const std::string& c="middleware") { return {LogLevel::INFO, m,c}; }
    static LogEntry warn (const std::string& m, const std::string& c="middleware") { return {LogLevel::WARN, m,c}; }
    static LogEntry error(const std::string& m, const std::string& c="middleware") { return {LogLevel::ERROR,m,c}; }

private:
    static LogLevel   s_minLevel;
    static std::mutex s_mutex;
};

inline LogLevel   Logger::s_minLevel = LogLevel::INFO;
inline std::mutex Logger::s_mutex;
