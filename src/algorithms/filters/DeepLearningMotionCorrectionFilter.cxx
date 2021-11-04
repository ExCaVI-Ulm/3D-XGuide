#include "DeepLearningMotionCorrectionFilter.h"
#include "MotionCorrectionFilterWidget.h"
#include "DLFilterWidget.h"
#include <vtkImageData.h>
#include "highgui.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include "curl/curl.h"
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;

using namespace std;

//#include <highgui.h> // debugging: lib from OpenCV for saving images from the buffer

DeepLearningMotionCorrectionFilter::DeepLearningMotionCorrectionFilter()
{

	referenceImage = NULL;
	referenceImageNew = NULL;

}

DeepLearningMotionCorrectionFilter::~DeepLearningMotionCorrectionFilter()
{
}

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
	size_t newLength = size*nmemb;
	try
	{
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc &e)
	{
		//handle memory problem
		return 0;
	}
	return newLength;
}

std::string get_str_between_two_str(const std::string &s,
	const std::string &start_delim,
	const std::string &stop_delim)
{
	unsigned first_delim_pos = s.find(start_delim);
	unsigned end_pos_of_first_delim = first_delim_pos + start_delim.length();
	unsigned last_delim_pos = s.find(stop_delim);

	return s.substr(end_pos_of_first_delim,
		last_delim_pos - end_pos_of_first_delim);
}


void DeepLearningMotionCorrectionFilter::ComputeMotion(vtkImageData* image, double& resultingMotionX, double& resultingMotionY)
{
	if (!referenceImage)
		return;

	if (!setReferenceFrame)
		return;

	clock_t time_start, time_stop;
	double duration;
	ofstream f;
	f.open("FileSavingTime.txt", ios::out | ios::app);
	time_start = clock();

	int dimensions[3];
	image->GetDimensions(dimensions);
	int widthIm = dimensions[0];
	int heightIm = dimensions[1];
	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());
	cv::Mat dst;
	cv::flip(img, dst, 0);
	imwrite("2.png", dst);
	cv::Mat imgRef(heightIm, widthIm, CV_8U, referenceImage->GetScalarPointer());
	cv::Mat dstRef;
	cv::flip(imgRef, dstRef, 0);
	//imgRef.convertTo(imgRef, CV_32F, 1.f / 255);
	imwrite("1.png", dstRef);

	time_stop = clock();
	duration = (double)(time_stop - time_start);
	f << duration << endl;
	f.close();


	CURL *curl;
	CURLcode res;
	std::string s,x,y;
	json j_string;

	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		if (MoveY && !MoveX)
		{
			curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/predict_y");
		}
		else if (MoveX && !MoveY)
		{
			curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/predict_x");
		}
		else if (MoveX && MoveY)
		{
			curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5000/predict_all");
		}
		else
		{
			return;
		}

		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "http");
		struct curl_slist *headers = NULL;
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		curl_mime *mime;
		curl_mimepart *part;
		mime = curl_mime_init(curl);
		part = curl_mime_addpart(mime);
		curl_mime_name(part, "image1");
		//curl_mime_data(part, img, CURL_ZERO_TERMINATED);
		curl_mime_filedata(part, "../build/1.png");
		part = curl_mime_addpart(mime);		
		curl_mime_name(part, "image2");
		curl_mime_filedata(part, "../build/2.png");
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

		clock_t time_start1, time_stop1;
		double duration1;
		ofstream f1;
		f1.open("PerformTime.txt", ios::out | ios::app);
		time_start1 = clock();

		res = curl_easy_perform(curl);

		time_stop1 = clock();
		duration1 = (double)(time_stop1 - time_start1);
		f1 << duration1 << endl;
		f1.close();


		curl_mime_free(mime);

		std::cout << s << std::endl;
		// create object from string variable
		// parse explicitly
		j_string = json::parse(s);
		if (MoveY && !MoveX)
		{
			x = "0";
			y = j_string.at("prediction_y").get<std::string>();
		}
		else if (MoveX && !MoveY)
		{			
			x = j_string.at("prediction_x").get<std::string>();
			y = "0";
		}
		else
		{
			x = j_string.at("prediction_x").get<std::string>();
			y = j_string.at("prediction_y").get<std::string>();
		}
		
		/*std::string start_delim_x = "prediction_x\":\"";
		std::string stop_delim_x = "\",\"prediction_y";
		std::string start_delim_y = "prediction_y\":\"";
		std::string stop_delim_y = "\",\"run time [s]";
		x = get_str_between_two_str(s, start_delim_x, stop_delim_x);
		y = get_str_between_two_str(s, start_delim_y, stop_delim_y);*/

	}
	curl_easy_cleanup(curl);		
	
	// build the vector from null position to current prediction
	resultingMotionX = atoi(x.c_str());
	resultingMotionY = -atoi(y.c_str());

	if (UseDLManualScaling) {
		resultingMotionX *= DLScaleX;
		resultingMotionY *= DLScaleY;
	}


	std::cout << "MotionX: " << resultingMotionX << std::endl;
	std::cout << "MotionY: " << resultingMotionY << std::endl;

}

void DeepLearningMotionCorrectionFilter::pointWasAdded(unsigned int listNumber, unsigned int pointNumber)
{
	
	Modified();
}

// clean up after point was deleted
void DeepLearningMotionCorrectionFilter::pointWasDeleted(unsigned int listNumber, unsigned int pointNumber)
{

	MotionX = 0.0;	// otherwise mesh is shifted when play,
	MotionY = 0.0;	//since computeMotion is called with motionX, motionY =/ 0

		
	Modified();
	
}

vector<AbstractFilterWidget*> DeepLearningMotionCorrectionFilter::getPropertiesGUI()
{
	vector<AbstractFilterWidget*> res;
	//res.push_back(new MotionCorrectionFilterWidget(this));
	res.push_back(new DLFilterWidget(this));

	return res;
}

void DeepLearningMotionCorrectionFilter::settingsChanged()
{

	Modified();
}
