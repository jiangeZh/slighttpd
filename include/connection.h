/*************************************************************************
    > File Name: connection.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 20时10分35秒
 ************************************************************************/

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <string>
#include <queue>

#include "event2/event.h"
#include "event2/util.h"

#include "util.h"

class Worker;

class Connection
{
	public:
		Connection();
		~Connection();

		bool InitConnection(Worker *worker);

		void WantRead();
		void NotWantRead();
		void WantWrite();
		void NotWantWrite(); 

		static void ConEventCallback(evutil_socket_t fd, short event, void *arg);

		Worker			   *con_worker;

		evutil_socket_t		con_sockfd;
		struct sockaddr_in	con_addr;
		struct event	   *con_event;	//这里用两个event，一个注册读，一个注册写会效率高点
		//struct event	   *write_event;
		//struct event	   *read_event;

		std::string			con_inbuf;
		std::string			con_intmp;
		std::string			con_outbuf;

		static void FreeConnection(Connection *con);
};

#endif
