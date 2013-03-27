#include "MyleftServer.h"

void send_message(const long fd, const char* message) {

    int ret = -1;
    int err;

    char sendbuf[TEMP_BUFFER_LENGTH];
    memset(sendbuf, '\0', sizeof (sendbuf));

    if (message != NULL) {
        log_write(LOG_DEBUG, "send_message:%s, to :%d,file:%s, line:%d\n", message, fd, __FILE__, __LINE__);
        //压缩
        char buf[MAX_BUFFER_LENGTH] = {0};
        unsigned long bufLen = sizeof (buf);

        memset(buf, '\0', sizeof (buf));
        err = compress(buf, &bufLen, message, strlen(message));
        log_write(LOG_DEBUG, "send_message compress err:%d, to :%d,file:%s, line:%d\n", err, fd, __FILE__, __LINE__);
        if (err == Z_OK) {

            unsigned int len = htonl(bufLen); //bufLen;
            log_write(LOG_DEBUG, "send_message bufLen:%lu, sizeof:%d, len:%lu, file:%s, line:%d\n", bufLen, sizeof (bufLen), len, __FILE__, __LINE__);
            memcpy(sendbuf, &len, sizeof (len));
            //strncat(sendbuf, buf, bufLen);
            int i;
            for (i = 0; i < bufLen; i++) {
                sendbuf[sizeof (len) + i] = buf[i];
            }
            //ret = write(fd, sendbuf, sizeof (len) + bufLen);
            ret = send(fd, sendbuf, sizeof (len) + bufLen, MSG_NOSIGNAL);

            log_write(LOG_DEBUG, "send_message len:%d, bufLen:%d, file:%s, line:%d \n", len, bufLen, __FILE__, __LINE__);
        } else {
            log_write(LOG_DEBUG, "compress error, file:%s, line:%d \n", __FILE__, __LINE__);
            //ret = write(fd, message, strlen(message));
        }
    }
    log_write(LOG_DEBUG, "ret is %d, file:%s, line:%d \n", ret, __FILE__, __LINE__);
}

//包括自己

void send_message_all(const long fd, const char* message) {

    if (fd_clients[fd] == NULL || message == NULL) return;
    clients *client = fd_clients[fd];
    if (client->roomid < MAX_ROOMS && client->roomid>-1 && rooms[client->roomid].r_client != NULL) {
        log_write(LOG_DEBUG, "send_message_all:%s, file:%s, line:%d\n", message, __FILE__, __LINE__);
        r_clients *p = rooms[client->roomid].r_client;
        clients *cur_client = NULL;
        while (p != NULL) {
            cur_client = p->client;
            if (cur_client->state == FD_STATE_SUCCESS) {
                log_write(LOG_DEBUG, "message:%s, to :%d,file:%s, line:%d\n", message, cur_client->fd, __FILE__, __LINE__);
                send_message(cur_client->fd, message);
            }
            p = p->next;
        }
    }
}

//睡眠

void sleep_thread(int sec) {
    struct timespec wake;

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

    time(&mytimestamp);
    p = gmtime(&mytimestamp);

    wake.tv_sec = mytimestamp + sec;

    //如果把上面的sec变成msec，并替换成下面的两句，就是实现微秒级别睡眠
    //nsec = now.tv_usec * 1000 + (msec % 1000000) * 1000;
    //wake.tv_sec = now.tv_sec + msec / 1000000 + nsec / 1000000000;
    wake.tv_nsec = 0;

    pthread_mutex_lock(&mutex);
    pthread_cond_timedwait(&cond, &mutex, &wake);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

//KeepAlive实现

void keepalive(long fd) {
    return;
    int keepAlive = 1; // 开启keepalive属性
    int keepIdle = 60; // 如该连接在60秒内没有任何数据往来,则进行探测
    int keepInterval = 5; // 探测时发包的时间间隔为5 秒
    int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepAlive, sizeof (keepAlive));
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*) &keepIdle, sizeof (keepIdle));
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void *) &keepInterval, sizeof (keepInterval));
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void *) &keepCount, sizeof (keepCount));
    //setsockopt(sock_cli, IPPROTO_TCP, TCP_NODELAY, &optval, len);//禁用NAGLE算法 粘包
}

