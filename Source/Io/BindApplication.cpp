/*
** Lua binding: BindApplication
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua++.h"

/* Exported function */
TOLUA_API int tolua_BindApplication_open (lua_State* tolua_S);

#include <Urho3D/LuaScript/ToluaUtils.h>
#include "Application.h"
using namespace Urho3D;

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"PodApplication");
}

/* method: GetCircuitName of class  PodApplication */
#ifndef TOLUA_DISABLE_tolua_BindApplication_PodApplication_GetCircuitName00
static int tolua_BindApplication_PodApplication_GetCircuitName00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isusertype(tolua_S,1,"PodApplication",0,&tolua_err) ||
 !tolua_isnoobj(tolua_S,2,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
  PodApplication* self = (PodApplication*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in function 'GetCircuitName'", NULL);
#endif
 {
  String tolua_ret = (String)  self->GetCircuitName();
 tolua_pushurho3dstring(tolua_S,(const char*)tolua_ret);
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetCircuitName'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* function: GetApplication */
#ifndef TOLUA_DISABLE_tolua_BindApplication_GetApplication00
static int tolua_BindApplication_GetApplication00(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (
 !tolua_isnoobj(tolua_S,1,&tolua_err)
 )
 goto tolua_lerror;
 else
#endif
 {
 {
  PodApplication* tolua_ret = (PodApplication*)  GetApplication();
  tolua_pushusertype(tolua_S,(void*)tolua_ret,"PodApplication");
 }
 }
 return 1;
#ifndef TOLUA_RELEASE
 tolua_lerror:
 tolua_error(tolua_S,"#ferror in function 'GetApplication'.",&tolua_err);
 return 0;
#endif
}
#endif //#ifndef TOLUA_DISABLE

/* Open function */
TOLUA_API int tolua_BindApplication_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,0);
 tolua_beginmodule(tolua_S,NULL);
 tolua_cclass(tolua_S,"PodApplication","PodApplication","",NULL);
 tolua_beginmodule(tolua_S,"PodApplication");
  tolua_function(tolua_S,"GetCircuitName",tolua_BindApplication_PodApplication_GetCircuitName00);
 tolua_endmodule(tolua_S);
 tolua_function(tolua_S,"GetApplication",tolua_BindApplication_GetApplication00);
 tolua_endmodule(tolua_S);
 return 1;
}


#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 501
 TOLUA_API int luaopen_BindApplication (lua_State* tolua_S) {
 return tolua_BindApplication_open(tolua_S);
};
#endif

