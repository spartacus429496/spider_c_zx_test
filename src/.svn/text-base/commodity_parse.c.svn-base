/*******************************************************************************
*[Filename]        commodity_parse.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 爬虫html解析代码，用于解析物品页面信息
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h> 
#include "spider.h"
#include "db_cmd.h"
#include "http_parse.h"
#include "http_req.h"
#include "common.h"
#include "shop_parse.h"
#include "commodity_parse.h"


/**
  * @function format_string
  * @brief 格式化字符串，删除\r  \n  " ' 以及空格
  * @param str    入参字符串
  * @return str    处理之后的字符串
  */
static char* format_string(char *str)
{
	while (strrep(str, "\r", ""));
	while (strrep(str, "\n", ""));
	while (strrep(str, "\"", ""));
	while (strrep(str, "\'", ""));
    strtrim(str);
	return str;
}

/**
  * @function jsonp_item_get
  * @brief 在json格式的数据中通过键值获取数据，通过返回值是否为空，来
               判断是否找到目标数据。
  * @param json    json格式的字符串
  * @param lbl    需要查找的标签值或者键值
  * @param text    查找到的数据
  * @return NULL   未查找到目标键值
  * @return p1  已查找到目标键值对应的数据
  */
static char* jsonp_item_get(const char *json, const char *lbl, char *text)
{
	char *p1 = NULL, *p2 = text;
	char tag[64];
	int count = 0;
	int cnt = 0;

	memset(tag, 0, sizeof(tag));
	sprintf(tag, "\"%s\":", lbl);

	p1 = strstr(json, tag);
	if (p1 == NULL)
	{
		return NULL;
	}
	p1 = p1 + strlen(tag);

	while (1)
	{
		if (*p1 == '{' && p1[-1] > 0) 
		{
		   count++;
		}
		else if (*p1 == '}')
		{
			count--;
		}
		else if (*p1 == ',')
		{
			if (count == 0) break;
		}
		else if (*p1 == '\0')
		{
			break;
		}

		if (++cnt > 250) break;
		*p2++ = *p1++;
	}
	return p1;
}

#define JSON_RET_SUC         0
#define JSON_RET_ERR        -1
#define JSON_RET_CON        -2

/**
  * @function http_parse_json_page
  * @brief 解析淘宝商品数据页面信息，主要是为了获取总共有多少页的商品
              以及当前是第几页的商品信息。为继续获取后面的商品信息使用。
  * @param json    json格式的字符串
  * @param total_page    获取总共有多少页的商品信息
  * @param current_page    获取当前的json格式数据是第几页的商品信息
  * @param page_size   每一页有多少商品条目
  * @return JSON_RET_ERR  解析失败
  * @return JSON_RET_SUC  解析成功
  */
static int http_parse_json_page(const char *json, int *total_page, int *current_page, int *page_size)
{
	char text[256], page[32];
	char *pt = NULL;

	if (json == NULL || total_page == NULL || current_page == NULL || page_size == NULL)
	{
		return JSON_RET_ERR;
	}
    
	*total_page = 0;
	*current_page = 0;
	*page_size = 0;
	memset(text, 0, sizeof(text));
    
	if (jsonp_item_get(json, "page", text) == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return JSON_RET_CON;
	}

	if (strstr(text, "null"))
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return JSON_RET_ERR;
	}

	memset(page, 0, sizeof(page));
    if (jsonp_item_get(text, "totalPage", page) == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return JSON_RET_CON;
	}
	format_string(page);
	*total_page = atoi(page);
	if (*total_page > MAX_PAGE_COUNT)
	{
		*total_page = MAX_PAGE_COUNT;
	}

	memset(page, 0, sizeof(page));
    if (jsonp_item_get(text, "currentPage", page) == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return JSON_RET_CON;
	}
	format_string(page);
	*current_page = atoi(page);


	memset(page, 0, sizeof(page));
    if (jsonp_item_get(text, "pageSize", page) == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return JSON_RET_CON;
	}
	format_string(page);
	*page_size = atoi(page);

	return JSON_RET_SUC;
}