void md5(const char *str, int len, char *dec) {
    int i;
    unsigned char digest[16];
    char tmp[3] = {'\0'};
    /*
        MD5_CTX ctx;

        MD5_Init(&ctx);
        MD5_Update(&ctx, str, len);
        MD5_Final(digest, &ctx);

        for (i = 0; i < sizeof (digest); i++) {
            sprintf(tmp, "%02x", digest[i]);
            strcat(dec, tmp);
        }
     */

    MD5(str, len, digest);
    for (i = 0; i < 16; i++) {
        sprintf(tmp, "%2.2x", digest[i]);
        strcat(dec, tmp);
    }
    log_write(LOG_DEBUG, "str: %s, md5的值：%s\n", dec);
}

int evutil_make_socket_nonblocking(long fd) {

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        log_write(LOG_ERR, "fcntl(O_NONBLOCK) failed, %s, %d", __FILE__, __LINE__);
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int join_room(clients *p, int roomid) {

    char msgbuffer[MAX_BUFFER_LENGTH];
    log_write(LOG_DEBUG, "join_room:%d\n", roomid);

    if (p == NULL || roomid > MAX_ROOMS || roomid < 0 || rooms[roomid].enable == 0 || p->roomid == roomid) {
        return RETURN_FAILURE;
    }
    if (p->anonymous == 1 && rooms[roomid].anonymous == 0) {
        log_write(LOG_ERR, "游客无法进入, %s, %d", __FILE__, __LINE__);
        //游客无法进入
        bzero(msgbuffer, sizeof (msgbuffer));
        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[游客无法进入 %s]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp, rooms[roomid].name);
        send_message(p->fd, msgbuffer);
        return RETURN_FAILURE;
    }
    if (p->roomid >= 0) {
        leave_room(p);
    }
    pthread_mutex_lock(&t_mutex_room[roomid]);
    if (roomid < MAX_ROOMS && roomid>-1 && rooms[roomid].enable) {
        p->roomid = roomid;
        r_clients *new_r_client = (r_clients *) malloc(sizeof (r_clients));
        new_r_client->client = p;
        new_r_client->next = NULL;
        if (rooms[roomid].r_client == NULL) {
            rooms[roomid].r_client = new_r_client;
        } else {
            r_clients *r_client = rooms[roomid].r_client;
            log_write(LOG_DEBUG, "第二位之后用户, %s, %d", __FILE__, __LINE__);
            while (r_client->next != NULL) {
                log_write(LOG_DEBUG, "下移一位, %s, %d", __FILE__, __LINE__);
                r_client = r_client->next;
            }
            r_client->next = new_r_client;
        }
        rooms[roomid].num++;
        //通知自己已经进入房间
        log_write(LOG_DEBUG, "通知自己已经进入房间:%d\n", p->fd, __FILE__, __LINE__);
        bzero(msgbuffer, sizeof (msgbuffer));
        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' roomid='%d' name='%s' message='成功进入房间'/>", EV_TYPE_CHANGE_ROOM, roomid, rooms[roomid].name);
        send_message(p->fd, msgbuffer);

        //通知所有用户有人进入房间
        bzero(msgbuffer, sizeof (msgbuffer));
        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' username='%s' character='%s' direction='%d' doing='%s' x='%d' y='%d'/>", EV_TYPE_USER_LOGIN, p->username, p->character, p->direction, p->doing, p->x, p->y);
        send_message_all(p->fd, msgbuffer);

        //发送用户列表
        log_write(LOG_DEBUG, "发送用户列表, %s, %d", __FILE__, __LINE__);
        send_userlist(p);
    }
    pthread_mutex_unlock(&t_mutex_room[roomid]);
    return RETURN_SUCCESS;
}

