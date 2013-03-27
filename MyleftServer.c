/*
 * File:   MyleftServer.c
 * Author: Dreamsxin
 *
 * Created on 2009年9月12日, 下午10:48
 */

/**
 * int getopt(int argc, char * const argv[], const char *optstring);
 * extern char *optarg;
 * extern int optind, opterr, optopt;
 **/

#include "MyleftServer.h"
#include <signal.h>

pthread_t t_heartbeat;
pthread_t t_policy;
pthread_t t_epoll;
void *serv_heartbeat(void *args);
void *serv_policy(void *args);
void *serv_epoll(void *args);

static void signal_catch(int signo) {
    log_write(LOG_ERR, "signo:%d, 断开的管道, %s, %d", signo, __FILE__, __LINE__);
}

void SetupSignal(void) {
    signal(SIGPIPE, SIG_IGN);
}

int main() {
    server s;
    t_min = 5;
    port = 5222;

    int i;
    for (i = 0; i < MAX_FDS; i++) {
        pthread_mutex_init(&t_mutex_fd[i], NULL);
        fd_clients[i] = NULL;
    }

    for (i = 0; i < MAX_ROOMS; i++) {
        pthread_mutex_init(&t_mutex_room[i], NULL);
        rooms[i].enable = 0;
        rooms[i].anonymous = 0;
        rooms[i].visable = 0;
        rooms[i].num = 0;
        rooms[i].name = NULL;
        rooms[i].r_client = NULL;
    }

    if (hash_create(MAX_USERS) == 0) {
        log_write(LOG_ERR, "hcreate error, %s, %d", __FILE__, __LINE__);
        return (EXIT_FAILURE);
    }

    rooms[0].enable = 1;
    rooms[0].anonymous = 1;
    rooms[0].visable = 1;
    rooms[0].name = (char *) malloc(MAX_CHAR_LENGTH);
    memset(rooms[0].name, 0, MAX_CHAR_LENGTH);
    strncpy(rooms[0].name, "Myleft聊天大厅", MAX_CHAR_LENGTH);

    lua_load_config();
    lua_load_room();
    authreg_init();

    pthread_mutex_init(&t_mutex, NULL);
    pthread_cond_init(&t_cond, NULL);

    pthread_mutex_init(&t_mutex_hash, NULL);
    pthread_mutex_init(&t_mutex_log, NULL);

    //开启心跳线程serv_heartbeat
    pthread_create(&t_heartbeat, NULL, serv_heartbeat, NULL);

    //开启flash权限验证线程serv_policy
    pthread_create(&t_policy, NULL, serv_policy, NULL);

    //初始化任务线程，开启两个线程来完成任务，线程之间会互斥地访问任务链表
    t_num = 0;
    while (t_num < t_min && t_num < T_MAX) {
        t_num++;
        pthread_create(&tid[t_num], NULL, readtask, NULL);
    }

    //创建epoll句柄
    epfd = epoll_create(MAX_FDS);

    int optval = 1; // 关闭之后重用socket
    unsigned int optlen = sizeof (optval);

    if ((s.listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_write(LOG_ERR, "socket error, %s, %d", __FILE__, __LINE__);
        destory();
        return (EXIT_FAILURE);
    }

    log_write(LOG_DEBUG, "listen socked created, %s, %d", __FILE__, __LINE__);

    setsockopt(s.listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optlen)); //端口重用，如意外没有释放端口，还可以绑定成功

    bzero(&s.listen_addr, sizeof (s.listen_addr)); //memset 适合大数据

    s.listen_addr.sin_family = AF_INET;
    s.listen_addr.sin_port = htons(port);
    s.listen_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("127.0.0.1");

    if (bind(s.listen_sockfd, (struct sockaddr*) & s.listen_addr, sizeof (s.listen_addr)) < 0) {
        log_write(LOG_ERR, "bind error, %s, %d", __FILE__, __LINE__);
        destory();
        return (EXIT_FAILURE);
    }

    if (listen(s.listen_sockfd, 3) < 0) {
        log_write(LOG_ERR, "listen error, %s, %d", __FILE__, __LINE__);
        destory();
        return (EXIT_FAILURE);
    }
    log_write(LOG_DEBUG, "监听端口:%d......\n", port);
    log_write(LOG_DEBUG, "线程数:%d......\n", t_num);

    if (pthread_create(&t_epoll, NULL, serv_epoll, NULL) != 0) {
        log_write(LOG_ERR, "serv_epoll pthread_create error, %s, %d", __FILE__, __LINE__);
        destory();
        return (EXIT_FAILURE);
    }

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof (client_addr);

    struct epoll_event ev = {0};
    //处理accept
    while ((client_fd = accept(s.listen_sockfd, (struct sockaddr *) & client_addr, &client_len)) > 0) {

        evutil_make_socket_nonblocking(client_fd);
        clients *node = (clients *) malloc(sizeof (clients));
        node->fd = client_fd;
        node->roomid = -1;
        node->state = FD_STATE_WAIT;
        node->type = CLIENT_TYPE_NONE;
        node->anonymous = 1;
        node->direction = 0;
        node->x = 1;
        node->y = 1;
        node->keepalivetime = mytimestamp;
        memset(node->username, 0, sizeof (node->username)); //bzero(c, sizeof(c));
        memset(node->character, 0, sizeof (node->character));
        memset(node->doing, 0, sizeof (node->doing));

        fd_clients[client_fd] = node;
        keepalive(client_fd); //设置心跳
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
        log_write(LOG_DEBUG, "EPOLL_CTL_ADD, %s, %d", __FILE__, __LINE__);
    }

    close(s.listen_sockfd);
    destory();
    return (EXIT_SUCCESS);
}

