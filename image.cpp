#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "haar.hpp"

using namespace cv;
using namespace std;

#define SAMPLE_COLS 21
#define SAMPLE_ROWS 24
#define DIMENSION 

vector<float> calcuHaarFeature_sample(Mat sample, float *raw_feature, int compactSize) //提取单个样本特征的函数
{
    Mat equalized;
    cv::equalizeHist(sample, equalized);
    Mat inte;
    cv::integral(equalized, inte);

    u32 * ptr_inte = (u32 *)inte.data;
    int status = calcuHaarFeature3(ptr_inte, sample.cols, sample.rows);
    vector<float> features(raw_feature, raw_feature+compactSize);
    return features;
}

vector<vector<int> > calcuHaarFeature_image(Mat image, int *raw_feature, int compactSize, int sample_cols, int sample_rows, int offset_x, int offset_y) //提取待检测图像特征的函数
{
    Mat inte;
    cv::integral(image, inte);

    vector<vector<int> > tile_feature;

    for(int i=0; i+sample_rows<image.rows; i+=offset_y)
        for(int j=0; j+sample_cols<image.cols; j+=offset_x)
        {
            Mat roi(image, Rect(j, i, sample_cols, sample_rows));
            Mat tile = roi.clone();

            u32 * ptr_inte = (u32 *)inte.data;
            int status = calcuHaarFeature3(ptr_inte, sample_cols, sample_rows);
            vector<int> features(raw_feature, raw_feature+compactSize);
            tile_feature.push_back(features);
        }

    return tile_feature;
}

vector<Mat> read_image_list(const char * filename)
{
    char buffer[256];
    fstream list_file;
    list_file.open(filename, ios::in);
    vector<Mat> image_list;
    string pgm(".pgm");
    while(!list_file.eof())
    {
        list_file.getline(buffer, 256, '\n');
        string file(buffer);
        if(file.length()>4 && 0 == file.compare(file.length()-4, 4, pgm))
        {
            Mat t = imread(buffer, 0);
            if(t.cols != SAMPLE_COLS || t.rows != SAMPLE_ROWS)
            {
                printf("%s\n", buffer);
                cout<<t.cols<<","<<t.rows<<endl;
                exit(1);
            }
            image_list.push_back(t);
        }
    }
    list_file.close();
    return image_list;
}

int prepare_sample(vector<vector<float> > &sample, vector<int> &flag)
{
    vector<Mat> pos_sample = read_image_list("./positive.txt");
    vector<Mat> nag_sample = read_image_list("./nagetive.txt");
    vector<int> pos_flag(pos_sample.size(), 1);
    vector<int> nag_flag(nag_sample.size(), 0);
    flag.insert(flag.end(), pos_flag.begin(), pos_flag.end());
    flag.insert(flag.end(), nag_flag.begin(), nag_flag.end());

    float *raw_feature;
    int compactSize;
    prepare(&raw_feature, &compactSize, SAMPLE_COLS, SAMPLE_ROWS);

    for(vector<Mat>::iterator i=pos_sample.begin(); i!=pos_sample.end(); ++i)
    {
        vector<float> features = calcuHaarFeature_sample(*i, raw_feature, compactSize);
        sample.push_back(features);
    }

    for(vector<Mat>::iterator i=nag_sample.begin(); i!=nag_sample.end(); ++i)
    {
        vector<float> features = calcuHaarFeature_sample(*i, raw_feature, compactSize);
        sample.push_back(features);
    }
    
    post_calculate();
    return 0;
}

/*
int main()
{
    vector<vector<float> > tx;
    vector<int> ty;
    prepare_sample(tx, ty);
    cout<<tx.size()<<endl;
    cout<<tx[0].size()<<endl;

    return 0;
}
*/

/*
int main_deprecated(int argc, char *argv[])
{
    //==========================prepare sample image========================
    //Mat sample = imread("sample.bmp");
    Mat sample = cv::Mat::zeros(20, 20, CV_8UC1);
    for(int i=0; i<20; ++i)
    {
        sample.col(i).setTo(2*i);
    }
    //for(int i=0; i<sample.rows; ++i)
    //{
    //    for(int j=0; j<sample.cols; ++j)
    //    {
    //        cout<<(int)*(ptr_sample+i*sample.cols+j)<<"\t";
    //    }
    //    cout<<endl;
    //}
    //cout<<endl;
    //
    cout<<sample.rows<<" "<<sample.cols<<endl;
    int width = sample.cols;
    int height = sample.rows;
    u8 * ptr_sample = (u8 *)sample.data;

    int *raw_feature;
    int compactSize;
    prepare(&raw_feature, &compactSize, width, height); //分配内存，返回的是特征的存贮空间指针，以及特征的长度
                                                        //raw_feature 特征的存储空间的指针
                                                        //compactSize 特征的长度
                                                        //width, height 样本宽、高
                                                        
    cout<<"prepare complete"<<endl;

    vector<int> features = calcuHaarFeature_sample(sample, raw_feature, compactSize);
    //vector<int> features2 = calcuHaarFeature_sample(sample2, raw_feature, compactSize); //第二个样本同理

    //vector<vector<int> > all_sample_feature //将上面计算的features 全部存到这个容器中以便权哥的训练函数进行处理
    post_calculate(); //free memory

    return 0;
}

//如果是要计算带检测图像的特征如下调用
int main2(int argc, char *argv[])
{
    Mat image_grayscale = imread("image.jpg"); //待检测图像处理成灰度图

    int *raw_feature;
    int compactSize;
    int sample_cols = 21; // 我给出来的所有的样本都是这个尺寸，这也是检测器的尺寸
    int sample_rows = 28;
    prepare(&raw_feature, &compactSize, sample_cols, sample_rows); //分配内存，返回的是特征的存贮空间指针，以及特征的长度
                                                        //raw_feature 特征的存储空间的指针
                                                        //compactSize 特征的长度
                                                        //width, height 样本宽、高

    int offset_x = 6;
    int offset_y = 8; //检测器每次位移的像素个数

    vector<vector<int> > all_tile_features = calcuHaarFeature_image(image_grayscale, raw_feature, compactSize, sample_cols, sample_rows, offset_x, offset_y);
    //返回的是检测遍历图像得到的所有特征，把这些特征一次通过权哥代码训练出的检测器
    //需要将不同缩放大小的图像依次传给该函数，因为检测器只能检测特定大小的人脸

    post_calculate(); //free memory

    return 0;
}
*/