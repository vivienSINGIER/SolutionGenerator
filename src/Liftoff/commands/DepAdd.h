#ifndef DEPADD_H_DEFINED
#define DEPADD_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error DepAdd(Workspace& _ws, Invocation const& _inv);
}

#endif
