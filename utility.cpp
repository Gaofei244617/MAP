#include "utility.h"
#include "3rdpart/tinyxml/tinyxml.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cstdlib>

namespace map
{
	// 解析xml配置文件
	Config parse_configfile(const string& cfg)
	{
		TiXmlDocument doc; // XML文档
		Config config;     // 存储从XML解析的配置参数

		if (!doc.LoadFile(cfg.c_str())) // 加载XML文档
		{
			std::cerr << doc.ErrorDesc() << std::endl;
			return config;
		}
		// 定义根节点变量并赋值为文档的第一个根节点
		TiXmlElement* root = doc.FirstChildElement();

		// 如果没有找到根节点,说明是空XML文档或者非XML文档
		if (root == nullptr)
		{
			std::cerr << "Failed to load file: No root element." << std::endl;
			doc.Clear(); // 清理内存
			return config;
		}

		// 遍历子节点
		for (TiXmlElement* elem = root->FirstChildElement(); elem != nullptr; elem = elem->NextSiblingElement())
		{
			std::string elem_name = elem->Value();   // 获取节点名称
			if (elem_name == "map_type")
			{
				config.map_type = elem->GetText();   // 获取节点值
				continue;
			}
			if (elem_name == "map_iou_thresh")
			{
				config.iou_thresh = std::atof(elem->GetText());   // 获取节点值
				continue;
			}

			if (elem_name == "scene")
			{
				string scene_name = elem->Attribute("name");
				string gt_box;
				unordered_map<string, string> det_box;
				for (TiXmlElement* secElem = elem->FirstChildElement(); secElem != nullptr; secElem = secElem->NextSiblingElement())
				{
					std::string sec_name = secElem->Value();
					if (sec_name == "gt_box")
					{
						gt_box = secElem->GetText();
						continue;
					}
					if (sec_name == "detection")
					{
						string obj_type = secElem->Attribute("name");
						string dt = secElem->GetText();
						det_box[obj_type] = dt;
						continue;
					}

				}
				config.files[scene_name] = make_tuple(gt_box, det_box);
				continue;
			}
		}
		return config;
	}

	// 获取ground truth boxes
	GT_BOX get_gt_boxes(const string& gt_boxes_file)
	{
		GT_BOX gt_boxes;
		// 逐行读取并解析ground truth boxes文件
		std::ifstream in(gt_boxes_file, std::ios::in);
		if (!in.is_open())
		{
			std::cout << "file: " << gt_boxes_file << " does not exists!" << std::endl;
			abort();
		}
		std::string line;
		while (getline(in, line))
		{
			std::stringstream ss(line);
			std::string image_name;
			std::string obj_type;
			Box box;
			ss >> image_name;
			ss >> obj_type;
			ss >> box.xmin;
			ss >> box.ymin;
			ss >> box.xmax;
			ss >> box.ymax;

			// 存储解析结果
			if (gt_boxes.find(obj_type) == gt_boxes.end())
			{
				gt_boxes[obj_type] = { { image_name, {box} } };
			}
			else
			{
				if (gt_boxes[obj_type].find(image_name) == gt_boxes[obj_type].end())
				{
					gt_boxes[obj_type][image_name] = { box };
				}
				else
				{
					gt_boxes[obj_type][image_name].push_back(box);
				}
			}
		}

		in.close();
		return gt_boxes;
	}

	// 按类别获取gt box
	// 返回值{"imagename" : ([Box,...], [false,...])}
	GT_BOX_CLASS get_gtBox(const GT_BOX& gtBox, const string& obj_type)
	{
		GT_BOX_CLASS ret;
		const auto& boxes = gtBox.at(obj_type);
		for (const auto& item : boxes)
		{
			ret[item.first] = make_tuple(item.second, vector<bool>(item.second.size(), false));
		}
		return ret;
	}

