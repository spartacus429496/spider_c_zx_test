/*******************************************************************************
*[Filename]        http_req.c
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 爬虫数据请求代码，主要用来发送http 请求，发起网页访问
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#include <zconf.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <math.h>
#include "spider.h"
#include "http_req.h"
#include "common.h"

#define REQ_FAILE    -1
#define REQ_REPLAY   -2
#define REQ_REDIRECT -3

/*数据接收结构体，用来接收HTTP请求所返回的html网页信息*/
typedef struct http_param
{
	char  recv_data[HTTP_RECV_BUFF_SIZE];  /*消息接收缓存*/
	int   recv_len;                        /*消息接收长度*/
}http_param_t;

/**
  * @function http_gunzip
  * @brief gzip解压函数
  * @param src    需要进行解压的数据包
  * @param src_size    数据包长度
  * @param dst    存放解压数据的缓冲区
  * @param dst_size   缓冲区长度
  * @return REQ_FAILE    解压失败
  * @return d_stream.total_out   解压后的长度
  */
static int http_gunzip(char *src, int src_size, char *dst, int dst_size)
{
	int err = 0, ret = dst_size;
    z_stream d_stream = {0};
    static char dummy_head[2] = 
    {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };

	if (src_size <= 0)
	{
		return REQ_FAILE;
	}

    d_stream.zalloc   = (alloc_func)0;
    d_stream.zfree    = (free_func)0;
    d_stream.opaque   = (voidpf)0;
    d_stream.next_in  = src;
    d_stream.avail_in = 0;
    d_stream.next_out = dst;
    if(inflateInit2(&d_stream, 47) != Z_OK)
	{
		return REQ_FAILE;
	}

    while (d_stream.total_out < ret && d_stream.total_in < src_size) 
	{
        d_stream.avail_in = d_stream.avail_out = 1;
        if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) 
		{
			break;
		}
        if(err != Z_OK )
        {
            if(err == Z_DATA_ERROR)
            {
                d_stream.next_in = (char*) dummy_head;
                d_stream.avail_in = sizeof(dummy_head);
                if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK) 
                {
                    return REQ_FAILE;
                }
            }
            else 
			{
				return REQ_FAILE;
			}
        }
    }
    if(inflateEnd(&d_stream) != Z_OK) 
	{
		return REQ_FAILE;
	}
    ret = d_stream.total_out;
    return ret;
}

/**
  * @function http_recv_content
  * @brief  CURL LIB用来接收HTTP请求数据的回调函数
  * @param buffer    CURL LIB接收到数据的缓存
  * @param size    缓存大小
  * @param nmemb    缓存单位
  * @param userp   我们用来接收缓存数据的内存
  * @return 0   未收到数据
  * @return len 收到数据长度
  */
static int http_recv_content(void *buffer, size_t size, size_t nmemb, void *userp)
{
	int len = size * nmemb;
	http_param_t *param = (http_param_t*)userp;
    
	if (param->recv_len > (HTTP_RECV_BUFF_SIZE - len))
    /*当接收内存不足时，直接返回。
          param->recv_len 是已经使用的内存，
          len是buffer中已使用的长度。
      */    
	{
	    param->recv_len = 0;
		return 0;
	}

	memcpy(param->recv_data + param->recv_len, buffer, len);
	param->recv_len = param->recv_len + len;

	return len;
}

/**
  * @function _send_http_request
  * @brief  发送HTTP请求函数
  * @param url    请求的URL
  * @param url_size    请求的URL长度
  * @param data    用来接收GZIP压缩数据的内存
  * @param size   处理GZIP压缩的内存长度
  * @param time_out   记录HTTP请求失败时，重新尝试的次数，后面会根据这个次数来
                                设置当前爬虫的休眠时间。
  * @return 0   未收到数据
  * @return len 收到数据长度
  */
