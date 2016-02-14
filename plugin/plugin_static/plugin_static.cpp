/*************************************************************************
    > File Name: plugin_static.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年02月11日 星期四 14时24分59秒
 ************************************************************************/

#include "plugin.h"
#include "connection.h"
#include "worker.h"
#include "http.h"

#include "event2/event.h"
#include "event2/util.h"

#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex.h>

#include <iostream>
#include <string>

enum StaticStatus
{
    INIT,
    READ,
    OVER,
    NON_EXIST,
    NON_ACCESS
};

struct StaticData
{
    StaticData()
    {
        s_fd     = -1;
        s_status = INIT;
    }

    int          s_fd;
    std::string  s_buf;
    std::string  s_all;
    StaticStatus s_status;
};

class PluginStatic: public Plugin
{
    virtual bool Init(Connection *con, int index)
    {
        StaticData *sdata = new StaticData();     
        sdata->s_fd = -1;
        sdata->s_buf.reserve(10 * 1024);  
        con->plugin_data_slots[index] = sdata;

        return true;
    }

    virtual bool ResponseStart(Connection *con, int index)
    {
        StaticData  *sdata   = (StaticData*)con->plugin_data_slots[index];
        HttpRequest *request = con->http_req_parsed;

        regex_t    reg;
        regmatch_t pmatch;
        int        nmatch;

        regcomp(&reg, "^/doc/[^/]*$", REG_EXTENDED);
        nmatch = regexec(&reg, request->http_url.c_str(), 1, &pmatch, 0);
        
        if (nmatch)
        {
            sdata->s_status = NON_ACCESS;
        }
        else
        {
            std::string path = request->http_url.substr(1);

            if (access(path.c_str(), R_OK) == -1)
            {
                sdata->s_status = NON_EXIST;
            }
            else
            {
                sdata->s_status = INIT;
            }
        }
        
        return true;
    }
    
    virtual plugin_state_t Write(Connection *con, int index)
    {
        HttpRequest *request = con->http_req_parsed;
        StaticData  *sdata   = (StaticData*)con->plugin_data_slots[index];

        if (sdata->s_status == INIT)
        {
            sdata->s_status = READ;
            sdata->s_fd     = open(request->http_url.substr(1).c_str(), O_RDONLY);
            return PLUGIN_NOT_READY;
        }
        else if (sdata->s_status == NON_ACCESS)
        {
            con->http_response.http_code    = 404;
            con->http_response.http_phrase 	= "Access Deny";
        }
        else if (sdata->s_status == NON_EXIST)
        {
            con->http_response.http_code    = 403;
            con->http_response.http_phrase 	= "File don't exist";
        }
        else
        {
            int ret = read(sdata->s_fd, &sdata->s_buf[0], sdata->s_buf.capacity());

            if (ret <= 0)
            {
                sdata->s_status = OVER;
                con->http_response.http_body += sdata->s_all;
            }
            else
            {
                sdata->s_all.append(&sdata->s_buf[0], 0, ret);
                return PLUGIN_NOT_READY;
            }
        }

        return PLUGIN_READY;
    }

    virtual bool ResponseEnd(Connection *con, int index)
    {
        StaticData *sdata = (StaticData*)con->plugin_data_slots[index];

        if (sdata->s_status == OVER)
        {
            close(sdata->s_fd);
            sdata->s_fd = -1;
            sdata->s_all.clear();
        }
        
        return true;
    }

    virtual void Close(Connection *con, int index)
    {
        StaticData *sdata = (StaticData*)con->plugin_data_slots[index];

        if (sdata->s_fd != -1)
        {
            close(sdata->s_fd);
        }

        delete sdata;
    }

};

extern "C" Plugin* SetupPlugin()
{
    return new PluginStatic();
}
extern "C" Plugin* RemovePlugin(Plugin *plugin)
{
    delete plugin;
}
