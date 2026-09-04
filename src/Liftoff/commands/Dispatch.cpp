#include "Dispatch.h"

#include "New.h"
#include "ProjectAdd.h"
#include "ProjectList.h"
#include "ProjectRemove.h"
#include "ProjectSet.h"
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
