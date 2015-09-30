/*******************************************************************************
*[Filename]        spider.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 网络交易监管系统爬虫代码，每一个爬虫设计为单进程多线程模式。

                 每一个爬虫进程含有三个线程:
                 主线程用来执行爬虫启动时下发的爬取任务。
                 另外两个线程中的一个用来响应命令行程序的请求。
                 另一个作为辅助爬虫，用来爬取之前抓取失败的网店。

                 每一个爬虫进程都是单独工作了，可以同时启动多个爬虫进程，
                 他们之间不会相互干扰。
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <locale.h>
#include <wchar.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "spider.h"
#include "db_cmd.h"
#include "http_req.h"
#include "http_parse.h"
#include "common.h"
#include "conf.h"
#include "ipc_msg.h"

static struct spider_info share_info;
static struct msg_handle *context = NULL;

/*记录每一个爬虫的工作信息*/
typedef struct http_request_param
{
	mysql_handle_t *mysql;                  /*mysql数据库句柄*/
	long shop_num1;                         /*爬虫工作起始shop id*/
	long shop_num2;                         /*爬虫工作结束shop id*/
	int  repeat;                            /*区分当前爬虫是不是辅助爬虫*/
}http_request_param_t;

/**
  * @function http_request_process_func
  * @brief 爬虫进行数据爬取的主策略函数。按照shop id 从小到大进行
              遍历式爬取。该策略以后可能需要修改
  * @param p    爬虫的工作信息
  * @return NULL    无信息
  */
static void* http_request_process_func(http_request_param_t *p)
{
	mysql_handle_t *mysql = p->mysql;
	char *recv_data = NULL;
	long  shop_num = 0;
	char  http_req[128];

	recv_data = (char*)malloc(RECV_BUFF_SIZE);
	if (recv_data == NULL)
	{
		debug("http_request_process: memory alloc failed\n");
		return NULL;
	}

	for (shop_num = p->shop_num1; shop_num < p->shop_num2; shop_num++)
	{
		int len = 0;
		memset(recv_data, 0, RECV_BUFF_SIZE);
		memset(http_req, 0, sizeof(http_req));
        /*拼接每一个店铺的主页url*/
		sprintf(http_req, TAOBAO_SHOP_URL, shop_num);
        
		if (p->repeat == 0)/*如果当前爬虫不是备份爬虫，
		需要记录下当前，正在爬取的shop id*/
		{
		    share_info.shop_num = shop_num;
		}

		if (share_info.status == STATUS_STOPPED) 
		{
			goto out;
		}
        
        /*step1: 发送每个店铺的主页请求*/
		len = send_http_request(http_req, recv_data, RECV_BUFF_SIZE);
		if (len <= 0)
		{
		    sleep(20);
			continue;
		}

		if (strstr(recv_data, "没有找到相应的店铺信息"))
		{
			continue;
		}
        /*step2: 解析返回的每个店铺的主页页面，并从中取出
                          店主信息页面，和该店铺下商品信息页面进行
                          进一步抓取，分析。                                    
             */
		http_parse_process(mysql, shop_num, recv_data, len, p->repeat);        
#if !DBG

        /*step3: 如果当前爬虫不是辅助爬虫，则需要记录下当前
                          正在抓取的shop id，以便于调试。
             */
		if (p->repeat == 0)
		{
			write_curr_shop(shop_num, share_info.proc_name);
		}
#endif
	}

out:
    free(recv_data);
	return NULL;
}

/**
  * @function daemon_process
  * @brief 将当前进程设置为守护进程
  * @param 无
  * @return 无
  */
static void daemon_process()
{
	int fd, pid;

    /*父进程退出*/
	if ((pid = fork()) < 0) exit(1);
    if (pid != 0) exit(0); 

    /*子进程继续*/

    /*进程是会话的领头进程时setsid()调用失败并返回（-1）。
	setsid()调用成功后，返回新的会话的ID，
	调用setsid函数的进程成为新的会话的领头进程，
	并与其父进程的会话组和进程组脱离。
	由于会话对控制终端的独占性，
	进程同时与控制终端脱离。*/
	setsid();

    // splite console and first child prcess quit
	if ((pid = fork()) < 0) exit(1);
    if (pid != 0) exit(0); 

    //
    // second child prcess go on
	//

	//close file descript
    //for (i = 0; i < NOFILE; i++)
	//	close(i);

	if (chdir("/"));
	if (umask(0));

	if ((fd = open("/dev/null", O_RDWR)) == -1) 
	{
		exit(1); 
	}
	dup2(fd, STDIN_FILENO);
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);

    // commit init process
	signal(SIGCHLD, SIG_IGN);
}