static int _send_http_request(char *url, int url_size, char *data, int size, int *time_out)
{
	CURL *purl          = NULL;
	http_param_t *param = NULL;
	char *loc_url       = NULL;
	struct curl_slist  *headers = NULL;
	int  ret = REQ_FAILE;

        debug("send seq url %s\n", url);
	param = (http_param_t*)malloc(sizeof(http_param_t));
	if (param == NULL)
	{
		write_log("_send_http_request:memory alloc faile\n");
		return REQ_FAILE;
	}
	memset(param, 0, sizeof(http_param_t));
	memset(data, 0, size);

    purl = curl_easy_init();
	if (purl == NULL)
	{
		write_log("curl init failed\n");
		free(param);
		return REQ_FAILE;
	}
    /*设置CURL选项*/
	curl_easy_setopt(purl, CURLOPT_CONNECTTIMEOUT, 30L);
	curl_easy_setopt(purl, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(purl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(purl, CURLOPT_LOCALPORT, 15000L);
	curl_easy_setopt(purl, CURLOPT_LOCALPORTRANGE, 100L);

	curl_easy_setopt(purl, CURLOPT_URL, url);
	curl_easy_setopt(purl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); 

    /*伪造报文头部*/
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.4 (KHTML, like Gecko) Chrome/22.0.1229.94 Safari/537.4");
    headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	headers = curl_slist_append(headers, "Accept-Encoding: gzip,deflate,sdch");
    headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.8");
	headers = curl_slist_append(headers, "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3");
	curl_easy_setopt(purl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(purl, CURLOPT_WRITEFUNCTION, http_recv_content);
    curl_easy_setopt(purl, CURLOPT_WRITEDATA, param);

    /*发送http请求，并接收数据*/
	do
	{
		CURLcode res = curl_easy_perform(purl);
		if (res != CURLE_OK)
	    {
	        /*请求失败，可能是网络问题，也可能是被淘宝暂时禁止访问*/
			(*time_out)++;
			ret = REQ_REPLAY;
            debug("curl_easy_perform() failed. err=%d, url=%s\n", res, url);
			write_log("curl_easy_perform() failed. err=%d, url=%s\n", res, url);
			break;
	    }
        
        /*获取重定向URL*/
		curl_easy_getinfo(purl, CURLINFO_REDIRECT_URL, &loc_url);

        /*若不再返回重定向的URL，则说明已经获取到了想要的数据
                 可以进行GZIP解压缩，取数据了。
             */
        if (loc_url == NULL)
		{
			ret = http_gunzip(param->recv_data, param->recv_len, data, size) ;
			break;
		}

        if (strstr(loc_url, TAOBAO_WAIT_SHOP_URL) != NULL || 
			strstr(loc_url, TAOBAO_CHECK_CODE)    != NULL ||
			strstr(loc_url, TAOBAO_ERROR_URL2)    != NULL ||
			strstr(loc_url, TAOBAO_ERROR_URL3)    != NULL )
	    {
	        /*接收到的数据有问题，可能有部分丢失了，需要重新尝试*/
			(*time_out)++;
		    ret = REQ_REPLAY;
	    }
	    else if (strstr(loc_url, TAOBAO_NO_SHOP_URL) != NULL ||
			     strstr(loc_url, TAOBAO_ERROR_URL1)  != NULL)
	    {
		    ret = REQ_FAILE;
	    }
	    else
	    {
	        /*获取到了重定向的URL，会接着爬取重定向后的URL*/
			ret = REQ_REDIRECT;
			memset(url, 0, url_size);
			strcpy(url, loc_url);
        }
	} while(0);

	free(param); param = NULL;
	curl_slist_free_all(headers); headers = NULL;
	curl_easy_cleanup(purl); purl = NULL;

	return ret;
}

/**
  * @function send_http_request
  * @brief  URL爬取接口函数，控制着爬虫每一个URL爬取时的处理策略。
  * @param url    请求的URL
  * @param data    用来接收GZIP压缩数据的内存
  * @param size   处理GZIP压缩的内存长度
  * @return REQ_FAILE   请求失败
  * @return ret>0  收到数据长度
  */
int send_http_request(const char *url, char *data, int size)
{
	char _url[2048];
	int ret = 0, time_out = 0;

	memset(_url, 0, sizeof(_url));
	strcpy(_url, url);

	while (1)
	{
		ret = _send_http_request(_url, sizeof(_url), data, size, &time_out);
		if (ret == REQ_FAILE)
		{
			ret = -1;
			break;
		}
		else if (ret == REQ_REPLAY)
		{    
		    /*代码走到这里，就有可能是淘宝禁止爬虫访问了，
                       需要休眠一段时间再继续访问。这里的休眠时间只是
                       做了简单递增，效果可能不是很好，可以更换策略。
                    */
			time_out = time_out % 100;
            if (dead_loop(time_out) < 0)
			{
				return -1;
			}
			continue;
		}
		else if (ret == REQ_REDIRECT)
		{
		    /*该分支应该走不到，重定向的情况应该在前面的请求函数中 处理过了*/
			continue;
		}
		else
		{
			break;
		}
	}

	return ret;
}

/**
  * @function http_init
  * @brief  CURL LIB全局初始化函数
  * @param 无
  * @return 0   初始化成功
  * @return 非0    初始化失败
  */
int http_init()
{
	return curl_global_init(CURL_GLOBAL_ALL);
}

/**
  * @function http_clean
  * @brief  CURL LIB资源释放函数
  * @param 无
  * @return 0  释放成功
  */
int http_clean()
{
	curl_global_cleanup();
	return 0;
}
