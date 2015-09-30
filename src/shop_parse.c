/*******************************************************************************
*[Filename]        http_parse.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 爬虫html解析代码，用于解析各种版本的淘宝或者天猫店铺页面
                 
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
  * @function str_format
  * @brief 移除字符串中的'\r\n'  ' ' 和"&nbsp;"。
  * @param str    入参字符串
  * @return str    处理之后的字符串
  */
static char* str_format(char *str)
{
	while (strrep(str, "\r", ""));
	while (strrep(str, "\n", ""));
	while (strrep(str, "&nbsp;", ""));
	while (strrep(str, "年", "-"));
	while (strrep(str, "月", "-"));
	while (strrep(str, "日", ""));
    strtrim(str);
	return str;
}

/**
  * @function http_shop_loc
  * @brief 获取店铺位置信息，当店主详细信息页面中没有位置信息时需要
               通过该函数来获取地址。
  * @param shop    数据库shop表，其中有shop各字段。
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_shop_loc(shop_t *shop)
{
	char url[256],temp[128];
	int ret = 0, mem_size = 2 * 1024 * 1024;
	char *recv_data = NULL;

	char *p1 = NULL, *p2 = NULL;
	const char *key1 = "\"loc\":", *key2 = "\"", *key3 = ",", *key4 = "\"totalNumber\":";

	if (*(shop->shop_location) != 0) 
	{
		return 0;
	}

	memset(url, 0, sizeof(url));
	memset(temp, 0, sizeof(temp));
    /*将GBK编码转换为UTF编码*/
	if (gbk2utf(shop->shop_owner, strlen(shop->shop_owner), temp, sizeof(temp)) < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	sprintf(url, TAOBAO_SHOP_LOC, temp);

	recv_data = (char*)malloc(mem_size);
	if (recv_data == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(recv_data, 0, mem_size);

	ret = send_http_request(url, recv_data, mem_size);
	if (ret < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}


    // totalNumber
	p1 = strstr(recv_data, key4);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key4);
    p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	while (strrep(temp, "\"", ""));
	strtrim(temp);
	if (atoi(temp) <= 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}

	// loc
	p1 = strstr(p1, key1);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key1);
    p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	while (strrep(temp, "\"", ""));
	strtrim(temp);
	strcpy(shop->shop_location, temp);

out:
	free(recv_data);
	return ret;
}

/**
  * @function http_parse_shop_class
  * @brief 解析店铺类型，区分是淘宝店铺还是天猫店铺
  * @param data    html页面信息
  * @param shop_class    带出店铺类型
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_class(const char *data, int *shop_class)
{
	char *p1 = NULL, *p2 = NULL;

	const char *prefix = "<title>", *postfix = "</title>";
	const char *key2 = "Tmall.com", *key3 = "天猫Tmall.com", *key4 = "strip-header-shop";
	unsigned char buf[512];

	// default show class is taobao shop
	*shop_class = SHOP_TAOBAO_TYPE;

	p1 = strstr(data, prefix);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	p1 = p1 + strlen(prefix);

	p2 = strstr(p1, postfix);
	if (p2 == NULL) 
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	strncpy(buf, p1, p2 - p1);

	// 2. check "tmall.com" key, classify shopper
	if (strstr(buf, key2) || strstr(data, key3) || strstr(data, key4))
	{
		*shop_class = SHOP_TMALL_TYPE;
	}
	else
	{
		*shop_class = SHOP_TAOBAO_TYPE;
	}

	return 0;
}

/**
  * @function http_parse_shop_tmall_name
  * @brief  解析天猫店铺信息
  * @param data    html页面信息
  * @param shop_class    带出店铺类型
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_tmall_name(const char *data, char *shop_name, char *detail_url, char *local)
{
	char *p1 = NULL, *p2 = NULL;
	const char *key = "<a class=\"slogo-shopname\"", *key1 = "<strong>", *key2 = "</strong>";
	const char *key3 = "id=\"dsr-ratelink\"", *key4 = "value=\"", *key5 ="\"/>";
	const char *key6 = "所 在 地：", *key7 = "<div class=\"right\">", *key8 = "</div>";

	char  temp[1024];

	p1 = strstr(data, key);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
    /*parse shop name*/
	p1 = p1 + strlen(key);
	p1 = strstr(p1, key1);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	p1 = p1 + strlen(key1);

	p2 = strstr(p1, key2);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(shop_name, temp);

    /*parse detail shop url*/
	p1 = strstr(data, key3);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
    p1 = strstr(p1, key4);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	p1 = p1 + strlen(key4);

	p2 = strstr(p1, key5);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);	
	strcpy(detail_url, temp);

	//
	// 所 在 地
	//
    p1 = p2 + strlen(key5);
	p1 = strstr(p1, key6);
	if (p1 != NULL)
	{
		p1 = p1 + strlen(key6);
		p1 = strstr(p1, key7);
		if (p1 == NULL)
		{
			return 0;
		}
		p1 = p1 + strlen(key7);
		p2 = strstr(p1, key8);
		if (p2 == NULL)
		{
			return 0;
		}

		memset(temp, 0, sizeof(temp));
		strncpy(temp, p1, p2 - p1);
		str_format(temp);
		strcpy(local, temp);
	}
	
	return 0;
}

