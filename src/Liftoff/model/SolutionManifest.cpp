#include "SolutionManifest.h"

SolutionManifest::SolutionManifest()
{
    m_schemaVersion = 1;
    
    m_name = "MySolution";
    m_startProject = "";
    
    m_configs = { "Debug", "Release" };
    m_platform = "x64";
    m_toolset = "v143";
    m_cppStandard = "20";
    
    m_characterSet = "Unicode";
    m_conformanceMode = true;
    m_warningLvl = "W4";
    m_warningsAsErrors = false;
}

Error SolutionManifest::Serialize(js& _js)
{
    if ()
}




