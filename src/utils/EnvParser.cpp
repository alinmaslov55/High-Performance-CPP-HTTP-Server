#include "http/utils/EnvParser.hpp"
#include "http/utils/Logger.hpp"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

namespace http {
namespace utils {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void EnvParser::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_WARN("Could not open {}, Relying on system env vars", filepath);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = trim(line.substr(0, delimiter_pos));
            std::string value = trim(line.substr(delimiter_pos + 1));

            // Strip quotes if they exist
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            // Set the environment variable (1 = overwrite existing)
            setenv(key.c_str(), value.c_str(), 1);
        }
    }
}

} // namespace utils
} // namespace http