/**
  * @function http_parse_shop_tmall_detail
  * @brief  解析天猫店铺详细信息
  * @param shop    存放天猫店铺的详细信息
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_tmall_detail(shop_t *shop)
{
	char *recv_data = NULL;
	int ret = 0, len = 0, mem_size = 2 * 1024 * 1024;

	char *p1 = NULL, *p2 = NULL;
	const char *key = "卖家信息", *key1 = "<div class=\"title\">", *key2 = ">", *key3 = "<";
	const char *key4 = "公司名称：", *key5 = "<div class=\"fleft2\">";
	const char *key6 = "当前主营：";
	const char *key7 = "所在地区：";
	const char *key8 = "服务电话：";
    int len_cut = 0;
	char temp[1024];

	recv_data = (char*)malloc(mem_size);
	if (recv_data == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(recv_data, 0, mem_size);

    /*发送获取店铺详细信息的URL请求*/
	len = send_http_request(shop->shop_detail_url, recv_data, mem_size);
	if (len < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
    
	//
	// 1. 掌柜名称
	//
	p1 = strstr(recv_data, key);
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
    p1 = strstr(p1, key2);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key2);

	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	len_cut = (int)(p2 - p1);
	if(len_cut > 1023)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strncpy(temp, p1, len_cut);
	str_format(temp);
	debug("http_parse_shop_tmall_detail shop_owner %s\n", temp);
	if(strlen(temp) > 31)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strcpy(shop->shop_owner, temp);

	//
	// 2. 公司名称
	//
	p2 = p2 + strlen(key3);
	p1 = strstr(p2, key4);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key4);
	p1 = strstr(p1, key5);

	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key5);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	len_cut = (int)(p2 - p1);
	if(len_cut > 1023)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strncpy(temp, p1, len_cut);
	str_format(temp);
	debug("http_parse_shop_tmall_detail shop_commany_name %s\n", temp);
	if(strlen(temp) > 127)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strcpy(shop->tmall.shop_commany_name, temp);

	//
	// 3. 行业
	//
	p2 = p2 + strlen(key3);
    p1 = strstr(p2, key6);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key6);
	p1 = strstr(p1, key2);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key2);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	len_cut = (int)(p2 - p1);
	if(len_cut > 1023)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strncpy(temp, p1, len_cut);
	str_format(temp);
	debug("http_parse_shop_tmall_detail shop_trade_range %s\n", temp);
	if(strlen(temp) > 31)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strcpy(shop->shop_trade_range, temp);

	//
	// 4. 所在地区
	//
	p2 = p2 + strlen(key3);
    p1 = strstr(p2, key7);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key7);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	len_cut = (int)(p2 - p1);
	if(len_cut > 1023)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strncpy(temp, p1, len_cut);
	str_format(temp);
	debug("http_parse_shop_tmall_detail shop_location %s\n", temp);
	if(strlen(temp) > 127)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
    }
	strcpy(shop->shop_location, temp);

	//
	// 5. 电话
	//
	p2 = p2 + strlen(key3);
	p1 = strstr(p2, key8);
	if (p1)
	{
		p1 = p1 + strlen(key8);
		p2 = strstr(p1, key3);
		if (p2 == NULL)
		{
		    debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		memset(temp, 0, sizeof(temp));
		len_cut = (int)(p2 - p1);
		if(len_cut > 1023)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		strncpy(temp, p1, len_cut);
	    str_format(temp);
		debug("http_parse_shop_tmall_detail shop_telephone %s\n", temp);
		if(strlen(temp) > 31)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
	    strcpy(shop->tmall.shop_telephone, temp);
	}

out:
	free(recv_data);
	return ret;
}

