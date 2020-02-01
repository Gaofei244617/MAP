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
	// ����ͼƬ��ground truth boxes, {obj_type:{image_name:[gt_box, ...]}}
	using GT_BOX = unordered_map<string, unordered_map<string, vector<Box>>>;
	//�������ļ����[(image_name,confidence,box), ...]
	using DETECT_BOX = vector<tuple<string, double, Box>>;
	// {"imagename" : ([Box,...], [false,...])}
	using GT_BOX_CLASS = unordered_map<string, tuple<vector<Box>, vector<bool>>>;
	// ([recall, ...], [precision, ...], [tp, ...], [fp, ...], [fn, ...], ap)
	using AP_ITEM = tuple<vector<int>, vector<int>, vector<int>, vector<double>, vector<double>, double>;
	// {"scene": {"obj_type": AP_ITEM}}
	using AP = unordered_map<string, unordered_map<string, AP_ITEM>>;

	// ���ò���
	struct Config
	{
		// {"scene": ("gt_file",{"obj_type": "det_file"})}
		using FILES = unordered_map<string, tuple<string, unordered_map<string, string>>>;
		string map_type;
		double iou_thresh;
		FILES files; 
	};

	// ����xml�����ļ�
	Config parse_configfile(const string& cfg);

	// ��ȡground truth boxes
	GT_BOX get_gt_boxes(const string& gt_boxes_file);

	// ��ȡĳһ����ļ����
	DETECT_BOX get_detect_boxes(const string& detect_file);

	// ������ȡgt box
    // ����ֵ{"imagename" : ([Box,...], [false,...])}
	GT_BOX_CLASS get_gtBox(const GT_BOX& gtBox, const string& obj_type);
	
	// ��ȡ�������Ŷ������ļ����
	// ����ֵ�����Ŷ���������, imgname_lsit, box_list��
	tuple<vector<int>, vector<string>, vector<Box>> get_sorted_det(const DETECT_BOX& det);

	// ��������box��IoU
	double cal_IoU(const Box& box_a, const Box& box_b);

	// ����AP,���[recall, ...], [precision, ...], "map_type"
	double cal_ap(vector<double>, vector<double>, string);

	// ����APֵ
	double cal_ap(vector<double> recall, vector<double> precision, const string map_type);
	
	// ����һ������һ��object type��AP�������
	AP_ITEM cal_map_one(const GT_BOX& gt_boxes, const DETECT_BOX& det_boxes, const string& obj_type, const string& map_type, double iou_thresh);
	
	// ����mAP
	AP cal_map_all(const string& cfg);
}