/**
  * @function sigterm_handler
  * @brief 终止信号处理回调
  * @param arg  回调入参，暂时没有用
  * @return 无
  */
static void sigterm_handler(int arg)
{
#if 0
    void *buffer[100];
    int  nptrs = 0, i = 0;
	char  **err_str = NULL;
	char  buf[1024];
	FILE *fp = NULL;

	signal(arg, SIG_DFL);
    
	memset(buffer, 0, sizeof(buffer));

	fp = fopen("/root/spider.log", "wb+");
	if (fp == NULL)
	{
		return;
	}
   
	fputs("call trace:\n", fp);
    nptrs = backtrace(buffer, 100);
	err_str = backtrace_symbols(buffer, nptrs);
	for (i = 0; i < nptrs; i++)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%08d %s \n", i, err_str[i]);
		fputs(buf, fp);
	}
    fputs("\n", fp);
	fclose(fp);
	free(err_str);
	exit(EXIT_FAILURE);
#endif

	share_info.status = STATUS_STOPPED;/*将进程状态设置为停止*/
	msg_close(context);/*关闭套接字*/
	return ;
}

/**
  * @function help_me
  * @brief   爬虫命令行帮助信息
  * @param 无
  * @return 无
  */
static void help_me()
{
	printf("spider usage:\n\n");
	printf("spider help\n");
	printf("spider version\n");
	printf("spider show\n");
	printf("\n");
	printf("spider start [proc_name]\n");
	printf("spider stop  [proc_name]\n");
	printf("\n");
	printf("spider conf add name [proc_name] \n");
	printf("                shop_num1 [num1] shop_num2 [num2] \n");
	printf("                dbhost [ip] dbname [name] dbuser [user] dbpasswd [passwd] dbport [port] \n");
	printf("                sleep [second]\n");
	printf("\n");
	printf("spider conf mod name [proc_name] \n");
	printf("                shop_num1 [num1] shop_num2 [num2] \n");
	printf("                dbhost [ip] dbname [name] dbuser [user] dbpasswd [passwd] dbport [port] \n");
	printf("                sleep [second]\n");
	printf("\n");
	printf("spider conf del name [proc_name]\n");
	//printf("spider daemon name:shop_num1:shop_num2:dbhost:dbname:dbuser:dbpasswd:dbport:sleep\n");
	return;
}

/**
  * @function start
  * @brief   爬虫启动函数
  * @param args  命令行参数，可以为空，也可以是指定的爬虫名称。
  * @return 无
  */
static void start(const char *args)
{
	conf_start(args);
	return;
}

/**
  * @function stop
  * @brief   爬虫终止函数
  * @param args  命令行参数，可以为空，也可以是指定的爬虫名称。
  * @return 无
  */
static void stop(const char *args)
{
	conf_stop(args);
	return;
}

/**
  * @function show
  * @brief   爬虫状态查看函数，查看所有爬虫的工作信息
  * @param 无
  * @return 无
  */
static void show()
{
	conf_show();
	return;
}

/**
  * @function parse_conf_xxx
  * @brief   解析爬虫配置时的命令行参数，解析增加或者修改配置命令
  * @param argc   命令行参数个数
  * @param argv   命令字符串
  * @param start   从第几个命令行参数开始解析
  * @param mode   判断当前是增加配置还是修改配置
  * @return 0  解析成功
  * @return -1  解析失败
  */
