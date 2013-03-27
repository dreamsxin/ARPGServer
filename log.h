/* 
 * File:   log.h
 * Author: dreamszhu
 *
 * Created on 2010年8月7日, 下午2:19
 */

#ifndef LOG_H
#define	LOG_H
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#ifdef	__cplusplus
extern "C" {
#endif

    #define MAX_LOG_LINE 1024
    //LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG
    static const char *log_level[] ={
        "emergency",
        "alert",
        "critical",
        "error",
        "warning",
        "notice",
        "info",
        "debug"
    };
    //log
    void log_write(int level, const char *msgfmt, ...);


#ifdef	__cplusplus
}
#endif

#endif	/* LOG_H */

