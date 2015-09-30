/*******************************************************************************
*[Filename]        http_parse.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 爬虫html解析代码，控制爬虫的解析流程
                 
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
  * @function http_parse_process
  * @brief HTML解析控制函数，控制着爬虫对每一家店铺信息的处理策略。
              先获取网店信息，然后再解析店铺商品信息。
  * @param mysql    数据库句柄
  * @param shop_num    当前爬取的shop id
  * @param data   接收html数据的内存
  * @param len    内存长度
  * @param repeat   是否是辅助爬虫
  * @return 0    解析成功
  * @return -1    解析成功(也可能是写库失败)
  */
int http_parse_process(mysql_handle_t *mysql, long shop_num, const char *data, int len, int repeat)
{
	shop_t  shop;
	int ret = 0;

	memset(&shop, 0, sizeof(shop));
	shop.shop_number = shop_num;
	sprintf(shop.shop_url, TAOBAO_SHOP_URL, shop_num);

	ret = http_parse_shop(mysql, data, len, &shop);
	debug("ret = %d\n", ret);

	switch(ret)
	{
	case RET_ERR_SUCCESS:
		http_parse_commodity(mysql, &shop); 
		break;

	case RET_ERR_RECORD:
		if (repeat) 
		{
		    /*如果是针对爬取错误信息的辅助爬虫，则不再重复记录*/
			break;
		}
        /*将爬取失败的SHOP 记录下来*/
		ret = db_cmd_add_error(mysql, shop.shop_number, shop.shop_url, shop.shop_name, shop.shop_owner, shop.shop_location);
		break;

	case RET_ERR_NO_RECORD: 
	default:
		break;
	}

	return ret;
}