void *serv_heartbeat(void *args) {
    int i;
    while (1) {
        sleep_thread(40);
        time(&mytimestamp);
        p = gmtime(&mytimestamp);
        for (i = 0; i < MAX_FDS; i++) {
            if (fd_clients[i] == NULL) {
                continue;
            }
            if ((mytimestamp - fd_clients[i]->keepalivetime) > 120) {

                log_write(LOG_DEBUG, "60秒内没有活动自动退出！, %s, %d", __FILE__, __LINE__);
                node_del(fd_clients[i], i);
            }
        }
        pthread_testcancel();
    }
    pthread_exit(NULL);
}

void *serv_policy(void *args) {
    int listenfd;
    int policy_port = 843;
    struct sockaddr_in listen_addr;
    int optval = 1; // 关闭之后重用socket
    unsigned int optlen = sizeof (optval);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_write(LOG_ERR, "serv_policy socket error, %s, %d", __FILE__, __LINE__);
        goto end;
    }

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optlen)); //端口重用，如意外没有释放端口，还可以绑定成功

    bzero(&listen_addr, sizeof (listen_addr)); //memset 适合大数据

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(policy_port);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("127.0.0.1");

    if (bind(listenfd, (struct sockaddr*) & listen_addr, sizeof (listen_addr)) < 0) {
        log_write(LOG_DEBUG, "bind error:%s, file:%s, line:%d\n", strerror(errno), __FILE__, __LINE__);
        goto end;
    }

    if (listen(listenfd, 3) < 0) {
        log_write(LOG_ERR, "serv_policy listen error, %s, %d", __FILE__, __LINE__);
        goto end;
    }

    log_write(LOG_DEBUG, "serv_policy监听端口:%d......, %s, %d", policy_port, __FILE__, __LINE__);
    /* 设置socket为非阻塞模式，使用libevent编程这是必不可少的。 */
    if (evutil_make_socket_nonblocking(listenfd) < 0) {
        log_write(LOG_ERR, "evutil_make_socket_nonblocking error, %s, %d", __FILE__, __LINE__);
        goto end;
    }

    struct epoll_event events[MAX_EVENTS];
    struct epoll_event ev = {0};

    int policy_epfd = epoll_create(MAX_EVENTS);

    ev.data.fd = listenfd;
    ev.events = EPOLLIN | EPOLLET;

    if ((epoll_ctl(policy_epfd, EPOLL_CTL_ADD, listenfd, &ev)) < 0) {
        log_write(LOG_ERR, "epoll_ctl error, %s, %d", __FILE__, __LINE__);
        close(listenfd);
        goto end;
    }

    int nfds;
    int i;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof (client_addr);
    int client_fd;
    int event_fd;
    int recv_bits, ret;
    FILE *fp;
    char buf[MAX_BUFFER_LENGTH];
    while (1) {
        nfds = epoll_wait(policy_epfd, events, MAX_EVENTS, -1);

        for (i = 0; i < nfds; i++) {
            event_fd = events[i].data.fd;
            if (event_fd == listenfd) {//主socket
                client_fd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
                if (client_fd < 0) {
                    continue;
                }

                log_write(LOG_DEBUG, "new Connection %d, %s, %d", client_fd, __FILE__, __LINE__);
                keepalive(client_fd); //设置心跳
                ev.data.fd = client_fd;
                ev.events = EPOLLIN | EPOLLET;
                if ((epoll_ctl(policy_epfd, EPOLL_CTL_ADD, client_fd, &ev)) < 0) {
                    log_write(LOG_ERR, "connect failed, %s, %d", client_fd, __FILE__, __LINE__);
                }
            } else if (events[i].events & EPOLLIN) {
                if (event_fd < 0) {
                    continue;
                }
                log_write(LOG_ERR, "serv_policy EPOLLIN, %s, %d", __FILE__, __LINE__);
                memset(buf, '\0', sizeof (buf));
                recv_bits = read(event_fd, buf, sizeof (buf));
                if (recv_bits <= 0) {
                    log_write(LOG_DEBUG, "serv_policy recv_bits <= 0, %s, %d", __FILE__, __LINE__);
                    events[i].data.fd = -1;
                    epoll_ctl(policy_epfd, EPOLL_CTL_DEL, event_fd, NULL);
                    close(event_fd);
                } else if (strncmp(buf, POLICY, sizeof (POLICY)) == 0) {
                    log_write(LOG_DEBUG, "serv_policy POLICY, %s, %d", __FILE__, __LINE__);
                    ev.data.fd = event_fd;
                    ev.events = EPOLLOUT | EPOLLET;
                    epoll_ctl(policy_epfd, EPOLL_CTL_MOD, event_fd, &ev);
                    continue;
                }
            } else if (events[i].events & EPOLLOUT) {
                fp = fopen("./conf/crossdomain.xml", "rb");
                if (fp != NULL) {
                    while (!feof(fp)) {

                        memset(buf, '\0', sizeof (buf));
                        recv_bits = fread(buf, 1, sizeof (buf), fp);
                        //ret = write(event_fd, buf, recv_bits + 1); //+1是为了，末尾发个\0，flashplayer才会接受
                        ret = send(event_fd, buf, recv_bits + 1, MSG_NOSIGNAL);
                        log_write(LOG_DEBUG, "ret:%d, buf:%s, %s, %d", ret, buf, __FILE__, __LINE__);
                    }
                }

                ev.data.fd = event_fd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(policy_epfd, EPOLL_CTL_MOD, event_fd, &ev);
            }
        }
        pthread_testcancel();
    }
