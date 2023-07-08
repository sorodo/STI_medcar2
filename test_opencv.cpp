#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <stack>
#include <time.h>
#include "vision.h"
#include "serial.h"
#define CONFIDENCE 0.9
using namespace cv;
using namespace std;

class Car_map
{
public:
    int map_OL,map_OR,map_ML,map_MR,map_MLL,map_MLR,map_MRL,map_MRR; // 定义各个病房的编号
    stack<char> action; // 跑路的动作
    int flag_return; // 返程和寻路的标记
    int question_flag;  //提高题的flag
    char postion; // 现在的位置‘L，R，M，O，I, S’分别记录各个路口的名称
    clock_t stop_clk; // 避让停止用时钟
    int position_get(char *num_data);
};

int Car_map::position_get(char *num_data)
{
    map_OL = num_data[1] - '0';
    map_OR = num_data[2] - '0';
    map_ML = num_data[3] - '0';
    map_MR = num_data[4] - '0';
    map_MLL = num_data[5] - '0';
    map_MLR = num_data[6] - '0';
    map_MRL = num_data[7] - '0';
    map_MRR = num_data[8] - '0';
    return num_data[9] - '0';
}


int main()
{
    clock_t last_time,cur_time;
    last_time = clock();
    cur_time = clock();
	VideoCapture cap;
    Vision sorodo;
    Serial port;
    cout << "init" << endl;
    string output_path = "/home/sorodo/Desktop/med_car/cap_v.avi";
	cap.open(0);
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    Size videoSize (640, 480);
    VideoWriter writer(output_path, VideoWriter::fourcc('M','J','P','G'), 25, videoSize);
    Mat dst; 
    vector<int> order;
    Mat frame;
	cap >> frame;

    // FD网络的参数获取
    vector<Vec2f> num_inner;
    // 部署FastestDet
    // FastestDet m_detect(CONFIDENCE, 0.4,true);
    // m_detect.path_names="./testlfile/FastestDet.txt";
    // m_detect.path_onnx="./testlfile/FastestDet.onnx";

 
    // init
    Car_map carmap;
    carmap.postion = 'I';
    int mid_x = 0;
    int signal = 0; // 路口标识
    int goal_room = -1;  // 目标病房
    int task_flag = 0;  // 任务执行标志 0为查找病房 1为找路 2为返程

    carmap.question_flag = 0;
    while (1)
    {
        char *orders;
        orders = port.read_msg();
        if (orders[0] == 'd')
        {
            if (orders[1] == '1')
            {
                carmap.question_flag = 1;
            }
            if (orders[1] == '2')
            {
                carmap.question_flag = 2;
            }
        }

        cout << port.read_msg() << endl;

        while (carmap.question_flag == 1)
        {
            while (1)
            {
                char *room_nums;
                room_nums = port.read_msg();
                if (room_nums[0] == 'm')
                {
                    goal_room = carmap.position_get(room_nums);
                    task_flag = 1;
                    break;
                }
            }


            while (task_flag == 1)
            {
                cap >> frame;
                if (frame.empty())continue;

                cv::waitKey(1);
                cv::namedWindow("4K",1);
                cv::resize(frame, frame, Size(160, 120));
                cv::Rect m_select = Rect(20,0,140,120);

                frame = frame(m_select);
                sorodo.color_cut(frame, 'r', dst);
                cv::imshow("4K", dst);
                cv::waitKey(1);
                // 获取信息
                order = sorodo.cross_detect(dst);
                mid_x = order[0];   // 中心x
                signal = order[2];
                // 传递偏差值
                std::stringstream bias_message;
                char message[4];
                if (mid_x < 10)bias_message << "b00" << mid_x;
                else if (mid_x < 100)bias_message << "b0" << mid_x;
                else  bias_message << "b" << mid_x;
                bias_message >> message;
                port.send_msg(message);
                cout << message  << endl;

                cur_time = clock();

                if (signal == 2 and cur_time-last_time > 1500000) // 2s之内不再进入检测的循环
                { 
                    cur_time = clock();
                    last_time = clock();
                    if (carmap.postion == 'I')// 如果d第一个路口
                    {
                        port.send_msg("el");  
                        carmap.action.push('g');    // 记录第2个动作
                        carmap.postion = 'T';       // 记录任务结束
                    }
                    if (carmap.postion == 'S')// 如果再次出发
                    {
                        port.send_msg("el");  
                        carmap.postion = 'O';       
                    }
                    if (carmap.postion == 'O')// 2路口
                    {
                        carmap.action.push('g');    
                        if (goal_room == carmap.map_ML)
                        {
                            port.send_msg("el");  
                            carmap.action.push('l');    // 记录第2个动作
                            carmap.postion = 'F';       // 记录任务结束
                        }
                        else
                        {
                            port.send_msg("er");  
                            carmap.action.push('r');    // 记录第2个动作
                            carmap.postion = 'F';       // 记录任务结束
                        }
                    }
                }

                if(carmap.postion == 'T' and signal == 3 and cur_time-last_time > 1500000){ 
                    port.send_msg("ez"); // 发送暂时停靠指令
                    carmap.postion = 'S'; //新的出发
                    break;
                }   

                if(carmap.postion == 'F' && signal == 3 and cur_time-last_time > 1500000){ // 已经循迹完成且检测到终点
                        task_flag = 2;
                        port.send_msg("ef"); 
                        break;
                }
            }

            
            while (task_flag == 2) // 返程任务
            {
                cap >> frame;
                if (frame.empty())continue;
                // imshow("capread",frame);

                cv::namedWindow("result",0);
                cv::imshow("result",frame);

                resize(frame, frame, Size(160, 120));
                sorodo.color_cut(frame, 'r', dst);
                // imshow("4K", dst);
                // 获取信息
                order = sorodo.cross_detect(dst);
                mid_x = order[0];   // 中心x
                signal = order[2];
                // 传递偏差值
                std::stringstream bias_message;
                char message[4];
                if (mid_x < 10)bias_message << "b00" << mid_x;
                else if (mid_x < 100)bias_message << "b0" << mid_x;
                else  bias_message << "b" << mid_x;
                bias_message >> message;
                port.send_msg(message);

                if (signal == 3)// 先倒车
                {
                    port.send_msg("et");
                    last_time = clock();
                    cur_time = clock();
                    while (signal !=2 or cur_time-last_time < 2000000)
                    // 检测到路口或者超时时候停止
                    {
                        cap >> frame;
                        if (frame.empty())continue;
                        resize(frame, frame, Size(160, 120));
                        sorodo.color_cut(frame, 'r', dst);
                        // imshow("4K", dst);
                        // 获取信息
                        order = sorodo.cross_detect(dst);
                        mid_x = order[0];   // 中心x
                        signal = order[2];
                        // 传递偏差值
                        std::stringstream bias_message;
                        char message[4];
                        if (mid_x < 10)bias_message << "b00" << mid_x;
                        else if (mid_x < 100)bias_message << "b0" << mid_x;
                        else  bias_message << "b" << mid_x;
                        bias_message >> message;
                        port.send_msg(message);
                        cur_time = clock();
                    }
                    port.send_msg("es");
                }
        
                cur_time = clock();
                if (signal == 2 and cur_time-last_time > 10000000)
                {// 在路口且冷却时间之后
                    std::stringstream bias_message;
                    char message[4];
                    port.send_msg("es");//停止一会
                    char turn_order = carmap.action.top();
                    carmap.action.pop();
                    switch (turn_order)
                    {
                    case 'l':
                        turn_order = 'r';
                        bias_message << "e" << turn_order;
                        break;
                    case 'r':
                        turn_order = 'l';
                        bias_message << "e" << turn_order;
                        break;
                    case 'g':
                        task_flag = 3;
                        bias_message << "eg";
                        break;
                    default:
                        break;
                    }
                    
                    bias_message >> message;
                    port.send_msg(message);
                }
            } 

            while (task_flag == 3) // 最后的直线返程
            {
                cap >> frame;
                if (frame.empty())continue;
                // imshow("capread",frame);

                cv::namedWindow("result",0);
                cv::imshow("result",frame);

                resize(frame, frame, Size(160, 120));
                sorodo.color_cut(frame, 'r', dst);
                // imshow("4K", dst);
                // 获取信息
                order = sorodo.cross_detect(dst);
                mid_x = order[0];   // 中心x
                signal = order[2];
                // 停止
                if (signal == 3)
                {
                    port.send_msg("es");
                    carmap.question_flag = 0;
                    task_flag = 0;
                    break;
                }
                // 传递偏差值
                std::stringstream bias_message;
                char message[4];
                if (mid_x < 10)bias_message << "b00" << mid_x;
                else if (mid_x < 100)bias_message << "b0" << mid_x;
                else  bias_message << "b" << mid_x;
                bias_message >> message;
                port.send_msg(message);
            }
 
        }
        

        while (carmap.question_flag == 2)
        {
            while (1)
            {
                char *room_nums;
                room_nums = port.read_msg();
                if (room_nums[0] == 'm')
                {
                    goal_room = carmap.position_get(room_nums);
                    task_flag = 1;
                    break;
                }
            }
            
            while (task_flag == 1) // 寻路
            { 
                // port.send_msg("b110er");
                // cout<< "ss" << endl;
                // 读取摄像头
                cap >> frame;
                if (frame.empty())continue;

                cv::waitKey(1);
                cv::namedWindow("4K",1);
                cv::resize(frame, frame, Size(160, 120));
                cv::Rect m_select = Rect(20,0,140,120);

                frame = frame(m_select);
                sorodo.color_cut(frame, 'r', dst);
                cv::imshow("4K", dst);
                cv::waitKey(1);
                // 获取信息
                order = sorodo.cross_detect(dst);
                mid_x = order[0];   // 中心x
                signal = order[2];
                // 传递偏差值
                std::stringstream bias_message;
                char message[4];
                if (mid_x < 10)bias_message << "b00" << mid_x;
                else if (mid_x < 100)bias_message << "b0" << mid_x;
                else  bias_message << "b" << mid_x;
                bias_message >> message;
                port.send_msg(message);
                cout << message  << endl;
            
                cur_time = clock();
                cout << "curtime: " << cur_time << "last_time" << last_time << endl;
                if (signal == 2 and cur_time-last_time > 2000000) // 2s之内不再进入检测的循环
                {    // 到路口时候检测数字
                                    
                        
                    if (carmap.postion == 'I')// 如果在第1个路口
                    {
                        port.send_msg("egegeg");  // 发送直行命令
                        carmap.action.push('g');    // 记录第1个动作为直行
                        carmap.postion = 'M';
                    }
                    if (carmap.postion == 'S')// 如果再次出发
                    {
                        port.send_msg("el");  
                        carmap.postion = 'O';       
                    }
                    if (carmap.postion == 'O')// 第2个路口——避让
                    {
                        port.send_msg("elelel");  // 发送左转
                        carmap.postion = 'T';       // 更改为临时位点
                    }
                    if (carmap.postion == 'T')     //  避让之后
                    {
                        port.send_msg("elelel");  // 发送左转
                        carmap.postion = 'M';       
                    }
                    if (carmap.postion == 'M')// 如果在第3个路口
                    {
                        if(goal_room == carmap.map_MLL or goal_room == carmap.map_MLR)
                        {
                            port.send_msg("el");  
                            carmap.action.push('l');    // 记录第2个动作
                            carmap.postion = 'l';       // 更新路口位置
                        }
                        if(goal_room == carmap.map_MRL or goal_room == carmap.map_MRR)
                        {
                            port.send_msg("er");  
                            carmap.action.push('r');    // 记录第2个动作
                            carmap.postion = 'r';       // 更新路口位置
                        }
                    }
                    if (carmap.postion == 'L')// 在远端病房的左侧路口
                    {
                        if(goal_room == carmap.map_MLL)
                        {
                            port.send_msg("el");  
                            carmap.action.push('l');    // 记录第3个动作
                            carmap.postion = 'F';       // 记录任务结束
                        }
                        if(goal_room == carmap.map_MLR)
                        {
                            port.send_msg("er");  
                            carmap.action.push('r');    // 记录第3个动作
                            carmap.postion = 'F';       // 记录任务结束
                        }
                    }
                    if (carmap.postion == 'R')// 在远端病房的右侧路口
                    {
                        if(goal_room == carmap.map_MRL)
                        {
                            port.send_msg("el");  
                            carmap.action.push('l');    // 记录第3个动作
                            carmap.postion = 'F';       // 记录任务结束
                        }
                        if(goal_room == carmap.map_MRR)
                        {
                            port.send_msg("er");  
                            carmap.action.push('r');    // 记录第3个动作
                            carmap.postion = 'F';       // 记录任务结束
                        }
                    } 

                        last_time = clock();
                        cur_time = clock();
                }
                if(carmap.postion == 'T' && signal == 3 and cur_time-last_time > 1500000){ 
                    port.send_msg("ez"); // 发送暂时停靠指令
                    carmap.postion = 'S'; //新的出发
                    break;
                }   
                if(carmap.postion == 'F' && signal == 3 and cur_time-last_time > 1500000){ // 已经循迹完成且检测到终点
                    carmap.question_flag = 0;
                    task_flag = 0;
                    port.send_msg("ef"); 
                    break;
                }
                
            }

        }
        
    }
    port.send_msg("ef");
    // cout << mid_x << "///" << signal << endl;
    if (carmap.action.size()>0)
        cout << "room" << endl;
    if (waitKey(1) == 27)
        cap.release();
    
    return 0;
}
