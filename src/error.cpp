#include "mcp/error.hpp"

namespace mcp {

Error::Error()
    : code_(kOk)
    , message_()
{
}

Error::Error(int code, const char* message)
    : code_(kOk)
    , message_()
{
    set(code, message);
}

bool Error::failed() const
{
    return code_ != kOk;
}

int Error::code() const
{
    return code_;
}

const std::string& Error::message() const
{
    return message_;
}

void Error::clear()
{
    code_ = kOk;
    message_.clear();
}

void Error::set(int code, const char* message)
{
    code_ = code;
    if (message == 0) {
        message_.clear();
    } else {
        message_ = message;
    }
}

}
