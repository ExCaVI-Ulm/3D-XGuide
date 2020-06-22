#include "CrosscorrelationMotionCorrectionFilter.h"

#include <vtkImageData.h>
#include "highgui.h"
#include <opencv2/imgproc.hpp>
#include "CrosscorrelationFilterWidget.h"

using namespace std;

//#include <highgui.h> // debugging: lib from OpenCV for saving images from the buffer

CrosscorrelationMotionCorrectionFilter::CrosscorrelationMotionCorrectionFilter()
{
	motionPositionSetManually = false;
	manualMotionPosition[0] = manualMotionPosition[1] = 0.0;
	pointUsedForManualPosition = -1;

}

CrosscorrelationMotionCorrectionFilter::~CrosscorrelationMotionCorrectionFilter()
{
}


//fstream f;
void CrosscorrelationMotionCorrectionFilter::ComputeMotion(vtkImageData* image, double& resultingMotionX, double& resultingMotionY)
{
	// no templates set
	if(correlationTemplates[0].empty()) return;
	
	int dimensions[3];
	image->GetDimensions(dimensions);

	int widthIm = dimensions[0];
	int heightIm = dimensions[1];

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());
	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat censusImg(heightIm, widthIm, CV_8U);

	if (UseUnsharpMasking == true){
		cv::Mat blurred;
		/*double sigma = 1, threshold = 5, amount = 1;*/
		double sigma = MaskingSigma, threshold = 5, amount = MaskingWeight;
		GaussianBlur(img, blurred, cv::Size(), sigma, sigma);
		cv::Mat lowConstrastMask = abs(img - blurred) < threshold;
		img = img*(1+amount) + blurred*(-amount);
		img.copyTo(img, lowConstrastMask);
		
		//tplSettingsModified = true;
	}
	if (UseSobelFilter == true) {
		cv::GaussianBlur(img, img, cv::Size(KSizeSobel, KSizeSobel), 0, 0, 4); //m0:sinnvoll das mit dem ksizeSobel zu koppeln? GaussianBlur Wird oben schon verwendet

		cv::Mat grad_x;
		cv::Sobel(img, grad_x, CV_16S, 1, 0, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::convertScaleAbs(grad_x, grad_x);

		cv::Mat grad_y;
		cv::Sobel(img, grad_y, CV_16S, 0, 1, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::convertScaleAbs(grad_y, grad_y);
		addWeighted(grad_x, 0.5, grad_y, 0.5, 0, img);
	}

	if (UseScharrFilter == true) {
		cv::GaussianBlur(img, img, cv::Size(3, 3), 0, 0, 4);

		cv::Mat grad_x;
		cv::Scharr(img, grad_x, CV_16S, 1, 0, ScaleScharr, DeltaScharr, 4);
		cv::convertScaleAbs(grad_x, grad_x);

		cv::Mat grad_y;
		cv::Scharr(img, grad_y, CV_16S, 0, 1, ScaleScharr, DeltaScharr, 4);
		cv::convertScaleAbs(grad_y, grad_y);
		addWeighted(grad_x, 0.5, grad_y, 0.5, 0, img);		

	}
	//if (UseScharrFilter == true) {
	//	//cv::Mat blurred, dst;
	//	//dst = cv::Scalar::all(0);
	//	//cv::blur(img, blurred, cv::Size(ScaleScharr, ScaleScharr));
	//	//cv::Canny(blurred, blurred, DeltaScharr, DeltaScharr * 5, ScaleScharr);
	//	//blurred.copyTo(dst, blurred);
	//	////img = img - dst;
	//	//dst.copyTo(img);

	//	cv::Mat blurred;
	//	cv::medianBlur(img, img, ScaleScharr);
	//	
	//}

	
	cv::Point matchPositionForMotionCorrection;

	// to find the template used for motion correction
	const int pointTypeForMotionCorrection = 1;
	int pointUsedForMotionCorrection = -1;

	PointVector pointList = getPointList();
	cv::Point matchPosition;
	// for Mat correlation
	cv::Mat matchImg;
	
	int width, height;

	for(unsigned int i = 0; i < correlationTemplates[0].size(); ++i) {
		
		// adjust template if necessary
		if (tplSettingsModified == true){
			setTemplates(img, lastPosition[0][i], pointList[i].type, 0, i);
		}
		
		if (useROI == true) {
			// set/adjust roi(region of interest)
			setROI(lastPosition[0][i], img, 0, i);
			// settings for Mat correlation
			width = templatesRoi[0][i].cols - correlationTemplates[0][i].cols +1;
			height = templatesRoi[0][i].rows - correlationTemplates[0][i].rows +1;
			matchImg = templatesRoi[0][i];
		} else {
			// settings for Mat correlation
			width = img.cols;
			height = img.rows;
			matchImg = img; 
		}
		
		// apply normalized crosscorrelation
		cv::Mat correlation(height, width, CV_8U);
		

		if ((useBasicTpl == true || useModelBasedTpl == true) && numberOfRotations > 1 && pointList[i].type == 0) {
			matchPosition = getBestMaxLoc(correlation, 0, i);	
		} else {
			if(pointList[i].type == pointTypeForMotionCorrection){
				trackMotionCorrectionPoint(matchImg, correlationTemplates[0][i], matchPosition, width, height);
			}
			else{
				cv::matchTemplate(matchImg, correlationTemplates[0][i], correlation, CV_TM_CCOEFF_NORMED);
				// find the maximum
				minMaxLoc(correlation, NULL, NULL, NULL, &matchPosition);
			}
		}

		// manually overwrite motion correction match, if the user has set the position
		if(motionPositionSetManually && pointUsedForManualPosition == i
			&& pointList[i].type == pointTypeForMotionCorrection) {
		
				matchPosition.x = manualMotionPosition[0];
				matchPosition.y = manualMotionPosition[1];

				// generate template of the manually set region
				setTemplates(img, matchPosition, pointList[i].type, 0, i);

		}
		else {
			matchPosition.x = matchPosition.x + (correlationTemplates[0][i].cols/2);
			matchPosition.y = matchPosition.y + (correlationTemplates[0][i].rows/2);
		}

		//setGoodFeaturesToTrack(matchImg, 0, i);
		
		// roi used -> adjust the matchPosition
		// Do not change with manual position, because this is always absolute.
		if (useROI
			&& !(pointList[i].type == pointTypeForMotionCorrection
							&& motionPositionSetManually && pointUsedForManualPosition == i)) {
			matchPosition.x += theOffset[0][i].x; 
			matchPosition.y += theOffset[0][i].y; 
		}
		
		//f.open("trackingresults.txt",  ios::out|ios::app);
		//f << "x:	" << matchPosition.x << " ; y:	" << matchPosition.y  << endl;
		//f.close();


		// remember position
		lastPosition[0][i] = cv::Point2f(matchPosition.x, matchPosition.y);

		int leftBorder, rightBorder, lowerBorder, upperBorder;

		if (UseCensus == true)
		{
			unsigned int census = 0;
			unsigned int bit = 0;
			int m = 3;
			int n = 3;//window size
			int k, j, x, y;
			int shiftCount = 0;

			leftBorder = lastPosition[0][i].y - templatesRoi[0][i].rows / 2;
			rightBorder = lastPosition[0][i].y + templatesRoi[0][i].rows / 2;
			lowerBorder = lastPosition[0][i].x - templatesRoi[0][i].cols / 2;
			upperBorder = lastPosition[0][i].x + templatesRoi[0][i].cols / 2;

			if (leftBorder < 0)
				leftBorder = 0;
			if (rightBorder > heightIm)
				rightBorder = heightIm;
			if (lowerBorder < 0)
				lowerBorder = 0;
			if (upperBorder > widthIm)
				upperBorder = widthIm;

			/*leftBorder[i].push_back(lastPosition[0][i].y - templatesRoi[0][i].rows / 2);
			rightBorder[i] = lastPosition[0][i].y + templatesRoi[0][i].rows / 2;
			lowerBorder[i] = lastPosition[0][i].x - templatesRoi[0][i].cols / 2;
			upperBorder[i] = lastPosition[0][i].x + templatesRoi[0][i].cols / 2;

			if (leftBorder[i] < 0)
			leftBorder[i] = 0;
			if (rightBorder[i] > width)
			rightBorder[i] = width;
			if (lowerBorder[i] < 0)
			lowerBorder[i] = 0;
			if (upperBorder[i] > height)
			upperBorder[i] = height;




			for (y = leftBorder[i] + m / 2; y < rightBorder[i] - m / 2; y++)
			{
			for (x = lowerBorder[i] + n / 2; x < upperBorder[i] - n / 2; x++)
			{*/

			/*for (y = lastPosition[0][i].y - templatesRoi[0][i].rows / 2 + m / 2; y < lastPosition[0][i].y + templatesRoi[0][i].rows / 2 - m / 2; y++)
			{
				for (x = lastPosition[0][i].x - templatesRoi[0][i].cols / 2 + n / 2; x < lastPosition[0][i].x + templatesRoi[0][i].cols / 2 - n / 2; x++)
				{*/
			for (y = leftBorder + m / 2; y < rightBorder - m / 2; y++)
			{
				for (x = lowerBorder + n / 2; x < upperBorder - n / 2; x++)
				{
					
					census = 0;
					shiftCount = 0;
					for (j = y - m / 2; j <= y + m / 2; j++)
					{
						for (k = x - n / 2; k <= x + n / 2; k++)
						{

							if (shiftCount != m*n / 2)//skip the center pixel
							{
								census <<= 1;
								if (img.at<uchar>(j, k) < img.at<uchar>(y, x))//compare pixel values in the neighborhood
									bit = 1;
								else
									bit = 0;
								census = census + bit;
								//cout<<census<<" ";*/

							}
							shiftCount++;
						}
					}
					//cout<<endl;

					censusImg.ptr<uchar>(y)[x] = census;
				}
			}



			/*for (y = leftBorder[i] + m / 2; y < rightBorder[i] - m / 2; y++)
			{
			for (x = lowerBorder[i] + n / 2; x < upperBorder[i] - n / 2; x++)
			{*/
			/*for (y = lastPosition[0][i].y - templatesRoi[0][i].rows / 2 + m / 2; y < lastPosition[0][i].y + templatesRoi[0][i].rows / 2 - m / 2; y++)
			{
				for (x = lastPosition[0][i].x - templatesRoi[0][i].cols / 2 + n / 2; x < lastPosition[0][i].x + templatesRoi[0][i].cols / 2 - n / 2; x++)
				{*/
				for (y = leftBorder + m / 2; y < rightBorder- m / 2; y++)
				{
				for (x = lowerBorder + n / 2; x < upperBorder - n / 2; x++)
				{
					img.ptr<uchar>(y)[x] = censusImg.at<uchar>(y, x);
				}
			}

		}
		// store position for return/call by reference
		/*resultingPositionsX.push_back(matchPosition.x);
		resultingPositionsY.push_back(matchPosition.y);
*/
		if (markersVisible) {
			// draw all markers
			drawMarkers(img, img, pointList[i].type,matchPosition, 0, i);
			// drawing may cause problems for the next matching -> restore image to initial state
			image->Initialize();
		}

		// check whether to use this point (i.e. template) for motion correction
		if (pointList[i].type == pointTypeForMotionCorrection) {
			matchPositionForMotionCorrection = matchPosition;
			pointUsedForMotionCorrection = i;
		}
	}
	
	if(pointUsedForMotionCorrection < 0) {
		//cout << "Warning: no point was used for motion correction!" << endl;
		return;
	}

	// build the vector from match null position to current match
	resultingMotionX = matchPositionForMotionCorrection.x - pointList[pointUsedForMotionCorrection].x;
	resultingMotionY = matchPositionForMotionCorrection.y - pointList[pointUsedForMotionCorrection].y;

	// Tracking for the next frames automatically, again.
	motionPositionSetManually = false;

	// Settings modifications are adapted now.
	tplSettingsModified = false;
}

void CrosscorrelationMotionCorrectionFilter::pointWasAdded(unsigned int listNumber, unsigned int pointNumber) 
{
	//listNumber can be ignored, because we have only one list.
	//now you can access point.x, point.y, point.type //
	Annotated2dPoint point = getPointList(listNumber)[pointNumber];   


	if (motionPositionSetManually)
	{
		vector<cv::Point2d>::iterator toDeleteLastPosition = lastPosition[listNumber].begin();
		toDeleteLastPosition += pointNumber;
		lastPosition[listNumber].erase(toDeleteLastPosition);
		pointUsedForManualPosition = pointNumber;

	}


	lastPosition[listNumber].push_back(cv::Point2d(point.x, point.y));

	vtkImageData* image;

	image = GetImageOutput();
			
	int dimensions[3];
	image->GetDimensions(dimensions);

	const int width = dimensions[0];
	const int height = dimensions[1];

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(height, width, CV_8U, image->GetScalarPointer());

	// if necessary set roi to represent the region of interest for template matching
	if (useROI)
		setROI(lastPosition[listNumber][pointNumber], img, listNumber, pointNumber); 

	setTemplates(img, cv::Point(point.x, point.y), point.type, listNumber, pointNumber);
	//setGoodFeaturesToTrack(img, listNumber, pointNumber);
	isInitialization = true;


	Modified();
}

// clean up after point was deleted
void CrosscorrelationMotionCorrectionFilter::pointWasDeleted(unsigned int listNumber, unsigned int pointNumber) {

	cleanUp(listNumber, pointNumber);

	isInitialization = true;

	if (lastPosition[listNumber].size() == 0)
	{
		MotionX = 0.0;	// otherwise mesh is shifted when play,
		MotionY = 0.0;	//since computeMotion is called with motionX, motionY =/ 0
		// ProcessImages = false;	// or so
	}
	
	Modified();
	
}

vector<AbstractFilterWidget*> CrosscorrelationMotionCorrectionFilter::getPropertiesGUI()
{
	vector<AbstractFilterWidget*> res = MotionCorrectionFilter::getPropertiesGUI();
	res.push_back(new CrosscorrelationFilterWidget(dynamic_cast<MotionCorrectionFilter*>(this)));

	return res;
}

void CrosscorrelationMotionCorrectionFilter::settingsChanged()
{
	isInitialization = true;
	Modified();
}

void CrosscorrelationMotionCorrectionFilter::overwriteMotionCorrectionMatchPosition(const double pos[2])
{
	manualMotionPosition[0] = pos[0];
	manualMotionPosition[1] = pos[1];

	motionPositionSetManually = true;

	Modified();
}