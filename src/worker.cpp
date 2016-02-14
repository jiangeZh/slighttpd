/*************************************************************************
    > File Name: worker.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月28日 星期四 12时06分22秒
 ************************************************************************/

#include "worker.h"
#include "master.h"
#include "connection.h"
#include "plugin.h"

#include <iostream>
#include <dlfcn.h>
#include <stdlib.h>

Worker::Worker(const std::string &ip, unsigned short port)
		:w_listener(ip, port)
{
	w_master 		= NULL;
	w_base 			= NULL;
	w_exit_event	= NULL;
	w_plugins	  	= NULL;
	w_plugin_cnt  	= 0;
}

Worker::~Worker()
{
	//需要在释放con之前，否则con释放时可能插件还在使用con的数据
	UnloadPlugins();
	//Master不进入run()，故不对w_exit_event初始化
	if (w_exit_event)
		event_free(w_exit_event);

	if (w_base)
	{
		ConnectionMap::iterator con_iter = w_con_map.begin();
		while (con_iter != w_con_map.end())
		{
			Connection *con = con_iter->second;
			delete con;
			++con_iter;
		}
		event_base_free(w_base);
	}
	//需要在释放con之后，此时con中插件的数据已经被清理掉
	RemovePlugins();
	std::cout<< "----total connection: " << w_listener.cnt_connection << "----" << std::endl;
}

bool Worker::Init(Master *master)
{
	w_master = master;

	if (!w_listener.InitListener(this))
	{
		return false;
	}

	if (!(SetupPlugins() && LoadPlugins()))
	{
		return false;
	}

	return true;
}

void Worker::Run()
{
	w_base = event_base_new();
	w_listener.AddListenEvent();
	w_exit_event = evsignal_new(w_base, SIGINT, Worker::WorkerExitSignal, w_base);
	evsignal_add(w_exit_event, NULL);
	
	event_base_dispatch(w_base);
	return;
}

void Worker::WorkerExitSignal(evutil_socket_t signo, short event, void *arg)
{	
	event_base_loopexit((struct event_base*)arg, NULL);
}

bool Worker::SetupPlugins()
{
	const char *path;

	for (int i = 0; plugin_config[i]; ++i)
	{
		path = plugin_config[i];
		
		void *so = dlopen(path, RTLD_LAZY);
		if (!so)
		{
			std::cerr << dlerror() << std::endl;
			return false;
		}

		Plugin::SetupPlugin setup_plugin = (Plugin::SetupPlugin)dlsym(so, "SetupPlugin");
		Plugin::RemovePlugin remove_plugin = (Plugin::RemovePlugin)dlsym(so, "RemovePlugin");
		if (!setup_plugin || !remove_plugin)
		{
			std::cerr << dlerror() << std::endl;
			dlclose(so);
			return false;
		}

		Plugin *plugin = setup_plugin();
		if (!plugin)
		{
			dlclose(so);
			return false;
		}

		plugin->setup_plugin = setup_plugin;
		plugin->remove_plugin = remove_plugin;
		plugin->plugin_so = so;
		plugin->plugin_index = i;

		w_plugins = (Plugin* *) realloc(w_plugins, sizeof(*w_plugins)*(w_plugin_cnt+1));
		w_plugins[w_plugin_cnt++] = plugin;
	}

	return true;
}

void Worker::RemovePlugins()
{
	Plugin *plugin;

	for (int i = 0; i < w_plugin_cnt; ++i)
	{
		plugin = w_plugins[i];
		Plugin::RemovePlugin remove_plugin = plugin->remove_plugin;
		void *so = plugin->plugin_so;
		remove_plugin(plugin);
		dlclose(so);
	}
	free(w_plugins);
}

bool Worker::LoadPlugins()
{
	Plugin *plugin;

	for (int i = 0; i < w_plugin_cnt; ++i)
	{
		plugin = w_plugins[i];
		if (plugin->LoadPlugin(this, i))
		{
			plugin->plugin_is_loaded = true;
		}
		else
		{
			return false;
		}
	}

	return true;
}

void Worker::UnloadPlugins()
{
	Plugin *plugin;

	for (int i = 0; i < w_plugin_cnt; ++i)
	{
		plugin = w_plugins[i];
		if (plugin->plugin_is_loaded)
		{
			plugin->FreePlugin(this, i);
		}
	}
}
