//
// Created by wsc on 2023/1/13.
//

#ifndef TESTONNX_FASTDET_H
#define TESTONNX_FASTDET_H

#include "opencv2/core.hpp"
#include "opencv2/ml.hpp"
#include "opencv2/dnn.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "iostream"
#include <fstream>

using namespace std;
using namespace cv;
class FastestDet {

public:
    string path_names="../testfile/FastestDet.txt";
    string path_onnx="../testfile/FastestDet.onnx";
    int inputWidth=352;
    int inputHeight=352;
    float confThreshold;
    float nmsThreshold;
    bool drawOutput;

    cv::dnn::Net net=cv::dnn::readNetFromONNX(path_onnx);
    vector<String> classNames= readClassNames(path_names);


    FastestDet(float  confThreshold=0.5, float nmsThreshold=0.4, bool drawOutput=false);
    float sigmoid(float x);
    void m_transfer(Mat &input,vector<Mat> &vector_mat);
    void draw_predict(Mat &frame,int x1,int y1,int x2,int y2,float score,string className);
    vector<Vec4f> post_process(Mat &frame,Mat &outs);
    vector<String> readClassNames(string &classNamesPath);
    vector<Vec4f> detect(Mat &frame);

};


#endif //TESTONNX_FASTDET_H
