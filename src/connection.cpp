/*************************************************************************
    > File Name: connection.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月28日 星期四 12时06分22秒
 ************************************************************************/

#include "connection.h"
#include "worker.h"
#include "plugin.h"
#include "master.h"

#include<iostream>

Connection::Connection()
{
	con_worker 				= NULL;
	con_event	 			= NULL;
	con_keep_alive			= true;
	con_req_cnt				= 0;
    http_req_parsed  		= NULL;
    http_req_parser  		= NULL;
	plugin_data_slots		= NULL;
	plugin_cnt				= 0;
}

Connection::~Connection()
{
	if (con_event)
	{
		FreePluginDataSlots();

		event_free(con_event);
		std::cout << con_sockfd << " closed" << std::endl;
		close(con_sockfd);

		HttpRequest *request;

        while (!req_queue.empty())
        {
            request = req_queue.front();    
            req_queue.pop();
            delete request;
        }

        if (http_req_parsed)
        {
            delete http_req_parsed;
        }

        if (http_req_parser)
        {
            delete http_req_parser;
        }
	}
}

//从worker中删除该连接，之后释放内存
void Connection::FreeConnection(Connection *con)
{
	Worker *worker = con->con_worker;

	if (con->con_event)
	{
		Worker::ConnectionMap::iterator con_iter = worker->con_map.find(con->con_sockfd);
		worker->con_map.erase(con_iter);
	}

	delete con;
}

bool Connection::InitConnection(Worker *worker)
{
	con_worker = worker;

	try
	{	
		con_intmp.reserve(10 * 1024);
		con_inbuf.reserve(10 * 1024);
		con_outbuf.reserve(10 * 1024);
	}
	catch(std::bad_alloc)
	{
		std::cout << "Here con" <<std::endl;
	}

    http_parser.InitParser(this);

	evutil_make_socket_nonblocking(con_sockfd);
	con_event = event_new(con_worker->w_base, con_sockfd, EV_PERSIST, Connection::ConEventCallback, this);
	event_add(con_event, NULL);

    if (!InitPluginDataSlots())
    {
        return false;
    }

    SetState(CON_STATE_REQUEST_START);

    if (!StateMachine())
    {
        return false;
    }

	return true;
}


void Connection::ConEventCallback(evutil_socket_t sockfd, short event, void *arg)
{

    Connection *con = (Connection*)arg;

    if (event & EV_READ) 
    {
        int cap = con->con_intmp.capacity();
        int ret = read(sockfd, &con->con_intmp[0], cap);
    
        if (ret == -1)
        {
           	if (errno != EAGAIN && errno != EINTR)
           	{
           	    FreeConnection(con);
              	return;
           	}
        }
        else if (ret == 0)
        {
           	FreeConnection(con); 
           	return;
        }
        else
        {
           	con->con_inbuf.append(con->con_intmp.c_str(), ret);
		}
    }

    if (event & EV_WRITE)
    {
        int ret = write(sockfd, con->con_outbuf.c_str(), con->con_outbuf.size());
        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                    FreeConnection(con);
                    return;
            }
        }
        else
        {
			con->con_outbuf.erase(con->con_outbuf.begin(), con->con_outbuf.begin() + ret);
            if (con->con_outbuf.size() == 0 && !con->con_want_write)
            {
                con->UnsetWriteEvent();
            }
        }
    }

	if (!con->StateMachine())
    {
        FreeConnection(con);
    }
}


bool Connection::StateMachine()
{
    request_state_t req_state;
	plugin_state_t  plugin_state;
    
    while (true)
    {
        switch (con_state)
        {
			case CON_STATE_CONNECT:
				ResetConnection();
				break;
			
            case CON_STATE_REQUEST_START:
                if (!PluginRequestStart())
				{
            		return false;
        		}

				++con_req_cnt;
                WantRead();
                SetState(CON_STATE_READ);
                break;

            case CON_STATE_READ:
                if (!PluginRead())
				{
            		return false;
        		}

                req_state = GetParsedRequest();
                if (req_state == REQ_ERROR) 
				{
                    return false;
                } 
                else if (req_state == REQ_IS_COMPLETE) 
				{
                    SetState(CON_STATE_REQUEST_END);
                    break;
                }
                else 
				{
                    return true;
                }
				break;

            case CON_STATE_REQUEST_END:
                if (!PluginRequestEnd())
				{
            		return false;
        		}

                NotWantRead();
				SetState(CON_STATE_RESPONSE_START);
				break;

            case CON_STATE_RESPONSE_START:
                if (!PluginResponseStart())
				{
            		return false;
        		}

                WantWrite();
                SetState(CON_STATE_WRITE);
                break;

            case CON_STATE_WRITE:
				plugin_state = PluginWrite();
                
                if (plugin_state == PLUGIN_ERROR) //need to send back 500(Server Internal Error)
				{
                    SetState(CON_STATE_ERROR);
                    continue;
                }
                else if (plugin_state == PLUGIN_NOT_READY) //Plugin isn't ready, we continue with other connection
				{                
			    	return true;
                }

                con_outbuf += http_response.GetResponse();
                SetState(CON_STATE_RESPONSE_END);
                break;

            case CON_STATE_RESPONSE_END:
                if (!PluginResponseEnd())
				{
            		return false;
        		}

				NotWantWrite(); //设置flag表示不想读，但是如果缓冲区还有数据，仍发送，发送完毕再注销写事件
                delete http_req_parsed;
                http_req_parsed = NULL;
                http_response.ResetResponse();
                SetState(CON_STATE_REQUEST_START);
                break;

            case CON_STATE_ERROR:
                http_response.ResetResponse();
                SetErrorResponse();
                con_outbuf += http_response.GetResponse();
				if (con_outbuf.empty())	//错误信息写出去之后才关闭连接
				{
            		return false;
        		}
                return true;

            default:
                return false;
        }
    }

    return true;
}