/**
  * @function http_parse_shop_trip_name
  * @brief  解析在html页面中中包含strip-header-shop 的天猫店铺
  * @param data   需要解析的html数据
  * @param shop_name   解析得到的店铺信息
  * @param detail_url   店铺详细信息URL
  * @param local   店铺位置信息
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_trip_name(const char *data, char *shop_name, char *detail_url, char *local)
{
	char *p1 = NULL, *p2 = NULL;
	const char *key = "掌　　柜：";
	const char *key1 = "href=\"";
	const char *key2 = "\">", *key3 = "<";
	const char *key6 = "所 在 地：", *key7 = "</li>";

	char  temp[1024];

	// 
	// detail_url
	//
	p1 = strstr(data, key);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}

	p1 = p1 + strlen(key);
	p1 = strstr(p1, key1);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	p1 = p1 + strlen(key1);

	p2 = strstr(p1, key2);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(detail_url, temp);


	//
	// 获取位置信息
	//
	p1 = p2 + strlen(key2);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(shop_name, temp);

	//
	// 所 在 地
	//
	p1 = p2 + strlen(key3);
	p1 = strstr(p1, key6);
	if (p1 != NULL)
	{
		p1 = p1 + strlen(key6);
		p2 = strstr(p1, key7);
		if (p2 == NULL)
		{
			return 0;
		}

		memset(temp, 0, sizeof(temp));
		strncpy(temp, p1, p2 - p1);
		str_format(temp);
		strcpy(local, temp);
	}

	return 0;
}

/**
  * @function http_parse_shop_trip_detail
  * @brief  解析trip天猫店铺的详细信息
  * @param shop   存放解析好的店铺信息
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_trip_detail(shop_t *shop)
{
	char *recv_data = NULL;
	int ret = 0, len = 0, mem_size = 2 * 1024 * 1024;

	char *p1 = NULL, *p2 = NULL;
	const char *key = "卖家信息", *key1 = "<div class=\"title\">", *key2 = ">", *key3 = "<";
	const char *key4 = "公司名称：", *key5 = "<div class=\"fleft2\">";
	const char *key6 = "当前主营：";
	const char *key7 = "所在地区：";
	const char *key8 = "服务电话：";

	char temp[1024];

	recv_data = (char*)malloc(mem_size);
	if (recv_data == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	memset(recv_data, 0, mem_size);

	len = send_http_request(shop->shop_detail_url, recv_data, mem_size);
	if (len < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
    
	//
	// 1. 掌柜名称
	//
	p1 = strstr(recv_data, key);
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
    p1 = strstr(p1, key2);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key2);

	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(shop->shop_owner, temp);

	//
	// 2. 公司名称
	//
	p2 = p2 + strlen(key3);
	p1 = strstr(p2, key4);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key4);
	p1 = strstr(p1, key5);

	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key5);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(shop->tmall.shop_commany_name, temp);

	//
	// 3. 行业
	//
	p2 = p2 + strlen(key3);
    p1 = strstr(p2, key6);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key6);
	p1 = strstr(p1, key2);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key2);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(shop->shop_trade_range, temp);

	//
	// 4. 所在地区
	//
	p2 = p2 + strlen(key3);
    p1 = strstr(p2, key7);
	if (p1 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	p1 = p1 + strlen(key7);
	p2 = strstr(p1, key3);
	if (p2 == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
	memset(temp, 0, sizeof(temp));
	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	strcpy(shop->shop_location, temp);

	//
	// 5. 电话
	//
	p2 = p2 + strlen(key3);
	p1 = strstr(p2, key8);
	if (p1)
	{
		p1 = p1 + strlen(key8);
		p2 = strstr(p1, key3);
		if (p2 == NULL)
		{
		    debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		memset(temp, 0, sizeof(temp));
		strncpy(temp, p1, p2 - p1);
	    str_format(temp);
	    strcpy(shop->tmall.shop_telephone, temp);
	}

out:
	free(recv_data);
	return ret;
}

/**
  * @function http_parse_shop_tmall
  * @brief  解析trip天猫店铺，解析入口函数
  * @param data   html数据
  * @param len  数据长度
  * @param shop   获取到的店铺信息
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_tmall(const char *data, int len, shop_t *shop)
{
	int ret = 0;
	const char *key4 = "strip-header-shop";

	if (strstr(data, key4) == NULL)
	{
	    /*解析店铺基本信息*/
		ret = http_parse_shop_tmall_name(data, shop->shop_name, shop->shop_detail_url, shop->shop_location);
		if (ret < 0)
		{
			return RET_ERR_RECORD;
		}

		if (*(shop->shop_location) != 0 && strstr(shop->shop_location, FLT_CITY) == NULL)
		{
			return RET_ERR_NO_RECORD;
		}
	    /*解析店铺详细信息*/
		ret = http_parse_shop_tmall_detail(shop);
		if (ret < 0)
		{
			ret = http_parse_shop_tmall_detail(shop);
			if (ret < 0)
			{
			    return RET_ERR_RECORD;
			}
		}
	}
	else
	{
        /*解析店铺基本信息*/
		ret = http_parse_shop_trip_name(data, shop->shop_name, shop->shop_detail_url, shop->shop_location);
		if (ret < 0)
		{
			return RET_ERR_RECORD;
		}

		if (*(shop->shop_location) != 0 && strstr(shop->shop_location, FLT_CITY) == NULL)
		{
			return RET_ERR_NO_RECORD;
		}
	    /*解析店铺详细信息*/
		ret = http_parse_shop_trip_detail(shop);
		if (ret < 0)
		{
			ret = http_parse_shop_trip_detail(shop);
			if (ret < 0)
			{
			    return RET_ERR_RECORD;
			}
		}
	}

	return RET_ERR_SUCCESS;
}

