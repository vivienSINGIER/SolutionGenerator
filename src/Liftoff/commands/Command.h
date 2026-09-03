#ifndef COMMAND_H_DEFINED
#define COMMAND_H_DEFINED

#include <string>
#include <vector>

namespace Cmd
{
    void PrintPlan(std::vector<std::string> _plan);
    bool ApplyPrompt(int _changeCount);
    
    void LogChange(std::string _change);
}

#endif