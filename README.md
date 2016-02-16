High performance HTTP Server.It is designed to study Lighttpd(so I call it "slighttpd").

* **支持平台**: linux
* **开发语言**: C++
* **开发平台**: ubuntu 14.04 LTS 
* **linux内核版本**: 3.19.0-41-generic
* **gcc 版本**: 4.8.2
* **[libevent](http://libevent.org/)版本**: 2.0.22-stable
* 


**Usage**：

```
$ cd slighttpd
$ make
$ cd plugin/plugin_static
$ make
$ cd ../..
$ ./master
```

**Test**：

1. 使用python脚本

```
$ python slighttpd/test/test.py
```

2. 使用webbench

```
webbench -t 30 -c 10000 -2 --get http://127.0.0.1:8000/htdocs/index.html
```
