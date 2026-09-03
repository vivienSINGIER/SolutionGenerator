#include "Dispatch.h"

#include "New.h"
#include "scan/Workspace.h"

Error Cmd::Dispatch(Invocation const& _inv)
{
    Workspace ws;
    
    switch (_inv.cmdId)
    {
    case CommandId::New:
        return Cmd::New(ws, _inv);
    case CommandId::ProjectAdd:
    case CommandId::ProjectRemove:
    case CommandId::ProjectList:
    case CommandId::ProjectSet:
    case CommandId::DepAdd:
    case CommandId::DepRemove:
    case CommandId::DepList:
    case CommandId::VendorAdd:
    case CommandId::VendorRemove:
    case CommandId::VendorList:
    case CommandId::Set:
    case CommandId::SyncBats:
    case CommandId::Make:
    case CommandId::Build:
    case CommandId::Run:
    case CommandId::Clean:
    case CommandId::Clear:
    case CommandId::Rebuild:
    case CommandId::None:
        return Error::Validation("unhandled command");    
    }
}
