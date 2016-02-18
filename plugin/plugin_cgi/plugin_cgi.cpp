/*************************************************************************
    > File Name: plugin_cgi.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年02月18日 星期四 18时41分25秒
 ************************************************************************/

#include "plugin.h"
#include "connection.h"
#include "worker.h"
#include "http.h"

#include "event2/event.h"
#include "event2/util.h"

#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex.h>

#include <iostream>
#include <string>

typedef enum
{
    INIT,
    NOT_ACCESS,
    NOT_EXIST,
    OK,
    NOT_OK,
    ERROR,
    DONE
} cgi_state_t;

typedef struct CgiData
{
	CgiData(Connection *con, int index)
	{
        c_status	= INIT;
        c_pipe[0]	= -1;
        c_pipe[1]	= -1;       
        c_buf.reserve(10 * 1024 * 1024);
        c_event		= NULL;
        c_con		= con;
        c_index		= index;
	}

    cgi_state_t   c_status;
    int           c_pipe[2];
    std::string   c_recv;
    std::string   c_buf;
    struct event *c_event;
    Connection   *c_con;
    int           c_index;
} cgi_data_t;

class PluginCgi: public Plugin
{
    char** AddEnv(char** env, int &size, const std::string &key, const std::string &value)
    {
        env = (char**)realloc(env, (++size) * sizeof(char*));
        std::string pair = key + "=" + value;
        env[size-2] = strdup(pair.c_str());
        env[size-1] = NULL;
        return env;
    }

    char* const * MakeCgiEnv(HttpRequest *request)
    {
        int    size = 1;
        char** env  = (char**) malloc(sizeof(char*));
        env[0]      = NULL;
        
        env = AddEnv(env, size, "method", request->http_method);
        env = AddEnv(env, size, "url", request->http_url);
        env = AddEnv(env, size, "body", request->http_body);
        
        header_iter_t iter = request->http_headers.begin();
        while (iter != request->http_headers.end())
        {
            env = AddEnv(env, size, iter->first, iter->second);
            ++iter;
        }

        return env;
    }

    virtual bool Init(Connection *con, int index)
    {
        cgi_data_t *data = new cgi_data_t(con, index);
        if (!data)
        {
            return false;
        }
        con->plugin_data_slots[index] = data;
        return true;
    }

    virtual bool ResponseStart(Connection *con, int index)
    {
        cgi_data_t	*data = static_cast<cgi_data_t*>(con->plugin_data_slots[index]);
        HttpRequest *request = con->http_req_parsed;
		const std::string &uri = request->http_url;
        std::string::size_type suffix_index = uri.rfind(".cgi");
        
        if (suffix_index == std::string::npos || suffix_index != uri.size() - 4)
        {
            data->c_status = NOT_ACCESS;
        }

        regex_t    reg;
        regmatch_t pmatch;
        int        ret;

        regcomp(&reg, "^/cgi/[^/]*.cgi$", REG_EXTENDED);
        ret = regexec(&reg, uri.c_str(), 1, &pmatch, 0);

        if (ret)
        {
            data->c_status = NOT_ACCESS;
        }
    	else
		{
        	if (access(uri.substr(1).c_str(), X_OK) == -1)
        	{
        	    data->c_status = NOT_EXIST;
        	}
			else
			{
        		data->c_status = INIT;
			}
		}
        return true;
    }
    
    virtual plugin_state_t Write(Connection *con, int index)
    {
        cgi_data_t	*data = static_cast<cgi_data_t*>(con->plugin_data_slots[index]);
        HttpRequest *request = con->http_req_parsed;
        if (data->c_status == INIT)
        {
            pipe(data->c_pipe);
            evutil_make_socket_nonblocking(data->c_pipe[0]);

            pid_t pid = fork();    
            if (pid == -1)
            {
                data->c_status = ERROR;
                return PLUGIN_ERROR;
            }
            else if (pid == 0)
            {
                dup2(data->c_pipe[1], 1);
                close(data->c_pipe[1]);

                std::string path = request->http_url.substr(1);
                char* const *env = MakeCgiEnv(request); 
                execle("/bin/bash", "sh", "-c", path.c_str(), NULL, env);
                exit(127); //Command Not Found
            }
            
            close(data->c_pipe[1]);
            data->c_pipe[1] = -1;
            
            data->c_event = event_new(con->con_worker->w_base,
                                           data->c_pipe[0],
                                           EV_READ | EV_PERSIST, 
                                           PluginCgiEventCallback, data); 
            event_add(data->c_event, NULL);

            data->c_status = NOT_OK;
        }
        else if (data->c_status == NOT_ACCESS)
        {
            return PLUGIN_READY;
        }
        else if (data->c_status == NOT_EXIST)
        {
            con->http_response.http_code    = 403;
            con->http_response.http_phrase = "CGI NOT EXSIT";
            return PLUGIN_READY;
        }
        else if (data->c_status == OK)
        {
            con->http_response.http_body = data->c_recv;
            return PLUGIN_READY;
        }
        else if (data->c_status == ERROR)
        {
            return PLUGIN_ERROR;
        }
		return PLUGIN_NOT_READY;
    }

    virtual bool ResponseEnd(Connection *con, int index)
    {
        cgi_data_t *data  = static_cast<cgi_data_t*>(con->plugin_data_slots[index]);
        if (data->c_status == OK)
        {
            close(data->c_pipe[0]);
            event_free(data->c_event);
        }

        data->c_event = NULL;
        data->c_pipe[0] = -1;
        data->c_pipe[1] = -1;
        data->c_recv.clear();
        data->c_status = DONE;

        return true;
    }

    virtual void Close(Connection *con, int index)
    {
        cgi_data_t *data  = static_cast<cgi_data_t*>(con->plugin_data_slots[index]);
        
        if (data->c_event)
        {
            event_free(data->c_event);
        }

        if (data->c_pipe[0] != -1)
        {
            close(data->c_pipe[0]);
        }

        if (data->c_pipe[1] != -1)
        {
            close(data->c_pipe[1]);
        }
        
        delete data;
    }

    static void PluginCgiEventCallback(evutil_socket_t sockfd, short event, void *arg)
    {
        cgi_data_t *data = static_cast<cgi_data_t*>(arg);
        
        int ret = read(data->c_pipe[0], &data->c_buf[0], data->c_buf.capacity());

        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                data->c_status = ERROR; 
            }
        }
        else if (ret == 0)
        {
            data->c_status = OK;
        }
        else
        {
            data->c_recv.append(&data->c_buf[0], 0, ret);
        }
    }
};

extern "C" Plugin* SetupPlugin()
{
    return new PluginCgi();
}

extern "C" Plugin* RemovePlugin(Plugin *plugin)
{
    delete plugin;
}
