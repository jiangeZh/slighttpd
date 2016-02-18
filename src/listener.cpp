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
	listen_con_cnt				= 0;
}


Listener::~Listener()
{
	if (listen_event)
	{
		event_free(listen_event);
		close(listen_sockfd);
	}
}

bool Listener::InitListener(Worker *worker)
{
	if (-1 == (listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		std::cerr<< "Listener::InitListener(): socket()" << std::endl;
		return false;
	}

	evutil_make_socket_nonblocking(listen_sockfd);

	int reuse = 1;

	setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	if (0 != bind(listen_sockfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)))
	{
		std::cerr<< "Listener::InitListener(): bind()" << std::endl;
		return false;
	}
	if (0 != listen(listen_sockfd, 5))
	{
		std::cerr<< "Listener::InitListener(): listen()" << std::endl;
		return false;
	}

	listen_worker = worker;

	return true;
}

void Listener::AddListenEvent()
{
	listen_event  = event_new(listen_worker->w_base, listen_sockfd, EV_READ | EV_PERSIST, Listener::ListenEventCallback, this);
	event_add(listen_event, NULL);
}

void Listener::ListenEventCallback(evutil_socket_t sockfd, short event, void *arg)
{
	evutil_socket_t con_fd;
	struct sockaddr_in con_addr;
	socklen_t addr_len	= sizeof(con_addr);
	if (-1 == (con_fd = accept(sockfd, (struct sockaddr*)&con_addr, &addr_len)))
	{
		//std::cout << "Thundering herd" <<std::endl;
		return ;
	}

	Listener *listener	= static_cast<Listener*>(arg);
	Connection *con		= listener->listen_worker->NewCon();
	if (con == NULL)
	{
		std::cerr<< "Listener::ListenEventCallback(): NewCon()" << std::endl;
		return ;
	}

	con->con_sockfd = con_fd;

	pid_t pid = getpid();

	std::cout << "listen accept: " << con->con_sockfd << " by process " << pid <<std::endl;

	if (!con->InitConnection(listener->listen_worker))
	{
		std::cerr<< "Listener::ListenEventCallback(): Connection::InitConnection()" << std::endl;
		Worker::CloseCon(con);
		return ;
	}
	con->con_worker->w_con_map[con->con_sockfd] = con;
	++(listener->listen_con_cnt);

}
