#include <iostream>

#include "cli/CommandTable.h"

int main()
{
    std::string commands[] = {
      "frob"
    };
    
    CommandSpec const* spec;
    size_t tokenCount;
    Error e = CommandTable::Resolve(commands, spec, tokenCount);
    
    if (e)                                                                                                                                                                                                                       
        std::cout << e.Format() << "\n";                                                                                                                                                                                         
    else                                                                                                                                                                                                                         
        std::cout << "resolved '" << spec->Path() << "', consumed " << tokenCount << "\n";
    
    return 0;
}
