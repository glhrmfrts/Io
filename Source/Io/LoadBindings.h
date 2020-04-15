#pragma once

#include <lua.h>

/* Exported function */
extern int tolua_BindApplication_open (lua_State* tolua_S);

void LoadBindings(lua_State* L);