void Connection::SetState(connection_state_t state)
{
    con_state = state;
}


void Connection::WantRead()
{
    con_want_read = true;
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event | EV_READ, Connection::ConEventCallback, this);
    event_add(con_event, NULL); 
}

void Connection::NotWantRead()
{
    con_want_read = false;
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event & (~EV_READ), Connection::ConEventCallback, this);
    event_add(con_event, NULL); 
}

void Connection::WantWrite() 
{
    con_want_write = true;
    SetWriteEvent();
}

void Connection::NotWantWrite()
{
    con_want_write = false;
    
    //不想写了，但还有未发送的数据
    if (!con_outbuf.size())     
    {
        UnsetWriteEvent();
    }
}

void Connection::SetWriteEvent()
{
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event | EV_WRITE, Connection::ConEventCallback, this);
    event_add(con_event, NULL); 
}

void Connection::UnsetWriteEvent() 
{
    short event = event_get_events(con_event);
    event_del(con_event);
    event_assign(con_event, con_worker->w_base, con_sockfd, event & (~EV_WRITE), Connection::ConEventCallback, this);
    event_add(con_event, NULL);
}

void Connection::ResetConnection()
{
	while (!req_queue.empty())
		req_queue.pop();
	con_req_cnt = 0;
}

void Connection::PrepareResponse()
{
	http_response.http_code    = 200;
	http_response.http_phrase = "ok";
	http_response.http_body = "<html><body>hello</body></html>"; //just for test
}

void Connection::SetErrorResponse()
{
	http_response.http_code = 500;
	http_response.http_phrase = "Server Error";
	http_response.http_headers["Date"] = "Fri, 27 October 2012 15:45:00 GMT";
}

request_state_t Connection::GetParsedRequest()
{
    if (!req_queue.empty())
    {
        http_req_parsed = req_queue.front();
        req_queue.pop();
        return REQ_IS_COMPLETE;
    }

    int ret = http_parser.HttpParseRequest(con_inbuf);

    if (ret == -1)
    {
        return REQ_ERROR;
    }

    if (ret == 0)	//读取当空串或者不完整的field-value对时，http-parser解析会返回0
    {
        return REQ_NOT_COMPLETE;
    }

    con_inbuf.erase(0, ret);

    if (!req_queue.empty())
    {
        http_req_parsed = req_queue.front();
        req_queue.pop();
        return REQ_IS_COMPLETE;
    }
    
    return REQ_NOT_COMPLETE;
}


bool Connection::InitPluginDataSlots()
{
    plugin_cnt = con_worker->master->m_plugin_cnt;
    
    if (!plugin_cnt)
    {
        return true;
    }

	try
	{
    	plugin_data_slots = new void*[plugin_cnt];
	}
	catch(std::bad_alloc)
	{
		std::cout << "Connection::InitPluginDataSlots()" <<std::endl;
	}

    for (int i = 0; i < plugin_cnt; ++i)
    {
        plugin_data_slots[i] = NULL;
    }

    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (!plugins[i]->Init(this, i)) 
        {
            return false;
        }
    }
    return true;
}

void Connection::FreePluginDataSlots()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (plugin_data_slots[i])
        {
            plugins[i]->Close(this, i);
        }
    }
    
    if (plugin_data_slots)
    { 
        delete []plugin_data_slots;
    }
}

bool Connection::PluginRequestStart()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (!plugins[i]->RequestStart(this, i))
		{
            return false;
        }
    }

    return true;
}

bool Connection::PluginRead()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (!plugins[i]->Read(this, i))
		{
            return false;
        }
    }

    return true;
}

bool Connection::PluginRequestEnd()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (!plugins[i]->RequestEnd(this, i))
		{
            return false;
        }
    }

	plugin_next = 0;

    return true;
}

bool Connection::PluginResponseStart()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (!plugins[i]->ResponseStart(this, i))
		{
            return false;
        }
    }

    return true;
}

/* NOTICE：if there are any plugin not ready， 
 * next time the HandleRequest() of those who 
 * are ready should not be called again.
 */
plugin_state_t Connection::PluginWrite()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = plugin_next; i < plugin_cnt; ++i)
    {
        plugin_state_t  plugin_state = plugins[i]->Write(this, i);

		plugin_next = i;

		if (plugin_state == PLUGIN_NOT_READY)
        {
            return PLUGIN_NOT_READY;
        }
        else if (plugin_state == PLUGIN_ERROR)
        {
            return PLUGIN_ERROR;
        }
    }

    return PLUGIN_READY;
}

bool Connection::PluginResponseEnd()
{
    Plugin* *plugins = con_worker->master->m_plugins;

    for (int i = 0; i < plugin_cnt; ++i)
    {
        if (!plugins[i]->ResponseEnd(this, i))
		{
            return false;
        }
    }

    return true;
}
