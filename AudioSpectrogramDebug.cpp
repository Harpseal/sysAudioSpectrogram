#include "iCV.h"
#include <math.h>
#include <typeinfo>
#include <assert.h>
#include <string>

template<class T> void ShowPCM(T* pData,int nData,int nChannels,int maxWidth,int rowHeight,char* name)
{
	const int margin = 10;
	if (maxWidth&0x3)
		maxWidth = maxWidth&(~0x3) + 4;
	std::string winName = "debugPCM_";

	int row = ceil(float(nData)/maxWidth);
	IplImage *pImg = cvCreateImage(cvSize(maxWidth,rowHeight*row+margin*(row+1)),IPL_DEPTH_8U,3);
	cvSetZero(pImg);
	float value;
	int ypos,xpos;
	for (int r=0;r<row;r++)
	{
		ypos = margin*(r+1) + rowHeight*r;
		cvLine(pImg,cvPoint(0,ypos),cvPoint(pImg->width-1,ypos),CV_RGB(255,255,255),1);
		ypos += rowHeight/2;
		cvLine(pImg,cvPoint(0,ypos),cvPoint(pImg->width-1,ypos),CV_RGB(100,100,100),1);
		ypos += rowHeight/2;
		cvLine(pImg,cvPoint(0,ypos),cvPoint(pImg->width-1,ypos),CV_RGB(255,255,255),1);
	}

	if (typeid(double) == typeid(T))
		winName+="double_";
	else if (typeid(float) == typeid(T))
		winName+="float_";
	else if (typeid(int) == typeid(T))
		winName+="int_";
	else if (typeid(short) == typeid(T))
		winName+="short_";
	else if (typeid(unsigned char) == typeid(T))
		winName+="byte_";
	else
		assert(false);

	for (int d=0;d<nData;d++)
	{
		int data = 0;
		xpos = d%maxWidth;
		ypos = d/maxWidth;
		ypos = margin*(ypos+1) + rowHeight*ypos + rowHeight/2;
		if (typeid(double) == typeid(T) || typeid(float) == typeid(T))
			value = pData[d*nChannels+0];
		else if (typeid(int) == typeid(T))
			value = float(data = pData[d*nChannels+0]) / 2147483648.f;
		else if (typeid(short) == typeid(T))
			value = float(pData[d*nChannels+0]) / 32768.f;
		else if (typeid(unsigned char) == typeid(T))
			value = float(pData[d*nChannels+0]) / 128.f - 1.f;
		else
			assert(false);
		assert(value<=1.001 && value>=-1.001);

		value *= rowHeight/2;
		cvLine(pImg,cvPoint(xpos,ypos),cvPoint(xpos,ypos+value),CV_RGB(0,255,0),1);


	}

	
	winName+=name;
	cvNamedWindow(winName.c_str());
	cvShowImage(winName.c_str(),pImg);
	cvReleaseImage(&pImg);
	cvWaitKey(1);

}