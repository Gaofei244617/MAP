#include "box.h"

namespace map
{
    // 构造函数
    Box::Box() 
        :xmin(0), ymin(0), xmax(0), ymax(0)
    {}
    // 构造函数
    Box::Box(double x1, double y1, double x2, double y2)                         
        :xmin(x1), ymin(y1), xmax(x2), ymax(y2)
    {}


    double Box::area()const
    {
        return (xmax - xmin + 1.0) * (ymax - ymin + 1.0);
    }

    double Box::width()const
    {
        return xmax - xmin;
    }

    double Box::height()const
    {
        return ymax - ymin;
    }

    std::tuple<double, double> Box::center()const
    {
        return std::make_tuple((xmin + xmax) / 2, (ymin + ymax) / 2);
    }
}