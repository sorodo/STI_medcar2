#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include "vision.h"
#define THRESHOLD_GRAY 120
#define RED 'r'
#define WIDTH 4
using namespace cv;
using namespace std;

/*=====================图像的简单预处理===================*/
// 开运算
Mat Vision::open_sum(Mat img, int eff)
{
    Mat dstImage;
    Mat element = getStructuringElement(MORPH_RECT, Size(eff, eff));
    morphologyEx(img, dstImage, MORPH_OPEN, element);
    return dstImage;
}

//闭运算
Mat Vision::close_sum(Mat img, int eff)
{
    Mat dstImage;
    Mat element = getStructuringElement(MORPH_RECT, Size(eff, eff));
    morphologyEx(img, dstImage, MORPH_CLOSE, element);
    return dstImage;
}

//图像基本处理
Mat Vision::filter_line(Mat img, int eff)
{
    Mat gray_img, gray_binary;
    cvtColor(img, gray_img, COLOR_BGR2GRAY);
    threshold(gray_img, gray_binary, THRESHOLD_GRAY, 255, THRESH_BINARY_INV); // >155设置为0
    Mat dstImage;
    Mat element = getStructuringElement(MORPH_RECT, Size(eff, eff));
    morphologyEx(gray_binary, dstImage, MORPH_OPEN, element);
    morphologyEx(gray_binary, dstImage, MORPH_CLOSE, element);
    return dstImage;
}

//颜色分割
void Vision::color_cut(InputArray img, char color, OutputArray dst)
{
    Scalar red0_low = Scalar(0, 80, 80);
    Scalar red1_low = Scalar(160, 80, 80);
    Scalar red0_up = Scalar(10, 255, 250);
    Scalar red1_up = Scalar(180, 255, 250);
    Mat hsv_img;
    cvtColor(img, hsv_img, COLOR_BGR2HSV);
    if (color == RED)
    {
        Mat red_mask0, red_mask1;
        inRange(hsv_img, red0_low, red0_up, red_mask0);
        inRange(hsv_img, red1_low, red1_up, red_mask1);
        bitwise_or(red_mask0, red_mask1, dst);
        Mat element = getStructuringElement(MORPH_RECT, Size(3, 3));
        morphologyEx(dst, dst, MORPH_OPEN, element);
        morphologyEx(dst, dst, MORPH_CLOSE, element);
    }
}

//找寻最轮廓中心
float* Vision::find_line(Mat binary_img)
{
    return 0;
}

//路口与各个特征的识别
vector<int> Vision::cross_detect(Mat binary_img)
{
    int width = binary_img.cols;
    int height = binary_img.rows;
    int up = 0, down = 0, left = 0, right = 0; 
    int sig_up = 0, sig_down = 0, sig_left = 0, sig_right = 0;
    int mid_x = 60, mid_y = 0, signal = 0; // singal_0表示停止
    for (int i = 0; i < width; i++)
    {
        int index = i;
        int data = (int)binary_img.data[index];
        if (data == 255)
        {
            sig_up += index;
            up++;
        }
    }
    for (int i = 0; i < width; i++)
    {
        int index = i + width * (height - 1);
        int data = (int)binary_img.data[index];
        if (data == 255)
        {
            sig_down += index;
            down++;
        }
    }
    for (int i = 0; i < height; i++)
    {
        int index = i * width;
        int data = (int)binary_img.data[index + 10];
        if (data == 255) 
        {
            sig_left += i;
            left++;
        }
    }
    for (int i = 0; i < height; i++)
    {
        int index = (i + 1) * width - 1;
        int data = (int)binary_img.data[index - 10];
        if (data == 255) 
        {
            sig_right += i;
            right++;
        }
    }
   
    if (up)
    {
        mid_x = int(sig_up/up);
    }
    
    if (up > WIDTH and left < WIDTH and right < WIDTH)
        signal = 1; //1表示前进
    if ((left > WIDTH and right > WIDTH and up > WIDTH) or
    ((down > WIDTH)and
    ((left > WIDTH and left < WIDTH + 4) or 
    (right > WIDTH and right < WIDTH + 4))))
        signal = 2; //2表示路口
    if (up == 0 and left == 0 and right == 0)
        signal = 3; //3表示终点
    vector<int> order = {mid_x, 0, signal};
    // cout << signal << " , " << mid_x << " , " << up << " "<< sig_up << endl;
    return order;
}