/**
  * @function http_parse_shop_taobao_pro_base
  * @brief  淘宝 店铺信息解析函数，截至目前(2014-12)淘宝店铺的店铺信息
               去掉了之前的很多分类，现在只有基础版和专业版，这两种版本
               的解析过程大致相同，没有太大差异。
  * @param data   html数据
  * @param len  数据长度
  * @param shop   获取到的店铺信息
  * @return -1  获取失败
  * @return 0  获取成功
  */
static int http_parse_shop_taobao_pro_base(const char *data, int len, shop_t *shop)
{
	char *p1 = NULL, *p2 = NULL;
	const char *key = "<span class=\"shop-name\">", *key1 = "\">", *key2 = "<";
    const char *key3 = "<span class=\"shop-rank\">", *key4 = "href=\"";
    char temp[1024];
//     char test1[100];
//     char test2[100];
    int i = 0;
    int len_cut = 0;
    const char* key6 = "<a class=\"shop-name \" href=";


    /*step1: parse shop name*/
    debug("parse form\n");
    p1 = strstr(data, key);
    if(p1 == NULL)
    {
        goto PARSE_FORM1;
    }
	p1 += strlen(key);

	p1 = strstr(p1, key1);
	if(p1 == NULL)
	{  
	     debug("%s line @%d\n", __func__, __LINE__);
             return -1;
	}
	p1 += strlen(key1);

	p2 = strstr(p1, key2);
	if(p2 == NULL)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
    }

	memset(temp, 0, sizeof(temp));
    len_cut = (int)(p2 -p1);
    if(len_cut >= 1023)
    {
        debug("%s line @%d\n", __func__, __LINE__);
        return -1;
    }
	strncpy(temp, p1, len_cut);
	str_format(temp);
    if(strlen(temp) > 63)
    {
        debug("parse shop name failed %s line @%d\n", __func__, __LINE__);
        return -1;
    }
	strcpy(shop->shop_name, temp);

	/*step2: parse shop detail url*/
	p1 = strstr(p1, key3);
	if(!p1)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
    }
    p1 += strlen(key3);
	p1 = strstr(p1, key4);
	if(!p1)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
    }
	p1 += strlen(key4);
    
	memset(temp, 0, sizeof(temp));
	p2 = temp;
	i = 0;
	while((p1[i] != '\"')&&(i<1023))
	{
	    p2[i] = p1[i];
	    i++;
	}
