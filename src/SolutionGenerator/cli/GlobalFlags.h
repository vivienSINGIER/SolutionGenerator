#ifndef GLOBAL_FLAGS_H
#define GLOBAL_FLAGS_H

#include <filesystem>

namespace fs = std::filesystem;

struct GlobalFlags
{
    bool dryRun = false;
    bool yes = false;
    bool verbose = false;
    
    bool hasConfigPath = false;
    fs::path configPath;
};

#endif