#ifndef PROJECTLIST_H_DEFINED
#define PROJECTLIST_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error ProjectList(Workspace& _ws, Invocation const& _inv);
}

#endif
