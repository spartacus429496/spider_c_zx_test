#ifndef __HTTP_PARASE_H__
#define __HTTP_PARASE_H__

// Parse recv html and get information
// Parement:
//    shop_num : shop number
//        data :  html data
//        size :  html data length
//        repeat : regeat again
//        mysql  : mysql database handle
// Return  :
//  Success return 0, Failed return -1;
int http_parse_process(mysql_handle_t *mysql, long shop_num, const char *data, int len, int repeat);
#endif