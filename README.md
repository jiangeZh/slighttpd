High performance HTTP Server.It is designed to study Lighttpd(so I call it "slighttpd").

* **支持平台**: linux
* **开发语言**: C++
* **开发平台**: ubuntu 14.04 LTS 
* **linux内核版本**: 3.19.0-41-generic
* **gcc 版本**: 4.8.2
* **[libevent](http://libevent.org/)版本**: 2.0.22-stable
* **项目讲解博客专栏**: http://blog.csdn.net/column/details/slighttpd.html
* 

**说明**:

关于网络编程，一开始看了《UNP》和《Linux网络编程》两本书。

但是书看完总是很容易忘，于是开始转向源代码的阅读。

在读了一部分lighttpd源码之后，觉得lighttpd的状态机设计的很有意思。

于是想写一个Simple HTTP Server Demo，模仿着lighttpd的状态机设计来做。

一方面于我来说可以在实践中学习，另一方面也可以分享给其他想学习lighttpd源码的朋友。

http解析使用第三方库：http-parser。

**项目概况**:

- 使用Watcher-Worker模式，Watcher负责监控，Worker处理请求，多进程可充分发挥多核的优势，且进程间相对独立。

- 项目基于Linux平台，使用epoll来实现异步事件（使用高性能网络库libevent，并没有直接操纵epoll）。

- 每个连接都包含一个状态机（state machine），效仿lighttpd，不过比lighttpd的状态机简化了许多。

- 使用连接池来复用连接对象，避免了频繁的new和delete。

- 插件（Plugin）类的公共接口函数为虚函数，由插件开发者继承，通过多态性，状态机只需要调用基类的接口函数，不用关心插件的具体实现。

**使用（需安装libevent）**：

```
$ cd slighttpd
$ make
$ cd plugin/plugin_static
$ make
$ cd ../plugin_cgi
$ make
$ cd ../..
$ ./master
```

浏览器打开：

http://127.0.0.1:8000/htdocs/index.html

**测试**：

1. 使用python脚本

```
$ python slighttpd/test/test.py
```

2. 使用webbench测试高并发时程序是否会崩溃

```
webbench -t 30 -c 10000 -2 --get http://127.0.0.1:8000/htdocs/index.html
```
