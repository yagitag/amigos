#ifndef __LOCALE__HPP__
#define __LOCALE__HPP__

#include <locale> 

namespace Common {

bool g_locale_init = false;

bool is_locale_init() 
{
    return g_locale_init;
}

/**
Examples:
    Common::init_locale("ru_RU.UTF8");
    Common::init_locale("en_US.UTF-8");
**/
void init_locale(const char * locale)
{
    std::locale loc(locale);
    std::locale::global(loc);

    g_locale_init = true;
}

} //namespace Common

#endif