	// 获取某一分类的检测结果
	DETECT_BOX get_detect_boxes(const string& detect_file)
	{
		DETECT_BOX ret; // vector<tuple<string, double, Box>>
		// 逐行读取并解析ground truth boxes文件
		std::ifstream in(detect_file, std::ios::in);
		if (!in.is_open())
		{
			std::cout << "file: " << detect_file << " does not exists!" << std::endl;
			abort();
		}

		std::string line;
		std::string image_name;
		Box box;
		double confidence;
		while (getline(in, line))
		{
			std::stringstream ss(line);
			ss >> image_name;
			ss >> confidence;
			ss >> box.xmin;
			ss >> box.ymin;
			ss >> box.xmax;
			ss >> box.ymax;
			ret.push_back(make_tuple(image_name, confidence, Box(box)));
		}
		in.close();
		return ret;
	}

	// 获取按照置信度排序后的检测结果
	tuple<vector<int>, vector<string>, vector<Box>> get_sorted_det(const DETECT_BOX& det)
	{
		vector<int> sorted_index(det.size());
		vector<string> sorted_imgNames;
		vector<Box> sorted_boxes;
		std::iota(sorted_index.begin(), sorted_index.end(), 0);
		std::sort(sorted_index.begin(), sorted_index.end(), [&det](int x, int y) {return get<1>(det[x]) > get<1>(det[y]); });
		for (int index : sorted_index)
		{
			sorted_imgNames.emplace_back(get<0>(det[index]));
			sorted_boxes.push_back(get<2>(det[index]));
		}
		return make_tuple(move(sorted_index), move(sorted_imgNames), move(sorted_boxes));
	}

	// 计算两个box的IoU
	double cal_IoU(const Box& box_a, const Box& box_b)
	{
		// 两矩形中心点x方向距离(2倍)
		double dx = std::fabs(box_a.xmin + box_a.xmax - box_b.xmin - box_b.xmax);
		// 两矩形中心点y方向距离(2倍)
		double dy = std::fabs(box_a.ymin + box_a.ymax - box_b.ymin - box_b.ymax);
		// 两box宽度之和
		double sw = box_a.width() + box_b.width();
		// 两box高度之和
		double sh = box_a.height() + box_b.height();
		// 两矩形中心点的距离小于两矩形边长和的一半,则两者相交
		if (dx < sw && dy < sh)
		{
			double tl_x = std::fmax(box_a.xmin, box_b.xmin);          // 相交区域左上角x坐标
			double tl_y = std::fmax(box_a.ymin, box_b.ymin);          // 相交区域左上角y坐标
			double br_x = std::fmin(box_a.xmax, box_b.xmax);  // 相交区域右下角x坐标
			double br_y = std::fmin(box_a.ymax, box_b.ymax);  // 相交区域右下角y坐标
			double inter_area = (br_x - tl_x + 1.0) * (br_y - tl_y + 1.0);        // 相交区域面积
			return inter_area / (box_a.area() + box_b.area() - inter_area);       // IoU
		}
		return 0;
	}

