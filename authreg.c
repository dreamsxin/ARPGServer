#include "authreg.h"
int authreg_init() {
    _db_config.conn = NULL;
    _db_config.type = NULL;
    _db_config.path = NULL;
    _db_config.host = NULL;
    _db_config.user = NULL;
    _db_config.pass = NULL;
    _db_config.dbname = NULL;
    _db_config.table = NULL;
    _db_config.fieldusername = NULL;
    _db_config.fieldpassword = NULL;
    _db_config.fieldsalt = NULL;
    lua_State *L = lua_init(); //Lua解释器指针
    if (L == NULL) {
        log_write(LOG_ERR, "authreg init fail, %s, %d", __FILE__, __LINE__);
        return RETURN_FAILURE;
    }
    log_write(LOG_DEBUG, "authreg init, %s, %d", __FILE__, __LINE__);
    int err = luaL_dofile(L, "conf/database.lua");
    if (err) {
        //如果错误，显示
        log_write(LOG_ERR, lua_tostring(L, -1), __FILE__, __LINE__);
        //弹出栈顶的这个错误信息
        lua_pop(L, 1);
        return;
    }
    _db_config.type = lua_getstring(L, "dbtype");
    _db_config.path = lua_getstring(L, "dbpath");
    _db_config.host = lua_getstring(L, "dbhost");
    _db_config.port = lua_getnumber(L, "dbport");
    _db_config.user = lua_getstring(L, "dbuser");
    _db_config.pass = lua_getstring(L, "dbpass");
    _db_config.dbname = lua_getstring(L, "dbname");
    _db_config.table = lua_getstring(L, "dbtable");
    _db_config.fieldusername = lua_getstring(L, "fieldusername");
    _db_config.fieldpassword = lua_getstring(L, "fieldpassword");
    _db_config.fieldsalt = lua_getstring(L, "fieldsalt");
    lua_close(L);
    return RETURN_SUCCESS;
}

void authreg_free() {
    if (_db_config.conn!=NULL) free(_db_config.conn);
    if (_db_config.type!=NULL) free(_db_config.type);
    if (_db_config.path!=NULL) free(_db_config.path);
    if (_db_config.host!=NULL) free(_db_config.host);
    if (_db_config.user!=NULL) free(_db_config.user);
    if (_db_config.pass!=NULL) free(_db_config.pass);
    if (_db_config.dbname!=NULL) free(_db_config.dbname);
    if (_db_config.table!=NULL) free(_db_config.table);
    if (_db_config.fieldusername!=NULL) free(_db_config.fieldusername);
    if (_db_config.fieldpassword!=NULL) free(_db_config.fieldpassword);
    if (_db_config.fieldsalt!=NULL) free(_db_config.fieldsalt);
}

int get_password(const char* username, char *password, char *salt) {
    if (strncmp(_db_config.type, DATABASE_TYPE_MYSQL, sizeof (DATABASE_TYPE_MYSQL)) == 0) {
        log_write(LOG_DEBUG, "%s, %s, %s, %d", _db_config.host, _db_config.pass, __FILE__, __LINE__);
        return db_get_password(username, password, salt);
    } else {
        return lua_getuserpasswd(username, password, salt);
    }
    return RETURN_FAILURE;
}