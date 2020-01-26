#pragma once
#include <tuple>

namespace map
{
    // 检测框 or 目标框
    struct Box
    {
        double xmin;        // 左上角x坐标
        double ymin;         // 左上角y坐标
        double xmax;       // 右下角x坐标 
        double ymax;      // 右下角y坐标

        Box();                         // 构造函数
        Box(double, double, double, double);

        double area()const;            // box面积
        double width()const;           // box宽度
        double height()const;          // box高度
        std::tuple<double, double> center()const; // box中心点坐标
    };
}
