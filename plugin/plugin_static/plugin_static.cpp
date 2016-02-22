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

typedef enum
{
    INIT,
    READ,
    DONE,
    NOT_EXIST,
    NOT_ACCESS
} static_state_t;

typedef struct StaticData
{
    StaticData(): s_fd(-1), s_state(INIT) {}

    int          s_fd;
    std::string  s_buf;
    std::string  s_data;
    static_state_t s_state;
} static_data_t;

class PluginStatic: public Plugin
{
    virtual bool Init(Connection *con, int index)
    {
        static_data_t *data = new static_data_t();
        data->s_buf.reserve(10 * 1024);  
        con->plugin_data_slots[index] = data;
        return true;
    }

    virtual bool ResponseStart(Connection *con, int index)
    {
        static_data_t  *data = static_cast<static_data_t*>(con->plugin_data_slots[index]);
        HttpRequest *request = con->http_req_parsed;

        regex_t    reg;
        regmatch_t pmatch;
        int        ret;

        regcomp(&reg, "^/htdocs/[^/]*$", REG_EXTENDED);
        ret = regexec(&reg, request->http_url.c_str(), 1, &pmatch, 0);
        
        if (ret)
        {
            data->s_state = NOT_ACCESS;
        }
        else
        {
            std::string path = request->http_url.substr(1);

            if (access(path.c_str(), R_OK) == -1)
            {
                data->s_state = NOT_EXIST;
            }
            else
            {
                data->s_state = INIT;
            }
        }
        
        return true;
    }
    
    virtual plugin_state_t Write(Connection *con, int index)
    {
        static_data_t  *data = static_cast<static_data_t*>(con->plugin_data_slots[index]);
        HttpRequest *request = con->http_req_parsed;

        if (data->s_state == INIT)
        {
            data->s_state = READ;
            data->s_fd     = open(request->http_url.substr(1).c_str(), O_RDONLY);
        }
        else if (data->s_state == NOT_ACCESS)
        {
            con->http_response.http_code    = 404;
            con->http_response.http_phrase 	= "Access Deny";
            return PLUGIN_READY;
        }
        else if (data->s_state == NOT_EXIST)
        {
            con->http_response.http_code    = 403;
            con->http_response.http_phrase 	= "File don't exist";
            return PLUGIN_READY;
        }

        int ret = read(data->s_fd, &data->s_buf[0], data->s_buf.capacity());

        if (ret <= 0)
        {
            data->s_state = DONE;
            con->http_response.http_body += data->s_data;
            return PLUGIN_READY;
        }
        else
        {
            data->s_data.append(&data->s_buf[0], 0, ret);
            return PLUGIN_NOT_READY;
        }
    }

    virtual bool ResponseEnd(Connection *con, int index)
    {
        static_data_t *data = static_cast<static_data_t*>(con->plugin_data_slots[index]);

        if (data->s_state == DONE)
        {
            close(data->s_fd);
            data->s_fd = -1;
            data->s_data.clear();
        }
        
        return true;
    }

    virtual void Close(Connection *con, int index)
    {
        static_data_t *data = static_cast<static_data_t*>(con->plugin_data_slots[index]);

        if (data->s_fd != -1)
        {
            close(data->s_fd);
        }

        delete data;
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
