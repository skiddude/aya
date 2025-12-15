

#pragma once

#ifndef Aya_strcasestr
namespace Aya
{

/* GCC often has strcasestr(); if not, you can use the following */

/* borrowed these definitions from Apache */
#define ap_tolower(c) (tolower(((unsigned char)(c))))
#define ap_toupper(c) (toupper(((unsigned char)(c))))

static const char* Aya_strcasestr(const char* h, const char* n)
{
    if (!h || !*h || !n || !*n)
    {
        return 0;
    }
    char *a = (char*)h, *e = (char*)n;
    while (*a && *e)
    {
        if (ap_toupper(*a) != ap_toupper(*e))
        {
            ++h;
            a = (char*)h;
            e = (char*)n;
        }
        else
        {
            ++a;
            ++e;
        }
    }
    return (const char*)(*e) ? 0 : h;
}
static inline const char* Aya_strcasestr(const char* h, char* n)
{
    return Aya_strcasestr(h, static_cast<const char*>(n));
}

static inline const char* Aya_strcasestr(char* h, const char* n)
{
    return Aya_strcasestr(static_cast<const char*>(h), n);
}
} // namespace Aya
#endif // defined Aya_strcasestr
