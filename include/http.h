/*************************************************************************
    > File Name: http.h
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 20时11分46秒
 ************************************************************************/

#ifndef _HTTP_H
#define _HTTP_H

#include <map>
#include <string>

#include "http_parser.h"

class Connection;

typedef std::map<std::string, std::string> header_t;
typedef header_t::iterator header_iter_t;

struct HttpRequest
{
	std::string	http_method;
	std::string	http_url;       
	//std::string http_version;
    
	header_t	http_headers;
	std::string	http_header_field; //field is waiting for value while parsing
    
	std::string	http_body;
};

struct HttpResponse
{
	//std::string http_version;
	int        	http_code;
	std::string	http_phrase;

	header_t	http_headers;

	std::string	http_body;

	std::string	GetResponse();
	void		ResetResponse();
};

class HttpParser
{
    public:

        void InitParser(Connection *con); 
        int  HttpParseRequest(const std::string &inbuf);

	private:

        static int OnMessageBeginCallback(http_parser *parser);
        static int OnUrlCallback(http_parser *parser, const char *at, size_t length);
        static int OnHeaderFieldCallback(http_parser *parser, const char *at, size_t length);
        static int OnHeaderValueCallback(http_parser *parser, const char *at, size_t length);
        static int OnHeadersCompleteCallback(http_parser *parser);
        static int OnBodyCallback(http_parser *parser, const char *at, size_t length);
        static int OnMessageCompleteCallback(http_parser *parser);

    private:

        http_parser		parser;
        http_parser_settings	settings;
};

#endif