int leave_room(clients *p) {

    log_write(LOG_DEBUG, "leave_room, %s, %d", __FILE__, __LINE__);

    if (p == NULL || p->roomid > MAX_ROOMS || p->roomid < 0) {
        return RETURN_FAILURE;
    }
    char msgbuffer[MAX_BUFFER_LENGTH];
    int roomid = p->roomid;
    pthread_mutex_lock(&t_mutex_room[roomid]);
    if (roomid < MAX_ROOMS && roomid>-1 && rooms[p->roomid].r_client != NULL) {
        log_write(LOG_DEBUG, "leave_room2, %s, %d", __FILE__, __LINE__);
        r_clients *r_client = rooms[roomid].r_client;
        r_clients *prev_r_client = NULL;
        while (r_client != NULL) {
            clients *cur_client = r_client->client;
            if (cur_client->fd == p->fd) {
                rooms[roomid].num--;
                log_write(LOG_DEBUG, "leave_room3, %s, %d", __FILE__, __LINE__);
                if (prev_r_client != NULL) {
                    prev_r_client->next = r_client->next;
                } else {
                    rooms[roomid].r_client = r_client->next;
                }
                free(r_client);
                break;
            }
            prev_r_client = r_client;
            r_client = r_client->next;
        }

        if (p->state == FD_STATE_SUCCESS) {
            bzero(msgbuffer, sizeof (msgbuffer));
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' username='%s'/>", EV_TYPE_USER_LOGOUT, p->username);
            send_message_all(p->fd, msgbuffer);
            p->roomid = -1;
        }
        log_write(LOG_DEBUG, "roomresetlock, %s, %d", __FILE__, __LINE__);
    }
    pthread_mutex_unlock(&t_mutex_room[roomid]);
    log_write(LOG_DEBUG, "leave_room end, %s, %d", __FILE__, __LINE__);
    return RETURN_SUCCESS;
}

int node_add(clients *p) {
    log_write(LOG_DEBUG, "node_add, %s, %d", __FILE__, __LINE__);
    if (p != NULL) {
        hash_item *item = (hash_item *) malloc(sizeof (hash_item));
        item->key = p->username;
        item->data = (void *) p->fd;
        item->next = NULL;
        hash_add(item);
    }
    log_write(LOG_DEBUG, "node_add end, %s, %d", __FILE__, __LINE__);
    return RETURN_SUCCESS;
}

int node_del(clients *p, long fd) {
    log_write(LOG_DEBUG, "node_del, %s, %d", __FILE__, __LINE__);
    if (p != NULL) {
        leave_room(p);
        hash_del(p->username, p->fd);
        fd_clients[p->fd] = NULL;
        close(p->fd);
        free(p);
        p = NULL;
        log_write(LOG_DEBUG, "node_del11, %s, %d", __FILE__, __LINE__);
    } else {
        log_write(LOG_DEBUG, "node_del2, %s, %d", __FILE__, __LINE__);
    }
    return RETURN_SUCCESS;
}

void send_userlist(clients *node) {
    char msgbuffer[TEMP_BUFFER_LENGTH];
    bzero(msgbuffer, sizeof (msgbuffer));

    char itembuffer[MAX_BUFFER_LENGTH];

    log_write(LOG_DEBUG, "发送用户列表roomid:%d,enable:%d, name:%s, %s, %d\n", node->roomid, rooms[node->roomid].enable, rooms[node->roomid].name, __FILE__, __LINE__);
    if (node->roomid < MAX_ROOMS && node->roomid>-1 && rooms[node->roomid].enable) {
        r_clients *p = rooms[node->roomid].r_client;
        clients *cur_client = NULL;
        while (p != NULL) {
            cur_client = p->client;
            if (cur_client->fd != node->fd && cur_client->state == FD_STATE_SUCCESS) {
                if (strlen(msgbuffer) + strlen(itembuffer) >= sizeof (msgbuffer)) {
                    send_message(node->fd, msgbuffer);
                    bzero(msgbuffer, sizeof (msgbuffer));
                }
                bzero(itembuffer, sizeof (itembuffer));
                snprintf(itembuffer, sizeof (itembuffer), "<event type='%d' username='%s' character='%s' direction='%d' doing='%s' x='%d' y='%d' />", EV_TYPE_USER_ADD, cur_client->username, cur_client->character, cur_client->direction, cur_client->doing, cur_client->x, cur_client->y);
                strncat(msgbuffer, itembuffer, strlen(itembuffer));
            }
            p = p->next;
        }
        if (strlen(msgbuffer) > 0) {
            send_message(node->fd, msgbuffer);
            bzero(msgbuffer, sizeof (msgbuffer));
        }
    }
}
//房间列表

