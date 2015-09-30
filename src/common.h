/*******************************************************************************
*[Filename]        common.h
*[Version]          v1.0
*[Revision Date]    2014/12/19
*[Author]           
*[Description]
                 基本的字符串操作，转码，文件读写接口函数。以及代码调试开关
                 
*
*
*[Copyright]
* Copyright (C) 2010 ZXSOFT Technology Incorporation. All Rights Reserved.
*******************************************************************************/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include "spider.h"

#define DBG   0


#if DBG
#define debug  printf
#else
#define debug
#endif


static inline char* strrep(char *str, const char *oldstr, const char *newstr)
{
	char *p = NULL;
    char tmp[256];

	memset(tmp, 0, sizeof(tmp));

	p = strstr(str, oldstr);
	if (p == NULL)
	{
		return NULL;
	}
	strncpy(tmp, str, p - str);
    strcat(tmp, newstr);
	strcat(tmp, p + strlen(oldstr));
	strcpy(str, tmp);
	return str;
}

static inline char* strltrim(char *pstr)
{
	char *ptr = NULL;

	ptr = pstr;
	if (strlen(pstr) == 0) 
	{
		return(pstr);
	}

	while (*ptr != 0 && *ptr == ' ') 
	{
		ptr++;
	}
	while (ptr < pstr + strlen(pstr))
	{
		*pstr++ = *ptr++;
	}
    *pstr = 0;
	return(pstr);
}

static inline char* strrtrim(char *pstr)
{
	int len = 0;

	if( pstr == NULL || pstr[0] == 0)
	{
		return pstr;
	}

	len = strlen(pstr);
	while( len >= 1 )
	{
		if(isspace(pstr[len - 1]))
		{
			pstr[len - 1] = 0;
			len--;
			continue;
		}
		else
		{
			break;
		}
	}
	return pstr;
}

static inline char* strtrim(char *pstr)
{
	return (strltrim(strrtrim(pstr)));
}

static inline int code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen) 
{ 
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == 0)
	{
		return -1;
	}
	memset(outbuf, 0, outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
	{
		return -1;
	}
	iconv_close(cd); 
	return 0; 
} 
static inline int utf2gbk(char *inbuf, size_t inlen, char *outbuf, size_t outlen) 
{ 
	return code_convert("utf-8","gbk", inbuf, inlen, outbuf, outlen); 
} 
static inline int gbk2utf(char *inbuf, size_t inlen, char *outbuf, size_t outlen) 
{ 
	return code_convert("gbk","utf-8", inbuf, inlen, outbuf, outlen); 
} 
static inline int write_log(const char *fmt, ...)
{
	char      log_buf[4096], *pt = NULL;;
	va_list   args;
	FILE     *fp = NULL;
	time_t    now_time = time(NULL);
	struct tm *tt = localtime(&now_time);

	memset(log_buf, 0,  sizeof(log_buf));
	pt = log_buf;
    sprintf(pt, "[%.2d/%.2d/%d-%.2d:%.2d:%.2d] ", 
		tt->tm_mday, tt->tm_mon + 1, tt->tm_year + 1900, tt->tm_hour, tt->tm_min, tt->tm_sec);
    pt = pt + strlen(pt);

    va_start(args, fmt);
    vsnprintf(pt, 2048, fmt, args);
    va_end(args);

	fp = fopen(SPIDER_LOG_FILE_PATH, "ab+");
	if (fp == NULL)
	{
		return -1;
	}
	if(fputs(log_buf, fp));
	fclose(fp);

	return 0;
}
static inline long read_curr_shop(const char *proc_name)
{
	char buf[64], *p = NULL;
	char name[128];
	FILE *fp = NULL;

	memset(name, 0, sizeof(name));
	sprintf(name, SPIDER_SHOP_NUM_RECORD, proc_name);

	fp = fopen(name, "r");
	if (fp == NULL)
	{
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	p = fgets(buf, sizeof(buf), fp);
	fclose(fp);

	if (p == NULL) 
	{
		return -1;
	}
	return atol(buf);
}
static inline int write_curr_shop(long shop, const char *proc_name)
{
	char buf[64];
	char name[128];
	FILE *fp = NULL;

	memset(name, 0, sizeof(name));
	sprintf(name, SPIDER_SHOP_NUM_RECORD, proc_name);

	fp = fopen(name, "w+");
	if (fp == NULL)
	{
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%ld", shop);
    if (fputs(buf, fp));
	fclose(fp);

	return 0;
}

static inline void dump_stack(void)
{
#if 0
    void *buffer[100];
    int  nptrs = 0, i = 0;
	char  **strings = NULL;
	char  buf[1024];
   
	write_log("call trace:\n");
	memset(buffer, 0, sizeof(buffer));
    nptrs = backtrace(buffer, 100);
	if (nptrs <= 0)
	{
		return;
	}

	strings = backtrace_symbols(buffer, nptrs);
	for (i = 0; i < nptrs; i++)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%02d %s \n", i, strings[i]);
		write_log(buf);
	}
    write_log("\n");
	free(strings);
#endif
	return;
}
#endif
