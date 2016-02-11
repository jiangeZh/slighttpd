/*************************************************************************
    > File Name: plugin.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年02月10日 星期三 18时46分22秒
 ************************************************************************/

#ifndef _PLUGIN_H
#define _PLUGIN_H

class Master;
class Connection;

typedef enum
{
	PLUGIN_READY,
	PLUGIN_NOT_READY,
	PLUGIN_ERROR
} plugin_state_t;

class Plugin
{
	public:
		Plugin();
		virtual ~Plugin();

		virtual bool Init(Connection *con, int index);
		virtual bool RequestStart(Connection *con, int index);
		virtual bool Read(Connection *con, int index);
		virtual bool RequestEnd(Connection *con, int index);
		virtual bool ResponseStart(Connection *con, int index);
		virtual plugin_state_t Write(Connection *con, int index);
		virtual bool ResponseEnd(Connection *con, int index);
		virtual void Close(Connection *con, int index);

		virtual bool Trigger(Master *master, int index);
		virtual bool LoadPlugin(Master *master, int index);
		virtual void FreePlugin(Master *master, int index);

		typedef Plugin* (*SetupPlugin)();
		typedef void	(*RemovePlugin)(Plugin *plugin);

		SetupPlugin setup_plugin;
		RemovePlugin remove_plugin;

		void* plugin_data;
		void* plugin_so;
		int   plugin_index;
		bool  plugin_is_loaded;
};

extern const char * plugin_config[];

#endif