void send_roomlist(clients *node) {
    char msgbuffer[TEMP_BUFFER_LENGTH];
    bzero(msgbuffer, sizeof (msgbuffer));

    char itembuffer[MAX_BUFFER_LENGTH];
    int i;
    for (i = 0; i < MAX_ROOMS; i++) {
        log_write(LOG_DEBUG, "第%d房间:%s\n", i, rooms[i].name);
        if (rooms[i].enable == 1 && rooms[i].visable == 1) {
            log_write(LOG_DEBUG, "第%d房间 开启:%s\n", i, rooms[i].name);
            if (strlen(msgbuffer) + strlen(itembuffer) >= sizeof (msgbuffer)) {
                send_message(node->fd, msgbuffer);
                bzero(msgbuffer, sizeof (msgbuffer));
            }
            bzero(itembuffer, sizeof (itembuffer));
            snprintf(itembuffer, sizeof (itembuffer), "<event type='%d' roomid='%d' name='%s' num='%d'/>", EV_TYPE_ROOM_ADD, i, rooms[i].name, rooms[i].num);
            strncat(msgbuffer, itembuffer, strlen(itembuffer));
        } else if (rooms[i].name == NULL) {
            break;
        }
    }
    if (strlen(msgbuffer) > 0) {
        send_message(node->fd, msgbuffer);
        bzero(msgbuffer, sizeof (msgbuffer));
    }
}

void other_same_username(clients *node) {
    char msgbuffer[MAX_BUFFER_LENGTH];
    if (node != NULL) {
        leave_room(node);
        log_write(LOG_DEBUG, "同名用户存在, %s, %d", __FILE__, __LINE__);
        if (node->state == FD_STATE_SUCCESS) {
            node->state = FD_STATE_WAIT;

            bzero(msgbuffer, sizeof (msgbuffer));
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' message='用户在其他地方登陆'/>", EV_TYPE_AUTH_OTHER_LOGIN);
            send_message(node->fd, msgbuffer);

            bzero(msgbuffer, sizeof (msgbuffer));

            //通知所有用户他退出了
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' username='%s'/>", EV_TYPE_USER_LOGOUT, node->username);
            send_message_all(node->fd, msgbuffer);
        }

    }
}

clients *get_fdnode_byname(const char *uname, long ufd) {
    log_write(LOG_DEBUG, "get_fdnode_byname,%s,%d", __FILE__, __LINE__);
    log_write(LOG_DEBUG, "%s, %s, %d", uname, __FILE__, __LINE__);
    hash_item *found_item = hash_search(uname);
    long fd = -1;
    while (found_item != NULL) {
        log_write(LOG_DEBUG, "%s, %s, %d", found_item->key, __FILE__, __LINE__);
        if (strcmp(found_item->key, uname) == 0 && (ufd < 0 || (long) found_item->data != ufd)) {
            fd = (long) found_item->data;
            break;
        }
        found_item = found_item->next;
    }

    return fd >= 0 ? fd_clients[fd] : NULL;
}

int get_attribute(char* xml, char* attribute, char* buffer, int len) {

    // Get to the start of the attribute
    char* as = strstr(xml, attribute);
    if (!as) return (RETURN_FAILURE);

    // Read the attribute
    char* start = NULL;
    int numchars = 0;
    int i = 0;
    for (i = 0; i < strlen(as); i++) {

        if (start) numchars++;

        if (*(as + i) == '\'' || *(as + i) == '"') {
            if (!start)
                start = (as + i + 1);
            else
                break;
        }
    }

    // Store the result in the buffer
    if (numchars > len) return (RETURN_FAILURE);
    strncpy(buffer, start, numchars);
    *(buffer + numchars - 1) = '\0';
    return (RETURN_SUCCESS);
}

// Extracts the contents of the xml tag given and stores it in the buffer

int get_tag(char* xml, char* tag, char* buffer, int len) {

    char tagopen[32];
    char tagclose[32];
    char inone[32];
    snprintf(tagopen, sizeof (tagopen), "<%s", tag);
    snprintf(tagclose, sizeof (tagclose), "</%s>", tag);
    snprintf(inone, sizeof (inone), "<%s/>", tag);

    // If the tag is present, but has no content, stop now
    if (strstr(xml, inone)) return (RETURN_SUCCESS);
    // Get to the start of the tag
    char* as = strstr(xml, tagopen);
    if (!as) return (RETURN_FAILURE);
    char* ap = strstr(as, ">") + sizeof (char);
    if (!as) return (RETURN_FAILURE);
    // Find the end

    int i = 0;
    while (*(ap + (sizeof (char) * i)) != '<') i++;
    // Store the result in the buffer

    if (i > len) return (RETURN_FAILURE);
    strncpy(buffer, ap, i);

    *(buffer + i) = '\0';
    return (RETURN_SUCCESS);
}