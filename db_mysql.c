#include "db_mysql.h"

int db_connect() {

    if (_db_config.conn != NULL) {
        return RETURN_SUCCESS;
    }

    if (_db_config.host == NULL || _db_config.dbname == NULL || _db_config.user == NULL || _db_config.pass == NULL) {
        log_write(LOG_ERR, "mysql: invalid driver config, %s, %d", __FILE__, __LINE__);
        return RETURN_FAILURE;
    }
    log_write(LOG_DEBUG, "%s, %s, %s, %d", _db_config.host, _db_config.pass, __FILE__, __LINE__);
    //数据库
    _db_config.conn = mysql_init(NULL);
    log_write(LOG_DEBUG, "%s, %s, %s, %d", _db_config.host, _db_config.pass, __FILE__, __LINE__);
    if (_db_config.conn == NULL) {
        log_write(LOG_ERR, "mysql: unable to allocate database connection state, %s, %d", __FILE__, __LINE__);
        return RETURN_FAILURE;
    }

    char value = 1;
    mysql_options(_db_config.conn, MYSQL_SET_CHARSET_NAME, "utf8");
    mysql_options(_db_config.conn, MYSQL_OPT_RECONNECT, (char *) & value);

    if (!mysql_real_connect(_db_config.conn, _db_config.host, _db_config.user, _db_config.pass, _db_config.dbname, MYSQL_PORT, NULL, CLIENT_INTERACTIVE)) {
        log_write(LOG_ERR, "%s, %s, %d", mysql_error(_db_config.conn), __FILE__, __LINE__);
        return RETURN_FAILURE;
    }
    return RETURN_SUCCESS;
}

void db_close() {
    if (_db_config.conn != NULL) {
        mysql_close(_db_config.conn);
        free(_db_config.conn);
    }
}

MYSQL_RES *get_user_tuple(const char *username) {
    if (_db_config.conn == NULL) {
        log_write(LOG_ERR, "_db_config.conn == NULL, %s, %d", __FILE__, __LINE__);
        return NULL;
    }
    char iuser[MYSQL_LU + 1];
    char euser[MYSQL_LU * 2 + 1];

    if (mysql_ping(_db_config.conn) != 0) {
        log_write(LOG_ERR, "mysql: connection to database lost, %s, %d", __FILE__, __LINE__);
        return NULL;
    }

    snprintf(iuser, MYSQL_LU + 1, "%s", username);
    mysql_real_escape_string(_db_config.conn, euser, iuser, strlen(iuser));

    char sql[1024 + MYSQL_LU * 2 + MYSQL_LR * 2 + 1];
    snprintf(sql, sizeof (sql), "SELECT * FROM %s WHERE %s='%s' limit 1", _db_config.table, _db_config.fieldusername, euser);
    log_write(LOG_DEBUG, "%s, %s, %d", sql, __FILE__, __LINE__);

    if (mysql_query(_db_config.conn, sql) != 0) {
        log_write(LOG_ERR, "%s, %s, %d", mysql_error(_db_config.conn), __FILE__, __LINE__);
        return NULL;
    }
    MYSQL_RES *res = mysql_store_result(_db_config.conn);
    if (res == NULL) {
        log_write(LOG_ERR, "%s, %s, %d", mysql_error(_db_config.conn), __FILE__, __LINE__);
        return NULL;
    }

    if (mysql_num_rows(res) != 1) {
        mysql_free_result(res);
        return NULL;
    }

    return res;
}

int db_get_password(const char* username, char *password, char *salt) {
    int ret = RETURN_FAILURE;
    if (db_connect(_db_config)==RETURN_FAILURE) {
        log_write(LOG_DEBUG, "db_connect faile, %s, %d\n", __FILE__, __LINE__);
        return RETURN_FAILURE;
    }
    MYSQL_RES *res = get_user_tuple(username);
    MYSQL_FIELD *field;
    MYSQL_ROW row;

    if (res == NULL) {
        goto end;
    }

    int i, fpassword = 0, fsalt = 0;
    for (i = mysql_num_fields(res) - 1; i >= 0; i--) {
        field = mysql_fetch_field_direct(res, i);
        log_write(LOG_DEBUG, "field->name, %s, %s, %d\n",  field->name, __FILE__, __LINE__);
        if (strcmp(field->name, _db_config.fieldpassword) == 0) {
            fpassword = i;
        }
        else if (strcmp(field->name, _db_config.fieldsalt) == 0) {
            fsalt = i;
        }
    }
    if ((row = mysql_fetch_row(res)) == NULL) {
        log_write(LOG_DEBUG, "mysql: sql tuple retrieval failed: %s\n", mysql_error(_db_config.conn));
        mysql_free_result(res);
        goto end;
    }

    if (row[fpassword] == NULL) {
        log_write(LOG_ERR, "row[fpassword] == NULL, %d, %s, %d\n",  fpassword, __FILE__, __LINE__);
        mysql_free_result(res);
        goto end;
    }

    strcpy(password, row[fpassword]);
    strcpy(salt, row[fsalt]);
    log_write(LOG_DEBUG, "password:%s, fieldsalt:%s, salt:%s, fsalt:%d, %s, %d\n", password, _db_config.fieldsalt, salt, fsalt,  __FILE__, __LINE__);
    mysql_free_result(res);
    ret = RETURN_SUCCESS;
end:
    return ret;
}
