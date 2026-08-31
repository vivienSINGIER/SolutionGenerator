#ifndef INVOCATION_H
#define INVOCATION_H

#include "ArgBag.h"
#include "CommandId.h"
#include "GlobalFlags.h"

struct Invocation
{
    CommandId cmdId = CommandId::None;
    ArgBag argBag;
    GlobalFlags globalFlags;
    bool done = false;
};

#endif
