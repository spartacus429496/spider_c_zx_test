#!/usr/bin/python
# -*- coding:utf-8 -*-

#你好


import urllib  
import urllib2  

url = 'http://www.taobao.com'

user_agent = 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:35.0) Gecko/20100101 Firefox/35.0'
values = {'name' : 'WHY',  
          'location' : 'SDU',  
          'language' : 'Python' }  

headers = { 'User-Agent' : user_agent }  
data = urllib.urlencode(values)  
req = urllib2.Request(url, data, headers)  
response = urllib2.urlopen(req)  
the_page = response.read() 
print the_page
