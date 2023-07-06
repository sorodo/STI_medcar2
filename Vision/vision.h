#ifndef __VISION_H__
#define __VISION_H__

#include <opencv2/opencv.hpp>
#include <vector>
using namespace cv;
using namespace std;

class Vision
{
private:
    /* data */
public:
    // Vision(/* args */);
    // ~Vision();
    Mat open_sum(Mat img, int eff);
    Mat close_sum(Mat img, int eff);
    Mat filter_line(Mat img, int eff);
    void color_cut(InputArray img, char color, OutputArray dst);
    float* find_line(Mat binary_img);
    vector<int> cross_detect(Mat binary_img);
};

// Vision::Vision(/* args */)
// {
// }

// Vision::~Vision()
// {
// }


#endif