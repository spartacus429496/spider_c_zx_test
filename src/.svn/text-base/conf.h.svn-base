#ifndef __CONF_H__
#define __CONF_H__

typedef struct conf
{
	char proc_name[32];

	long shop_num1;
	long shop_num2;

	char db_host[16];
	char db_name[32];
	char db_user[16];
	char db_passwd[16];
	int  db_port;

	int  proc_sleep; //second
}conf_t;

int conf_add(conf_t *conf);
int conf_mod(conf_t *conf);
int conf_del(const char *proc_name);
int conf_start(const char *proc_name);
int conf_stop(const char *proc_name);

int get_sipder_info(const char *proc_name, int *status, long *shop_num);
int conf_show();
#endif
