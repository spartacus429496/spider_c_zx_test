/*******************************************************************************
*[Filename]        db_cmd.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 ��װ��һЩ���ݿ������������Ʒ��Ϣ��ѯ�޸ģ�
                 ������Ϣ��ѯ�޸ĵȵ�
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "spider.h"
#include "common.h"
#include "db_cmd.h"
#include "mysql_esp.h"

static char db_host[16], db_name[32], db_user[32], db_passwd[32];
static int  db_port = 3306;

/**
  * @function shop_show
  * @brief ��ӡshop�и��ֶε�ֵ�����ڵ���
  * @param shop   ������Ϣ
  * @return ��
  */
static void shop_show(shop_t *shop)
{
	char utf8[512];
	debug("\nshop_url %s:\n", shop->shop_url);

	gbk2utf(shop->shop_name, strlen(shop->shop_name), utf8, sizeof(utf8));
	debug("name=%s, ", utf8);
	gbk2utf(shop->shop_owner, strlen(shop->shop_owner), utf8, sizeof(utf8));
	debug("owner=%s, ", utf8);
	gbk2utf(shop->shop_location, strlen(shop->shop_location), utf8, sizeof(utf8));
	debug("loc=%s, ", utf8);
	gbk2utf(shop->shop_trade_range, strlen(shop->shop_trade_range), utf8, sizeof(utf8));
	debug("trade=%s\n", utf8);

	if (shop->shop_classify == SHOP_TMALL_TYPE)
	{
		debug("    tmall.com:\n");
		gbk2utf(shop->tmall.shop_commany_name, strlen(shop->tmall.shop_commany_name), utf8, sizeof(utf8));
		debug("    commany=%s, ", utf8);
		gbk2utf(shop->tmall.shop_telephone, strlen(shop->tmall.shop_telephone), utf8, sizeof(utf8));
		debug("tel=%s\n", utf8);
	}
	else
	{
		debug("    taobao.com:\n");
		debug("    create=%s, pop=%s, credit=%s\n", shop->taobao.shop_create_time, shop->taobao.shop_popularity, shop->taobao.shop_credit);
	}
}

/**
  * @function shop_commodity
  * @brief ��ӡcommodity�и��ֶε�ֵ�����ڵ���
  * @param commodity   ��Ʒ��Ϣ
  * @return ��
  */
static void shop_commodity(commodity_t *commodity)
{
	char utf8[512];

	gbk2utf(commodity->commodity_name, strlen(commodity->commodity_name), utf8, sizeof(utf8));
	debug("commodity_name=%s, ", utf8);

	gbk2utf(commodity->commodity_id, strlen(commodity->commodity_id), utf8, sizeof(utf8));
	debug("commodity_id=%s, ", utf8);

	gbk2utf(commodity->commodity_url, strlen(commodity->commodity_url), utf8, sizeof(utf8));
	debug("commodity_url=%s, ", utf8);

	gbk2utf(commodity->commodity_price, strlen(commodity->commodity_price), utf8, sizeof(utf8));
	debug("commodity_price=%s, ", utf8);

	gbk2utf(commodity->commodity_promo_price, strlen(commodity->commodity_promo_price), utf8, sizeof(utf8));
	debug("commodity_promo_price=%s, ", utf8);

	gbk2utf(commodity->commodity_sales, strlen(commodity->commodity_sales), utf8, sizeof(utf8));
	debug("commodity_sales=%s, ", utf8);

	gbk2utf(commodity->commodity_stock, strlen(commodity->commodity_stock), utf8, sizeof(utf8));
	debug("commodity_stock=%s, ", utf8);

	gbk2utf(commodity->commodity_comment, strlen(commodity->commodity_comment), utf8, sizeof(utf8));
	debug("commodity_comment=%s\n", utf8);
}

/**
  * @function connect_mysql
  * @brief  ����mysql ���ݿ�
  * @param ��
  * @return mysql_handle_t  ���ݿ���
  */
mysql_handle_t* connect_mysql()
{
	return open_mysql(db_host, db_name, db_user, db_passwd, db_port);
}

/**
  * @function connect_mysql
  * @brief  �ͷ����ݿ���
  * @param mysql  ���ݿ���
  * @return ��
  */
void free_mysql(mysql_handle_t *mysql)
{
	close_mysql(mysql);
	return;
}

/**
  * @function db_param_set
  * @brief �������ݿ����
  * @param host  ���ݿ��ַ
  * @param name  ���ݿ�����
  * @param user  ���ݿ��û���
  * @param passwd  ����
  * @param port  �˿ں�
  * @return ��
  */
