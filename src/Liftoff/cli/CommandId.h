#ifndef COMMAND_ID_H
#define COMMAND_ID_H

enum class CommandId
{
    None = 0,
    New, 
    ProjectAdd, ProjectRemove, ProjectList, ProjectSet,
    DepAdd, DepRemove, DepList,
    VendorAdd, VendorRemove, VendorList,
    Set, SyncBats,
    Make, Build, Run, Clean, Clear, Rebuild,
};

#endif