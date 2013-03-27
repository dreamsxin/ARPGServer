#include "log.h"
//日志写入

void log_write(int level, const char *fmt, ...) {

    char *pos, message[MAX_LOG_LINE + 1];
    int sz, len;
    time_t t;
    memset(message, 0, sizeof (message));

    t = time(NULL);
    pos = ctime(&t);
    sz = strlen(pos);
    pos[sz - 1] = ' ';

    len = snprintf(message, MAX_LOG_LINE, "%s[%s] ", pos, log_level[level]);
    if (len > MAX_LOG_LINE)
        len = MAX_LOG_LINE;

    message[len] = '\0';

    pos = message;
    pos += len;

    va_list arg_ptr;
    va_start(arg_ptr, fmt);

    vsnprintf(pos, MAX_LOG_LINE - len, fmt, arg_ptr);
    // vsprintf(pos, fmt, arg_ptr);
    va_end(arg_ptr);

    printf("%s\n", message);
}