static int parse_conf_xxx(int argc, char *argv[], int start, int mode)
{
	int i = 0, ret = 0;
	char num[16];
	conf_t info;

	if (argc < 19)
	{
		help_me();
		return -1;
	}

	memset(&info, 0, sizeof(info));
	for (i = start; i < argc; i++)
	{
		char *args = argv[i];
		if (strncmp(args, "name", 4) == 0)
		{
			strncpy(info.proc_name, argv[++i], sizeof(info.proc_name));
		}
		else if (strncmp(args, "shop_num1", 9) == 0)
		{
			memset(num, 0, sizeof(num));
			strncpy(num, argv[++i], sizeof(num));
			info.shop_num1 = atol(num);
		}
		else if (strncmp(args, "shop_num2", 9) == 0)
		{
			memset(num, 0, sizeof(num));
			strncpy(num, argv[++i], sizeof(num));
			info.shop_num2 = atol(num);
		}
		else if (strncmp(args, "dbhost", 6) == 0)
		{
			strncpy(info.db_host, argv[++i], sizeof(info.db_host));
		}
		else if (strncmp(args, "dbname", 6) == 0)
		{
			strncpy(info.db_name, argv[++i], sizeof(info.db_name));
		}
		else if (strncmp(args, "dbuser", 6) == 0)
		{
			strncpy(info.db_user, argv[++i], sizeof(info.db_user));
		}
		else if (strncmp(args, "dbpasswd", 8) == 0)
		{
			strncpy(info.db_passwd, argv[++i], sizeof(info.db_passwd));
		}
		else if (strncmp(args, "dbport", 8) == 0)
		{
			memset(num, 0, sizeof(num));
			strncpy(num, argv[++i], sizeof(num));
			info.db_port = atoi(num);
		}
		else if (strncmp(args, "sleep", 8) == 0)
		{
			memset(num, 0, sizeof(num));
			strncpy(num, argv[++i], sizeof(num));
			info.proc_sleep = atoi(num);
		}
		else
		{
			help_me();
			ret = -1;
			break;
		}
	}
	ret = mode ? conf_add(&info) : conf_mod(&info);
    return ret;
}

/**
  * @function parse_conf
  * @brief   解析爬虫配置时的命令行参数
  * @param argc   命令行参数个数
  * @param argv   命令字符串
  * @param start   从第几个命令行参数开始解析
  * @return 0  解析成功
  * @return -1  解析失败
  */
static int parse_conf(int argc, char *argv[], int start)
{
	int i = 0, ret = 0;

	if (argc < 5)
	{
		help_me();
		return -1;
	}

	for (i = start; i < argc; i++)
	{
		char *args = argv[i];
		if (strncmp(args, "add", 3) == 0 || strncmp(args, "mod", 3) == 0)
		{
			ret = parse_conf_xxx(argc, argv, i + 1, (strncmp(args, "add", 3) == 0) ? 1 : 0);
			break;
		}
		else if (strncmp(args, "del", 3) == 0)
		{
			if (strncmp(argv[3], "name", 4))
			{
				ret = -1;
				break;
			}
			ret = conf_del(argv[4]);
			break;
		}
		else
		{
			help_me();
			break;
		}
	}
    return ret;
}

/**
  * @function dead_loop
  * @brief   爬虫休眠函数，控制爬虫的休眠时间
  * @param second   指定爬虫的休眠时间
  * @return 0  休眠结束
  * @return -1  休眠过程中爬虫停止 了
  */
int dead_loop(int second)
{
	while (second--)
	{
		sleep(1);
		if (share_info.status == STATUS_STOPPED)
		{
			return -1;
		}
	}
	return 0;
}

/**
  * @function assister_func
  * @brief   辅助爬虫工作函数，主要用来爬取之前抓取失败的店铺IP
  * @param p   暂时没有使用
  * @return 0  暂时无用
  */
static void* assister_func(void *p)
{
    /*尝试链接数据库*/
	mysql_handle_t *mysql = connect_mysql();
	if (mysql == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		share_info.status = STATUS_STOPPED;
		return NULL;
    }

	while(share_info.status)
	{
		int i = 0, second = 0, count = 0;
		http_request_param_t req_param;
	    long shop_num[100];

		memset(shop_num, 0, sizeof(shop_num));
        /*查询数据库中前XX位的错误ship id，进行爬取，爬完之后再查找再爬取*/
		count = db_cmd_get_error_info(mysql, shop_num, (sizeof(shop_num) / sizeof(long)));
		for (i = 0; i < count; i++)
		{
			memset(&req_param, 0, sizeof(req_param));
			req_param.repeat = 1;
            
			req_param.mysql     = mysql;
			req_param.shop_num1 = shop_num[i];
			req_param.shop_num2 = shop_num[i] + 1;
            /*解析店铺信息*/
			http_request_process_func(&req_param);

			second = 1;
            /*休眠后进行下一轮爬取*/
			dead_loop(second);
		}
		second = 120;
		dead_loop(second);
	}

	free_mysql(mysql);
	return 0;
}

