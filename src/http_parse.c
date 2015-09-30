/*******************************************************************************
*[Filename]        http_parse.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 ����html�������룬��������Ľ�������
                 
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
  * @brief HTML�������ƺ����������������ÿһ�ҵ�����Ϣ�Ĵ�����ԡ�
              �Ȼ�ȡ������Ϣ��Ȼ���ٽ���������Ʒ��Ϣ��
  * @param mysql    ���ݿ���
  * @param shop_num    ��ǰ��ȡ��shop id
  * @param data   ����html���ݵ��ڴ�
  * @param len    �ڴ泤��
  * @param repeat   �Ƿ��Ǹ�������
  * @return 0    �����ɹ�
  * @return -1    �����ɹ�(Ҳ������д��ʧ��)
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
		    /*����������ȡ������Ϣ�ĸ������棬�����ظ���¼*/
			break;
		}
        /*����ȡʧ�ܵ�SHOP ��¼����*/
		ret = db_cmd_add_error(mysql, shop.shop_number, shop.shop_url, shop.shop_name, shop.shop_owner, shop.shop_location);
		break;

	case RET_ERR_NO_RECORD: 
	default:
		break;
	}

	return ret;
}
