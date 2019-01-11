/* 
 * File:   luafunction.h
 * Author: dreamszhu
 *
 * Created on 2009年11月12日, 上午9:49
 */

#ifndef _LUAFUNCTION_H
#define	_LUAFUNCTION_H

//lua
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>

lua_State *lua_init();
int lua_getfieldint(lua_State *L, char *key);
char *lua_getfieldstring(lua_State *L, char *key);
char *lua_getstring(lua_State *L, char *key);
int lua_getnumber(lua_State *L, char *key);
void lua_exec(int fd, char *str);

//register function
void lua_register_function(lua_State *L);
int lua_send_message(lua_State *L);
int lua_send_message_all(lua_State *L);


void lua_load_config();
void lua_load_room();
char *lua_getcrossdomain();
char *lua_getaccountkey(char *account);
int lua_getuserrole(const char *username);
int lua_getuserpasswd(const char *username, char *password, char *salt);

#endif	/* _LUAFUNCTION_H */

