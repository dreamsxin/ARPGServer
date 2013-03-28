#include "MyleftServer.h"

void *readtask(void *args) {
    while (1) {
        //互斥访问任务队列
        pthread_mutex_lock(&t_mutex);
        //等待到任务队列不为空
        while (task_head == NULL) {
            //线程阻塞，释放互斥锁，当等待的条件等到满足时，它会再次获得互斥锁
            pthread_cond_wait(&t_cond, &t_mutex);
        }

        //从任务队列取出一个读任务
        struct task *temp = task_head;
        task_head = task_head->next;
        pthread_mutex_unlock(&t_mutex);
        log_write(LOG_DEBUG, "readtask, %s, %d", __FILE__, __LINE__);
        parse_message(temp);
        pthread_mutex_unlock(&t_mutex_fd[temp->fd]);
        if (temp->data != NULL) free(temp->data);
        free(temp);
        temp = NULL;
        pthread_testcancel();
    }
    pthread_exit(NULL);
}

// Parses the message and sends a response

void parse_message(struct task * args) {
    log_write(LOG_DEBUG, "parse_message,%s, %d", __FILE__, __LINE__);
    clients *fdnode = args->client;
    int fd = args->fd;
    char in[TEMP_BUFFER_LENGTH];

    int logout = 0;
    if (args->recv_bits > 0) {
        log_write(LOG_DEBUG, "data:%s, data_bits:%d, %s, %d", args->data, args->data_bits, __FILE__, __LINE__);

        memset(in, '\0', sizeof (in));
        //解压缩
        unsigned long inLen = sizeof (in);
        int err = uncompress(in, &inLen, args->data, args->data_bits);
        if (err != Z_OK) {
            log_write(LOG_DEBUG, "解压出错,%s, %d", __FILE__, __LINE__);
            return; //退出
        }

        log_write(LOG_DEBUG, "in:%s, inLen:%d, %s, %d\n", in, inLen, __FILE__, __LINE__);
        if (strncmp(in, QUIT, sizeof (QUIT)) == 0) {
            log_write(LOG_DEBUG, "QUIT", __FILE__, __LINE__);
            logout = 1;
        } else if (strncmp(in, PING, sizeof (PING)) == 0) {
            send_message(fd, "<event type='0'/>");
            return; //退出
        }
    } else {
        logout = 1;
    }

    if (logout) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        log_write(LOG_DEBUG, "退出%s,%d", __FILE__, __LINE__);
        node_del(fdnode, fd);
        return;
    }

    log_write(LOG_DEBUG, "parse_message:%s, file:%s, line:%d", in, __FILE__, __LINE__);

    ezxml_t event = ezxml_parse_str(in, sizeof (in));
    if (!ezxml_attr(event, "type")) {
        goto end;
    }
    int ev_type = atoi(ezxml_attr(event, "type"));
    const char *username = ezxml_attr(event, "username");
    const char *character = ezxml_attr(event, "character");
    const char *password = ezxml_attr(event, "password");
    const char *anonymous = ezxml_attr(event, "anonymous");
    const char *account = ezxml_attr(event, "account");
    const char *sign = ezxml_attr(event, "sign");
    const char *timestamp_char = ezxml_attr(event, "timestamp");

    int roomid = 0;
    if (ezxml_attr(event, "roomid")) {
        roomid = atoi(ezxml_attr(event, "roomid"));
        roomid = roomid >= MAX_ROOMS ? 0 : roomid;
    }

    time_t timestamp = 0;
    if (timestamp_char) timestamp = atol(timestamp_char);

    const char *to = ezxml_attr(event, "to");
    const char *body;
    if (ezxml_child(event, "body")) body = ezxml_child(event, "body")->txt;
    //char *body = strescape(temp_body);

    time(&mytimestamp);
    p = gmtime(&mytimestamp);

    log_write(LOG_DEBUG, "ev_type%d,file:%s, line:%d\n", ev_type, __FILE__, __LINE__);
    char msgbuffer[MAX_BUFFER_LENGTH];
    if (ev_type == EV_TYPE_AUTH) {
        log_write(LOG_DEBUG, "登陆验证, %s, %d", __FILE__, __LINE__);
        if (fdnode->state == FD_STATE_SUCCESS) {
            bzero(msgbuffer, sizeof (msgbuffer));
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' result='1' message='已经登陆'/>", EV_TYPE_AUTH);
            send_message(fd, msgbuffer);
            goto end;
        } else if (anonymous && atoi(anonymous) == 1) {//匿名登陆
            log_write(LOG_DEBUG, "匿名登陆, %s, %d", __FILE__, __LINE__);
            fdnode->anonymous = 1;
            snprintf(fdnode->username, MAX_CHAR_LENGTH, "游客%d%d%d", p->tm_hour, p->tm_min, p->tm_sec);
        } else if (username && password) {
            log_write(LOG_DEBUG, "密码验证, %s, %d", __FILE__, __LINE__);
            //db获取用户密码
            char db_pw_value[MAX_CHAR_LENGTH];
            char db_salt_value[MAX_CHAR_LENGTH];
            memset(db_pw_value, 0, sizeof (db_pw_value));
            memset(db_salt_value, 0, sizeof (db_salt_value));
            if (get_password(username, db_pw_value, db_salt_value) != RETURN_SUCCESS) {
                log_write(LOG_DEBUG, "密码获取失败, %s, %d", __FILE__, __LINE__);
                bzero(msgbuffer, sizeof (msgbuffer));
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' message='用户不存在'/>", EV_TYPE_AUTH_FAILURE);
                //write(fd, out, strlen(out));
                send_message(fd, msgbuffer);
                goto end;
            }


            log_write(LOG_DEBUG, "db_pw_value:%s, db_salt_value：%s, %s, %d", db_pw_value, db_salt_value, __FILE__, __LINE__);
            //char password[MAX_CHAR_LENGTH] = "13145200";
            char buf[MAX_CHAR_LENGTH] = {'\0'};

            md5(password, strlen(password), buf);
            log_write(LOG_DEBUG, "md5 password:%s, %s, %d", buf, __FILE__, __LINE__);
            char temp_password[MAX_CHAR_LENGTH];
            sprintf(temp_password, "%s%s", buf, db_salt_value);

            bzero(&buf, sizeof (buf));
            md5(temp_password, strlen(temp_password), (char*) buf);
            log_write(LOG_DEBUG, "md5 password2:%s, %s, %d", buf, __FILE__, __LINE__);
            
            if (strcmp(buf, db_pw_value) != 0) {
                log_write(LOG_DEBUG, "密码比较失败, %s, %d", __FILE__, __LINE__);
                bzero(msgbuffer, sizeof (msgbuffer));
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' message='密码错误'/>", EV_TYPE_AUTH_FAILURE);
                //write(fd, out, strlen(out));
                send_message(fd, msgbuffer);
                goto end;
            }
            strncpy(fdnode->username, username, MAX_CHAR_LENGTH);
        } else if (username && sign && account && timestamp) {
            log_write(LOG_DEBUG, "验证串验证, %s, %d", __FILE__, __LINE__);
            fdnode->anonymous = 0;
            if (account && strncmp("myleft", account, sizeof (account)) != 0) {
                strncpy(fdnode->username, account, 32);
                strncat(fdnode->username, username, 32);
            } else {
                strncpy(fdnode->username, username, MAX_CHAR_LENGTH);
            }
            log_write(LOG_DEBUG, username, __FILE__, __LINE__);
            //判断是密码登陆还是加密接口登陆

            //判断是否过期
            long sec = mytimestamp - timestamp;
            if (sec >= 0 && sec < 60) {
                char crypt_key[MAX_BUFFER_LENGTH];
                bzero(crypt_key, sizeof (crypt_key));

                char crypt_str[MAX_CHAR_LENGTH];
                bzero(crypt_str, sizeof (crypt_str));
                char *temp_key = lua_getaccountkey((char *) account);
                log_write(LOG_DEBUG, "加密串：%s, %s, %d", temp_key, __FILE__, __LINE__);
                if (temp_key != NULL) {

                    strncpy(crypt_key, temp_key, MAX_CHAR_LENGTH);
                    free(temp_key);
                    char temp_str[MAX_BUFFER_LENGTH];
                    bzero(temp_str, sizeof (temp_str));
                    strncat(temp_str, crypt_key, 18);
                    strcat(temp_str, username);
                    strcat(temp_str, timestamp_char);

                    strncpy(crypt_str, crypt(temp_str, sign), sizeof (crypt_str));
                }
                //判断加密串是否正确
                if (strcmp(crypt_str, sign) != 0) {
                    bzero(msgbuffer, sizeof (msgbuffer));
                    snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' message='验证码错误'/>", EV_TYPE_AUTH_FAILURE);
                    //write(fd, out, strlen(out));
                    send_message(fd, msgbuffer);
                    goto end;
                }
            } else {
                bzero(msgbuffer, sizeof (msgbuffer));
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' message='验证过期'/>", EV_TYPE_AUTH_FAILURE);
                send_message(fd, msgbuffer);
                goto end;
            }
        } else {
            bzero(msgbuffer, sizeof (msgbuffer));
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' message='请输入用户名和密码'/>", EV_TYPE_AUTH_FAILURE);
            send_message(fd, msgbuffer);
            goto end;
        }

        if (character)
        {
            bzero(fdnode->character, sizeof (fdnode->character));
            strncpy(fdnode->character, character, sizeof (fdnode->character));
        }

        //判断该用户名的用户在其他地方是否有登陆
        clients *node = get_fdnode_byname(fdnode->username, fdnode->fd);
        if (node != NULL) {
            log_write(LOG_DEBUG, "存在同名用户%s,%d", __FILE__, __LINE__);
            other_same_username(node);
        }

        node_add(fdnode);

        fdnode->type = lua_getuserrole(fdnode->username);
        fdnode->state = FD_STATE_SUCCESS;

        //发送登陆成功消息
        bzero(msgbuffer, sizeof (msgbuffer));
        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' username='%s' usertype='%d' message='登陆成功'/>", EV_TYPE_AUTH_SUCCESS, fdnode->username, fdnode->type);
        send_message(fd, msgbuffer);

        //加入房间
        join_room(fdnode, roomid);
        goto end;
    } else if (ev_type == EV_TYPE_USER_CHANGE_STATE && fdnode->state == FD_STATE_SUCCESS) {
        int state = 0;
        if (!ezxml_attr(event, "state")) state = atoi(ezxml_attr(event, "state"));

        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' username='%s' state='%d'/>", EV_TYPE_USER_CHANGE_STATE, fdnode->username, state);
        send_message_all(fd, msgbuffer);
        goto end;
    } else if (ev_type == EV_TYPE_CHANGE_CHARACTER && fdnode->state == FD_STATE_SUCCESS) { //改变角色
        strncpy(fdnode->character, character, CHAR_LENGTH);

        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' username='%s' character='%s'/>", EV_TYPE_CHANGE_CHARACTER, fdnode->username, fdnode->character);
        send_message_all(fd, msgbuffer);
        goto end;
    } else if (ev_type == EV_TYPE_PRIVATE_MESSAGE && fdnode->state == FD_STATE_SUCCESS && to && body) {

        bzero(msgbuffer, sizeof (msgbuffer));
        if (fdnode->type == CLIENT_TYPE_SHUTUP) {
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[您目前处于禁言状态]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp);
            send_message(fd, msgbuffer);
            goto end;
        }
        clients *tonode;
        if (strcmp(fdnode->username, to) == 0) {
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' from='%s' to='%s' timestamp='%ld'><body><![CDATA[%s]]></body></event>", EV_TYPE_PRIVATE_MESSAGE, fdnode->username, to, mytimestamp, body);
            send_message(fd, msgbuffer);
        } else if ((tonode = get_fdnode_byname(to, -1)) != NULL) {// tonode->roomid == fdnode->roomid 同房间
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' from='%s' to='%s' timestamp='%ld'><body><![CDATA[%s]]></body></event>", EV_TYPE_PRIVATE_MESSAGE, fdnode->username, to, mytimestamp, body);
            send_message(tonode->fd, msgbuffer);
            send_message(fd, msgbuffer);
        } else {
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[请您重新选择发言对象]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp);
            send_message(fd, msgbuffer);
        }
        goto end;
    } else if (ev_type == EV_TYPE_PUBLIC_MESSAGE && fdnode->state == FD_STATE_SUCCESS && body) {

        bzero(msgbuffer, sizeof (msgbuffer));
        if (fdnode->type == CLIENT_TYPE_SHUTUP) {
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[您目前处于禁言状态]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp);
            send_message(fd, msgbuffer);
            goto end;
        }
        log_write(LOG_DEBUG, "timestamp:%ld", mytimestamp);
        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' from='%s' timestamp='%ld'><body><![CDATA[%s]]></body></event>", EV_TYPE_PUBLIC_MESSAGE, fdnode->username, mytimestamp, body);
        send_message_all(fd, msgbuffer);
        goto end;
    } else if (ev_type == EV_TYPE_CHANGE_ROOM && fdnode->state == FD_STATE_SUCCESS) {
        if (fdnode->roomid != roomid) {
            //加入房间
            join_room(fdnode, roomid);
        }
        goto end;
    } else if (ev_type == EV_TYPE_GET_ROOM_LIST && fdnode->state == FD_STATE_SUCCESS) {
        send_roomlist(fdnode);
        goto end;
    } else if (ev_type == EV_TYPE_LEAVE_ROOM && fdnode->state == FD_STATE_SUCCESS) {
        //离开房间
        leave_room(fdnode);
        goto end;
    } else if (ev_type == EV_TYPE_ADMIN_COMMAND_GOOUT && fdnode->state == FD_STATE_SUCCESS) {
        log_write(LOG_DEBUG, "GOOUT SHUTUP username: %s, %s, %d", username, __FILE__, __LINE__);
        if (fdnode->type != CLIENT_TYPE_ADMIN) {
            bzero(msgbuffer, sizeof (msgbuffer));
            snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[您没有权限进行此操作]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp);
            send_message(fd, msgbuffer);
            goto end;
        } else if (username != NULL) {
            clients *tonode;
            tonode = NULL;
            if (strcmp(fdnode->username, username) == 0) {
                tonode = fdnode;
            } else {
                tonode = get_fdnode_byname(username, -1);
            }

            if (tonode != NULL) {
                log_write(LOG_DEBUG, "GOOUT SHUTUP3, %s, %d", __FILE__, __LINE__);
                bzero(msgbuffer, sizeof (msgbuffer));
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[%s被管理员请出了聊天室]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp, tonode->username);
                send_message_all(tonode->fd, msgbuffer);
                node_del(tonode, tonode->fd);
            } else {
                log_write(LOG_DEBUG, "tonode is NULL, %s, %d", __FILE__, __LINE__);
            }
        } else {
            log_write(LOG_DEBUG, "username is NULL, %s, %d", __FILE__, __LINE__);
        }
        goto end;
    }
    else if (fdnode->state == FD_STATE_SUCCESS)
    {
        if (ev_type == EV_TYPE_ADMIN_COMMAND_SHUTUP)
        {
            if (fdnode->type != CLIENT_TYPE_ADMIN) {
                bzero(msgbuffer, sizeof (msgbuffer));
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[您没有权限进行此操作]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp);
                send_message(fd, msgbuffer);
                goto end;
            } else if (username) {
                clients *tonode;

                if (strcmp(fdnode->username, username) == 0) {
                    tonode = fdnode;
                } else {
                    tonode = get_fdnode_byname(username, -1);
                }
                if (tonode != NULL) {
                    bzero(msgbuffer, sizeof (msgbuffer));
                    if (tonode->type != CLIENT_TYPE_SHUTUP) {
                        tonode->type = CLIENT_TYPE_SHUTUP;
                        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[%s被管理员禁言]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp, tonode->username);
                    } else {
                        tonode->type = CLIENT_TYPE_NONE;
                        snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' timestamp='%ld'><body><![CDATA[%s被管理员解除禁言]]></body></event>", EV_TYPE_SYSTEM_MESSAGE, mytimestamp, tonode->username);
                    }
                    send_message_all(tonode->fd, msgbuffer);
                }
            }
            goto end;
        } else if (ev_type == EV_TYPE_MOVE) {
            const char *direction = ezxml_attr(event, "direction");
            if (direction)
            {
                fdnode->direction = atoi(direction);
            }
            const char *x = ezxml_attr(event, "x");
            const char *y = ezxml_attr(event, "y");
            if (x && y) {
                fdnode->x = atoi(x);
                fdnode->y = atoi(y);
                log_write(LOG_DEBUG, "EV_TYPE_MOVE x:%d, y:%d\n", fdnode->x, fdnode->y);
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' from='%s' direction='%d' x='%d' y='%d'/>", EV_TYPE_MOVE, fdnode->username, fdnode->direction, fdnode->x, fdnode->y);

                log_write(LOG_DEBUG, "EV_TYPE_MOVE, %s, %d", __FILE__, __LINE__);
                send_message_all(fdnode->fd, msgbuffer);
            }
            goto end;
        } else if (ev_type == EV_TYPE_DOING) {

            const char *doing = ezxml_attr(event, "doing");

            if (doing) {//暂不记录
                bzero(fdnode->doing, sizeof (fdnode->doing));
                strncpy(fdnode->doing, doing, sizeof (fdnode->doing));
                snprintf(msgbuffer, sizeof (msgbuffer), "<event type='%d' from='%s' doing='%s'/>", EV_TYPE_DOING, fdnode->username, doing);

                log_write(LOG_DEBUG, "EV_TYPE_DOING, %s, %d", __FILE__, __LINE__);
                send_message_all(fdnode->fd, msgbuffer);
            }
            goto end;
        }
    }else {
        /* 运行脚本 */
        lua_exec(fd, ezxml_toxml(event));
        goto end;
    }
    goto end;
end:
    //if (body!=NULL) free(body);
    ezxml_free(event);
}
