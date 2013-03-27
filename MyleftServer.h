/*
 * File:   MyleftServer.h
 * Author: dreamszhu
 *
 * Created on 2009年9月12日, 下午10:48
 */

#ifndef _MYLEFTSERVER_H
#define	_MYLEFTSERVER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h> //相关函数atoi，atol，strtod，strtol，strtoul

#include <unistd.h>
#include <crypt.h>

#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include <openssl/md5.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <zlib.h>

#include "global.h"
#include "hash.h"
#include "luafunction.h"
#include "ezxml.h"
#include "log.h"
#include "authreg.h"

#define MAX_CLIENTS 1023
#define MAX_MAPS 24
#define CHAR_LENGTH 32
#define MAX_CHAR_LENGTH 64
#define MAX_BUFFER_LENGTH 1024
#define TEMP_BUFFER_LENGTH 2048

	//状态
#define FD_STATE_NONE 0
#define FD_STATE_WAIT 1
#define FD_STATE_SUCCESS 2

	//客户类型
#define CLIENT_TYPE_NONE 0
#define CLIENT_TYPE_SHUTUP 1
#define CLIENT_TYPE_ADMIN 9

	//验证类型
#define AUTHTYPE_FILE "file"
#define AUTHTYPE_MYSQL "mysql"

#define POLICY "<policy-file-request/>"
#define QUIT "<quit/>"
#define PING "<ping/>"
#define NONE "<none/>\n"

#define ALL_USERS "all"

#define T_MAX 50
#define MAX_FDS 20480
#define MAX_USERS 100
#define MAX_ROOMS 50
#define MAX_EVENTS 1024
	
	int hashlock;
	int t_min;
	int port;
	int epfd; //epoll句柄

	int t_num; //线程数
	pthread_t tid[T_MAX];
	pthread_mutex_t t_mutex;
	pthread_cond_t t_cond;

	pthread_mutex_t t_mutex_fd[MAX_FDS];
	pthread_mutex_t t_mutex_room[MAX_ROOMS];
	pthread_mutex_t t_mutex_hash;
	pthread_mutex_t t_mutex_log;

	//需要处理的任务列表

	struct task {
		int fd;
		struct clients *client;
		int recv_bits;
		int data_bits;
		int eventi;
		char *data;
		struct task *next;
	};
	struct task *task_head;
	struct task *task_last;

	//线程的任务函数
	void * readtask(void *args);

	//客户端连接数据结构体

	typedef struct clients {
		long fd;
		int roomid;
		int state;
		int type;
		int anonymous;
		int keepalivetime;
		char username[64]; //用户名
		char character[32]; //角色
		char doing[24]; //动作
		int direction;
		int x;
		int y;
		int level;
	} clients;

	//所有客户端列表
	clients *fd_clients[MAX_FDS];

	//房间内所有客户端列表

	typedef struct _r_clients {
		struct clients *client;
		struct _r_clients *next;
	} r_clients;
	//聊天室房间

	typedef struct room {
		int enable;
		int anonymous;
		int visable;
		int num;
		char *name;
		struct _r_clients *r_client;
	} room;

	room rooms[MAX_ROOMS];

	//用户名哈希

	typedef struct server {
		int listen_sockfd;
		struct sockaddr_in listen_addr;
	} server;

	//时间
	time_t mytimestamp;
	struct tm *p;

	void destory();

	//function
	void sleep_thread(int sec);
	char *strescape(const char *buf);

	void keepalive(long fd);
	void md5(const char *str, int len, char *dec);
	int evutil_make_socket_nonblocking(long fd);

	int join_room(clients *p, int roomid);
	int leave_room(clients *p);
	int node_add(clients *p);
	int node_del(clients *p, long fd);

	void other_same_username(clients *node);
	void send_logout(clients *node);
	void send_userlist(clients *node);
	void send_roomlist(clients *node);
	void send_message(const long fd, const char* message);
	void send_message_all(const long fd, const char* message);


	int get_attribute(char* xml, char* attribute, char* buffer, int len);
	int get_tag(char* xml, char* tag, char* buffer, int len);

	clients *get_fdnode_byname(const char *uname, long ufd);

	//message
	void *readtask(void *args);
	void parse_message(struct task * args);


#ifdef	__cplusplus
}
#endif

#endif	/* _MYLEFTSERVER_H */
