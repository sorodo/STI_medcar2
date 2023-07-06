//
// Created by wsc on 2023/1/13.
//

#include "FastestDet.h"

FastestDet::FastestDet(float  confThreshold, float nmsThreshold, bool drawOutput){
    this->confThreshold=confThreshold;
    this->nmsThreshold=nmsThreshold;
    this->drawOutput=drawOutput;
}

void FastestDet::m_transfer(Mat &input, vector<Mat> &vector_mat) {

    int h=input.size[1];
    int w= input.size[2];
    int c= input.size[3];

    Mat temp(c,h,CV_32FC1);

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < c; j++) {
            for (int k = 0; k < h; k++) {
                temp.ptr<float>(j)[k]=input.ptr<float>(0,k,i)[j];
            }
        }
        vector_mat.push_back(temp.clone());
    }
}

float FastestDet::sigmoid(float x)
{
    return 1.0/(1.0+ exp(-x));
}

void FastestDet::draw_predict(Mat &frame, int x1, int y1, int x2, int y2, float score, string className) {
    rectangle(frame,Point(x1,y1),Point(x2,y2),Scalar(0,0,255),2);
    char temp[20];
    sprintf(temp,"%.2f",score);
    string tempstr(temp);
    string text= tempstr+className;
    //cout<<text<<endl;
    putText(frame,text,Point(x1,y1-10),\
            FONT_HERSHEY_SIMPLEX,1,Scalar(0,0,255),2);
}

vector<Vec4f> FastestDet::post_process(Mat &frame, Mat &outs) {

    int frameHeight=frame.rows;
    int frameWidth=frame.cols;

    vector<Mat> mats;
    vector<Rect> boxes;
    vector<float> scores;
    vector<int> indices;
    vector<Vec6f> preds;
    vector<Vec4f> ret;

    this->m_transfer(outs,mats);

    for (int i=0; i<mats.size(); i++) {
        float obj_score,cls_score,score,classID;
        float x_offset,y_offset;
        float box_width,box_height;
        float box_cx,box_cy;
        float x1,x2,y1,y2;
        double maxValue;
        Point maxIdx;

        for (int j = 0; j < mats[i].rows; j++) {
            obj_score=mats[i].ptr<float>(j)[0];
            Mat temp=mats[i](Range(j,j+1),Range(5,mats[i].cols));
            minMaxLoc(temp, NULL, &maxValue, NULL, &maxIdx);
            cls_score=(float)maxValue;
            score= pow(obj_score,0.6)* pow(cls_score,0.4);

            if(score>this->confThreshold) {

                x_offset= tanh(mats[i].ptr<float>(j)[1]);
                y_offset= tanh(mats[i].ptr<float>(j)[2]);

                box_width= this->sigmoid(mats[i].ptr<float>(j)[3]);
                box_height= this->sigmoid(mats[i].ptr<float>(j)[4]);

                box_cx=(j+x_offset)/22.0;
                box_cy=(i+y_offset)/22.0;

                x1=box_cx-0.5*box_width;    y1=box_cy-0.5*box_height;
                x2=box_cx+0.5*box_width;    y2=box_cy+0.5*box_height;

                x1=int(x1*frameWidth); x2=int(x2*frameWidth);
                y1=int(y1*frameHeight);  y2=int(y2*frameHeight);

                preds.push_back(Vec6f(x1,y1,x2,y2,score,maxIdx.x));
                boxes.push_back(Rect(x1,y1,x2-x1,y2-y1));
                scores.push_back(score);
            }
        }
    }

    dnn::NMSBoxes(boxes,scores, this->confThreshold,this->nmsThreshold,indices);

    for(auto item:indices)
    {
        Vec6f pred=preds[item];
        float center_x,center_y;
        center_x=(pred[0]+pred[2])/2;
        center_y=(pred[1]+pred[3])/2;
        if(this->drawOutput)
            this->draw_predict(frame,pred[0],pred[1],pred[2],pred[3],pred[4],this->classNames[pred[5]]);
        ret.push_back(Vec4f(center_x,center_y,pred[4],pred[5]));
    }
    return ret;
}

vector<Vec4f> FastestDet::detect(Mat &frame) {

    Mat blob = dnn::blobFromImage(frame,1.0/255 , Size(this->inputWidth, this->inputHeight));
    this->net.setInput(blob);
    Mat predict = this->net.forward();
    return this->post_process(frame,predict);
}

vector<String> FastestDet::readClassNames(string &classNamesPath)
{
    std::vector<String> classNames;

    std::ifstream fp(classNamesPath);
    if (!fp.is_open())
    {
        cout<<"could not open file,please check your file path."<<endl;
        exit(-1);
    }
    std::string name;
    while (!fp.eof())
    {
        std::getline(fp, name);
        //cout<<name<<endl;
        if (name.length())
            classNames.push_back(name);
    }
    fp.close();
    return classNames;
}


