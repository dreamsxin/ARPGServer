/* 
 * File:   global.h
 * Author: dreamszhu
 *
 * Created on 2010年8月7日, 下午9:35
 */

#ifndef GLOBAL_H
#define	GLOBAL_H

#ifdef	__cplusplus
extern "C" {
#endif


#define DEBUG 1

#define RETURN_FAILURE -1
#define RETURN_SUCCESS 1

//消息类型
#define EV_TYPE_PING 0
#define EV_TYPE_AUTH 10
#define EV_TYPE_AUTH_OTHER_LOGIN 11
#define EV_TYPE_AUTH_SUCCESS 12
#define EV_TYPE_AUTH_FAILURE 13
#define EV_TYPE_CHANGE_CHARACTER 14
#define EV_TYPE_USER_LOGIN 20
#define EV_TYPE_USER_LOGOUT 21
#define EV_TYPE_USER_ADD 22
#define EV_TYPE_USER_CHANGE_STATE 23
#define EV_TYPE_MESSAGE 30
#define EV_TYPE_SYSTEM_MESSAGE 31
#define EV_TYPE_PUBLIC_MESSAGE 32
#define EV_TYPE_PRIVATE_MESSAGE 33
#define EV_TYPE_CHANGE_ROOM 40
#define EV_TYPE_GET_ROOM_LIST 41
#define EV_TYPE_ROOM_ADD 42
#define EV_TYPE_LEAVE_ROOM 43
#define EV_TYPE_ADMIN_COMMAND_GOOUT 90
#define EV_TYPE_ADMIN_COMMAND_SHUTUP 91

//特殊
#define EV_TYPE_MOVE 100
#define EV_TYPE_DOING 101

#ifdef	__cplusplus
}
#endif

#endif	/* GLOBAL_H */

