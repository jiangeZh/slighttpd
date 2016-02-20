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
#include <vector>

#include "event2/event.h"
#include "event2/util.h"

#include "util.h"

class Master;
class Connection;
class Plugin;

class Worker
{
	public:

		Worker(const std::string &ip, unsigned short port);
		~Worker();

		bool Init(Master *master);
		void Run();
		Connection* NewCon();
		static void CloseCon(Connection* con);

	public:

		typedef std::map<evutil_socket_t, Connection*> ConnectionMap;
		ConnectionMap		 w_con_map;

		struct event_base	*w_base;

		Plugin*			*w_plugins;
		int			 w_plugin_cnt;

	private:

		bool SetupPlugins(); 		//get plugin object from so
		bool LoadPlugins(); 		//call each plugin's Load callback to init some global plugin data
		void RemovePlugins();
		void UnloadPlugins();

		void InitConPool();
		Connection* GetFreeCon();
		bool AddConToFreePool(Connection* con);
		static void FreeCon(Connection *con);

		static void WorkerExitSignal(evutil_socket_t signo, short event, void *arg);

	private:

		Master		*w_master;
		Listener	 w_listener;
		struct event	*w_exit_event;

		typedef std::vector<Connection*> con_pool_t;
		con_pool_t	 con_pool;
		int		 con_pool_size;
		int		 con_pool_cur;
};

#endif
