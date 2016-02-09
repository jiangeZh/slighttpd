/*************************************************************************
    > File Name: listener.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时48分56秒
 ************************************************************************/

#include "listener.h"
#include "worker.h"
#include "connection.h"

#include<iostream>

Listener::Listener(const std::string &ip, unsigned short port)
{
	//ipv4
	listen_addr.sin_family		= AF_INET;
	listen_addr.sin_addr.s_addr	= inet_addr(ip.c_str());
	listen_addr.sin_port		= htons(port);
	listen_event				= NULL;
	cnt_connection	= 0;
	std::cout << "Init listener" << std::endl;
}


Listener::~Listener()
{
	if (listen_event)
	{
		event_free(listen_event);
		close(listen_sockfd);
	}
	std::cout << "Listener closed" << std::endl;
}

bool Listener::InitListener(Worker *worker)
{
	if (-1 == (listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		return false;
	}

	evutil_make_socket_nonblocking(listen_sockfd);

	int reuse = 1;

	setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	if (0 != bind(listen_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)))
	{
		return false;
	}
	if (0 != listen(listen_sockfd, 5))
	{
		return false;
	}

	listen_worker = worker;
	//当return false时有bug，可能出在event_del上
	return true;
}

void Listener::AddListenEvent()
{
	listen_event  = event_new(listen_worker->w_base, listen_sockfd, EV_READ | EV_PERSIST, Listener::ListenEventCallback, this);
	event_add(listen_event, NULL);
}

void Listener::ListenEventCallback(evutil_socket_t sockfd, short event, void *arg)
{
	Listener *listener	= (Listener*)arg;
	Connection *con		= NULL;
	try
	{
		con = new Connection();
	}
	catch(std::bad_alloc)
	{
		std::cout << "Here listen" <<std::endl;
	}
	socklen_t addr_len	= sizeof(con->con_addr);
	//需要处理惊群
	//这里不应该退出，不然尝试失败的程序就结束了
	if (-1 == (con->con_sockfd = accept(sockfd, (struct sockaddr*)&con->con_addr, &addr_len)))
	{

		delete con;
		return ;
/*		
		if (errno != EAGAIN && errno != EINTR)
		{
			event_base_loopexit(listener->worker->w_base, NULL);
		}
*/
	}
	pid_t pid = getpid();
	std::cout << "listen accept: " << con->con_sockfd << " by process " << pid <<std::endl;

	if (!con->InitConnection(listener->listen_worker))
	{
		Connection::FreeConnection(con);
		return ;
	}
	con->con_worker->con_map[con->con_sockfd] = con;
	++(listener->cnt_connection);

}
