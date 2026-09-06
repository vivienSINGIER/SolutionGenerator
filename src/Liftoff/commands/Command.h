#ifndef COMMAND_H_DEFINED
#define COMMAND_H_DEFINED

#include <filesystem>
#include <string>
#include <vector>

#include "cli/Invocation.h"
#include "core/Error.h"
#include "io/FileHelper.h"
#include "scan/Workspace.h"

namespace Cmd
{
    void PrintPlan(std::vector<std::string> _plan);
    bool ApplyPrompt(int _changeCount);
    void LogChange(std::string _change);
    
    void Verbose(std::string _msg, bool _b);
    
    Error GetDefaultManifestPath(std::filesystem::path& _out);
    Error GetRootPath(std::filesystem::path& _out);
    
    Error New(Workspace& _ws, Invocation const& _inv);
    Error Set(Workspace& _ws, Invocation const& _inv);
    
    Error ProjectAdd(Workspace& _ws, Invocation const& _inv);
    Error ProjectRemove(Workspace& _ws, Invocation const& _inv);
    Error ProjectList(Workspace& _ws, Invocation const& _inv);
    Error ProjectSet(Invocation const& _inv);
    
    Error DepAdd(Workspace& _ws, Invocation const& _inv);
    Error DepRemove(Workspace& _ws, Invocation const& _inv);
    Error DepList(Workspace& _ws, Invocation const& _inv);
    
    Error SyncBats(Workspace& _ws, Invocation const& _inv);
}

#endif