/**
  * @function http_parse_json_page
  * @brief 解析获取当前商品的库存信息，该信息需要访问商品主页才能获取到。
              商品主页的URL在之前解析json格式数据时，已记录在commodity->commodity_url中了。
  * @param commodity    获取商品库存信息
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_stock_commodity(commodity_t *commodity)
{
	char *p1 = NULL, *p2 = NULL, *data = NULL;
	int mem_size = 512 * 1024;
	int ret = 0;
	char temp[1024];

	const char *key = "库存", *key1 = ">", *key2 = "<";

    data = (char*)malloc(mem_size);
	if (data == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}

	if (send_http_request(commodity->commodity_url, data, mem_size) <= 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}

	p1 = strstr(data, key);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key);
	p1 = strstr(p1, key1);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key1);
	p2 = strstr(p1, key2);
    if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	format_string(temp);
	strcpy(commodity->commodity_stock, temp);

out:
   free(data);
   return ret;
}

/**
  * @function http_parse_json_commodity
  * @brief 解析json格式商品信息
  * @param mysql    数据库句柄
  * @param shop    店铺信息
  * @param item_list    json格式的item链表，链表结构类似于python中的链表
  * @return -1  解析失败
  * @return 0  解析成功
  */
static int http_parse_json_commodity(mysql_handle_t *mysql, shop_t *shop, char *item_list)
{
	commodity_t commodity;
	char *ptemp = NULL;
	char value[256];

	if (item_list == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
    
	ptemp = item_list;
	while (ptemp)
	{
		memset(&commodity, 0, sizeof(commodity));
		commodity.shop_number = shop->shop_number;

		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "title", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_name, value);

		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "price", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_price, value);

		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "currentPrice", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_promo_price, value);

		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "tradeNum", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_sales, value);


		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "nick", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		if (strstr(value, shop->shop_owner) == NULL)
		{
		    /*若发现商品信息中的店主名称与店铺主页中的店主名称不同*/
			break;
		}

		if (shop->shop_seller_id == 0)
		{
			memset(value, 0, sizeof(value));
			ptemp = jsonp_item_get(ptemp, "sellerId", value);
			if (ptemp == NULL)
			{
				break;
			}
			format_string(value);
			shop->shop_seller_id = atol(value);
		}

		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "itemId", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_id, value);


		memset(value, 0, sizeof(value));
        /*获取商品主页信息*/
		ptemp = jsonp_item_get(ptemp, "href", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_url, value);


		memset(value, 0, sizeof(value));
		ptemp = jsonp_item_get(ptemp, "commend", value);
		if (ptemp == NULL)
		{
			break;
		}
		format_string(value);
		strcpy(commodity.commodity_comment, value);

		http_parse_stock_commodity(&commodity);

		db_cmd_commodity(mysql, shop, &commodity);
	}
	return 0;
}

/**
  * @function http_parse_commodity
  * @brief 商品信息解析入口函数。商品解析，主要是利用
              淘宝高级搜索功能在动态加载时JS所请求的一个URL所返回的
              json格式的商品列表。
              只需要在该URL的指定位置加入店主名称就可获得该JSON格式数据。
  * @param mysql    数据库句柄
  * @param shop    店铺信息
  * @return -1  解析失败
  * @return 0  解析成功
  */
int http_parse_commodity(mysql_handle_t *mysql, shop_t *shop)
{
	int ret = 0, mem_size = 512 * 1024;
	int start_item = 0, total_page = 0, current_page = 0, page_size = 0;
	int limit = 0;
	char http_req[256], nick[128];
	char *data = NULL;

	data = (char *)malloc(mem_size);
	if (data == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}

	memset(nick, 0, sizeof(nick));
    /*转码*/
	if (gbk2utf(shop->shop_owner, strlen(shop->shop_owner), nick, sizeof(nick)) < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}

	while (1)
	{
	    sleep(20);
		memset(http_req, 0, sizeof(http_req));
		sprintf(http_req, TAOBAO_SHOP_COMMODITY, nick, start_item);
        /*发送一个合成的URL请求来获取商品列表*/
		ret = send_http_request(http_req, data, mem_size);
		if (ret <= 0)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			break;
		}

		if (limit > 5 ) 
		{
			break;
		}
        /*解析JSON页面，获取页面控制信息(总共多少页，当前第几页
              ,页大小等)
             */
		ret = http_parse_json_page(data, &total_page, &current_page, &page_size);
		if (ret == JSON_RET_ERR)
		{
			break;
		}
		else if (ret == JSON_RET_CON)
		{
			limit++;
			continue;
		}

		debug("total=%d, current=%d, page=%d\n", total_page, current_page, page_size);
        /*解析JSON格式的商品列表*/
        http_parse_json_commodity(mysql, shop, strstr(data, "itemList"));

        if (total_page == current_page) 
		{
			break;
		}
		start_item = current_page * page_size;
	}

    /*信息入库*/
	db_cmd_shop_seller_id(mysql, shop);
out:
	free(data);
	return ret;
}
