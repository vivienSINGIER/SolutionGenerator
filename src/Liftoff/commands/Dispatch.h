#ifndef DISPATCH_H_DEFINED
#define DISPATCH_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"

namespace Cmd
{
    Error Dispatch(Invocation const& _inv);
}
    
#endif
