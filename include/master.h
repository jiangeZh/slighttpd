/*************************************************************************
    > File Name: master.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时33分00秒
 ************************************************************************/

#ifndef _MASTER_H
#define _MASTER_H

#include "worker.h"
#include "config.h"

#include <string>

#include "event2/event.h"
#include "event2/util.h"

class Master
{
	public:

		Master();
		~Master();
		
		bool StartMaster(int argc, char *argv[]);

		Config			conf_para;

	private:

		static void MasterExitSignal(evutil_socket_t signo, short event, void *arg);
		static void MasterChldSignal(evutil_socket_t signo, short event, void *arg);

	private:

		Worker			 m_worker;

		struct event_base	*m_base;
		struct event		*m_exit_event;
		struct event		*m_chld_event;

		int			 nums_of_child;
};

#endif
