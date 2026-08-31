#ifndef DISPATCH_H_DEFINED
#define DISPATCH_H_DEFINED

#include "cli/Invocation.h"
#include "core/Error.h"

Error Dispatch(Invocation const& _inc);

#endif
