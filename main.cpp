#include "utility.h"
#include "iostream"
#include <ctime>

using namespace map;

int main()
{
	clock_t startTime, endTime;
	
	startTime = clock();//��ʱ��ʼ
	auto ap = cal_map_all("config.xml");
	endTime = clock();//��ʱ����
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
