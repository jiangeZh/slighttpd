/*************************************************************************
    > File Name: main.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时29分26秒
 ************************************************************************/

#include "master.h"

#include <iostream>

int main(int argc, char * argv[])
{
	Master master("0.0.0.0", 8000);
	std::cout << "----Slighttpd	0.0.0.0	8000----" << std::endl;
	if (!master.StartMaster())
		return -1;
	return 0;
}
