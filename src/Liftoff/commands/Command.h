#ifndef COMMAND_H_DEFINED
#define COMMAND_H_DEFINED

#include <filesystem>
#include <string>
#include <vector>

#include "core/Error.h"
#include "io/FileHelper.h"

namespace Cmd
{
    void PrintPlan(std::vector<std::string> _plan);
    bool ApplyPrompt(int _changeCount);
    void LogChange(std::string _change);
    
    void Verbose(std::string _msg, bool _b);
    
    Error GetDefaultManifestPath(std::filesystem::path& _out);
    Error GetRootPath(std::filesystem::path& _out);
}

#endif