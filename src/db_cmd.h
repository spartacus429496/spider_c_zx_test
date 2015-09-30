#ifndef __DB_CMD_H__
#define __DB_CMD_H__

#include "mysql_esp.h"

typedef struct shop
{
	long  shop_number;
	long  shop_seller_id;
	int   shop_classify;        // 1 means tianmao shop; 0 means taobao shop
	int   shop_reserver;
	char  shop_name[64];
	char  shop_owner[32];
	char  shop_url[64];
	char  shop_detail_url[128];
	char  shop_location[32];
	char  shop_trade_range[32];
	
	union
	{
		struct 
		{
			char  shop_commany_name[128];
			char  shop_telephone[32];
		}tmall;

		struct 
		{
			char  shop_create_time[32];
			char  shop_popularity[32];
			char  shop_credit[16];
		}taobao;
	};
}shop_t;

typedef struct commodity
{
	long  shop_number;
	char  commodity_name[256];
	char  commodity_id[32];
	char  commodity_url[128];
	char  commodity_price[16];
	char  commodity_promo_price[16]; 
    char  commodity_sales[16];
	char  commodity_stock[16];
	char  commodity_comment[16];
}commodity_t;

void db_param_set(const char *host, const char *name, const char *user, const char *passwd, const int port);

mysql_handle_t* connect_mysql();
void free_mysql(mysql_handle_t *mysql);

int db_cmd_shop(mysql_handle_t *mysql, shop_t *shop);
int db_cmd_commodity(mysql_handle_t *mysql, shop_t *shop, commodity_t *commodity);
int db_cmd_shop_seller_id(mysql_handle_t *mysql, shop_t *shop);
int db_cmd_add_error(mysql_handle_t *mysql,long shop_id, const char *shop_url, const char *shop_name, const char *shop_owner, const char *shop_location);

int db_cmd_get_error_info(mysql_handle_t *mysql, long *shop_num, int max_count);
#endif