#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "spider.h"
#include "common.h"
#include "ipc_msg.h"


struct msg_handle* msg_create(int type, const char *name)
{
	struct msg_handle *context = NULL;
    struct sockaddr_un  host;

    context = (struct msg_handle*)malloc(sizeof(struct msg_handle));
    if (context == NULL)
    {
        return NULL;
    }
    memset(context, 0, sizeof(struct msg_handle));

    context->ctx_type = type;
    switch(context->ctx_type)
    {
        case TYPE_SERVER:
            sprintf(context->sun_path, "/tmp/%s", name);
            break;

        case TYPE_CLIENT:
            sprintf(context->sun_path, CLIENT_NAME, getpid());
            break;

        default:
            goto FAILED;
    }

    // each time you create before, delete previous link
    unlink(context->sun_path);

    context->sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (context->sock_fd < 0)
    {
        goto FAILED;
    }

    IPC_MSG_SET_HOST(&host, context->sun_path);
    if (bind(context->sock_fd, (struct sockaddr*)&host, sizeof(host)) < 0)
    {
        close(context->sock_fd);
        goto FAILED;
    }
    return context;

FAILED:
    free(context);
    return NULL;
}

void msg_close(struct msg_handle *context)
{
	if (context != NULL)
    {
        unlink(context->sun_path);
        close(context->sock_fd);
        free(context);
    }
    return;
}
int msg_recv(struct msg_handle *context, char *data, int len)
{
	socklen_t addr_len = 0;

    if (context == NULL || data == NULL || len <= 0)
    {
        return -1;
    }

    addr_len = sizeof(context->addr);
    len = (len < 8192) ? len : 8192;

    memset(&(context->addr), 0, addr_len);
    len = recvfrom(context->sock_fd, data, len, 0, (struct sockaddr*)&(context->addr), &addr_len);

    if (len < sizeof(struct msg_head))
    {
        return -1;
    }

    return len;
}
int msg_send(struct msg_handle *context, int msg_id, void *data, int len, const char *name)
{
	char   packet[8192], path[64];
    struct sockaddr_un to;

    if (context == NULL || len > (8192 - sizeof(struct msg_head)))
    {
		return -1;
    }

    memset(packet, 0, sizeof(packet));
    IPC_MSG_ID(packet)     = msg_id;
    IPC_MSG_LENGTH(packet) = len;
    IPC_MSG_FLAG(packet)   = 0;
    memcpy(IPC_MSG_DATA(packet), data, len);
    len = len + sizeof(struct msg_head);

    switch(context->ctx_type)
    {
        case TYPE_SERVER:
            memcpy(&to, &(context->addr), sizeof(to));
            break;

        case TYPE_CLIENT:
			memset(path, 0, sizeof(path));
			sprintf(path, "/tmp/%s", name);
            IPC_MSG_SET_HOST(&to, path);
            break;

        default:
            return -1;
    }

    return sendto(context->sock_fd, packet, len, 0, (struct sockaddr*)&to, sizeof(to));
}
