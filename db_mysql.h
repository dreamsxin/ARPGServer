/* 
 * File:   db_mysql.h
 * Author: dreamszhu
 *
 * Created on 2010年8月8日, 下午1:23
 */

#ifndef DB_MYSQL_H
#define	DB_MYSQL_H
#include "authreg.h"
#ifdef	__cplusplus
extern "C" {
#endif

#define MYSQL_LU  1024
#define MYSQL_LR   256
#define MYSQL_LP   256
MYSQL *conn;
int db_connect();
void db_close();
MYSQL_RES *get_user_tuple(const char *username);
int db_get_password(const char* username, char *password, char *salt);

#ifdef	__cplusplus
}
#endif

#endif	/* DB_MYSQL_H */

