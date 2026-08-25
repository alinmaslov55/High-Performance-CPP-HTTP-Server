#include <http/http/HttpParser.hpp>

#include <stdexcept>

namespace http {

namespace utils {

HttpMethod parseMethod(std::string_view method){
    if(method == "GET") return HttpMethod::GET;
    if(method == "POST") return HttpMethod::POST;
    if(method == "PUT") return HttpMethod::PUT;
    if(method == "PATCH") return HttpMethod::PATCH;
    if(method == "DELETE") return HttpMethod::DELETE;
    if(method == "HEAD") return HttpMethod::HEAD;
    if(method == "OPTIONS") return HttpMethod::OPTIONS;
    if(method == "CONNECT") return HttpMethod::CONNECT;
    if(method == "TRACE") return HttpMethod::TRACE;


    return HttpMethod::UNKNOWN;
}

std::string_view trimOws(std::string_view value){
    while (!value.empty() &&
           (value.front() == ' ' ||
            value.front() == '\t'))
    {
        value.remove_prefix(1);
    }

    while (!value.empty() &&
           (value.back() == ' ' ||
            value.back() == '\t'))
    {
        value.remove_suffix(1);
    }

    return value;
}

bool isValidHeaderName(std::string_view name){
    if(name.empty()){
        return false;
    }

    for (const unsigned char character : name)
    {
        const bool valid =
            std::isalnum(character) ||
            character == '!' ||
            character == '#' ||
            character == '$' ||
            character == '%' ||
            character == '&' ||
            character == '\'' ||
            character == '*' ||
            character == '+' ||
            character == '-' ||
            character == '.' ||
            character == '^' ||
            character == '_' ||
            character == '`' ||
            character == '|' ||
            character == '~';

        if (!valid)
        {
            return false;
        }
    }

    return true;
}

bool isValidHeaderValue(
    std::string_view value
)
{
    for (const unsigned char character : value)
    {
        if (character == '\r' ||
            character == '\n')
        {
            return false;
        }

        if (character < 0x20 &&
            character != '\t')
        {
            return false;
        }
    }

    return true;
}

} // namespace utils

ParseResult HttpParser::parse(
    std::string_view data,
    HttpRequest& request
) {
    while (true)
    {
        switch (state_)
        {
        case State::RequestLine:
        {
            // TODO
            break;
        }

        case State::Headers:
        {
            // TODO
            break;
        }

        case State::Body:
        {
            // TODO
            break;
        }

        case State::Complete:
            return ParseResult::Complete;

        case State::Error:
            return ParseResult::Invalid;
        }
    }
}

ParseResult HttpParser::parseRequestLine(std::string_view data, HttpRequest& request) const {
    const std::size_t lineEnd = data.find("\r\n");

    if(lineEnd == std::string_view::npos){
        return ParseResult::Incomplete;
    }

    const std::string_view line = data.substr(0, lineEnd);

    const std::size_t firstSpace = line.find(' ');

    if(firstSpace == std::string_view::npos){
        return ParseResult::Invalid;
    }

    const std::size_t secondSpace = line.find(' ', firstSpace + 1);

    if(secondSpace == std::string_view::npos){
        return ParseResult::Invalid;
    }

    const std::string_view method = line.substr(0, firstSpace);

    const std::string_view target = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    const std::string_view version = line.substr(secondSpace + 1);

    if(target.empty()){
        return ParseResult::Invalid;
    }

    if(version != "HTTP/1.1"){
        return ParseResult::Invalid;
    }

    const HttpMethod parsedMethod = utils::parseMethod(method);

    if(parsedMethod == HttpMethod::UNKNOWN){
        return ParseResult::Invalid;
    }

    request = HttpRequest(parsedMethod, std::string(target), std::string(version));

    return ParseResult::Complete;
}

ParseResult HttpParser::parseHeaders(std::string_view data, HttpRequest& request) const{
    const std::size_t headersEnd =
        data.find("\r\n\r\n");

    if (headersEnd == std::string_view::npos)
    {
        return ParseResult::Incomplete;
    }

    if (headersEnd > MAX_HEADER_SIZE)
    {
        return ParseResult::Invalid;
    }

    std::size_t position = 0;
    std::size_t headerCount = 0;

    while (position < headersEnd)
    {
        const std::size_t lineEnd =
            data.find("\r\n", position);

        if (lineEnd == std::string_view::npos ||
            lineEnd > headersEnd)
        {
            return ParseResult::Invalid;
        }

        const std::string_view line =
            data.substr(
                position,
                lineEnd - position
            );

        const std::size_t colon =
            line.find(':');

        if (colon == std::string_view::npos)
        {
            return ParseResult::Invalid;
        }

        const std::string_view name =
            line.substr(0, colon);

        const std::string_view value =
            utils::trimOws(line.substr(colon + 1));

        if (!utils::isValidHeaderName(name))
        {
            return ParseResult::Invalid;
        }

        if(!utils::isValidHeaderValue(value));

        ++headerCount;

        if(headerCount > MAX_HEADERS){
            return ParseResult::Invalid;
        }

        request.setHeader(
            std::string(name),
            std::string(value)
        );

        position = lineEnd + 2;
    }

    return ParseResult::Complete;
}

void HttpParser::reset() noexcept
{
    state_ = State::RequestLine;
    consumed_ = 0;

} // namespace http