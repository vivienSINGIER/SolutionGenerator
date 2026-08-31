#ifndef GLOBAL_FLAGS_H
#define GLOBAL_FLAGS_H

#include <filesystem>
#include <span>

namespace fs = std::filesystem;

enum class GlobalId : int { DryRun, Yes, Verbose, Help, Version, Config };                                                                                                                                                             
                                                                                                                                                                                                                                   
struct GlobalFlagDef
{
    GlobalId id; 
    std::string name; 
    char alias; 
    bool takesValue; 
    std::string help;
};   

namespace GlobalFlagsTable
{
    std::span<GlobalFlagDef const> All();   
}

struct GlobalFlags
{
    bool dryRun = false;
    bool yes = false;
    bool verbose = false;
    
    bool hasConfigPath = false;
    fs::path configPath;
};



#endif