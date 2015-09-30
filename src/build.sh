rm -v Spider.*

CC=gcc
CFLAGS="-g -o2 -m64"
#CFLAGS="-g -o2"

LIBS="-lz -lcurl -pthread -L/usr/lib64/mysql -lmysqlclient -liconv -lcrypt -lnsl -lm -L/usr/lib64 -lssl -lcrypto"
SOURCE="spider.c http_req.c http_parse.c db_cmd.c shop_parse.c commodity_parse.c mysql_esp.c ipc_msg.c conf.c"

$CC $CFLAGS $SOURCE $LIBS -o spider