	// 计算AP值
	double cal_ap(vector<double> recall, vector<double> precision, const string map_type)
	{
		double ap = 0;
		if (map_type == "VOC2007")
		{
			const vector<double> conf = { 0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
			for (double t : conf)
			{
				int count = 0;
				vector<int> index;
				vector<double> prec;
				for (int i = 0; i < recall.size(); i++)
				{
					if (recall[i] >= t)
					{
						count++;
						index.push_back(i);
						prec.push_back(precision[i]);
					}
				}
				double p = 0;
				if (count == 0)
				{
					p = 0;
				}
				else
				{
					p = *std::max(prec.begin(), prec.end());
				}

				ap = ap + p / 11.0;
			}
			return ap;
		}
		if (map_type == "VOC2012")
		{
			recall.push_back(1.0);
			recall.insert(recall.begin(), 0);
			precision.push_back(0);
			precision.insert(precision.begin(), 0);
			for (auto i = precision.size() - 1; i > 0; i--)
			{
				precision[i - 1] = std::max(precision[i - 1], precision[i]);
			}
			for (int i = 0; i < precision.size() - 1; i++)
			{
				ap = ap + (recall[i + 1] - recall[i]) * precision[i + 1];
			}
			return ap;
		}
		return ap;
	}
	
	// 计算一个场景一个object type下AP相关数据
	AP_ITEM cal_map_one(const GT_BOX& gt_boxes, const DETECT_BOX& det_boxes, const string& obj_type, const string& map_type, double iou_thresh)
	{
		auto det = get_gtBox(gt_boxes, obj_type);

		vector<int> sorted_index;             // 按置信度降序排序后的下标索引
		vector<string> sorted_imgNames;       // 按置信度降序排序后的图片名称
		vector<Box> sorted_boxes;             // 按置信度降序排序后的检测框
		std::tie(sorted_index, sorted_imgNames, sorted_boxes) = get_sorted_det(det_boxes);

		const auto len = sorted_boxes.size();  // 检测框数量 
		vector<int> tp(len, 0);
		vector<int> fp(len, 0);
		vector<int> fn(len, 0);
		// 用每一个预测的目标框去匹配真实目标框
		for (int i = 0; i < len; i++)
		{
			vector<Box>& gt_bb = std::get<0>(det[sorted_imgNames[i]]);      // 预测框所在图片的所有gt box
			vector<bool>& det_flag = std::get<1>(det[sorted_imgNames[i]]);  // gt box是否被检出标志位
			Box det_box = sorted_boxes[i];    // 预测的目标框
			if (gt_bb.size() > 0)
			{
				vector<double> iou(gt_bb.size(), 0);  // 检测框与图片中所有gt box的IoU
				for (int k = 0; k < gt_bb.size(); k++)
				{
					iou[k] = cal_IoU(gt_bb[k], det_box);
				}
				auto max_iter = std::max_element(iou.begin(), iou.end()); // 最大IoU
				auto jmax = std::distance(iou.begin(), max_iter); // 最大IoU的位置（匹配到的gt box的位置）
				if (*max_iter > iou_thresh)
				{
					if (det_flag[jmax] == false)
					{
						tp[i] = 1;
						det_flag[jmax] = true;
					}
					else
					{
						fp[i] = 1;
					}
				}
				else
				{
					fp[i] = 1;
				}
			}
		}

		int gtBox_num = 0;                    // gt box数量
		for (const auto& item : det)
		{
			gtBox_num += int(std::get<0>(item.second).size());
		}

		vector<double> recall(len, 0);
		vector<double> precision(len, 0);
		recall[0] = tp[0] / static_cast<double>(gtBox_num);
		precision[0] = tp[0] / static_cast<double>(tp[0] + fp[0]);
		for (int i = 1; i < len; i++)
		{
			tp[i] = tp[i] + tp[i - 1];
			fp[i] = fp[i] + fp[i - 1];
			recall[i] = tp[i] / static_cast<double>(gtBox_num);
			precision[i] = tp[i] / static_cast<double>(tp[i] + fp[i]);
		}
		// 计算fn
		for (size_t i = 0; i < tp.size(); i++)
		{
			fn[i] = gtBox_num - tp[i];
		}
		double ap = cal_ap(recall, precision, map_type);
		return make_tuple(move(recall), move(precision), move(tp), move(fp), move(fn), ap);
	}

	AP cal_map_all(const string& cfg)
	{
		const Config config = parse_configfile(cfg);
		const double iou_thresh = config.iou_thresh;
		const string map_type = config.map_type;
		AP ret;   	// {"scene": {"obj_type": AP_ITEM}}
		for (const auto& item : config.files)  // config.files: {"scene": ("gt_file",{"obj_type": "det_file"})}
		{
			string scene = item.first;
			auto gt_boxes = get_gt_boxes(std::get<0>(item.second));
			for (const auto& it : std::get<1>(item.second))
			{
				string obj_type = it.first;
				auto det_boxes = get_detect_boxes(it.second);
				auto ap_item = cal_map_one(gt_boxes, det_boxes, obj_type, map_type, iou_thresh);
				if (ret.find(scene) == ret.end())
				{
					ret[scene] = { {obj_type , std::move(ap_item)} };
				}
				else
				{
					ret[scene][obj_type] = std::move(ap_item);
				}
			}
		}
		return ret;
	}
}

