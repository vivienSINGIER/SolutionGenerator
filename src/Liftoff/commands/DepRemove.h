#ifndef DEPREMOVE_H_DEFINED
#define DEPREMOVE_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error DepRemove(Workspace& _ws, Invocation const& _inv);
}

#endif
