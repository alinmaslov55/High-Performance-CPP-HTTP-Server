#ifndef ENV_PARSER_HPP
#define ENV_PARSER_HPP

#include <string>

namespace http {
namespace utils {

class EnvParser {
public:
    static void load(const std::string& filepath = ".env");
};

} // namespace utils
} // namespace http

#endif // ENV_PARSER_HPP