end:
    log_write(LOG_ERR, "serv_policy exit, %s, %d", __FILE__, __LINE__);
    pthread_exit(NULL);
}

void *serv_epoll(void *args) {
    int nfds;
    int recv_bits, data_bits;
    clients *client_fdnode;
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event ev = {0};
    int event_fd;
    int i;
    int readnum;
    int len = 0;
    char buf[MAX_BUFFER_LENGTH];
    while (1) {
        //等待epoll事件的发生
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1); //-1 一直等到有数据到达 3000
        //处理所发生的所有事件
        for (i = 0; i < nfds; ++i) {
            time(&mytimestamp);
            p = gmtime(&mytimestamp);
            log_write(LOG_DEBUG, "fd:%d, file:%s, line:%d\n", events[i].data.fd, __FILE__, __LINE__);

            event_fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                if (event_fd < 0) {
                    continue;
                }
                client_fdnode = fd_clients[event_fd];
                if (client_fdnode == NULL) {
                    events[i].data.fd = -1;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, event_fd, NULL);
                    close(event_fd);
                    continue;
                }

                client_fdnode->keepalivetime = mytimestamp;
                readnum = 0;

read:
                memset(buf, '\0', sizeof (buf));
                data_bits = 0;
                recv_bits = read(event_fd, &len, sizeof (len));

                log_write(LOG_DEBUG, "recv_bits:%d, readnum:%d, len:%d, file:%s, line:%d\n", recv_bits, readnum, len, __FILE__, __LINE__);

                if (recv_bits <= 0 && readnum > 0) {
                    continue;
                } else if (recv_bits == sizeof (len)) {
					len = ntohl(len);
					log_write(LOG_DEBUG, "len:%d, file:%s, line:%d\n", len, __FILE__, __LINE__);
                    
                    if (len <= sizeof (buf)) {
                        data_bits = read(event_fd, buf, len);
                    } else {
                        recv_bits = 0; //关闭连接
                    }
                    log_write(LOG_DEBUG, "data_bits:%d, buf:%s, file:%s, line:%d\n", data_bits, buf, __FILE__, __LINE__);
                }

                if (recv_bits > 0 && data_bits <= 0) {//数据已读完
                    continue;
                }
                log_write(LOG_DEBUG, "添加新的读任务, file:%s, line:%d\n", __FILE__, __LINE__);
                //添加新的读任务
                struct task *new_task = (struct task *) malloc(sizeof (struct task));
                if (new_task != NULL) {
                    new_task->client = client_fdnode;
                    new_task->fd = event_fd;
                    new_task->eventi = i;
                    new_task->recv_bits = recv_bits;
                    new_task->data_bits = data_bits;
                    if (data_bits <= 0) {
                        new_task->data = NULL;
                    } else {
                        new_task->data = (char *) malloc(data_bits + 1);
                        bzero(new_task->data, data_bits + 1);
                        memcpy(new_task->data, &buf, data_bits);
                    }
                    new_task->next = NULL;
                    log_write(LOG_DEBUG, "new_task, file:%s, line:%d\n", __FILE__, __LINE__);
                    pthread_mutex_lock(&t_mutex);
                    if (task_head == NULL) {
                        task_head = new_task;
                    } else {
                        task_last->next = new_task;
                    }
                    task_last = new_task;
                    //唤醒其中一个线程即可
                    log_write(LOG_DEBUG, "唤醒所有等待cond1条件的线程, %s, %d", __FILE__, __LINE__);
                    pthread_cond_signal(&t_cond);

                    //唤醒所有等待cond1条件的线程
                    //log_write(LOG_DEBUG, "唤醒所有等待cond1条件的线程", __FILE__, __LINE__);
                    //pthread_cond_broadcast(&t_cond);

                    pthread_mutex_unlock(&t_mutex);
                }
                readnum++;
                if (recv_bits > 0) {
                    goto read;
                }
            } else if (events[i].events & EPOLLOUT) {
                if (events[i].data.fd < 0) {
                    continue;
                }

                //设置用于读操作的文件描述符
                ev.data.fd = events[i].data.fd;
                //设置用于注测的读操作事件
                ev.events = EPOLLIN | EPOLLET;
                //修改sockfd上要处理的事件为EPOLIN
                epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
            } else {
                //perror("other event");
            }

            log_write(LOG_DEBUG, "小循环结束, %s, %d", __FILE__, __LINE__);
        }
        pthread_testcancel();
        /* 查看内存泄漏 */
        //show_memory();
        //show_memory_summary();
    }
    pthread_exit(NULL);
}

