ARPGServer
==========

This Server is simple RPG server, use C write, run in Linux.

C实现的RPG游戏服务端模型。 
项目原名AS3Chat，托管地址：http://code.google.com/p/as3chat

Demo：http://myleft.org 点击“登录”即可。

it's more than just a chat room, chat room is only one of its applications. 
it can achieve many functions via socket communication. 
This is what i am trying to implement,which is simple but useful.

Please write me comments on:

email:dreamsxin@qq.com

----it's written by Belle at 13:00,Nov 21,2009

这个不只是为了做聊天室而做的，通过socket通信可以完成很多功能。这就是我要实现的东西，简单，实用。
功能其实很简单，希望能通过简单的功能做出好用的东西，因为做的比较匆忙，有些BUG、功能也没有时间去修复和完善，如果可以的话，可以大家一起来完善，成为一个真正可以拿来用的项目。
服务端可以运行在Linux内核2.6以上版本(epoll)。

集成Lua5.2

其它项目
https://github.com/dreamsxin/ARPGClient
https://github.com/dreamsxin/CKohana

QQ:176013762
Email:dreamsxin@qq.com

Installer
---------

#### Requirements
We need some packages previously installed.

* SSL development resources
* MySQLClient development resources
* Zlib development resources
* Lua5.2 development resources
* GCC compiler

Ubuntu:

```bash
sudo apt-get install libssl-dev libmysqlclient-dev zlib1g-dev liblua5.2-dev
```
