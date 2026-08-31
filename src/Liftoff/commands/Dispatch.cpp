#include "Dispatch.h"

Error Dispatch(Invocation const& _inc)
{
    switch (_inc.cmdId)
    {
        
    case CommandId::None:
    default:
        return Error::Validation("unhandled command");    
    }
}
