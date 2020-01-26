#pragma once
#include <tuple>

namespace map
{
    // ���� or Ŀ���
    struct Box
    {
        double xmin;        // ���Ͻ�x����
        double ymin;         // ���Ͻ�y����
        double xmax;       // ���½�x���� 
        double ymax;      // ���½�y����

        Box();                         // ���캯��
        Box(double, double, double, double);

        double area()const;            // box���
        double width()const;           // box���
        double height()const;          // box�߶�
        std::tuple<double, double> center()const; // box���ĵ�����
    };
}
