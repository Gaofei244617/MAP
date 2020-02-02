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
	// ����xml�����ļ�
	Config parse_configfile(const string& cfg)
	{
		TiXmlDocument doc; // XML�ĵ�
		Config config;     // �洢��XML���������ò���

		if (!doc.LoadFile(cfg.c_str())) // ����XML�ĵ�
		{
			std::cerr << doc.ErrorDesc() << std::endl;
			return config;
		}
		// ������ڵ��������ֵΪ�ĵ��ĵ�һ�����ڵ�
		TiXmlElement* root = doc.FirstChildElement();

		// ���û���ҵ����ڵ�,˵���ǿ�XML�ĵ����߷�XML�ĵ�
		if (root == nullptr)
		{
			std::cerr << "Failed to load file: No root element." << std::endl;
			doc.Clear(); // �����ڴ�
			return config;
		}

		// �����ӽڵ�
		for (TiXmlElement* elem = root->FirstChildElement(); elem != nullptr; elem = elem->NextSiblingElement())
		{
			std::string elem_name = elem->Value();   // ��ȡ�ڵ�����
			if (elem_name == "map_type")
			{
				config.map_type = elem->GetText();   // ��ȡ�ڵ�ֵ
				continue;
			}
			if (elem_name == "map_iou_thresh")
			{
				config.iou_thresh = std::atof(elem->GetText());   // ��ȡ�ڵ�ֵ
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

	// ��ȡground truth boxes
	GT_BOX get_gt_boxes(const string& gt_boxes_file)
	{
		GT_BOX gt_boxes;
		// ���ж�ȡ������ground truth boxes�ļ�
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

			// �洢�������
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

	// ������ȡgt box
	// ����ֵ{"imagename" : ([Box,...], [false,...])}
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

	// ��ȡĳһ����ļ����
	DETECT_BOX get_detect_boxes(const string& detect_file)
	{
		DETECT_BOX ret; // vector<tuple<string, double, Box>>
		// ���ж�ȡ������ground truth boxes�ļ�
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

	// ��ȡ�������Ŷ������ļ����
	tuple<vector<int>, vector<double>, vector<string>, vector<Box>> get_sorted_det(const DETECT_BOX& det)
	{
		vector<int> sorted_index(det.size());
		vector<double> sorted_scores;
		vector<string> sorted_imgNames;
		vector<Box> sorted_boxes;
		std::iota(sorted_index.begin(), sorted_index.end(), 0);
		std::sort(sorted_index.begin(), sorted_index.end(), [&det](int x, int y) {return get<1>(det[x]) > get<1>(det[y]); });
		for (int index : sorted_index)
		{
			sorted_scores.push_back(get<1>(det[index]));
			sorted_imgNames.emplace_back(get<0>(det[index]));
			sorted_boxes.push_back(get<2>(det[index]));
		}
		return make_tuple(move(sorted_index), move(sorted_scores), move(sorted_imgNames), move(sorted_boxes));
	}

	// ��������box��IoU
	double cal_IoU(const Box& box_a, const Box& box_b)
	{
		// ���������ĵ�x�������(2��)
		double dx = std::fabs(box_a.xmin + box_a.xmax - box_b.xmin - box_b.xmax);
		// ���������ĵ�y�������(2��)
		double dy = std::fabs(box_a.ymin + box_a.ymax - box_b.ymin - box_b.ymax);
		// ��box���֮��
		double sw = box_a.width() + box_b.width();
		// ��box�߶�֮��
		double sh = box_a.height() + box_b.height();
		// ���������ĵ�ľ���С�������α߳��͵�һ��,�������ཻ
		if (dx < sw && dy < sh)
		{
			double tl_x = std::fmax(box_a.xmin, box_b.xmin);          // �ཻ�������Ͻ�x����
			double tl_y = std::fmax(box_a.ymin, box_b.ymin);          // �ཻ�������Ͻ�y����
			double br_x = std::fmin(box_a.xmax, box_b.xmax);  // �ཻ�������½�x����
			double br_y = std::fmin(box_a.ymax, box_b.ymax);  // �ཻ�������½�y����
			double inter_area = (br_x - tl_x + 1.0) * (br_y - tl_y + 1.0);        // �ཻ�������
			return inter_area / (box_a.area() + box_b.area() - inter_area);       // IoU
		}
		return 0;
	}
	
	// ����APֵ VOC2007
	double cal_ap_voc2007(vector<double> recall, vector<double> precision)
	{
		double ap = 0;
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

	// ����APֵ VOC2012
	double cal_ap_voc2012(vector<double> recall, vector<double> precision)
	{
		double ap = 0;
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
	
	// ����һ������һ��object type��AP�������
	AP_ITEM cal_map_one(const GT_BOX& gt_boxes, const DETECT_BOX& det_boxes, const string& obj_type, const string& map_type, double iou)
	{
		auto det = get_gtBox(gt_boxes, obj_type);

		vector<int> sorted_index;             // �����ŶȽ����������±�����
		vector<double> sorted_scores;         // ���ŶȽ�������
		vector<string> sorted_imgNames;       // �����ŶȽ���������ͼƬ����
		vector<Box> sorted_boxes;             // �����ŶȽ��������ļ���
		std::tie(sorted_index, sorted_scores, sorted_imgNames, sorted_boxes) = get_sorted_det(det_boxes);

		vector<double> iou_vec;
		if (map_type == "COCO")
		{
			iou_vec = {0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95};
		}
		else
		{
			iou_vec = {iou};
		}

		double ap = 0;
		const auto len = sorted_boxes.size();  // ��������
		vector<int> tp;
		vector<int> fp;
		vector<int> fn;
		vector<double> recall(len, 0);
		vector<double> precision(len, 0);
		for (const double iou_thresh : iou_vec)
		{
			tp.assign(len, 0);
			fp.assign(len, 0);
			fn.assign(len, 0);
			// ��ÿһ��Ԥ���Ŀ���ȥƥ����ʵĿ���
			for (int i = 0; i < len; i++)
			{
				vector<Box>& gt_bb = std::get<0>(det[sorted_imgNames[i]]);      // Ԥ�������ͼƬ������gt box
				vector<bool>& det_flag = std::get<1>(det[sorted_imgNames[i]]);  // gt box�Ƿ񱻼����־λ
				det_flag.assign(det_flag.size(), false);    // ���ݳ�ʼ��
				Box det_box = sorted_boxes[i];    // Ԥ���Ŀ���
				if (gt_bb.size() > 0)
				{
					vector<double> iou(gt_bb.size(), 0);  // ������ͼƬ������gt box��IoU
					for (int k = 0; k < gt_bb.size(); k++)
					{
						iou[k] = cal_IoU(gt_bb[k], det_box);
					}
					auto max_iter = std::max_element(iou.begin(), iou.end()); // ���IoU
					auto jmax = std::distance(iou.begin(), max_iter); // ���IoU��λ�ã�ƥ�䵽��gt box��λ�ã�
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

			int gtBox_num = 0;                    // gt box����
			for (const auto& item : det)
			{
				gtBox_num += int(std::get<0>(item.second).size());
			}

			vector<double> rec(len, 0);
			vector<double> prec(len, 0);
			rec[0] = tp[0] / static_cast<double>(gtBox_num);
			prec[0] = tp[0] / static_cast<double>(tp[0] + fp[0]);
			for (int i = 1; i < len; i++)
			{
				tp[i] = tp[i] + tp[i - 1]; // cumsum
				fp[i] = fp[i] + fp[i - 1]; // cumsum
				rec[i] = tp[i] / static_cast<double>(gtBox_num);
				prec[i] = tp[i] / static_cast<double>(tp[i] + fp[i]);
			}
			double p = 0;
			if (map_type == "VOC2007")
			{
				p = cal_ap_voc2007(rec, prec);
			}
			else
			{
				p = cal_ap_voc2012(rec, prec);
			}
			ap = ap + p / iou_vec.size();

			// ����recall, precision, fn
			for (size_t i = 0; i < len; i++)
			{
				recall[i] = recall[i] + rec[i] / iou_vec.size();
				precision[i] = precision[i] + prec[i] / iou_vec.size();
				fn[i] = gtBox_num - tp[i];
			}
		}
		if (map_type == "COCO")
		{
			return make_tuple(move(sorted_scores), vector<int>(), vector<int>(), vector<int>(), move(recall), move(precision), ap);
		}
		return make_tuple(move(sorted_scores), move(tp), move(fp), move(fn), move(recall), move(precision), ap);
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

