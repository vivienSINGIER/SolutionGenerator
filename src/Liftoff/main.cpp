#include <iostream>

#include "cli/CommandTable.h"
#include "cli/Cli.h"
#include "commands/Dispatch.h"
#include "core/Logger.h"

char** MakeCharArray(const char* input) {
    std::vector<char*> temp;
    std::string s(input);
    size_t start = 0;

    while (start < s.size()) {
        size_t end = s.find(' ', start);
        if (end == std::string::npos) end = s.size();
        if (end > start) {
            std::string word = s.substr(start, end - start);
            char* copy = new char[word.size() + 1];
            strcpy_s(copy, word.size() + 1, word.c_str());
            temp.push_back(copy);
        }
        start = end + 1;
    }
    temp.push_back(nullptr); // null-terminate for the null-terminated version

    char** result = new char*[temp.size()];
    std::copy(temp.begin(), temp.end(), result);
    return result;
}

int main(int _argc, char** _argv)
{
    Invocation inv;
    Error e = Cli::Parse(_argc, _argv, inv);
    
    // Error e = Cli::Parse(4, MakeCharArray("./Liftoff new ../ --name"), inv);
    
    if (inv.done) 
        return e.Exit();
    if (e)
    {
        Log::Log(LogType::Error, e.Format() + '\n');
        return e.Exit();
    }
    
    e = Dispatch(inv);
    if (e)
    {
        Log::Log(LogType::Error, e.Format() + '\n');
        return e.Exit();
    }
        
    return 0;
}
