#ifndef PROJECTADD_H_DEFINED
#define PROJECTADD_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error ProjectAdd(Workspace& _ws, Invocation const& _inv);
}

#endif
