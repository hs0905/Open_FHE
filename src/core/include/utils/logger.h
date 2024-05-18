#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <chrono>
#include <string>

class Logger {
public:
    Logger(const std::string& filename);
    ~Logger();

    void logFunctionStart(const std::string& function_name);
    void logFunctionEnd();

private:
    std::ofstream log_file_;
    std::chrono::high_resolution_clock::time_point start_time_;
    std::string current_function_;
};

#endif // LOGGER_H
