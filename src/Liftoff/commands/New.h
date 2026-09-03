#ifndef COMMANDS_NEW_HPP
#define COMMANDS_NEW_HPP

#include "cli/Invocation.h"
#include "core/Error.h"
#include "scan/Workspace.h"

namespace Cmd
{
    Error New(Workspace& _ws, Invocation const& _inv);
}


#endif
