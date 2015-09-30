/*******************************************************************************
*[Filename]        commodity_parse.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 ����html�������룬���ڽ�����Ʒҳ����Ϣ
                 
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
  * @brief ��ʽ���ַ�����ɾ��\r  \n  " ' �Լ��ո�
  * @param str    ����ַ���
  * @return str    ����֮����ַ���
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
  * @brief ��json��ʽ��������ͨ����ֵ��ȡ���ݣ�ͨ������ֵ�Ƿ�Ϊ�գ���
               �ж��Ƿ��ҵ�Ŀ�����ݡ�
  * @param json    json��ʽ���ַ���
  * @param lbl    ��Ҫ���ҵı�ǩֵ���߼�ֵ
  * @param text    ���ҵ�������
  * @return NULL   δ���ҵ�Ŀ���ֵ
  * @return p1  �Ѳ��ҵ�Ŀ���ֵ��Ӧ������
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
  * @brief �����Ա���Ʒ����ҳ����Ϣ����Ҫ��Ϊ�˻�ȡ�ܹ��ж���ҳ����Ʒ
              �Լ���ǰ�ǵڼ�ҳ����Ʒ��Ϣ��Ϊ������ȡ�������Ʒ��Ϣʹ�á�
  * @param json    json��ʽ���ַ���
  * @param total_page    ��ȡ�ܹ��ж���ҳ����Ʒ��Ϣ
  * @param current_page    ��ȡ��ǰ��json��ʽ�����ǵڼ�ҳ����Ʒ��Ϣ
  * @param page_size   ÿһҳ�ж�����Ʒ��Ŀ
  * @return JSON_RET_ERR  ����ʧ��
  * @return JSON_RET_SUC  �����ɹ�
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
  * @brief ������ȡ��ǰ��Ʒ�Ŀ����Ϣ������Ϣ��Ҫ������Ʒ��ҳ���ܻ�ȡ����
              ��Ʒ��ҳ��URL��֮ǰ����json��ʽ����ʱ���Ѽ�¼��commodity->commodity_url���ˡ�
  * @param commodity    ��ȡ��Ʒ�����Ϣ
  * @return -1  ��ȡʧ��
  * @return 0  ��ȡ�ɹ�
  */
static int http_parse_stock_commodity(commodity_t *commodity)
{
	char *p1 = NULL, *p2 = NULL, *data = NULL;
	int mem_size = 512 * 1024;
	int ret = 0;
	char temp[1024];

	const char *key = "���", *key1 = ">", *key2 = "<";

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
  * @brief ����json��ʽ��Ʒ��Ϣ
  * @param mysql    ���ݿ���
  * @param shop    ������Ϣ
  * @param item_list    json��ʽ��item��������ṹ������python�е�����
  * @return -1  ����ʧ��
  * @return 0  �����ɹ�
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
		    /*��������Ʒ��Ϣ�еĵ��������������ҳ�еĵ������Ʋ�ͬ*/
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
        /*��ȡ��Ʒ��ҳ��Ϣ*/
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
  * @brief ��Ʒ��Ϣ������ں�������Ʒ��������Ҫ������
              �Ա��߼����������ڶ�̬����ʱJS�������һ��URL�����ص�
              json��ʽ����Ʒ�б�
              ֻ��Ҫ�ڸ�URL��ָ��λ�ü���������ƾͿɻ�ø�JSON��ʽ���ݡ�
  * @param mysql    ���ݿ���
  * @param shop    ������Ϣ
  * @return -1  ����ʧ��
  * @return 0  �����ɹ�
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
    /*ת��*/
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
        /*����һ���ϳɵ�URL��������ȡ��Ʒ�б�*/
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
        /*����JSONҳ�棬��ȡҳ�������Ϣ(�ܹ�����ҳ����ǰ�ڼ�ҳ
              ,ҳ��С��)
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
        /*����JSON��ʽ����Ʒ�б�*/
        http_parse_json_commodity(mysql, shop, strstr(data, "itemList"));

        if (total_page == current_page) 
		{
			break;
		}
		start_item = current_page * page_size;
	}

    /*��Ϣ���*/
	db_cmd_shop_seller_id(mysql, shop);
out:
	free(data);
	return ret;
}
