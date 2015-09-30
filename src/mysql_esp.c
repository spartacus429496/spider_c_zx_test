/*******************************************************************************
*[Filename]        mysql_esp.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 数据库处理接口函数，提供了增删改查，以及
                 批量操作的接口函数。
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "spider.h"
#include "common.h"
#include "mysql_esp.h"


typedef struct col
{
	char         name[MYSQL_FIELD_NAME_LEN];
	void        *value;
	unsigned int value_len;
}col_t;

typedef struct row
{
    col_t field[MYSQL_FIELD_CNT_LIMIT];
	int   cnt;
}row_t;


mysql_handle_t* open_mysql(const char *host, const char *dbname, const char *user, const char *passwd, int port)
{
	mysql_handle_t *mysql = NULL;
	unsigned int timeout = 30;
	my_bool reconnect = 1;

	if (host[0] == 0 || user[0] == 0 || passwd[0] == 0 || dbname[0] == 0 || port > 65536 || port < 1)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}
	
	mysql = (mysql_handle_t*)malloc(sizeof(mysql_handle_t));
	if (mysql == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	memset(mysql, 0, sizeof(mysql_handle_t));
	mysql_init(&(mysql->mysql));
	mysql_options(&(mysql->mysql), MYSQL_OPT_CONNECT_TIMEOUT, (char*)&timeout);
	mysql_options(&(mysql->mysql), MYSQL_OPT_RECONNECT,       (char*)&reconnect);

	if(mysql_real_connect(&(mysql->mysql), host, user, passwd, dbname, port, NULL, 0) == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		close_mysql(mysql);
		return NULL;
	}

	if(mysql_set_character_set(&(mysql->mysql), "gbk"))
	{
		debug("%s line @%d\n", __func__, __LINE__);
		close_mysql(mysql);
		return NULL;
	}

	mysql_set_server_option(&(mysql->mysql), MYSQL_OPTION_MULTI_STATEMENTS_ON);

	return mysql;
}

void close_mysql(mysql_handle_t *mysql)
{
	if (mysql == NULL)
	{
		return;
	}

    free_query_result(mysql);
	mysql_close(&(mysql)->mysql);
	free(mysql);
	return;
}

int execute_no_query_array(mysql_handle_t *mysql, char **sql, int count)
{
	int ret = 0, i = 0;

	ret = mysql_autocommit(&(mysql->mysql), 0);
	if (ret != 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return ret;
	}

	for (i = 0; i < count; i++)
	{
		ret = mysql_query(&(mysql->mysql),sql[i]);
		if (ret != 0)
		{
			return ret;
		}
	}

	ret = mysql_commit(&(mysql->mysql));
	if (ret != 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		mysql_rollback(&(mysql->mysql));
		return ret;
	}
	mysql_autocommit(&(mysql->mysql), 1); 

	return ret;
}
int execute_no_query(mysql_handle_t *mysql, char *sql)
{
	return mysql_query(&(mysql->mysql),sql);
}

int execute_query(mysql_handle_t *mysql, const char *sql)
{
	int ret = 0;
	unsigned int row = 0, col = 0, i = 0, j = 0;
	MYSQL_ROW mysql_row;
	MYSQL_FIELD *mysql_field = NULL;
	row_t *rows = NULL;

	// free the last query result
	free_query_result(mysql);

	ret = mysql_query(&(mysql->mysql), sql);
	if (ret != 0)
	{
		write_log("mysql_query error");
		return ret;
	}

    mysql->mysql_res = mysql_store_result(&(mysql->mysql));
    if(mysql->mysql_res == NULL)
	{
		write_log("mysql_store_result error");
		return -1;
	}

    row = (unsigned int)mysql_num_rows(mysql->mysql_res);
    col = (unsigned int)mysql_num_fields(mysql->mysql_res);
	mysql->result_len = row;
   
   for(i = 0; i < row; i++)
   {
      rows = (row_t*)malloc(sizeof(row_t));
	  if (rows == NULL)
	  {
		  write_log("alloc failed\n");
		  exit(1);
	  }
	  memset(rows, 0, sizeof(row_t));
	  rows->cnt = col;

	  mysql_row = mysql_fetch_row(mysql->mysql_res);
	  for(j = 0; j < col; j++)
	  {
		  mysql_field = mysql_fetch_field_direct(mysql->mysql_res, j);
		  strcpy(rows->field[j].name, mysql_field->name);
		  rows->field[j].value = mysql_row[j];
		  rows->field[j].value_len = mysql_field->max_length;
	  }
	  mysql->result[i] = (void*)rows;
	  if (i >= MYSQL_SELECT_ROW_LIMIT)
	  {
		  debug("%s line @%d\n", __func__, __LINE__);
		  mysql->result_len = MYSQL_SELECT_ROW_LIMIT;
		  break;
	  }
   }
   return mysql->result_len;
}

void free_query_result(mysql_handle_t *mysql)
{
	int i = 0;
	row_t *rows = NULL;

	for (i = 0; i < mysql->result_len; i++)
	{
		rows = (row_t*)mysql->result[i];
		if(rows != NULL)
		{
			free(rows);
			mysql->result[i] = NULL;
		}
	}
	mysql->result_len = 0;

	if(mysql->mysql_res)
	{
		mysql_free_result(mysql->mysql_res);
		mysql->mysql_res = NULL;
	}
	return;
}

int open_prepare(mysql_handle_t *mysql, const char *sql)
{
	mysql->mysql_stmt = mysql_stmt_init(&(mysql->mysql));
	if (mysql->mysql_stmt == NULL)
	{
	    debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}

	if (mysql_stmt_prepare(mysql->mysql_stmt, sql, strlen(sql)))
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

int bind_param(mysql_handle_t *mysql, MYSQL_BIND param[])
{
	if (mysql_stmt_bind_param(mysql->mysql_stmt, param))
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}
int execute_prepare(mysql_handle_t *mysql)
{
	if (mysql_stmt_execute(mysql->mysql_stmt))
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	return mysql_stmt_affected_rows(mysql->mysql_stmt);
}
int close_prepare(mysql_handle_t *mysql)
{
	if (mysql_stmt_close(mysql->mysql_stmt))
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	return 0;
}

int get_int_value_by_name(mysql_handle_t *mysql, int row, char *fd_name)
{
	row_t *rows = NULL;
	int len = mysql->result_len, ret = 0, i = 0;

	if(row >= len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	for(i = 0; i < rows->cnt; i++)
	{
		if(!strcmp(rows->field[i].name, fd_name))
		{
			ret = atoi((char *)rows->field[i].value);
			break;
		}
	}
	return ret;
}
int get_int_value_by_index(mysql_handle_t *mysql, int row, int fd_num)
{
	row_t *rows = NULL;
	int len = mysql->result_len, ret = 0;

	if(row >= len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}
	ret = rows->field[fd_num].value ? atoi((char *)(rows->field[fd_num].value)) : 0;
	return ret;
}

char* get_string_value_by_name(mysql_handle_t *mysql, int row, char *fd_name, unsigned int *len)
{
	row_t *rows = NULL;
	int i = 0;
	char *ret = NULL;

	if(row >= mysql->result_len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	for(i = 0; i < rows->cnt; i++)
	{
		if(!strcmp(rows->field[i].name, fd_name))
		{
			*len = (unsigned int)(rows->field[i].value_len);
			ret  = (char*)(rows->field[i].value);
			break;
		}
	}
	return ret;
}
char* get_string_value_by_index(mysql_handle_t *mysql, int row, int fd_num, unsigned int *len)
{
	row_t *rows = NULL;
	char *ret = NULL;

	if(row >= mysql->result_len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	*len = (unsigned int)(rows->field[fd_num].value_len);
	ret  = (char*)(rows->field[fd_num].value);
	return ret;
}

char* get_datatime_value_by_name(mysql_handle_t *mysql, int row, char *fd_name)
{
	row_t *rows = NULL;
	char * ret = NULL, i = 0;

    if(row >= mysql->result_len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}
	for(i = 0; i < rows->cnt; i++)
	{
		if(!strcmp(rows->field[i].name, fd_name))
		{
			ret = (char*)(rows->field[i].value);
			break;
		}
	}
	return ret;
}
char* get_datatime_value_by_index(mysql_handle_t *mysql, int row, int fd_num)
{
	row_t *rows = NULL;

    if(row >= mysql->result_len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return NULL;
	}

	return (char *)(rows->field[fd_num].value);
}

int get_binary_value_by_name(mysql_handle_t *mysql, int row, char *fd_name, char *out_buf)
{
	row_t *rows = NULL;
	int i = 0, len = 0;

	if (out_buf == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

    if(row >= mysql->result_len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	for(i = 0; i < rows->cnt; i++)
	{
		if(!strcmp(rows->field[i].name, fd_name))
		{
			len = (int)rows->field[i].value_len;
			memcpy(out_buf, (char *)(rows->field[i].value), len);
			break;
		}
	}
	return len;
}
int get_binary_value_by_index(mysql_handle_t *mysql, int row,int fd_num, char *out_buf)
{
	row_t *rows = NULL;
	int len = 0, i = 0;

	if (out_buf == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	if(row >= mysql->result_len)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	rows = (row_t*)(mysql->result[row]);
	if (rows == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return 0;
	}

	len = rows->field[fd_num].value_len;
	memcpy(out_buf, (char *)(rows->field[fd_num].value), len);
	return len;
}
