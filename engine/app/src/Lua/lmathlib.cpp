/*
** $Id: lmathlib.c,v 1.67.1.1 2007/12/27 13:02:25 roberto Exp $
** Standard mathematical library
** See Copyright Notice in lua.h
*/


#include <stdlib.h>
#include <math.h>

#define lmathlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"


#undef PI
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI / 180.0)

static int math_abs(lua_State* L)
{
    lua_pushnumber(L, fabs(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sin(lua_State* L)
{
    lua_pushnumber(L, sin(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sinh(lua_State* L)
{
    lua_pushnumber(L, sinh(luaL_checknumber(L, 1)));
    return 1;
}

static int math_cos(lua_State* L)
{
    lua_pushnumber(L, cos(luaL_checknumber(L, 1)));
    return 1;
}

static int math_cosh(lua_State* L)
{
    lua_pushnumber(L, cosh(luaL_checknumber(L, 1)));
    return 1;
}

static int math_tan(lua_State* L)
{
    lua_pushnumber(L, tan(luaL_checknumber(L, 1)));
    return 1;
}

static int math_tanh(lua_State* L)
{
    lua_pushnumber(L, tanh(luaL_checknumber(L, 1)));
    return 1;
}

static int math_asin(lua_State* L)
{
    lua_pushnumber(L, asin(luaL_checknumber(L, 1)));
    return 1;
}

static int math_acos(lua_State* L)
{
    lua_pushnumber(L, acos(luaL_checknumber(L, 1)));
    return 1;
}

static int math_atan(lua_State* L)
{
    lua_pushnumber(L, atan(luaL_checknumber(L, 1)));
    return 1;
}

static int math_atan2(lua_State* L)
{
    lua_pushnumber(L, atan2(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int math_ceil(lua_State* L)
{
    lua_pushnumber(L, ceil(luaL_checknumber(L, 1)));
    return 1;
}

static int math_floor(lua_State* L)
{
    lua_pushnumber(L, floor(luaL_checknumber(L, 1)));
    return 1;
}

static int math_fmod(lua_State* L)
{
    lua_pushnumber(L, fmod(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int math_modf(lua_State* L)
{
    double ip;
    double fp = modf(luaL_checknumber(L, 1), &ip);
    lua_pushnumber(L, ip);
    lua_pushnumber(L, fp);
    return 2;
}

static int math_sqrt(lua_State* L)
{
    lua_pushnumber(L, sqrt(luaL_checknumber(L, 1)));
    return 1;
}

static int math_pow(lua_State* L)
{
    lua_pushnumber(L, pow(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int math_log(lua_State* L)
{
    lua_pushnumber(L, log(luaL_checknumber(L, 1)));
    return 1;
}

static int math_log10(lua_State* L)
{
    lua_pushnumber(L, log10(luaL_checknumber(L, 1)));
    return 1;
}

static int math_exp(lua_State* L)
{
    lua_pushnumber(L, exp(luaL_checknumber(L, 1)));
    return 1;
}

static int math_deg(lua_State* L)
{
    lua_pushnumber(L, luaL_checknumber(L, 1) / RADIANS_PER_DEGREE);
    return 1;
}

static int math_rad(lua_State* L)
{
    lua_pushnumber(L, luaL_checknumber(L, 1) * RADIANS_PER_DEGREE);
    return 1;
}

static int math_frexp(lua_State* L)
{
    int e;
    lua_pushnumber(L, frexp(luaL_checknumber(L, 1), &e));
    lua_pushinteger(L, e);
    return 2;
}

static int math_ldexp(lua_State* L)
{
    lua_pushnumber(L, ldexp(luaL_checknumber(L, 1), luaL_checkint(L, 2)));
    return 1;
}



static int math_min(lua_State* L)
{
    int n = lua_gettop(L); /* number of arguments */
    lua_Number dmin = luaL_checknumber(L, 1);
    int i;
    for (i = 2; i <= n; i++)
    {
        lua_Number d = luaL_checknumber(L, i);
        if (d < dmin)
            dmin = d;
    }
    lua_pushnumber(L, dmin);
    return 1;
}


static int math_max(lua_State* L)
{
    int n = lua_gettop(L); /* number of arguments */
    lua_Number dmax = luaL_checknumber(L, 1);
    int i;
    for (i = 2; i <= n; i++)
    {
        lua_Number d = luaL_checknumber(L, i);
        if (d > dmax)
            dmax = d;
    }
    lua_pushnumber(L, dmax);
    return 1;
}


static int math_random(lua_State* L)
{
    /* the `%' avoids the (rare) case of r==1, and is needed also because on
       some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
    lua_Number r = (lua_Number)(rand() % RAND_MAX) / (lua_Number)RAND_MAX;
    switch (lua_gettop(L))
    { /* check number of arguments */
    case 0:
    {                         /* no arguments */
        lua_pushnumber(L, r); /* Number between 0 and 1 */
        break;
    }
    case 1:
    { /* only upper limit */
        int u = luaL_checkint(L, 1);
        luaL_argcheck(L, 1 <= u, 1, "interval is empty");
        lua_pushnumber(L, floor(r * u) + 1); /* int between 1 and `u' */
        break;
    }
    case 2:
    { /* lower and upper limits */
        int l = luaL_checkint(L, 1);
        int u = luaL_checkint(L, 2);
        luaL_argcheck(L, l <= u, 2, "interval is empty");
        lua_pushnumber(L, floor(r * (u - l + 1)) + l); /* int between `l' and `u' */
        break;
    }
    default:
        return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}


static int math_randomseed(lua_State* L)
{
    srand(luaL_checkint(L, 1));
    return 0;
}

static int math_clamp(lua_State* L)
{
    lua_Number val = luaL_checknumber(L, 1);
    lua_Number lower = luaL_checknumber(L, 2);
    lua_Number upper = luaL_checknumber(L, 3);

    if (val < lower)
        val = lower;
    else if (val > upper)
        val = upper;

    lua_pushnumber(L, val);
    return 1;
}

static int math_sign(lua_State* L)
{
    lua_Number val = luaL_checknumber(L, 1);
    if (val > 0)
        lua_pushnumber(L, 1);
    else if (val < 0)
        lua_pushnumber(L, -1);
    else
        lua_pushnumber(L, 0);
    return 1;
}

static int math_round(lua_State* L)
{
    lua_Number val = luaL_checknumber(L, 1);
    lua_pushnumber(L, floor(val + 0.5));
    return 1;
}

static const unsigned char kPerlinHash[257] = {151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8,
    99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87,
    174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92,
    41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159,
    86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58,
    17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108,
    110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249,
    14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29,
    24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180, 151};

const float kPerlinGrad[16][3] = {{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},
    {0, -1, 1}, {0, 1, -1}, {0, -1, -1}, {1, 1, 0}, {0, -1, 1}, {-1, 1, 0}, {0, -1, -1}};

inline float perlin_fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float perlin_lerp(float t, float a, float b)
{
    return a + t * (b - a);
}

inline float perlin_grad(int hash, float x, float y, float z)
{
    const float* g = kPerlinGrad[hash & 15];
    return g[0] * x + g[1] * y + g[2] * z;
}

static float perlin(float x, float y, float z)
{
    float xflr = floorf(x);
    float yflr = floorf(y);
    float zflr = floorf(z);

    int xi = int(xflr) & 255;
    int yi = int(yflr) & 255;
    int zi = int(zflr) & 255;

    float xf = x - xflr;
    float yf = y - yflr;
    float zf = z - zflr;

    float u = perlin_fade(xf);
    float v = perlin_fade(yf);
    float w = perlin_fade(zf);

    const unsigned char* p = kPerlinHash;

    int a = (p[xi] + yi) & 255;
    int aa = (p[a] + zi) & 255;
    int ab = (p[a + 1] + zi) & 255;

    int b = (p[xi + 1] + yi) & 255;
    int ba = (p[b] + zi) & 255;
    int bb = (p[b + 1] + zi) & 255;

    float la = perlin_lerp(u, perlin_grad(p[aa], xf, yf, zf), perlin_grad(p[ba], xf - 1, yf, zf));
    float lb = perlin_lerp(u, perlin_grad(p[ab], xf, yf - 1, zf), perlin_grad(p[bb], xf - 1, yf - 1, zf));
    float la1 = perlin_lerp(u, perlin_grad(p[aa + 1], xf, yf, zf - 1), perlin_grad(p[ba + 1], xf - 1, yf, zf - 1));
    float lb1 = perlin_lerp(u, perlin_grad(p[ab + 1], xf, yf - 1, zf - 1), perlin_grad(p[bb + 1], xf - 1, yf - 1, zf - 1));

    return perlin_lerp(w, perlin_lerp(v, la, lb), perlin_lerp(v, la1, lb1));
}

static int math_noise(lua_State* L)
{
    lua_Number x = luaL_optnumber(L, 1, 0.0);
    lua_Number y = luaL_optnumber(L, 2, 0.0);
    lua_Number z = luaL_optnumber(L, 3, 0.0);

    lua_pushnumber(L, perlin(x, y, z));
    return 1;
}

static int math_lerp(lua_State* L)
{
    lua_Number a = luaL_checknumber(L, 1);
    lua_Number b = luaL_checknumber(L, 2);
    lua_Number t = luaL_checknumber(L, 3);

    lua_pushnumber(L, a + (b - a) * t);
    return 1;
}

static int math_map(lua_State* L)
{
    lua_Number value = luaL_checknumber(L, 1);
    lua_Number inMin = luaL_checknumber(L, 2);
    lua_Number inMax = luaL_checknumber(L, 3);
    lua_Number outMin = luaL_checknumber(L, 4);
    lua_Number outMax = luaL_checknumber(L, 5);

    luaL_argcheck(L, inMin != inMax, 3, "input range cannot be zero");

    lua_Number mappedValue = (value - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
    lua_pushnumber(L, mappedValue);
    return 1;
}

static const luaL_Reg mathlib[] = {{"abs", math_abs}, {"acos", math_acos}, {"asin", math_asin}, {"atan2", math_atan2}, {"atan", math_atan},
    {"ceil", math_ceil}, {"cosh", math_cosh}, {"cos", math_cos}, {"deg", math_deg}, {"exp", math_exp}, {"floor", math_floor}, {"fmod", math_fmod},
    {"frexp", math_frexp}, {"ldexp", math_ldexp}, {"log10", math_log10}, {"log", math_log}, {"max", math_max}, {"min", math_min}, {"modf", math_modf},
    {"pow", math_pow}, {"rad", math_rad}, {"random", math_random}, {"randomseed", math_randomseed}, {"sinh", math_sinh}, {"sin", math_sin},
    {"sqrt", math_sqrt}, {"tanh", math_tanh}, {"tan", math_tan}, {"clamp", math_clamp}, {"sign", math_sign}, {"round", math_round},
    {"noise", math_noise}, {"lerp", math_lerp}, {"map", math_map}, {NULL, NULL}};


/*
** Open math library
*/
LUALIB_API int luaopen_math(lua_State* L)
{
    luaL_register(L, LUA_MATHLIBNAME, mathlib);
    lua_pushnumber(L, PI);
    lua_setfield(L, -2, "pi");
    lua_pushnumber(L, HUGE_VAL);
    lua_setfield(L, -2, "huge");
#if defined(LUA_COMPAT_MOD)
    lua_getfield(L, -1, "fmod");
    lua_setfield(L, -2, "mod");
#endif
    return 1;
}
