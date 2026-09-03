#ifndef MANIFEST_INL_DEFINED
#define MANIFEST_INL_DEFINED

// Definitions for the templates declared in Manifest.h.
// Included at the bottom of Manifest.h - do not include directly.

namespace Manifest
{
    template <typename T>
    bool ReadRequired(js const& _js, char const* _key, T& _out, Error& _err)
    {
        if (!_js.contains(_key))
        {
            _err.details.push_back(std::string("missing required field '") + _key + "'");
            return false;
        }
        try
        {
            _out = _js.at(_key).get<T>();
            return true;
        }
        catch (js::exception const&)
        {
            _err.details.push_back(std::string("field '") + _key + "' has the wrong type");
            return false;
        }
    }

    template <typename T>
    void ReadOptional(js const& _js, char const* _key, T& _out)
    {
        if (_js.contains(_key))
            _out = _js.at(_key).get<T>();
    }
}

#endif
