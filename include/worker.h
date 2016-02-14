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
class Plugin;

class Worker
{
	public:

		typedef std::map<evutil_socket_t, Connection*> ConnectionMap;

		Worker(const std::string &ip, unsigned short port);
		~Worker();

		bool Init(Master *master);
		void Run();

		static void WorkerExitSignal(evutil_socket_t signo, short event, void *arg);

		Master			   *w_master;
		Listener			w_listener;
		ConnectionMap		w_con_map;

		struct event_base  *w_base;
		struct event	   *w_exit_event;

		Plugin*			   *w_plugins;
		int				    w_plugin_cnt;
	private:
		bool SetupPlugins(); 		//get plugin object from so
		bool LoadPlugins(); 		//call each plugin's Load callback to init some global plugin data
		void RemovePlugins();
		void UnloadPlugins();
};

#endif
