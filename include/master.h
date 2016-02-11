/*************************************************************************
    > File Name: master.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时33分00秒
 ************************************************************************/

#ifndef _MASTER_H
#define _MASTER_H

#include "worker.h"

#include <string>

#include "event2/event.h"
#include "event2/util.h"

class Plugin;

class Master
{
	public:

		Master(const std::string &ip, unsigned short port);
		~Master();
		
		bool StartMaster();
		
		void RemovePlugins();
		void UnloadPlugins();

		static void MasterExitSignal(evutil_socket_t signo, short event, void *arg);
		static void MasterChldSignal(evutil_socket_t signo, short event, void *arg);

		Worker			    worker;

		struct event_base  *m_base;
		struct event	   *m_exit_event;
		struct event	   *m_chld_event;

		int					nums_of_child;	

		Plugin*			   *m_plugins;
		int				    m_plugin_cnt;

	private:
		bool SetupPlugins(); //get plugin object from so
		bool LoadPlugins(); //call each plugin's Load callback to init some global plugin data
};

#endif