void db_param_set(const char *host, const char *name, const char *user, const char *passwd, const int port)
{
	memset(db_host, 0, sizeof(db_host));
	memset(db_name, 0, sizeof(db_name));
	memset(db_user, 0, sizeof(db_user));
	memset(db_passwd, 0, sizeof(db_passwd));

	strcpy(db_host,   host);
	strcpy(db_name,   name);
	strcpy(db_user,   user);
	strcpy(db_passwd, passwd);
	db_port = port;
}

/**
  * @function db_cmd_shop
  * @brief ��������Ϣ���������ݿ�
  * @param mysql  ���ݿ���
  * @param shop  ������Ϣ
  * @return 0  �ɹ�
  * @return -1  ʧ��
  */
int db_cmd_shop(mysql_handle_t *mysql, shop_t *shop)
{
	int ret = 0;
#if !DBG
	char time_buf[32];
	time_t  now;
    struct tm *timenow = NULL;
	char sql[2048];

	if (mysql == NULL)
	{
		write_log("db_cmd_shop:mysql handle is null\n");
		return -1;
	}

	time(&now);
    timenow = localtime(&now);
    memset(time_buf, 0, sizeof(time_buf));
    sprintf(time_buf, "%d-%.2d-%.2d %.2d:%.2d:%.2d",timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

	memset(sql, 0, sizeof(sql));
	if (shop->shop_classify == SHOP_TMALL_TYPE)
	{
		sprintf(sql, "replace into shop_info("
			         "shop_id,shop_name,shop_owner,shop_location,shop_type,"
					 "shop_firmname,shop_firmphone,shop_trade,shop_url,"
					 "shop_last_time) values(%ld,'%s','%s','%s',%d,'%s','%s','%s','%s','%s')", 
					  shop->shop_number, shop->shop_name, shop->shop_owner, shop->shop_location, 1, shop->tmall.shop_commany_name, 
					  shop->tmall.shop_telephone, shop->shop_trade_range, shop->shop_url, time_buf);
	}
	else
	{
		sprintf(sql, "replace into shop_info("
			         "shop_id,shop_name,shop_owner,shop_location,shop_type,"
					 "shop_setuptime,shop_goodrate,shop_credit,shop_trade,"
					 "shop_url,shop_last_time) values(%ld,'%s','%s','%s',%d,'%s','%s',%s,'%s','%s','%s')", 
					 shop->shop_number, shop->shop_name, shop->shop_owner, shop->shop_location, 0, shop->taobao.shop_create_time, 
					 shop->taobao.shop_popularity, shop->taobao.shop_credit, shop->shop_trade_range, shop->shop_url, time_buf);
	}

	ret = execute_no_query(mysql, sql);
	if (ret < 0)
	{
		write_log("db_cmd_shop:insert shop info failed\n");
	}
#else
	shop_show(shop);
#endif

	return ret;
}

/**
  * @function db_cmd_commodity
  * @brief ����Ʒ��Ϣ���������ݿ�
  * @param mysql  ���ݿ���
  * @param shop  ������Ϣ
  * @param commodity  ��Ʒ��Ϣ
  * @return 0  �ɹ�
  * @return -1  ʧ��
  */
int db_cmd_commodity(mysql_handle_t *mysql, shop_t *shop, commodity_t *commodity)
{
	int ret = 0;
#if !DBG
	char time_buf[32], sql[1024];
	time_t  now;
    struct tm *timenow = NULL;

	time(&now);
    timenow = localtime(&now);
    memset(time_buf, 0, sizeof(time_buf));
    sprintf(time_buf, "%d%.2d",timenow->tm_year + 1900, timenow->tm_mon + 1);

	if (mysql == NULL)
	{
		write_log("db_cmd_commodity:mysql handle is null\n");
		return -1;
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, 
		"insert into shop_commodity("
		"commodity_id,storage_date,shop_id,commodity_name,"
		"commodity_price,commodity_promo,commodity_sales,"
		"commodity_stock,commodity_comment,commodity_url,shop_type) values(%s,%s,%ld,'%s',%f,%f,%d,%d,%d,'%s',%d)", 
		 commodity->commodity_id, time_buf, commodity->shop_number, commodity->commodity_name, 
		 atof(commodity->commodity_price), atof(commodity->commodity_promo_price), 
		 atoi(commodity->commodity_sales), atoi(commodity->commodity_stock), 
		 atoi(commodity->commodity_comment), commodity->commodity_url, shop->shop_classify);

	if ((ret = execute_no_query(mysql, sql)) != 0)
	{
		memset(sql, 0, sizeof(sql));
	    sprintf(sql, 
		"update shop_commodity set shop_id=%ld, commodity_name='%s', commodity_price=%f, "
		"commodity_promo=%f, commodity_sales=%d, commodity_stock=%d, "
		"commodity_comment=%d, commodity_url='%s' where commodity_id=%s and storage_date=%s", 
		 commodity->shop_number, commodity->commodity_name, atof(commodity->commodity_price), 
		 atof(commodity->commodity_promo_price), atoi(commodity->commodity_sales), atoi(commodity->commodity_stock), 
		 atoi(commodity->commodity_comment), commodity->commodity_url, commodity->commodity_id, time_buf);

		 ret = execute_no_query(mysql, sql);
		 if (ret < 0)
	     {
			write_log("db_cmd_commodity:insert commodity info failed\n");
	     }
	}
#else
	shop_commodity(commodity);
#endif
	return ret;
}

/**
  * @function db_cmd_shop_seller_id
  * @brief �����̼�ID
  * @param mysql  ���ݿ���
  * @param shop  ������Ϣ
  * @return 0 �ɹ�
  * @return -1 ʧ��
  */
int db_cmd_shop_seller_id(mysql_handle_t *mysql, shop_t *shop)
{
	int  ret = 0;
#if !DBG
	char sql[256];

	if (mysql == NULL)
	{
		write_log("db_cmd_shop_seller_id:mysql handle is null\n");
		return -1;
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "update shop_info set shop_sellerid=%ld where shop_id=%ld", shop->shop_seller_id, shop->shop_number);
	ret = execute_no_query(mysql, sql);
	if (ret < 0)
	{
		write_log("db_cmd_shop_seller_id:update seller info failed\n");
	}
#endif

	return ret;
}

/**
  * @function db_cmd_add_error
  * @brief  �����ݿ��������ȡʧ�ܵĵ�����Ϣ
  * @param mysql   ���ݿ���
  * @param shop_id   ��ȡʧ�ܵ�SHOP ID
  * @param shop_url   ����URL
  * @param shop_name   ��������
  * @param shop_owner   ��������
  * @param shop_location   ����λ��
  * @return 0  д��ɹ�  
  * @return -1  д��ʧ��
  */
int db_cmd_add_error(mysql_handle_t *mysql, long shop_id, const char *shop_url, const char *shop_name, const char *shop_owner, const char *shop_location)
{
	int  ret = 0;
#if !DBG
	char sql[256];
	char time_buf[32];
	time_t  now;
    struct tm *timenow = NULL;

	if (mysql == NULL)
	{
		write_log("db_cmd_add_error:mysql handle is null\n");
		return -1;
	}

	time(&now);
    timenow = localtime(&now);
    memset(time_buf, 0, sizeof(time_buf));
    sprintf(time_buf, "%d-%.2d-%.2d %.2d:%.2d:%.2d",
		timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour,timenow->tm_min,timenow->tm_sec);

	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, 
		"insert into error_info(shop_id,shop_url,shop_name,shop_owner,shop_location,error_time) values(%ld, '%s','%s','%s','%s','%s')", 
         shop_id, shop_url, shop_name, shop_owner, shop_location, time_buf);
	ret = execute_no_query(mysql, sql);
    if (ret < 0)
	{
		write_log("db_cmd_add_error: insert error info failed\n");
	}
#endif

	return ret;
}

