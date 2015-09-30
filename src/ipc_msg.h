#ifndef __IPC_MSG_H__
#define __IPC_MSG_H__

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define CLIENT_NAME         "/tmp/spider_%d"

#define TYPE_SERVER         1
#define TYPE_CLIENT         2

struct msg_handle
{
    int   sock_fd;
    int   ctx_type;
    char  sun_path[32];
    struct sockaddr_un addr;
};

struct msg_head
{
    int msg_id;
	int msg_len;
	int msg_flag;
};


#define MSG_ID_GET_INFO 1
#define MSG_ID_SET_INFO 2

struct spider_info
{
	char  proc_name[32];
	int   status;
	long  shop_num;
};


#define IPC_MSG_HEAD(data)   ((struct msg_head*)((void*)data))
#define IPC_MSG_ID(data)     (IPC_MSG_HEAD(data)->msg_id)
#define IPC_MSG_FLAG(data)   (IPC_MSG_HEAD(data)->msg_flag)
#define IPC_MSG_LENGTH(data) (IPC_MSG_HEAD(data)->msg_len)
#define IPC_MSG_DATA(data)   ((char*)(IPC_MSG_HEAD(data) + 1))

#define IPC_MSG_SET_HOST(host, name)                      \
{                                                         \
    memset(host, 0, sizeof(*host));                       \
    ((struct sockaddr_un*)host)->sun_family = AF_UNIX;    \
    strcpy(((struct sockaddr_un*)host)->sun_path, name);  \
}

struct msg_handle* msg_create(int type, const char *name);
void msg_close(struct msg_handle *context);
int msg_recv(struct msg_handle *context, char *data, int len);
int msg_send(struct msg_handle *context, int msg_id, void *data, int len, const char *name);

#endif