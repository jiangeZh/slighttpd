/*************************************************************************
    > File Name: master.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时37分23秒
 ************************************************************************/

#include "master.h"
#include "worker.h"
#include "plugin.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <iostream>

Master::Master(const std::string &ip, unsigned short port)
	:worker(ip, port)
{
	m_base = NULL;
	m_exit_event  = NULL;
	m_chld_event  = NULL;
	nums_of_child = 4;
	m_plugins	  = NULL;
	m_plugin_cnt  = 0;
}

Master::~Master()
{
	if (m_base)
	{
        event_free(m_exit_event);
        event_free(m_chld_event);
		event_base_free(m_base);
	}
	std::cout << "Master Closed" << std::endl;
}


bool Master::StartMaster()
{
	std::cout << "Start Master" << std::endl;

	worker.master = this;

	if (!worker.listener.InitListener(&worker))
	{
		return false;
	}

	if (!(SetupPlugins() && LoadPlugins()))
	{
		return false;
	}
	//创建一定数量的worker
	while (nums_of_child > 0)
	{
		switch (fork())
		{
			case -1:
				return false;
			case 0:
			{
				worker.Run();
				return true;
			}	
			default:
				--nums_of_child;
				break;
		}
	}

	m_base = event_base_new();
	m_exit_event = evsignal_new(m_base, SIGINT, Master::MasterExitSignal, m_base);
	m_chld_event = evsignal_new(m_base, SIGCHLD, Master::MasterChldSignal, this);
	evsignal_add(m_exit_event, NULL);
	evsignal_add(m_chld_event, NULL);
	event_base_dispatch(m_base);

	return true;
}

void Master::MasterExitSignal(evutil_socket_t signo, short event, void *arg)
{	
	//kill(0, SIGINT); //终端下是不需要的，因为所以子进程跟终端关联，所以都会收到INT
	event_base_loopexit((struct event_base*)arg, NULL);
}


void Master::MasterChldSignal(evutil_socket_t signo, short event, void *arg)
{	
	Master *master = (Master *)arg;
	pid_t	pid;
	int		stat;
	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{
		++(master->nums_of_child);
		std::cout << "Child " << pid << " terminated" << std::endl;
	}
}

bool Master::SetupPlugins()
{
	const char *path;
	int			index;

	for (index = 0; plugin_config[index]; ++index)
	{
		path = plugin_config[index];
		
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
		plugin->plugin_index = index;

		m_plugins = (Plugin* *) realloc(m_plugins, sizeof(*m_plugins)*(m_plugin_cnt+1));
		m_plugins[m_plugin_cnt++] = plugin;
	}

	return true;
}

void Master::RemovePlugins()
{
	Plugin *plugin;

	for (int i = 0; i < m_plugin_cnt; ++i)
	{
		plugin = m_plugins[i];
		Plugin::RemovePlugin remove_plugin = plugin->remove_plugin;
		remove_plugin(plugin);
		dlclose(plugin->plugin_so);
	}
	free(m_plugins);
}

bool Master::LoadPlugins()
{
	Plugin *plugin;

	for (int i = 0; i < m_plugin_cnt; ++i)
	{
		plugin = m_plugins[i];
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

void Master::UnloadPlugins()
{
	Plugin *plugin;

	for (int i = 0; i < m_plugin_cnt; ++i)
	{
		plugin = m_plugins[i];
		if (plugin->plugin_is_loaded)
		{
			plugin->FreePlugin(this, i);
		}
	}
}