/**
  * @function db_cmd_get_error_info
  * @brief  �����ݿ��л�ȡδ����ȷ��ȡ��shop��ͬʱ������ÿһ��
               ��error_info����ȡ����shop�����Ὣ���error_info����ɾ������
               ��һ����ȡ��ʧ���ˣ���Ὣ��shop������⡣
  * @param mysql   ���ݿ���
  * @param shop_num   ��ȡ����shop id������
  * @param mysql   ���ݿ���
  * @return ret  ��ȡ���ĳ���shop�ĸ���
  */
int db_cmd_get_error_info(mysql_handle_t *mysql, long *shop_num, int max_count)
{
	int  i = 0, ret = 0;
	char sql[2048], select[128];
	char number[16];
	
	memset(sql, 0, sizeof(sql));
	strcpy(sql, "delete from error_info where error_id in (");

	memset(select, 0, sizeof(select));
	sprintf(select, "select error_id,shop_id from error_info limit %d", max_count);

	ret = execute_query(mysql, select);
	if (ret < 0)
	{
		write_log("db_cmd_get_error_info: get error info faile\n");
		return ret;
	}

    for (i = 0; i < ret; i++)
	{
		memset(number, 0, sizeof(number));
		sprintf(number, "%d,", get_int_value_by_index(mysql, i, 0));
		strcat(sql, number);/*��0������λ�ý�Ҫɾ����error_id��¼����*/

        /*��1������λ�ã���ȡ��shop id*/
		shop_num[i] = get_int_value_by_index(mysql, i, 1);
	}
	strcat(sql, "0)");
	free_query_result(mysql);
    /*ɾ���Ѿ���ȷ��ȡ����shop*/
	ret = ret > 0 ? execute_no_query(mysql, sql) : ret;
	if (ret < 0)
	{
		write_log("db_cmd_get_error_info: delete error info faile\n");
	}

	return ret;
}
