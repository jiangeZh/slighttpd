/*************************************************************************
    > File Name: worker.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 20时10分35秒
 ************************************************************************/

#ifndef _WORKER_H
#define _WORKER_H

#include "listener.h"

#include <string>
#include <map>

#include "event2/event.h"
#include "event2/util.h"

#include "util.h"

class Master;
class Connection;

class Worker
{
	public:

		typedef std::map<evutil_socket_t, Connection*> ConnectionMap;

		Worker(const std::string &ip, unsigned short port);
		~Worker();

		void Run();

		static void WorkerExitSignal(evutil_socket_t signo, short event, void *arg);

		Master			   *master;
		Listener			listener;
		ConnectionMap		con_map;

		struct event_base  *w_base;
		struct event	   *w_exit_event;

		std::string			s_inbuf;
		std::string			s_intmp;
		std::string			s_outbuf;

};

#endif
