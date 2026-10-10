#include "Dispatch.h"

#include "Command.h"

#include "scan/Workspace.h"

Error Cmd::Dispatch(Invocation const& _inv)
{
    Workspace ws;
    
    switch (_inv.cmdId)
    {
    case CommandId::New:
        return Cmd::New(ws, _inv);
    case CommandId::ProjectAdd:
        return Cmd::ProjectAdd(ws, _inv);
    case CommandId::ProjectRemove:
        return Cmd::ProjectRemove(ws, _inv);
    case CommandId::ProjectList:
        return Cmd::ProjectList(ws, _inv);
    case CommandId::ProjectSet:
        return Cmd::ProjectSet(_inv);
    case CommandId::DepAdd:
        return Cmd::DepAdd(ws, _inv);
    case CommandId::DepRemove:
        return Cmd::DepRemove(ws, _inv);
    case CommandId::DepList:
        return Cmd::DepList(ws, _inv);
    case CommandId::Set:
        return Cmd::Set(ws, _inv);
    case CommandId::SyncBats:
        return Cmd::SyncBats(ws, _inv);
    case CommandId::Make:
        return Cmd::Make(ws, _inv);
    case CommandId::Clean:
        return Cmd::Clean(ws, _inv);
    case CommandId::Clear:
        return Cmd::Clear(_inv);
    case CommandId::VendorAdd:
        return Cmd::VendorAdd(ws, _inv);
    case CommandId::VendorRemove:
        return Cmd::VendorRemove(ws, _inv);
    case CommandId::VendorList:
        return Cmd::VendorList(ws, _inv);
    case CommandId::None:
        return Error::Validation("unhandled command");    
    }
}