/**
  * @function msg_recv_handle
  * @brief   消息处理封装函数，主要处理来自命令行的消息。
  * @param p   暂时没有使用
  * @return 0 函数执行成功
  */
static void* msg_recv_handle(void *p)
{
	char   packet[8192];
	int    len = 0;
	struct spider_info *info = NULL;

	while(share_info.status)
	{
		memset(packet, 0, sizeof(packet));
		len = msg_recv(context, packet, sizeof(packet));
		if (len <= 0)
		{
			printf("msg_recv failed\n");
			share_info.status = STATUS_STOPPED;
			break;
		}

		info = (struct spider_info*)IPC_MSG_DATA(packet);

		switch(IPC_MSG_ID(packet))
		{
		case MSG_ID_GET_INFO:
			{
				msg_send(context, MSG_ID_GET_INFO, &share_info, sizeof(share_info), NULL);
				break;
			}
		case MSG_ID_SET_INFO:
			{
				if (strcmp(share_info.proc_name, info->proc_name) == 0)
				{
					share_info.status = info->status;
				}
				msg_send(context, MSG_ID_GET_INFO, &share_info, sizeof(share_info), NULL);
				break;
			}
		default: break;
		}
	}

	return 0;
}

/**
  * @function start_action
  * @brief   爬虫启动函数，通过解析命令行传递的参数，来时爬虫正常工作。
                该进程会启动两个辅助线程，一个用于响应命令请求。
                另一个作为辅助爬虫，进行之前处理失败的shop id爬起。
  * @param start_args   爬虫配置信息
  * @return 0  爬虫执行成功
  * @return -1  爬虫执行失败
  */
