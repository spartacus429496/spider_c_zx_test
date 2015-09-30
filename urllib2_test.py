#!/usr/bin/python
# -*- coding:utf-8 -*-

#你好
import urllib2
req = urllib2.Request('http://www.taobao.com/')
#response = urllib2.urlopen('http://www.taobao.com/')
response = urllib2.urlopen(req)
html = response.read()
print response
print html 