//	strncpy(temp, p1, p2 - p1);
	str_format(temp);
    if(strlen(temp) > 127)
    {
		debug("%s line @%d\n", __func__, __LINE__);
        return -1;
    }
	strcpy(shop->shop_detail_url, temp);
       
    return 0;

PARSE_FORM1:
    /*step1: parse shop name*/
    debug("parse form1\n");
    p1 = strstr(data, key6);
    if(p1 == NULL)
    {
        debug("%s line @%d\n", __func__, __LINE__);
        return -1;
    }
    p1 += strlen(key6);

    p1 = strstr(data, key1);
    if(p1 == NULL)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
    }
    p1 += strlen(key1);

    p2 = strstr(p1, key2);
    memset(temp, 0, sizeof(temp));
	len_cut = (int)(p2 -p1);
	if(len_cut >= 1023)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
	strncpy(temp, p1, len_cut);
    str_format(temp);
	if(strlen(temp) > 63)
	{
        debug("parse shop name failed %s line @%d\n", __func__, __LINE__);
	    return -1;
	}
	strcpy(shop->shop_name, temp);
	p2 += strlen(key2);
	p1 = p2;
      
    /*step2: parse shop detail url*/
	p1 = strstr(p1, key4);
	if(!p1)
    {
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
    }
	p1 += strlen(key4);
    
	memset(temp, 0, sizeof(temp));
	p2 = temp;
	i = 0;
	while((p1[i] != '\"')&&(i<1023))
	{
	    p2[i] = p1[i];
	    i++;
	}
	//	strncpy(temp, p1, p2 - p1);
	str_format(temp);
	if(strlen(temp) > 127)
    {
		debug("%s line @%d\n", __func__, __LINE__);
        return -1;
    }
	strcpy(shop->shop_detail_url, temp);

	return 0;
}

/**
  * @function http_parse_shop_taobao_pro_base_detail
  * @brief  解析淘宝店铺详细信息
  * @param shop   获取到的店铺信息
  * @return -1  解析失败
  * @return 0  解析成功
  */
