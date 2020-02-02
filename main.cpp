#include "utility.h"
#include "iostream"
#include <ctime>

using namespace map;

int main()
{
	clock_t startTime, endTime;
	
	startTime = clock();//计时开始
	auto ap = cal_map_all("config.xml");
	endTime = clock();//计时结束
	for (auto scene : ap)
	{
		for (auto item : scene.second)
		{
			string name = item.first;
			double a = std::get<6>(item.second);
			std::cout << name << "  " << a << std::endl;
		}
	}
	std::cout << "The run time is: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << std::endl;
}
