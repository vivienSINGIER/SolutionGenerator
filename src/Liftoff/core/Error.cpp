#include "core/Error.h"

namespace
{
    char const* Label(ErrorCode _code)
    {
        switch (_code)
        {
        case ErrorCode::Ok:         return "ok";
        case ErrorCode::Validation: return "validation error";
        case ErrorCode::Io:         return "I/O error";
        case ErrorCode::Declined:   return "declined";
        case ErrorCode::Build:      return "build error";
        }
        return "error";
    }
}

std::string Error::Format() const
{
    if (IsOk())
        return "ok";

    std::string out = Label(code);
    out += ": ";
    out += message;

    if (!path.empty())
    {
        out += " (";
        out += path.string();
        out += ')';
    }

    for (std::string const& detail : details)
    {
        out += "\n  - ";
        out += detail;
    }

    return out;
}

Error Error::Ok()
{
    return Error();
}

Error Error::Io(std::string _message, fs::path _path)
{
    Error e;
    e.code    = ErrorCode::Io;
    e.message = std::move(_message);
    e.path    = std::move(_path);
    return e;
}

Error Error::Validation(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Validation;
    e.message = std::move(_message);
    return e;
}

Error Error::Declined(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Declined;
    e.message = std::move(_message);
    return e;
}

Error Error::Build(std::string _message)
{
    Error e;
    e.code    = ErrorCode::Build;
    e.message = std::move(_message);
    return e;
}
