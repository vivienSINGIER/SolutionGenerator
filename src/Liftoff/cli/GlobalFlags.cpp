#include "GlobalFlags.h"

namespace GlobalFlagsTable
{
    static GlobalFlagDef const g_globalFlags[] = 
    {                                                                                                                                                                        
        { GlobalId::DryRun,  "dry-run", 'n',  false, "print the plan (files to create, modify, delete) and change nothing" },
        { GlobalId::Yes,     "yes",     'y',  false, "assume yes on confirmation prompts" },
        { GlobalId::Verbose, "verbose", 'v',  false, "verbose logging" },
        { GlobalId::Help,    "help",    'h',  false, "show help for the current command" },
        { GlobalId::Version, "version", '\0', false, "print the version and exit" },
        { GlobalId::Config,  "config",  '\0', true , "explicit path to solution.json (skips the upward search)" },
    };

    std::span<GlobalFlagDef const> All()
    {
        return g_globalFlags;
    }
}