static int start_action(char *start_args)
{
	int  port = 0;
	char host[16], name[32], user[16], passwd[16];
	char num[16], proc_name[64];
	char *p1 = NULL, *p2 = NULL;
	http_request_param_t p;

	pthread_t tid = 0;
	int status = 0;
	long tmp_shop = 0;

#if !DBG
    /*将当前进程设置为守护进程*/
	daemon_process();
#endif
	signal(SIGPIPE, SIG_IGN);
	//signal(SIGILL,  sigterm_handler);
	signal(SIGTERM, sigterm_handler);
	//signal(SIGSEGV, sigterm_handler);
	//signal(SIGABRT, sigterm_handler);

	memset(&p, 0, sizeof(p));
	memset(host, 0, sizeof(host));
	memset(name, 0, sizeof(name));
	memset(user, 0, sizeof(user));
	memset(passwd, 0, sizeof(passwd));

    /*step1: 解析命令参数*/
	p1 = start_args;
	//name
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	memset(proc_name, 0, sizeof(proc_name));
	strncpy(proc_name, p1, p2 - p1);

	//shop_num1
	memset(num, 0, sizeof(num));
	p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(num, p1, p2 - p1);
	p.shop_num1 = atol(num);

	//shop_num2
    memset(num, 0, sizeof(num));
	p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(num, p1, p2 - p1);
	p.shop_num2 = atol(num);

	//dbhost
	p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(host, p1, p2 - p1);

	//dbname
	p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(name, p1, p2 - p1);

	//dbuser
	p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(user, p1, p2 - p1);

	//dbpasswd
	p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(passwd, p1, p2 - p1);

	//dbport
	memset(num, 0, sizeof(num));
    p1 = p2 + 1;
	p2 = strchr(p1, ':');
	if (p2 == NULL)
	{
		return -1;
	}
	strncpy(num, p1, p2 - p1);
	port = atoi(num);
	db_param_set(host, name, user, passwd, port);

	//proc_sleep
    memset(num, 0, sizeof(num));
	p1 = p2 + 1;
	strcpy(num, p1);

    /*step2: 做一些爬虫启动前的准备工作，设置进程状态等*/
	p.repeat = 0;

	memset(&share_info, 0, sizeof(share_info));
	strcpy(share_info.proc_name, proc_name);
	share_info.status = STATUS_RUNNING;

    /*step3: 尝试获取一下当前爬虫信息，其实就是show一下名称为share_info.proc_name
                   的爬虫。
                   如果能够获取到信息，则说明该爬虫已经启动过了，为了
                   避免重复启动，会直接退出。
                   如果获取不到信息，就说明该爬虫之前没有启动，可以启动
                   当前爬虫。
       */
	if (get_sipder_info(share_info.proc_name, &status,  &tmp_shop) == 0)
	{
		return -1;
	}

    /*step4: 开始启动当前爬虫，
                   首先，创建服务端socket，存入全局变量context中
                   接着，启动两个辅助线程，一个作为服务端线程响应命令行的请求。
                   另一个作为辅助爬虫。
                   最后，开始主线程爬虫的循环爬取工作。
      */
	context = msg_create(TYPE_SERVER, share_info.proc_name);
	if (context == NULL)
	{
		printf("msg_create failed\n");
		return -1;
	}
    
	http_init();

#if !DBG
    /*启动两个辅助线程*/
    pthread_create(&tid, NULL, assister_func, NULL);
	pthread_create(&tid, NULL, msg_recv_handle, NULL);
    
    /*从配置文件中读取，之前这个爬虫爬到哪个shop id了，会
         从这个shop id开始接着爬。
    */
	tmp_shop = read_curr_shop(share_info.proc_name);
	p.shop_num1 = tmp_shop > 0 ? (tmp_shop + 1) : p.shop_num1;
#endif

    /*主爬虫线程开始循环爬取*/
	do
	{
#if !DBG
		mysql_handle_t *mysql = connect_mysql();
	    if (mysql == NULL)
	    {
			write_log("connect_mysql failed\n");
		    share_info.status = STATUS_STOPPED;
			return 0;
        }

		p.mysql = mysql;
#endif
		http_request_process_func(&p);

		if (share_info.status == STATUS_RUNNING)
		{
			int second = atoi(num);
			share_info.status = STATUS_SUSPEND;

			dead_loop(second);
		}
#if !DBG
		free_mysql(mysql);
#endif
	}
	while(share_info.status);

	msg_close(context);
	http_clean();
	return 0;
}

/**
  * @function main
  * @brief  爬虫main函数，用来处理爬虫的各种命令
  * @param argc   命令行参数个数
  * @param argv   命令行参数
  * @return 0  命令行参数执行成功
  * @return -1  命令行参数解析失败
  */
int main(int argc, char *argv[])
{
	int i = 0, ret = 0;

#if DBG
	start_action("spider_0:69718518:69718519:127.0.0.1:taobao:root:zxsoft_egs:3306:60");
	return 0;
#endif

	if (argc <= 1)
	{
		help_me();
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		char *args = argv[i];
		if (strncmp(args, "help", 4) == 0)
		{
			help_me();
			break;
		}
		else if (strncmp(args, "version", 7) == 0)
		{
			printf("spider: version %s\n", SPIDER_VERSION);
			break;
		}
        /*执行show命令*/
		else if (strncmp(args, "show", 4) == 0)
		{
			conf_show();
			break;
		}
        /*执行爬虫开始命令，会读取配置文件然后启动爬虫*/
		else if (strncmp(args, "start", 5) == 0)
		{
			start(argv[i + 1]);
			break;
		}
        /*执行开始命令*/
		else if (strncmp(args, "stop", 4) == 0)
		{
			stop(argv[i + 1]);
			break;
		}
        /*解析执行配置命令*/
		else if (strncmp(args, "conf", 4) == 0)
		{
			ret = parse_conf(argc, argv, i + 1);
			break;
		}        
        /*爬虫启动*/
		else if (strncmp(args, "daemon", 6) == 0)
		{
			ret = start_action(argv[i+1]);
			break;
		}
		else
		{
			help_me();
			break;
		}
	}

    return 0;
}
