#ifndef PROJECTREMOVE_H_DEFINED
#define PROJECTREMOVE_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error ProjectRemove(Workspace& _ws, Invocation const& _inv);
}

#endif
