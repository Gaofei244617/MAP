#pragma once

#include "box.h"
#include <cmath>
#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>

namespace map
{
	using namespace std;
	// 所有图片的ground truth boxes, {obj_type:{image_name:[gt_box, ...]}}
	using GT_BOX = unordered_map<string, unordered_map<string, vector<Box>>>;
	//所有类别的检测结果[(image_name,confidence,box), ...]
	using DETECT_BOX = vector<tuple<string, double, Box>>;
	// {"imagename" : ([Box,...], [false,...])}
	using GT_BOX_CLASS = unordered_map<string, tuple<vector<Box>, vector<bool>>>;
	// ([recall, ...], [precision, ...], [tp, ...], [fp, ...], [fn, ...], ap)
	using AP_ITEM = tuple<vector<int>, vector<int>, vector<int>, vector<double>, vector<double>, double>;
	// {"scene": {"obj_type": AP_ITEM}}
	using AP = unordered_map<string, unordered_map<string, AP_ITEM>>;

	// 配置参数
	struct Config
	{
		// {"scene": ("gt_file",{"obj_type": "det_file"})}
		using FILES = unordered_map<string, tuple<string, unordered_map<string, string>>>;
		string map_type;
		double iou_thresh;
		FILES files; 
	};

	// 解析xml配置文件
	Config parse_configfile(const string& cfg);

	// 获取ground truth boxes
	GT_BOX get_gt_boxes(const string& gt_boxes_file);

	// 获取某一分类的检测结果
	DETECT_BOX get_detect_boxes(const string& detect_file);

	// 按类别获取gt box
    // 返回值{"imagename" : ([Box,...], [false,...])}
	GT_BOX_CLASS get_gtBox(const GT_BOX& gtBox, const string& obj_type);
	
	// 获取按照置信度排序后的检测结果
	// 返回值（置信度排序索引, imgname_lsit, box_list）
	tuple<vector<int>, vector<string>, vector<Box>> get_sorted_det(const DETECT_BOX& det);

	// 计算两个box的IoU
	double cal_IoU(const Box& box_a, const Box& box_b);

	// 计算AP,入参[recall, ...], [precision, ...], "map_type"
	double cal_ap(vector<double>, vector<double>, string);

	// 计算AP值
	double cal_ap(vector<double> recall, vector<double> precision, const string map_type);
	
	// 计算一个场景一个object type下AP相关数据
	AP_ITEM cal_map_one(const GT_BOX& gt_boxes, const DETECT_BOX& det_boxes, const string& obj_type, const string& map_type, double iou_thresh);
	
	// 计算mAP
	AP cal_map_all(const string& cfg);
}
