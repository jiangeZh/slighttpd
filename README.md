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

- 支持简单的用户配置。

**配置**：

@2016.2.22：昨天添加了配置模块，已经可以使用啦～

配置方式：支持默认配置、命令行配置、文件配置三种配置方式。

优先级：默认配置 < 命令行配置 < 文件配置。

查看命令行配置帮助以及默认配置：

```
$ ./slighttpd -h
./slighttpd [option]...
  -l|--ListenPort<number>	Default: 8000
  -m|--MaxWorker<number>	Default: 4
  -i|--InitConPool<number> 	Default: 200
  -a|--ListenIP<address>	Default: 0.0.0.0
  -o|--DocumentRoot<path> 	Default: ./htdocs/
  -c|--CGIRoot<path> 		Default: ./cgi/
  -d|--DefaultFile<filename> 	Default: index.html
  -t|--TimeOut<seconds>		Default: 3
  -f|--ConfigFile<filename> 	Default: ./slighttpd.conf
```

目前 -o -c -d 三个配置暂未支持，其他项可进行配置。

配置文件默认在项目根目录下，约定如下：

1.以"#"开头的为注释行

2.等号左右至少留一个空格(支持多个空格)

3.行末不能有空格（否则将被视为值的一部分）

配置样例：

```
#CGI根路径
CGIRoot = ./cgi/
#默认文件名称
DefaultFile = index.html
#根文件路径
DocumentRoot = ./htdocs/
#绑定地址
ListenIP = 0.0.0.0
#侦听端口
ListenPort = 8000
#最大worker数量
MaxWorker = 4
#超时时间
TimeOut = 3
#初始化连接池大小
InitConPool = 200
#插件列表
Plugin = plugin/plugin_static/plugin_static.so
Plugin = plugin/plugin_cgi/plugin_cgi.so
```

**使用（需安装libevent）**：

```
$ cd slighttpd
$ make
$ cd plugin/plugin_static
$ make
$ cd ../plugin_cgi
$ make
$ cd ../..
$ ./slighttpd
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
