#include "MyleftServer.h"

// lua 脚本里面的函数由C调用

lua_State *lua_init() {
    /* 初始化Lua */
    //lua_State *L = lua_open();
	lua_State* L = luaL_newstate();  
    /* 载入Lua基本库 */
    luaL_openlibs(L);

    return L;
}

char *lua_getstring(lua_State *L, char *key) {
    lua_getglobal(L, key);
    char *str = NULL;
    if (lua_isstring(L, -1)) {
        int len = lua_rawlen(L, -1);
        str = (char *) malloc(len + 1);
        memset(str, '\0', len + 1);
        strncpy(str, lua_tostring(L, -1), len);
    }
    lua_pop(L, 1);
    return str;
}

int lua_getnumber(lua_State *L, char *key) {
    lua_getglobal(L, key);
    int num = 0;
    if (lua_isnumber(L, -1)) {
        num = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return num;
}

int lua_send_message(lua_State *L) {
    int n = lua_gettop(L);
    if (n != 2) {
        lua_pushstring(L, "参数个数不对");
        lua_error(L);
    }
    int fd;
    char message[MAX_BUFFER_LENGTH];
    bzero(message, sizeof (message));
    if (!lua_isnumber(L, 1)) {
        lua_pushstring(L, "第一个参数类型错误，应该是socket句柄");
        lua_error(L);
    }
    fd = (int) lua_tointeger(L, 1);

    if (!lua_isstring(L, 2)) {
        lua_pushstring(L, "第二个参数类型错误!");
        lua_error(L);
    }
    //strcpy(message, (char*)lua_tostring(L, 2));
    strncpy(message, (char*) lua_tostring(L, 2), sizeof (message));
    log_write(LOG_DEBUG, "fd:%d, message:%s, file:%s, line:%d\n", fd, message, __FILE__, __LINE__);
    if (fd > 0 && strlen(message) > 0) {
        send_message(fd, message);
    }
    return 0;
}

int lua_send_message_all(lua_State *L) {
    int n = lua_gettop(L);
    if (n != 1) {
        lua_pushstring(L, "参数个数不对");
        lua_error(L);
    }

    char message[MAX_BUFFER_LENGTH];
    bzero(message, sizeof (message));
    if (!lua_isstring(L, 1)) {
        lua_pushstring(L, "第一个参数类型错误!");
        lua_error(L);
    }

    strncpy(message, lua_tostring(L, 1), sizeof (message));
    log_write(LOG_DEBUG, "message:%s, file:%s, line:%d\n", message, __FILE__, __LINE__);
    if (strlen(message) > 0) {
        //send_message_all(message);
    }
    return 0;
}

static int c_cont(lua_State *L) {                                     
  /* 这里什么都不用做：因为你的原函数里面就没做什么 */               
  return 0;                                                           
}                                                                     
                                                                       
static int c_callback(lua_State *L){                                  
  /* 使用 lua_pcallk，而不是lua_pcall */                             
  int ret = lua_pcallk(L, 0, 0, 0, 0, c_cont);                        
  if(ret) {                                                           
    fprintf(stderr, "Error: %s\n", lua_tostring(L, -1));              
    lua_pop(L, 1);                                                    
    exit(1);                                                          
  }                                                                   
  /* 因为你这里什么都没做，所以c_cont里面才什么都没有。如果这里需要做 
   * 什么东西，将所有内容挪到c_cont里面去，然后在这里简单地调用       
   * return c_cont(L);                                                
   * 即可。                                                           
   */                                                                
  return 0;                                                           
}                                                                     
                                                                       
static const luaL_Reg c[] = {                                         
  {"callback", c_callback},                                           
  {NULL, NULL}                                                        
};                                                                    
                                                                       
LUALIB_API int luaopen_c (lua_State *L) {                             
  /* 使用新的 luaL_newlib 函数 */                                    
  luaL_newlib(L, c);                                                  
  return 1;                                                           
}

void lua_register_function(lua_State *L) {
    lua_register(L, "send_message", lua_send_message);
    lua_register(L, "send_message_all", lua_send_message_all);
}

void lua_load_config() {
    lua_State *L = lua_init(); //Lua解释器指针

    log_write(LOG_DEBUG, "lua_load_config, %s, %d", __FILE__, __LINE__);
    /* 运行脚本 */
    int err = luaL_dofile(L, "conf/config.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, lua_tostring(L, -1), __FILE__, __LINE__);
        //弹出栈顶的这个错误信息
        lua_pop(L, 1);
        return;
    }
    port = lua_getnumber(L, "port");
    t_min = lua_getnumber(L, "pthread");
    lua_close(L);
}

char *lua_getaccountkey(char *account) {
    lua_State *L = lua_init();
    /* 运行脚本 */
    int err = luaL_dofile(L, "conf/account.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, lua_tostring(L, -1), __FILE__, __LINE__);
        return NULL;
    }

    char *key = lua_getstring(L, account);
    lua_close(L);
    return key;
}

int lua_getfieldint(lua_State *L, char *key) {
    int result = 0;
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (lua_isnumber(L, -1)) {
        result = (int) lua_tointeger(L, -1);
    }
    lua_pop(L, 1);
    return result;
}

char *lua_getfieldstring(lua_State *L, char *key) {
    char* result = NULL;
    lua_pushstring(L, key);
    lua_gettable(L, -2);
    if (lua_isstring(L, -1)) {
        int len = lua_rawlen(L, -1);
        result = (char *) malloc(len + 1);
        memset(result, '\0', len + 1);
        strncpy(result, lua_tostring(L, -1), len);
    }
    lua_pop(L, 1);
    return result;
}

void lua_load_room() {
    lua_State *L = lua_init();
    /* 运行脚本 */
    int err = luaL_dofile(L, "conf/room.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, "%s, %s, %d", lua_tostring(L, -1), __FILE__, __LINE__);
        return;
    }

    /* 获取变量 */
    lua_getglobal(L, "rooms");

    int l = lua_gettop(L);

    //int len = lua_rawlen(L, l); //下标从1开始的，如果从0开始，要+1

    log_write(LOG_DEBUG, "lua_load_room n:%d, 1:%d, -1:%d, %s, %d", l, lua_rawlen(L, l), lua_rawlen(L, -1), __FILE__, __LINE__);

    if (lua_istable(L, -1)) {
        int i = 0;
        lua_pushnil(L);
        //lua_rawgeti(L, 1, i); lua_isstring(L, -1)
        while (lua_next(L, -2) && i < MAX_ROOMS) {
            switch (lua_type(L, -1)) {
                case LUA_TNIL:
                    log_write(LOG_DEBUG, "[nil] nil\n");
                    break;
                case LUA_TBOOLEAN:
                    log_write(LOG_DEBUG, "[boolean] %s\n", lua_toboolean(L, -1) ? "true" : "false");
                    break;
                case LUA_TLIGHTUSERDATA:
                    log_write(LOG_DEBUG, "[lightuserdata] 0x%x\n", (long) lua_topointer(L, -1));
                    break;
                case LUA_TNUMBER:
                    log_write(LOG_DEBUG, "[number] %f\n", lua_tonumber(L, -1));
                    break;
                case LUA_TSTRING:
                    i++;
                    log_write(LOG_DEBUG, "[string] %s\n", lua_tostring(L, -1));
                    rooms[i].enable = lua_getnumber(L, "enable");
                    rooms[i].visable = lua_getnumber(L, "visable");
                    rooms[i].anonymous = lua_getnumber(L, "anonymous");
                    rooms[i].name = lua_getstring(L, "name");
                    break;
                case LUA_TTABLE:
                    i++;
                    log_write(LOG_DEBUG, "[table]\n");
                    rooms[i].enable = lua_getfieldint(L, "enable");
                    rooms[i].visable = lua_getfieldint(L, "visable");
                    rooms[i].anonymous = lua_getfieldint(L, "anonymous");
                    rooms[i].name = lua_getfieldstring(L, "name");
                    log_write(LOG_DEBUG, "房间%s, enable:%d, visable:%d, anonymous:%d, %s, %d", rooms[i].name, rooms[i].enable, rooms[i].visable, rooms[i].anonymous, __FILE__, __LINE__);
                    break;
                case LUA_TFUNCTION:
                    log_write(LOG_DEBUG, "[function] 0x%x\n", (long) lua_topointer(L, -1));
                    break;
                case LUA_TUSERDATA:
                    log_write(LOG_DEBUG, "userdata] 0x%x\n", (long) lua_topointer(L, -1));
                    break;
                case LUA_TTHREAD:
                    log_write(LOG_DEBUG, "[thread] 0x%x\n", (long) lua_topointer(L, -1));
                    break;
                default:
                    log_write(LOG_DEBUG, "[none]\n");
            }
            lua_pop(L, 1);
        }
    } else {
        log_write(LOG_DEBUG, "lua_istable(L, -1) 返回 false\n");
    }
    lua_close(L);
}

int lua_getuserrole(const char *username) {
    int result = CLIENT_TYPE_NONE;
    lua_State *L = lua_init();
    /* 运行脚本 */
    int err = luaL_dofile(L, "conf/user.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, lua_tostring(L, -1), __FILE__, __LINE__);
        return result;
    }

    /* 获取变量 */
    lua_getglobal(L, username);

    if (lua_istable(L, -1)) {
        lua_pushstring(L, "role");
        lua_gettable(L, -2);
        if (lua_isnumber(L, -1)) {
            result = (int) lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_close(L);
    return result;
}

int lua_getuserpasswd(const char *username, char *password, char *salt) {
    int ret = RETURN_FAILURE;
    lua_State *L = lua_init();
    /* 运行脚本 */
    int err = luaL_dofile(L, "conf/user.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, "%s, %s, %d", lua_tostring(L, -1), __FILE__, __LINE__);
        goto end;
    }

    /* 获取变量 */
    lua_getglobal(L, username);

    if (lua_istable(L, -1)) {
        lua_pushstring(L, "passwd");
        lua_gettable(L, -2);

        if (lua_isstring(L, -1)) {
            strcpy(password, lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        lua_pushstring(L, "salt");
        lua_gettable(L, -2);

        if (lua_isstring(L, -1)) {
            strcpy(salt, lua_tostring(L, -1));
        }
        lua_pop(L, 1);
        ret = RETURN_SUCCESS;
    }
    lua_close(L);
end:
    return ret;
}

void lua_exec(int fd, char *str) {
    lua_State *L = lua_init();
    int err = luaL_dofile(L, "conf/parsemessage.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, lua_tostring(L, -1), __FILE__, __LINE__);
        //弹出栈顶的这个错误信息
        lua_pop(L, 1);
        return;
    }

    lua_register_function(L);

    log_write(LOG_DEBUG, "调用脚本解析 fd:%d, str:%s, %s, %d", fd, str, __FILE__, __LINE__);

    lua_getglobal(L, "parsemessage");
    lua_pushnumber(L, fd);
    lua_pushstring(L, str);
    lua_call(L, 2, 0);
}
