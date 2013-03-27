/* 
 * File:   authreg.h
 * Author: dreamszhu
 *
 * Created on 2010年8月7日, 下午4:11
 */

#ifndef AUTHREG_H
#define	AUTHREG_H
#include "log.h"
#include "global.h"
#include "luafunction.h"

#include <mysql/mysql.h>
#ifdef	__cplusplus
extern "C" {
#endif
#define	DATABASE_TYPE_MYSQL "mysql"
#define	DATABASE_TYPE_FILE "file"

    struct _db_config {
        char *type;
        char *path;
        MYSQL *conn;
        char *host;
        char *user;
        char *pass;
        char *dbname;
        char *table;
        char *fieldusername;
        char *fieldpassword;
        char *fieldsalt;
        int port;
    };
    struct _db_config _db_config;

int authreg_init();
void authreg_free();
int get_password(const char* username, char *password, char *salt);

#ifdef	__cplusplus
}
#endif

#endif	/* AUTHREG_H */

