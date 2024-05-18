#include "utils/logger.h"

Logger::Logger(const std::string& filename) : start_time_(std::chrono::high_resolution_clock::now()) {
    log_file_.open(filename, std::ios::out | std::ios::app);
    log_file_ << "FunctionName,StartTime,EndTime,Duration\n";  // CSV Header
}

Logger::~Logger() {
    log_file_.close();
}

void Logger::logFunctionStart(const std::string& function_name) {
    start_time_ = std::chrono::high_resolution_clock::now();
    current_function_ = function_name;
}

void Logger::logFunctionEnd() {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_).count();
    log_file_ << current_function_ << "," << start_time_.time_since_epoch().count() << ","
              << end_time.time_since_epoch().count() << "," << duration << "\n";
}
