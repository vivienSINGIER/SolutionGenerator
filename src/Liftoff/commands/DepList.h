#ifndef DEPLIST_H_DEFINED
#define DEPLIST_H_DEFINED
#include "cli/Invocation.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error DepList(Workspace& _ws, Invocation const& _inv);
}

#endif
