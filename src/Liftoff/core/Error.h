#ifndef CORE_ERROR_H
#define CORE_ERROR_H

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class ErrorCode : int
{
    Ok         = 0,
    Validation = 1,   // bad JSON, missing field, unknown dependency, cycle, duplicate project
    Io         = 2,   // cannot read / write / delete, permission denied, path collision
    Declined   = 3,   // user answered no at a confirmation prompt
    Build      = 4,   // MSBuild returned non-zero
};

struct Error
{
    ErrorCode                code = ErrorCode::Ok;
    std::string              message;   // one human sentence, no trailing newline
    fs::path                 path;      // the file / dir involved, when there is one
    std::vector<std::string> details;   // every problem, for "report them all in one go" (validation)

    bool IsOk() const              { return code == ErrorCode::Ok; }
    explicit operator bool() const { return !IsOk(); }
    int  Exit() const              { return static_cast<int>(code); }

    // "I/O error: cannot replace X (path)" followed by one indented line per detail.
    std::string Format() const;

    static Error Ok();
    static Error Io(std::string _message, fs::path _path = {});
    static Error Validation(std::string _message);
    static Error Declined(std::string _message);
    static Error Build(std::string _message);
};

#endif
