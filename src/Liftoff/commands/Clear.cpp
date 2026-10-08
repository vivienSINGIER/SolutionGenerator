#include "Command.h"

namespace
{
    bool Prompt(fs::path const& _ide, bool _prompt)
    {
        std::vector<std::string> plan;

        plan.push_back("Delete the '" + _ide.string() + "' directory & its content");

        Cmd::PrintPlan(plan);

        if (_prompt == false)
            return true;
        return Cmd::ApplyPrompt(static_cast<int>(plan.size()));
    }
}

Error Cmd::Clear(Invocation const& _inv)
{
    bool isVerbose = _inv.globalFlags.verbose;

    fs::path root;
    if (Error e = Cmd::GetRootPath(root))
        return e;

    fs::path ide = root / "ide";

    if (_inv.globalFlags.dryRun)
    {
        Prompt(ide, false);
        return Error::Ok();
    }
    if (_inv.globalFlags.yes == false)
    {
        bool res = Prompt(ide, true);
        if (res == false)
            return Error::Declined("user declined any changes");
    }

    if (Error e = FileHelper::DeleteDir(ide))
        return e;
    Verbose("Deleted directory '" + ide.string() + "'", isVerbose);

    return Error::Ok();
}
