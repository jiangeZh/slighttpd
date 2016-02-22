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
	Master master;
	if (!master.StartMaster(argc,argv))
		return -1;
	return 0;
}
