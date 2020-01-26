#include "utility.h"
#include "iostream"
#include<ctime>

using namespace map;

int main()
{
	clock_t startTime, endTime;
	
	Config config = parse_configfile();	  // ���������ļ�

	startTime = clock();//��ʱ��ʼ
	auto ap = cal_map_all(config);
	endTime = clock();//��ʱ����
	for (auto scene : ap)
	{
		for (auto item : scene.second)
		{
			string name = item.first;
			double a = std::get<5>(item.second);
			std::cout << name << "  " << a << std::endl;
		}
	}
	std::cout << "The run time is: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << std::endl;
}