/*************************************************************************
    > File Name: plugin.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年02月10日 星期三 18时48分29秒
 ************************************************************************/

#include "plugin.h"
#include "worker.h"
#include "connection.h"

Plugin::Plugin()
{
	plugin_data		= NULL;
	plugin_is_loaded= false;
	setup_plugin	= NULL;
	remove_plugin	= NULL;
}

Plugin::~Plugin()	{}

bool Plugin::Init(Connection *con, int index)
{
	return true;
}

bool Plugin::RequestStart(Connection *con, int index)
{
	return true;
}

bool Plugin::Read(Connection *con, int index)
{
	return true;
}

bool Plugin::RequestEnd(Connection *con, int index)
{
	return true;
}

bool Plugin::ResponseStart(Connection *con, int index)
{
	return true;
}

plugin_state_t Plugin::Write(Connection *con, int index)
{
	return PLUGIN_READY;
}

bool Plugin::ResponseEnd(Connection *con, int index)
{
	return true;
}

void Plugin::Close(Connection *con, int index)	{}

bool Plugin::Trigger(Worker* worker, int index)
{
	return true;
}

bool Plugin::LoadPlugin(Worker* worker, int index)
{
	return true;
}

void Plugin::FreePlugin(Worker* worker, int index)	{}

const char * plugin_config[] = 
{
	"plugin/plugin_static/plugin_static.so",
	NULL
};
