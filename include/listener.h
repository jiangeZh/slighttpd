/*************************************************************************
    > File Name: listener.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时46分34秒
 ************************************************************************/

#ifndef _LISTENER_H
#define _LISTENER_H

#include <string>

#include "event2/event.h"
#include "event2/util.h"

#include "util.h"

class Worker;

class Listener
{
	public:

		Listener(const std::string &ip, unsigned short port);
		~Listener();

		bool InitListener(Worker *worker);
		void AddListenEvent();
	
		static void ListenEventCallback(evutil_socket_t fd, short event, void *arg);

	public:

		Worker			   *listen_worker;
		evutil_socket_t		listen_sockfd;
		struct sockaddr_in	listen_addr;
		struct event	   *listen_event;
		int					cnt_connection;


};


#endif

