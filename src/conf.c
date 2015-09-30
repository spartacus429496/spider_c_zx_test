/*******************************************************************************
*[Filename]        conf.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 配置文件操作接口函数
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "common.h"
#include "spider.h"
#include "conf.h"
#include "ipc_msg.h"


static int read_file(void *buf, int size)
{
	FILE   *fp = NULL;
	size_t  len = 0;

	fp = fopen(SPIDER_CONF_PATH SPIDER_CONF_FILE, "r+");
	if (fp == NULL)
	{
		return 0;
	}

	len = fread(buf, sizeof(char), size, fp);
    fclose(fp);

	return (int)len;
}

static int write_file(void *buf, int size)
{
	FILE *fp = NULL;
	size_t len = 0;
	char cmd[64];

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "mkdir %s 2>>/dev/null", SPIDER_CONF_PATH);
	if (system(cmd));

	fp = fopen(SPIDER_CONF_PATH SPIDER_CONF_FILE, "w+");
	if (fp == NULL)
	{
		return 0;
	}

	len = fwrite(buf, sizeof(char), size, fp);
	fclose(fp);

	return (int)len;
}

static int search_conf(conf_t *info, int count, const char *proc_name)
{
	int index = -1, i = 0;

	for (i = 0; i < count; i++)
	{
		if (strcmp(info[i].proc_name, proc_name) == 0)
		{
			index = i;
			break;
		}
	}
	return index;
}

static char* status_to_string(int status)
{
	static char string[8];

	memset(string, 0, sizeof(string));
	switch(status)
	{
	case STATUS_STOPPED: strcpy(string, "stopped"); break;
	case STATUS_RUNNING: strcpy(string, "running"); break;
	case STATUS_SUSPEND: strcpy(string, "suspend"); break;
	default: strcpy(string, "unknow"); break;
	}
	return string;
}

int get_sipder_info(const char *proc_name, int *status, long *shop_num)
{
	struct spider_info info;
	struct msg_handle *context = NULL;

	char   packet[8192];
	int    len = 0;

	context = msg_create(TYPE_CLIENT, NULL);
	if (context == NULL)
	{
		return -1;
	}

	memset(&info, 0, sizeof(info));
	strcpy(info.proc_name, proc_name);

    if (msg_send(context, MSG_ID_GET_INFO, &info, sizeof(info), proc_name) <= 0)
	{
		msg_close(context);
		return -1;
	}

	memset(packet, 0, sizeof(packet));
    len = msg_recv(context, packet, sizeof(packet));
	if (len <= 0)
	{
		msg_close(context);
		return -1;
	}

	*status = ((struct spider_info*)IPC_MSG_DATA(packet))->status;
	*shop_num = ((struct spider_info*)IPC_MSG_DATA(packet))->shop_num;

	msg_close(context);
	return 0;
}

int conf_show()
{
	conf_t info[128];
	int    count = 0, i = 0;

	memset(info, 0, sizeof(info));
    count = read_file(info, sizeof(info));
    count = count / sizeof(conf_t);

	printf("All Config:\n");
	for (i = 0; i < count; i++)
	{
	    int  status = STATUS_STOPPED;
	    long shop_num = 0;
        
		get_sipder_info(info[i].proc_name, &status, &shop_num);

		printf("proc_name=%s,shop_num1=%ld,shop_num2=%ld,db_host=%s,db_name=%s,db_user=%s,db_passwd=%s,db_port=%d,proc_sleep=%d,status=%s,curr_shop=%ld\n", 
			   info[i].proc_name, info[i].shop_num1, info[i].shop_num2, info[i].db_host, 
			   info[i].db_name, info[i].db_user, info[i].db_passwd, info[i].db_port, info[i].proc_sleep, status_to_string(status), shop_num);
	}
	return 0;
}

int conf_start(const char *proc_name)
{
	conf_t info[128];
	char   cmd[256], i = 0;

	int    count = 0, ret = 0;

	memset(info, 0, sizeof(info));
	count = read_file(info, sizeof(info));
	count = count / sizeof(conf_t);

	if (proc_name == NULL)
	{
		for (i = 0; i < count; i++)
		{
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "./spider daemon %s:%ld:%ld:%s:%s:%s:%s:%d:%d", 
				info[i].proc_name, info[i].shop_num1, info[i].shop_num2, info[i].db_host, 
				info[i].db_name, info[i].db_user, info[i].db_passwd, info[i].db_port, info[i].proc_sleep);
			ret = system(cmd);
		}
	}
	else
	{
		int index = search_conf(info, count, proc_name);
		if (index >= 0)
		{
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "./spider daemon %s:%ld:%ld:%s:%s:%s:%s:%d:%d", 
				info[index].proc_name, info[index].shop_num1, info[index].shop_num2, info[index].db_host, 
				info[index].db_name, info[index].db_user, info[index].db_passwd, info[index].db_port, info[index].proc_sleep);
			ret = system(cmd);
		}
	}
	return ret;
}

static void kill_process(const char *proc_name)
{
	struct spider_info info;
	struct msg_handle *context = NULL;

	context = msg_create(TYPE_CLIENT, NULL);
	if (context == NULL)
	{
		return ;
	}

	memset(&info, 0, sizeof(info));
	strcpy(info.proc_name, proc_name);
	info.status = STATUS_STOPPED;
    msg_send(context, MSG_ID_SET_INFO, &info, sizeof(info), proc_name);

	msg_close(context);
	return ;
}

int conf_stop(const char *proc_name)
{
	if (proc_name == NULL)
	{
	    conf_t info[128];
	    int    count = 0, i = 0;

        memset(info, 0, sizeof(info));
        count = read_file(info, sizeof(info));
        count = count / sizeof(conf_t);
		
		for (i = 0; i < count; i++)
		{
			kill_process(info[i].proc_name);
		}
	}
	else
	{
		kill_process(proc_name);
	}
	
	return 0;
}
int conf_add(conf_t *conf)
{
	conf_t info[128];
	int    count = 0;

	memset(info, 0, sizeof(info));
    count = read_file(info, sizeof(info));
    count = count / sizeof(conf_t);

	if (search_conf(info, count, conf->proc_name) >= 0)
	{
		return -1;
	}

	memcpy(&info[count], conf, sizeof(conf_t));
	count++;

	write_curr_shop(0, conf->proc_name);
	return write_file(info, count * sizeof(conf_t)) > 0 ? 0 : -1;
}
int conf_mod(conf_t *conf)
{
	conf_t info[128];
	int    count = 0, index = 0;

	memset(info, 0, sizeof(info));
    count = read_file(info, sizeof(info));
    count = count / sizeof(conf_t);

	index = search_conf(info, count, conf->proc_name);
	if (index < 0)
	{
		return -1;
	}
    
	memcpy(&info[index], conf, sizeof(conf_t));
	write_curr_shop(0, conf->proc_name);

	return write_file(info, count * sizeof(conf_t)) > 0 ? 0 : -1;
}
int conf_del(const char *proc_name)
{
	conf_t info[128];
	int    count = 0, index = 0;

	memset(info, 0, sizeof(info));
    count = read_file(info, sizeof(info));
    count = count / sizeof(conf_t);

	index = search_conf(info, count, proc_name);
	if (index < 0)
	{
		return -1;
	}
    
	conf_stop(info[index].proc_name);

	memmove(&info[index], &info[index + 1], (count - index - 1) * sizeof(conf_t));
	count--;
	write_curr_shop(0, proc_name);

	return write_file(info, count * sizeof(conf_t)) > 0 ? 0 : -1;
}