//注销程序

void destory() {
    log_write(LOG_DEBUG, "destory, %s, %d", __FILE__, __LINE__);
    authreg_free();
    log_write(LOG_DEBUG, "authreg_free, %s, %d", __FILE__, __LINE__);

    int i;
    if (t_epoll) pthread_cancel(t_epoll);
    if (t_heartbeat) pthread_cancel(t_heartbeat);
    for (t_num; t_num > 0; t_num--) {
        if (tid[t_num]) pthread_cancel(tid[t_num]);
    }
    log_write(LOG_DEBUG, "开始释放client, %s, %d", __FILE__, __LINE__);
    for (i = 0; i < MAX_FDS; i++) {
        if (fd_clients[i] != NULL) {
            node_del(fd_clients[i], i);
        }
        pthread_mutex_destroy(&t_mutex_fd[i]);
    }
    log_write(LOG_DEBUG, "MAX_ROOMS, %s, %d", __FILE__, __LINE__);
    for (i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].name != NULL) {
            free(rooms[i].name);
        }
        pthread_mutex_destroy(&t_mutex_room[i]);
    }

    pthread_mutex_destroy(&t_mutex_log);
    pthread_mutex_destroy(&t_mutex_hash);
    pthread_mutex_destroy(&t_mutex);
    pthread_cond_destroy(&t_cond);
    db_close();
    hash_destroy();
}