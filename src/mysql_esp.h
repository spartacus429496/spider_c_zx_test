#ifndef __MYSQL_ESP_H__
#define __MYSQL_ESP_H__

#include <mysql/mysql.h>



typedef struct mysql_handle
{
	MYSQL       mysql;
	MYSQL_STMT *mysql_stmt;
    MYSQL_RES  *mysql_res;
	void       *result[MYSQL_SELECT_ROW_LIMIT];
	int         result_len;
}mysql_handle_t;


//
// open and close mysql handle
//
mysql_handle_t* open_mysql(const char *host, const char *dbname, const char *user, const char *passwd, int port);
void close_mysql(mysql_handle_t *mysql);


//
// execute update, insert, delete and so on...
//

/*
Example:
mysql_handle_t *mysql = NULL;
char *sql[1];

mysql = open_mysql("192.168.61.138", "spider", "root", "root", 3306);
if (mysql == NULL)
{
    debug("connect failed\n");
    return -1;
}

sql[0] = (char*)malloc(100);
memset(sql[0], 0, 100);
sprintf(sql[0], "insert into shop_info(shop_id,shop_name,price) value(%d,'%s', %d)", 10000, "商家1", 99);
if (execute_no_query(mysql, sql, 1))
{
	printf("insert failed\n");
}
free(sql[0]);
close_mysql(mysql);
*/
int execute_no_query_array(mysql_handle_t *mysql, char **sql, int count);
int execute_no_query(mysql_handle_t *mysql, char *sql);

//
// execute select
//

/*
Example:
int i = 0;
mysql_handle_t *mysql = NULL;

mysql = open_mysql("192.168.61.138", "spider", "root", "root", 3306);
if (mysql == NULL)
{
    debug("connect failed\n");
    return -1;
}

int line = execute_query(mysql, "select shop_id from shop_info");
printf("line=%d\n", line);

for (i = 0; i < line; i++)
{
printf("shop_id=%d, ", get_int_value_by_index(mysql, i, 0));
printf("shop_name=%s, ", get_datatime_value_by_index(mysql, i, 1));
printf("price=%d\n", get_int_value_by_index(mysql, i, 2));
}
free_query_result(mysql);
close_mysql(mysql);
*/
int execute_query(mysql_handle_t *mysql, const char *sql);
void free_query_result(mysql_handle_t *mysql);



//
// execute execute update, insert, delete and so on by bind parameter method
// sql eg. INSERT INTO test_table(col1,col2,col3) VALUES(?,?,?)
// 

/*
Example:
mysql_handle_t *mysql = NULL;
MYSQL_BIND bind[3];
int  shop_id = 0;
char shop_name[32];
unsigned long name_len;

int  price = 999;
char *sql = "insert into shop_info(shop_id,shop_name,price) value(?, ?, ?)";

mysql = open_mysql("192.168.61.138", "spider", "root", "root", 3306);
if (mysql == NULL)
{
    debug("connect failed\n");
    return -1;
}

if (open_prepare(mysql, sql))
{
    printf("prepare faile\n");
    goto out;
}

memset(bind, 0, sizeof(bind));
PREPARE_INTEGER(bind[0], &shop_id);
PREPARE_STRING(bind[1], shop_name, sizeof(shop_name), &name_len);         
PREPARE_INTEGER(bind[2], &price);
if (bind_param(mysql, bind))
{
    printf("bind failed\n");
    close_prepare(mysql);
    goto out;
}

shop_id = 100000;
strcpy(shop_name, "代销\'店");
name_len = strlen(shop_name);

if (execute_prepare(mysql) <= 0)
{
    printf("execute_prepare failed\n");
    close_prepare(mysql);
    goto out;
}

close_prepare(mysql);
close_mysql(mysql);
*/
#define PREPARE_PARAMETER(param, type, buf_ptr, buf_len, len_ptr)   \
{                                                                   \
	memset(&param, 0, sizeof(param));                               \
	param.buffer_type   = type;                                     \
	param.buffer        = (char*)buf_ptr;                           \
	param.buffer_length = buf_len  ;                                \
	param.is_null       = 0;                                        \
	param.length        = len_ptr;                                 \
}
#define PREPARE_INTEGER(param, buf_ptr)                    PREPARE_PARAMETER(param, MYSQL_TYPE_LONG, buf_ptr, 0, 0)
#define PREPARE_SMALLINT(param, buf_ptr)                   PREPARE_PARAMETER(param, MYSQL_TYPE_SHORT, buf_ptr, 0, 0)
#define PREPARE_STRING(param, buf_ptr, buf_len, len_ptr)   PREPARE_PARAMETER(param, MYSQL_TYPE_STRING, buf_ptr, buf_len, len_ptr)

int open_prepare(mysql_handle_t *mysql, const char *sql);
int bind_param(mysql_handle_t *mysql, MYSQL_BIND param[]);
int execute_prepare(mysql_handle_t *mysql);
int close_prepare(mysql_handle_t *mysql);


//
// get select result
// fd_num start from 0
//
int get_int_value_by_name(mysql_handle_t *mysql, int row, char *fd_name);
int get_int_value_by_index(mysql_handle_t *mysql, int row, int fd_num);

char* get_string_value_by_name(mysql_handle_t *mysql, int row, char *fd_name, unsigned int *len);
char* get_string_value_by_index(mysql_handle_t *mysql, int row, int fd_num, unsigned int *len);

char* get_datatime_value_by_name(mysql_handle_t *mysql, int row, char *fd_name);
char* get_datatime_value_by_index(mysql_handle_t *mysql, int row, int fd_num);

int get_binary_value_by_name(mysql_handle_t *mysql, int row, char *fd_name, char *out_buf);
int get_binary_value_by_index(mysql_handle_t *mysql, int row, int fd_num, char *out_buf);
#endif
