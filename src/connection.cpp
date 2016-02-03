/*************************************************************************
    > File Name: connection.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月28日 星期四 12时06分22秒
 ************************************************************************/

#include "connection.h"
#include "worker.h"

#include<iostream>

Connection::Connection()
{
	con_worker = NULL;

	con_event	 = NULL;
}

Connection::~Connection()
{
	if (con_event)
	{
		event_free(con_event);
		std::cout << con_sockfd << " closed" << std::endl;
		close(con_sockfd);
	}
	//std::cout << "Connection closed" << std::endl;
}

void Connection::FreeConnection(Connection *con)
{
	Worker *worker = con->con_worker;

	if (con->con_event)
	{
		Worker::ConnectionMap::iterator con_iter = worker->con_map.find(con->con_sockfd);
		worker->con_map.erase(con_iter);
	}

	delete con;
}

bool Connection::InitConnection(Worker *worker)
{
	con_worker = worker;

try
{	
	//这里超过100个connection会爆内存！
/*	con_intmp.reserve(10 * 1024 * 1024);
	con_inbuf.reserve(10 * 1024 * 1024);
	con_outbuf.reserve(10 * 1024 * 1024);
*/
	con_intmp.reserve(10 * 1024);
	con_inbuf.reserve(10 * 1024);
	con_outbuf.reserve(10 * 1024);

	evutil_make_socket_nonblocking(con_sockfd);
	//test：监听读事件，从客户端读，然后回显
	con_event = event_new(con_worker->w_base, con_sockfd, EV_READ | EV_PERSIST, Connection::ConEventCallback, this);
}
catch(std::bad_alloc)
{
	std::cout << "Here con" <<std::endl;
}
	event_add(con_event, NULL);

	return true;
}

void Connection::WantRead()
{
    //m_want_read = true;
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event | EV_READ, Connection::ConEventCallback, this);
    event_add(con_event, NULL); 
}

void Connection::NotWantRead()
{
    //m_want_read = false;
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event & (~EV_READ), Connection::ConEventCallback, this);
    event_add(con_event, NULL); 
}

void Connection::WantWrite()
{
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event | EV_WRITE, Connection::ConEventCallback, this);
    event_add(con_event, NULL); 
}

void Connection::NotWantWrite() 
{
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event & (~EV_WRITE), Connection::ConEventCallback, this);
    event_add(con_event, NULL);
}

void Connection::ConEventCallback(evutil_socket_t sockfd, short event, void *arg)
{
/*
	std::cout << "Want read" <<std::endl;
	Connection *con = (Connection*)arg;
	ssize_t n;
	int cap = con->con_inbuf.capacity();
	if ( (n = read(con->con_sockfd, &con->con_inbuf[0], cap)) == 0)
	{
		//返回0，对方关闭连接，删除事件。
		event_del((struct event*)arg);
	}
*/
    Connection *con = (Connection*)arg;

    if (event & EV_READ) 
    {
        int cap = con->con_intmp.capacity();
        int ret = read(sockfd, &con->con_intmp[0], cap);
    
        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                FreeConnection(con);
                return;
            }
        }
        else if (ret == 0)
        {
            FreeConnection(con); 
            return;
        }
        else
        {
			con->con_inbuf.clear();
            con->con_inbuf.append(con->con_intmp.c_str(), ret);
        }
		con->con_outbuf = "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\n\r\n<html>\r\n<body>\r\nhello\r\n</body>\r\n</html>";
		con->NotWantRead();
		con->WantWrite();
    }
    
    if (event & EV_WRITE)
    {
        int ret = write(sockfd, con->con_outbuf.c_str(), con->con_outbuf.size());

        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                FreeConnection(con);
                return;
            }
        }
        else
        {

        }
		con->NotWantWrite();
		con->WantRead();
    }	
}
