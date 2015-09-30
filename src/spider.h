#ifndef __SPIDER_H__
#define __SPIDER_H__

#define SPIDER_VERSION         "1.0"
#define SPIDER_CONF_PATH       "/etc/spider/"
#define SPIDER_CONF_FILE       "spider.conf"

#define SPIDER_LOG_FILE_PATH   "/var/log/spider.log"
#define SPIDER_SHOP_NUM_RECORD "/etc/spider/%s.cur"

#define TAOBAO_SHOP_URL        "http://shop%ld.taobao.com"

#define TAOBAO_NO_SHOP_URL     "noshop.htm"
#define TAOBAO_WAIT_SHOP_URL   "wait.php"
#define TAOBAO_CHECK_CODE      "checkcode.php"
#define TAOBAO_ERROR_URL1      "error1.html"
#define TAOBAO_ERROR_URL2      "error2.html"
#define TAOBAO_ERROR_URL3      "error3.html"
#define TAOBAO_SHOP_LOC        "http://list.taobao.com/itemlist/default.htm?nick=%s&_input_charset=utf-8&json=on&callback=jsonp161"
#define TAOBAO_SHOP_COMMODITY  "http://list.taobao.com/itemlist/default.htm?nick=%s&data-key=s&data-value=%d&module=page&_input_charset=utf-8&json=on&callback=jsonp731"
#define TAOBAO_COMMENT_DETAIL  "http://rate.taobao.com/detail_rate.htm?userNumId=%s&auctionNumId=%s&currentPage=%d"

#define SHOP_TAOBAO_TYPE       0
#define SHOP_TMALL_TYPE        1

#define RECV_BUFF_SIZE         (4 * 1024 * 1024)
#define HTTP_RECV_BUFF_SIZE    (RECV_BUFF_SIZE - sizeof(int))
#define MAX_PAGE_COUNT         10


#define MYSQL_SELECT_ROW_LIMIT 1024
#define MYSQL_FIELD_NAME_LEN   32
#define MYSQL_FIELD_CNT_LIMIT  24  


#define STATUS_STOPPED         0
#define STATUS_RUNNING         1
#define STATUS_SUSPEND         2
#define STATUS_ZOMBIE          3

#define FLT_CITY              "ºÏ·Ê"

#define RET_ERR_SUCCESS         0
#define RET_ERR_RECORD         -1
#define RET_ERR_NO_RECORD      -2
#define RET_ERR_DATABASE       -3

int dead_loop(int second);
#endif