static int http_parse_shop_taobao_pro_base_detail(shop_t *shop)
{
	char *recv_data = NULL;
	int ret = 0, len = 0, mem_size = 2 * 1024 * 1024;
    int i = 0;
	char *p1 = NULL, *p2 = NULL;
	const char *key = "卖家信息", *key1 = "当前主营：", *key2 = "所在地区：", *key3 = "卖家信用：";
	const char *key4 = ">", *key5 = "</", *key6 = "<";
	const char *key7 = "个人信息", *key8 = "<dd>";
	const char *key9 = "<div class=\"title\">";
	const char *key11 = "好评率：", *key12 = "卖家信用评价展示 ";
	const char *key13 = "<input type=\"hidden\" name=\"shopStartDate\" id=\"J_showShopStartDate\" value=\"";
	char temp[1024];

	recv_data = (char*)malloc(mem_size);
	if (recv_data == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return -1;
	}
         memset(recv_data, 0, mem_size);

      debug("req detail url %s\n", shop->shop_detail_url);
	len = send_http_request(shop->shop_detail_url, recv_data, mem_size);
	if (len < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		ret = -1;
		goto out;
	}
    
	/*解析店铺创建时间*/
	p1 = strstr(recv_data, key13);
    if(p1 != NULL)
    {
        debug("parse START TIME\n");
        p1 += strlen(key13);
        i = 0;
        p2 = temp;
		while((p1[i] != '\"')&&(i < 1023))
	    {
	        p2[i] = p1[i];
			i++;
	    }	
		str_format(temp);
		if(strlen(temp) > 31)
		{
		    goto PARSE_POP;
		}
		strcpy(shop->taobao.shop_create_time, temp);
    }
	
	/*解析好评率*/
PARSE_POP:	
	p1 = strstr(recv_data, key12);
	if (p1 != NULL)
	{	    
        debug("parse POP\n");
	    p1 = p1 + strlen(key12);
        p1= strstr(p1, key11);

		if(p1 != NULL)
		{
		p1 += strlen(key11);
		p2 = strstr(p1, key5);
		if (p2 == NULL)
		{
		    debug("%s line @%d\n", __func__, __LINE__);
		    return -1;
		}
		memset(temp, 0, sizeof(temp));
		len = 0;
		len = (p2 - p1);
		if(len > 1023)
		{
		    strcpy(shop->taobao.shop_popularity, "-0%");
		    goto PARSE_OWNER;
		}
		strncpy(temp, p1, len);
		str_format(temp);
		if(strlen(temp) > 31)
		{
		    strcpy(shop->taobao.shop_popularity, "-0%");
		    goto PARSE_OWNER;
		}
		strcpy(shop->taobao.shop_popularity, temp);
		}
		else
		{
			strcpy(shop->taobao.shop_popularity, "-0%");
			goto PARSE_OWNER;
		}
	}
	else
	{
		strcpy(shop->taobao.shop_popularity, "-0%");
		goto PARSE_OWNER;
	}
    
	/*解析店主名称*/
PARSE_OWNER:
	p1 = strstr(recv_data, key);
	if (p1 != NULL)
	{
	    debug("parse SHOP OWNER\n");
		p1 = p1 + strlen(key);
        /*step1: parse shop owner*/
		p1 = strstr(p1, key9);
		if(!p1)
	    {
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
	    }
		p1 += strlen(key9);
		p1 = strstr(p1, key4);
		if(!p1)
	    {
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
	    }
		p1 += strlen(key4);

		p2 = strstr(p1, key5);
		if(!p2)
	    {
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
	    }
		memset(temp, 0, sizeof(temp));
        len = 0;
        len = (p2 - p1);
        if(len > 1023)
        {
            debug("%s line @%d\n", __func__, __LINE__);
            ret = -1;
			goto out;
        }
        strncpy(temp, p1, len);
		str_format(temp);
        if(strlen(temp) > 31)
        {
             debug("%s line @%d\n", __func__, __LINE__);
             ret = -1;
             goto out;
        }
		strcpy(shop->shop_owner, temp);
        
        /*解析当前主营*/       
		p1 = strstr(p1, key1);
		if (p1 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		p1 = p1 + strlen(key1);
		p1 = strstr(p1, key4);
		if (p1 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		p1 = p1 + strlen(key4);
		p2 = strstr(p1, key5);
		if (p2 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		memset(temp, 0, sizeof(temp));
        len = 0;
        len = (p2 - p1);
        if(len > 1023)
        {
             debug("%s line @%d\n", __func__, __LINE__);
             ret = -1;
             goto out;
        }
		strncpy(temp, p1, len);
		str_format(temp);
        if(strlen(temp) > 31)
        {
             write_log("%s line @%d\n", __func__, __LINE__);
             ret = 0;
             strcpy(shop->shop_trade_range, "NULL");
             goto PARSE_LOCATION;
        }
		strcpy(shop->shop_trade_range, temp);
        p1 = p1 + strlen(key5);

        /*解析所在地区*/
PARSE_LOCATION:
		if(p1 == NULL)
        {
           p1 = recv_data;
        }      
		p1 = strstr(p1, key2);
		if (p1 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		p1 = p1 + strlen(key2);
		p2 = strstr(p1, key5);
		if (p2 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		memset(temp, 0, sizeof(temp));
		len = 0;
		len = (p2 - p1);
		if(len > 1023)
		{
		    debug("%s line @%d\n", __func__, __LINE__);
		    ret = -1;
	        goto out;
		}
		strncpy(temp, p1, len);
		str_format(temp);
		if(strlen(temp) > 31)
		{
	        write_log("parse location failed %s line @%d\n", __func__, __LINE__);
	        ret = -1;
	        goto out;
		}
		strcpy(shop->shop_location, temp);
        p1 = p1 + strlen(key5);

        /*解析卖家信用*/
//PARSE_CREDIT:
		if(p1 == NULL)
		{
		    p1 = recv_data;
		}
		p1 = strstr(p1, key3);
		if (p1 == NULL)
		{
			strcpy(shop->taobao.shop_credit, "0");
			ret = 0;
			goto out;
		}
		p1 = p1 + strlen(key3);
		p2 = strstr(p1, key6);
		if (p2 == NULL)
		{
			strcpy(shop->taobao.shop_credit, "0");
			write_log("%s line @%d\n", __func__, __LINE__);
			ret = 0;
			goto out;
		}
		memset(temp, 0, sizeof(temp));
		len = 0;
		len = (p2 - p1);
		if(len > 1023)
		{
		   strcpy(shop->taobao.shop_credit, "0");
		   write_log("%s line @%d\n", __func__, __LINE__);
		   ret = 0;
		   goto out;
		}
		strncpy(temp, p1, len);
		str_format(temp);
		if(strlen(temp) > 15)
		{
			strcpy(shop->taobao.shop_credit, "0");
			write_log("%s line @%d\n", __func__, __LINE__);
			ret = 0;
			goto out;
		}
		strcpy(shop->taobao.shop_credit, temp);
	}
	else
	{
		p1 = strstr(recv_data, key7);
		if (p1 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
        
		//
		// 当前主营
		//
		strcpy(shop->shop_trade_range, "Not filled");

		//
		// 卖家信用
		//
		strcpy(shop->taobao.shop_credit, "0");

		//
		// 所在地区
		//
		p1 = p1 + strlen(key7);
		p1 = strstr(p1, key2);
		if (p1 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		p1 = p1 + strlen(key2);
		p1 = strstr(p1, key8);
		if (p1 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		p1 = p1 + strlen(key8);
		p2 = strstr(p1, key5);
		if (p2 == NULL)
		{
			debug("%s line @%d\n", __func__, __LINE__);
			ret = -1;
			goto out;
		}
		memset(temp, 0, sizeof(temp));
		strncpy(temp, p1, p2 - p1);
		str_format(temp);
		strcpy(shop->shop_location, temp);
	}

out:
	free(recv_data);
	return ret;
}

/**
  * @function http_parse_shop_taobao
  * @brief  淘宝店铺解析入口函数
  * @param data   html数据
  * @param len   数据长度
  * @param shop   获取到的店铺信息
  * @return RET_ERR_RECORD  解析失败
  * @return 0  解析成功
  */
static int http_parse_shop_taobao(const char *data, int len, shop_t *shop)
{
#if 0
	const char *key = " <div id=\"header\" class=\"tshop-psm-shop-header2\">", 
		       *key1 = "<div class=\"shop-info-simple\">", 
			   *key2 = "<div id=\"shop-info\" class=\"shop-intro hslice\">";
	int ret = 0;

	if (strstr(data, key))
	{
#endif	
		int ret = 0;
#if DBG
        printf("parse shop base\n");
#endif
		ret = http_parse_shop_taobao_pro_base(data, len, shop);
		if (ret < 0 )
		{
			return RET_ERR_RECORD;
		}

		if (*(shop->shop_location) != 0 && strstr(shop->shop_location, FLT_CITY) == NULL)
		{
			return RET_ERR_NO_RECORD;
		}

		debug("parse shop detail\n");
	
        ret = http_parse_shop_taobao_pro_base_detail(shop);
		if (ret == RET_ERR_RECORD)
		{
			ret = http_parse_shop_taobao_pro_base_detail(shop);
		}
#if 0		
	}
	else if (strstr(data, key1))
	{
		ret = http_parse_shop_taobao_std_exp_sup(data, len, shop);
		if (ret < 0 )
		{
			return RET_ERR_RECORD;
		}

		ret = http_parse_shop_taobao_std_exp_sup_detail(shop);
		if (ret == RET_ERR_RECORD)
		{
			ret = http_parse_shop_taobao_std_exp_sup_detail(shop);
		}
	}
	else if (strstr(data, key2))
	{
		ret = http_parse_shop_taobao_other(data, len, shop);
		if (ret < 0 )
		{
			return RET_ERR_RECORD;
		}
		ret = http_parse_shop_taobao_other_detail(shop);
		if (ret == RET_ERR_RECORD)
		{
			ret = http_parse_shop_taobao_other_detail(shop);
		}
	}
	else 
	{
		printf("%s line @%d\n", __func__, __LINE__);
		ret = RET_ERR_RECORD;
	}
#endif	
	return ret;
}

/**
  * @function http_parse_shop
  * @brief  店铺信息解析入口函数，根据店铺类型进行解析，并入库
  * @param data   html数据
  * @param len   数据长度
  * @param shop   获取到的店铺信息
  * @return RET_ERR_RECORD  解析失败
  * @return 0  解析成功
  */
int http_parse_shop(mysql_handle_t *mysql, const char *data, int len, shop_t *shop)
{
	int ret = 0;

	if (data == NULL || shop == NULL)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return RET_ERR_NO_RECORD;
	}
    /*进行店铺分类*/
	if (http_parse_shop_class(data, &(shop->shop_classify)) < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return RET_ERR_RECORD;
	}
    
	if (shop->shop_classify == SHOP_TMALL_TYPE)
	{
	    /*解析天猫店铺*/
		ret = http_parse_shop_tmall(data, len, shop);
	}
	else
	{
	    /*解析淘宝店铺*/
		ret = http_parse_shop_taobao(data, len, shop);
	}

	if (ret == RET_ERR_NO_RECORD)
	{
		return RET_ERR_NO_RECORD;
	}

	if (ret == RET_ERR_RECORD || (*(shop->shop_name) == 0 || *(shop->shop_owner) == 0))
	{
		debug("ret = %d, %s line @%d\n", ret, __func__, __LINE__);
		return RET_ERR_RECORD;
	}

    /*
          用来获取店铺地址，因为有些店铺在店主详细信息的页面中
          没有填写店铺地址。该函数获取的地址就是商品信息中的发货地址。
      */
	if (http_shop_loc(shop) < 0)
	{
		http_shop_loc(shop);
	}

	debug("shop_num=%ld, local=%s, city=%s\n", shop->shop_number, shop->shop_location, FLT_CITY);

	if (strstr(shop->shop_location, FLT_CITY) == NULL)
	{
	    /*非合肥地区的卖家*/
		return RET_ERR_NO_RECORD;
	}

    /*店铺信息入库*/
	if (db_cmd_shop(mysql, shop) < 0)
	{
		debug("%s line @%d\n", __func__, __LINE__);
		return RET_ERR_RECORD;
	}

	return RET_ERR_SUCCESS;
}
