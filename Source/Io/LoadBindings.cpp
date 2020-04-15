#include "LoadBindings.h"

void LoadBindings(lua_State* L)
{
    tolua_BindApplication_open(L);
}