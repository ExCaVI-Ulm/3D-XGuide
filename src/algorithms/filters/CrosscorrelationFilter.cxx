#include "CrosscorrelationFilter.h"
#include "highgui.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
using namespace std;

CrosscorrelationFilter::CrosscorrelationFilter(int numberOfImages)
:numberOfRotations(0), templateSizeX(20), templateSizeY(20),factorROISize(2.0), 
useROI(true), useCensus(true), restrictY(false), useBasicTpl(true), useModelBasedTpl(false), markersVisible(true), tplSettingsModified(false)/*, useUnsharpMasking(false), maskingSigma(1.0), maskingAmount(1.0)*/
{
	correlationTemplates.resize(numberOfImages);
	templatesRoi.resize(numberOfImages);
	theOffset.resize(numberOfImages);
	lastPosition.resize(numberOfImages);
	corrLines.resize(numberOfImages);
	theCorners.resize(numberOfImages);
	rotationTemplates.resize(numberOfImages);
	correlationList.resize(numberOfImages);

}

CrosscorrelationFilter::~CrosscorrelationFilter()
{

}

void CrosscorrelationFilter::setnumberOfRotations(int number)
{
	numberOfRotations = number;

	settingsChanged();
}

int CrosscorrelationFilter::getnumberOfRotations() const
{
	return numberOfRotations;
}

void CrosscorrelationFilter::setUseROI(bool isChecked)
{
	useROI = isChecked;
}

bool CrosscorrelationFilter::getUseROI() const
{
	return useROI;
}

bool CrosscorrelationFilter::getRestrictY() const
{
	return restrictY;
	
}

void CrosscorrelationFilter::setRestrictY(bool isChecked)
{
	restrictY = isChecked;

}


void CrosscorrelationFilter::setTplSize(int x, int y)
{
	templateSizeX = x;
	templateSizeY = y;
	tplSettingsModified = true;
}


int CrosscorrelationFilter::getTplSize(int index) const
{
		return index == 0 ? templateSizeX : templateSizeY;
}

void CrosscorrelationFilter::setFactorROISize(double factor)
{
	factorROISize = factor;
}

double CrosscorrelationFilter::getFactorROISize() const
{
	return factorROISize;
}

void CrosscorrelationFilter::setUseBasicTpl(bool isChecked)
{
	useBasicTpl = isChecked;
}

bool CrosscorrelationFilter::getUseBasicTpl() const
{
	return useBasicTpl;
}

void CrosscorrelationFilter::setUseModelBasedTpl(bool isChecked)
{
	useModelBasedTpl = isChecked;
}

bool CrosscorrelationFilter::getUseModelBasedTpl() const
{
	return useModelBasedTpl;
}

void CrosscorrelationFilter::setMarkersVisible(bool isChecked)
{
	markersVisible = isChecked;
}

bool CrosscorrelationFilter::getMarkersVisible() const
{
	return markersVisible;	
}

//void CrosscorrelationFilter::setUseUnsharpMasking(bool isChecked)
//{
//	useUnsharpMasking = isChecked;
//}
//
//bool CrosscorrelationFilter::getUseUnsharpMasking() const
//{
//	return useUnsharpMasking;
//}
//
//void CrosscorrelationFilter::setUnsharpMaskingSigma(double factor)
//{
//	maskingSigma = factor;
//	settingsChanged();
//}
//
//double CrosscorrelationFilter::getUnsharpMaskingSigma() const
//{
//	return maskingSigma;
//}
//
//void CrosscorrelationFilter::setUnsharpMaskingWeight(double factor)
//{
//	maskingAmount = factor;
//	settingsChanged();
//}
//
//double CrosscorrelationFilter::getUnsharpMaskingWeight() const
//{
//	return maskingAmount;
//}

void CrosscorrelationFilter::setTemplates(cv::Mat img, cv::Point point, int pointType, unsigned int imageNumber, unsigned int pointNumber)
{
	cv::Mat imgTpl;

	if (useBasicTpl == true || pointType == 1){
		
		int yStart = point.y - templateSizeY/2;
		int yEnd = point.y + templateSizeY/2;
		int xStart = point.x - templateSizeX/2;
		int xEnd = point.x + templateSizeX/2;
		// check and adjust the range if necessary
		checkAndAdjustRange(img, yStart, yEnd, xStart, xEnd);
		
		/* set tpl in addition to the filter settings
		   to represent the template for each point*/
		cv::Mat imgBasicTpl(img, cv::Range(yStart, yEnd),
			cv::Range(xStart, xEnd));

		imgBasicTpl.copyTo(imgTpl);
	} else {
		//imgTpl = cvLoadImageM("templateModel.tif", CV_LOAD_IMAGE_GRAYSCALE);
	}

	// copy the data, because the original is not guaranteed to exist any longer
	cv::Mat tpl;
	imgTpl.copyTo(tpl);

	if ((correlationTemplates[imageNumber].empty()) || (correlationTemplates[imageNumber].size() < pointNumber+1))
		correlationTemplates[imageNumber].push_back(tpl);
	else
		correlationTemplates[imageNumber][pointNumber] = tpl;

	// templates rotation only useful for catheter tracking
	if (pointType == 0 && numberOfRotations > 1){
		CvMat *rotTpl = cvCreateMatHeader(tpl.rows, tpl.cols, CV_8U);
		//cv::Mat res;
		CvMat* res = cvCreateMat(tpl.rows, tpl.cols, CV_8U);
		cvInitMatHeader(rotTpl, tpl.rows, tpl.cols, CV_8U, tpl.data);
		float angle = 0.0;
		for (int i = 0; i < numberOfRotations; ++i) {
			rotateTemplate(rotTpl, angle, imageNumber, res);
			cv::Mat tmp(res->rows, res->cols, CV_8U, res->data.ptr);

			if((rotationTemplates[imageNumber].empty()) || (rotationTemplates[imageNumber].size() < pointNumber+1))
			{
				rotationTemplates[imageNumber].push_back(tmp);
			}
			else
			{
				rotationTemplates[imageNumber][pointNumber] = tmp;
			}
			angle = angle + (360.0f/numberOfRotations);
		}
	}
}

// set/adjust the region of interest
void CrosscorrelationFilter::setROI(cv::Point position, cv::Mat img, unsigned int imageNumber, unsigned int pointNumber)
{
	cv::Size size;
	cv::Point theOfs;
	
	double r = 1.0;
	if (restrictY) {
		r = 1.0 / (factorROISize*2.0);
	}

	int yStart = position.y - cvRound(templateSizeY*factorROISize);
	int yEnd = position.y + cvRound(templateSizeY*factorROISize);
	int xStart = position.x - cvRound(templateSizeX*factorROISize*r);
	int xEnd = position.x + cvRound(templateSizeX*factorROISize*r);

	// INA: if ROI set at the image borders
	if (xStart < 0)
		xStart = 0;
	if (xEnd > img.cols)
		xEnd = img.cols;
	if (yStart < 0)
		yStart = 0;
	if (yEnd > img.rows)
		yEnd = img.rows;

	// check and adjust the range if necessary
	checkAndAdjustRange(img, yStart, yEnd, xStart, xEnd);

	// set region of interest
	cv::Mat imgROI(img, cv::Range(yStart, yEnd),
		cv::Range(xStart, xEnd));
	// locate in the original image for the offset
	imgROI.locateROI(size, theOfs);
	cv::Mat roi;
	imgROI.copyTo(roi);
	if ((templatesRoi[imageNumber].empty()) || (templatesRoi[imageNumber].size() < pointNumber+1)){
		templatesRoi[imageNumber].push_back(roi);
		theOffset[imageNumber].push_back(theOfs);
	}else{
		templatesRoi[imageNumber][pointNumber] = roi;
		theOffset[imageNumber][pointNumber] = theOfs;
	}
}

// rotate the given template clockwise
void CrosscorrelationFilter::rotateTemplate(const CvMat* src, float angle, int pointNumber, CvMat* res)
{
	/* create the 2x3 map matrix, where the left 2x2 matrix
	   is the transform and the right 2x1 is the dimensions. */
	float matrixElements[6];
	CvMat mapMatrix = cvMat(2, 3, CV_32F, matrixElements);
	float angleRadians = angle * ((float)CV_PI / 180.0f);
	// set the matrix elemnats
	matrixElements[0] = (float)( cos(angleRadians) );
	matrixElements[1] = (float)( sin(angleRadians) );
	matrixElements[3] = -matrixElements[1];
	matrixElements[4] = matrixElements[0];
	// dimensions
	matrixElements[2] = src->cols*0.5f;  
	matrixElements[5] = src->rows*0.5f;  
	// create destination
	CvMat* templateRotated = cvCreateMat(src->rows, src->cols, CV_8U);
	// rotate and transform
	cvGetQuadrangleSubPix( src, templateRotated, &mapMatrix);
	*res = *templateRotated;
	// write result into multimap
	//rotationTemplates.insert(pair<int, Mat>(pointNumber, templateRotated));
	// only for debugging
	//debug::saveImages(templateRotated, "C:\\temp\\", "TplRotated", pointNumber, rotationTemplates.count(pointNumber), "tif");
}

// get the best maximum location
cv::Point CrosscorrelationFilter::getBestMaxLoc(cv::Mat correlation, unsigned int imageNumber, unsigned int pointNumber)
{
	cv::Mat bestCorrelation;
	cv::Point    maxLoc, bestMaxLoc;
	double   maxVal, bestMaxVal;
	bestMaxVal = 0.0;
			
	//multimap<int, Mat>::iterator rotTpl;
	//rotTpl = rotationTemplates.[imageNumber][pointNumber];
	// loop and return the best maximum location
	for (unsigned int i = 0; i < rotationTemplates[imageNumber].size(); ++i) 
		{
			matchTemplate(templatesRoi[imageNumber][pointNumber], rotationTemplates[imageNumber][i], correlation, CV_TM_CCOEFF_NORMED);
			minMaxLoc(correlation, NULL, &maxVal, NULL, &maxLoc);
			if (maxVal > bestMaxVal) {
				bestMaxLoc = maxLoc;
				bestMaxVal = maxVal;
			}
		}
		
	return bestMaxLoc;
}

// mark rois, templates & catheter tips
void CrosscorrelationFilter::drawMarkers(cv::Mat img1, cv::Mat img2, int pointType, cv::Point2d matchPosition, unsigned int imageNumber, unsigned int pointNumber){
	
	//Annotated2dPoint point = getPointList(imageNumber)[pointNumber];
	
	int colorRoi = 255;
	int colorTpl = 0;
	int colorTip = 255;
	// type 0 -> 
	if (pointType == 0) {
		colorRoi = 0;
		colorTpl = 255;
		colorTip = 150;
	}
	
	if (useROI) {
	// visualize Roi
	rectangle(img1, cv::Point2d(matchPosition.x - templatesRoi[imageNumber][pointNumber].cols/2,
		                    matchPosition.y - templatesRoi[imageNumber][pointNumber].rows/2), 
			        cv::Point2d(matchPosition.x + templatesRoi[imageNumber][pointNumber].cols/2,
						    matchPosition.y + templatesRoi[imageNumber][pointNumber].rows/2), (colorRoi), 1, 8, 0);
	}
	// visualize Template
	rectangle(img1, cv::Point2d(matchPosition.x - correlationTemplates[imageNumber][pointNumber].cols/2,
		                    matchPosition.y - correlationTemplates[imageNumber][pointNumber].rows/2), 
					cv::Point2d(matchPosition.x + correlationTemplates[imageNumber][pointNumber].cols/2,
						    matchPosition.y + correlationTemplates[imageNumber][pointNumber].rows/2), (colorTpl, colorTpl, colorTpl), 1, 8, 0);


	
	//save Template in extern file
	int yStart = matchPosition.y - correlationTemplates[imageNumber][pointNumber].rows/2;
	int yEnd = matchPosition.y + correlationTemplates[imageNumber][pointNumber].rows/2;
	int xStart = matchPosition.x - correlationTemplates[imageNumber][pointNumber].cols/2;
	int xEnd = matchPosition.x + correlationTemplates[imageNumber][pointNumber].cols/2;
	checkAndAdjustRange(img1, yStart, yEnd, xStart, xEnd);
	cv::Mat selectedTmpl(img1, cv::Range(yStart, yEnd),
						cv::Range(xStart, xEnd));
	/*
	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(9);
	if(imageNumber == 0){
		imwrite("template_left.png", selectedTmpl, compression_params);
	}
	else{
		imwrite("template_right.png", selectedTmpl, compression_params);
	}
	*/

	// visualize matchPosition
	circle(img1, matchPosition, 3, (colorTip, colorTip, colorTip), 1, 4, 0);

	//draw epilines after both tracking points are selectes
	if(!corrLines[imageNumber].empty() && (corrLines[0].size() == corrLines[1].size()) && corrLines[0].size() > pointNumber && pointType == 0){
		if(imageNumber == 1){
			line(img1, cv::Point2d(corrLines[0][pointNumber][0].x, corrLines[0][pointNumber][0].y),
				cv::Point2d(corrLines[0][pointNumber][1].x, corrLines[0][pointNumber][1].y),
					   (255, 255, 255), 1, 8, 0);
			line(img2, cv::Point2d(corrLines[1][pointNumber][0].x, corrLines[1][pointNumber][0].y),
				cv::Point2d(corrLines[1][pointNumber][1].x, corrLines[1][pointNumber][1].y),
					   (255, 255, 255), 1, 8, 0);
		}
		else{
			line(img1, cv::Point2d(corrLines[1][pointNumber][1].x, corrLines[1][pointNumber][1].y),
				cv::Point2d(corrLines[1][pointNumber][0].x, corrLines[1][pointNumber][0].y),
					   (255, 255, 255), 1, 8, 0);
			line(img2, cv::Point2d(corrLines[0][pointNumber][1].x, corrLines[0][pointNumber][1].y),
				cv::Point2d(corrLines[0][pointNumber][0].x, corrLines[0][pointNumber][0].y),
					   (255, 255, 255), 1, 8, 0);
		}
	}


	// visualize the most prominent corners in the image
	// theCorners is the result of the function setGoodFeaturesToTrack
	/*
	if (!theCorners[imageNumber].empty()){
		int size = 3;
		for (unsigned int i = 0; i < theCorners[imageNumber][pointNumber].size();++i){
			line( img1, Point2d(theCorners[imageNumber][pointNumber][i].x + size, theCorners[imageNumber][pointNumber][i].y + size),
			Point2d(theCorners[imageNumber][pointNumber][i].x - size, theCorners[imageNumber][pointNumber][i].y - size), (255));
			line( img1, Point2d(theCorners[imageNumber][pointNumber][i].x - size, theCorners[imageNumber][pointNumber][i].y + size),
			Point2d(theCorners[imageNumber][pointNumber][i].x + size, theCorners[imageNumber][pointNumber][i].y - size), (255));
		}
	}*/

	// visualize the correlationLocalMaxima
	if(!correlationList[imageNumber].empty() && correlationList[imageNumber].size() > pointNumber){

		int size = 3;
		int labelNr = 1;
		//Mat imgflipped;
		for(list<PointCorrelation>::const_iterator ci = correlationList[imageNumber][pointNumber].begin(); 
			ci != correlationList[imageNumber][pointNumber].end(); ++ci){
			//flip(img1, imgflipped, -1);
			line( img1, cv::Point2d(ci->point.x + size, ci->point.y + size), cv::Point2d(ci->point.x - size, ci->point.y - size), (255));
			line( img1, cv::Point2d(ci->point.x - size, ci->point.y + size), cv::Point2d(ci->point.x + size, ci->point.y - size), (255));
	
			//putText(img1, "1", Point2d(ci->point.x, ci->point.y), 0, 1, (255));
		
		}
	}
}

vector<cv::Vec3f> CrosscorrelationFilter::getCorrespondingEpiline(cv::Mat fundamentalMatrix, cv::Point point, unsigned int imageNumber)
{
	vector<cv::Vec3f> corrLine;
	cv::Mat points(1,1,CV_32FC2);

	points.at<cv::Vec2f>(0,0) = cv::Vec2f((float)point.x, (float)point.y);

	computeCorrespondEpilines(points, imageNumber+1, fundamentalMatrix, corrLine);

	return corrLine;
}

// set/adjust the epilines
void CrosscorrelationFilter::setEpilinePoints(vector<cv::Point2d> epiLinePoints, unsigned int imageNumber, unsigned int pointNumber)
{
	if ((corrLines[imageNumber].empty()) || (corrLines[imageNumber].size() < pointNumber+1)){
		corrLines[imageNumber].push_back(epiLinePoints);
	}else{
		corrLines[imageNumber][pointNumber] = epiLinePoints;
	}
}

void CrosscorrelationFilter::setGoodFeaturesToTrack(cv::Mat img, unsigned int imageNumber, unsigned int pointNumber)
{
	vector<cv::Point2f> corners;
	if (useROI){
		goodFeaturesToTrack(templatesRoi[imageNumber][pointNumber], corners, 5, 0.04, 1.0, cv::Mat(), 3, true);
	}else{
		goodFeaturesToTrack(img, corners, 5, 0.04, 1.0, cv::Mat(), 8, true, 0.04);
	}

	for (unsigned int i = 0; i < corners.size(); ++i){
		if (useROI){
			corners[i].x = corners[i].x + theOffset[imageNumber][pointNumber].x;
			corners[i].y = corners[i].y + theOffset[imageNumber][pointNumber].y;
		}
		
		if ((theCorners[imageNumber].empty()) || (theCorners[imageNumber].size() < pointNumber+1))
			theCorners[imageNumber].push_back(corners);
					
		else
			theCorners[imageNumber][pointNumber] = corners;		
	}
}

void CrosscorrelationFilter::settingsChanged()
{
	// Do nothing in this default implementation.
}

void CrosscorrelationFilter::cleanUp(unsigned int listNumber, unsigned int pointNumber)
{
	// delete the template corresponding to the deleted point.
	if (!correlationTemplates[listNumber].empty()){
		myMatVector::iterator toDelete = correlationTemplates[listNumber].begin();
		toDelete += pointNumber;
		correlationTemplates[listNumber].erase(toDelete);
	}
	// delete the roi corresponding to the deleted point
	if (!templatesRoi[listNumber].empty()){
		myMatVector::iterator toDeleteRoi = templatesRoi[listNumber].begin();
		toDeleteRoi += pointNumber;
		templatesRoi[listNumber].erase(toDeleteRoi);
	}
	// delete the offset corresponding to the deleted point
	if (!theOffset[listNumber].empty()){
		myPointVector::iterator toDeleteOfs = theOffset[listNumber].begin();
		toDeleteOfs += pointNumber;
		theOffset[listNumber].erase(toDeleteOfs);
	}
	// delete lastPosition corresponding to the deleted point
	if (!lastPosition[listNumber].empty()){
		vector<cv::Point2d>::iterator toDeleteLastPosition = lastPosition[listNumber].begin();
		toDeleteLastPosition += pointNumber;
		lastPosition[listNumber].erase(toDeleteLastPosition);
	}
	// delete the rotation templates corresponding to the deleted point
	if (!rotationTemplates[listNumber].empty()){
		myMatVector::iterator toDeleteRotationTemplates = rotationTemplates[listNumber].begin();
		toDeleteRotationTemplates += pointNumber;
		rotationTemplates[listNumber].erase(toDeleteRotationTemplates);
	}
	// delete the corrLines corresponding to the deleted point
	if (!corrLines[listNumber].empty()){
		vector< vector<cv::Point2d> >::iterator toDeleteCorrLine = corrLines[listNumber].begin();
		toDeleteCorrLine += pointNumber;
		corrLines[listNumber].erase(toDeleteCorrLine);
	}
	// delete theCorners corresponding to the deleted point
	if (!theCorners[listNumber].empty()) {
		vector< vector<cv::Point2f> >::iterator toDeleteTheCorners = theCorners[listNumber].begin();
		toDeleteTheCorners += pointNumber;
		theCorners[listNumber].erase(toDeleteTheCorners);
	}


	//delete correlationList corresponding to the deleted point
	if (!correlationList[listNumber].empty()){
		vector < list<PointCorrelation> >::iterator toDeleteCorrelationList = correlationList[listNumber].begin();
		toDeleteCorrelationList += pointNumber;
		correlationList[listNumber].erase(toDeleteCorrelationList);
	}
}

// 
void CrosscorrelationFilter::checkAndAdjustRange(cv::Mat img, int& yStart, int& yEnd, int& xStart, int& xEnd)
{
	if (yStart < 0)
		yStart = 0;
	if (xStart < 0)
		xStart = 0;

	if (yEnd >= img.rows)
		yEnd = img.rows;
	if (xEnd >= img.cols)
		xEnd = img.cols;
}

/* Loop through the crosscorrelation results and identify local maxima.
   A local maxima is found if no higher correlation value is found in the
   neighborhood of the examined point */
void CrosscorrelationFilter::findLocalMaxima(cv::Mat correlation, int imageNumber, int pointNumber, bool addOffset, int pointType)
{
	correlationList[imageNumber].resize(pointNumber+1);
	

	// clean up first
	if (!correlationList[imageNumber][pointNumber].empty())
		correlationList[imageNumber][pointNumber].clear();

	if(pointType == 0)
	{
		// set the threshold for correlation value
		double myThreshold = 0.1;
		// loop through correlation results
		for (int y = 0; y < correlation.rows; ++y){
			for (int x = 0; x < correlation.cols; ++x)
			{
				double corr = correlation.at<float>(y,x);
				// check if correlation value satisfy the threshold
				if (corr >= myThreshold){
					bool isMaximum = true;
					// analyze the neighborhood points
					for (int i = -1; i <= 1; ++i){
						for (int j = -1; j <=1; ++j){
							int coord1 = x+i;
							int coord2 = y+j;
							if ((coord1 >= 0) && (coord1 < correlation.cols) && (coord2 >= 0) && (coord2 < correlation.rows)){
								double corrN = correlation.at<float>(coord2,coord1);
								if (corrN > corr) 
									isMaximum = false;
							}
						}
					}
					// maximum is found
					if (isMaximum){			
						PointCorrelation corrList;
						// compute original image coordinates					
						double newX = x + (correlationTemplates[imageNumber][pointNumber].cols/2);
						double newY = y + (correlationTemplates[imageNumber][pointNumber].rows/2);

						// roi used -> adjust the matchPosition
						// Do not change with manual position, because this is always absolute.
						if (addOffset) {
								newX += theOffset[imageNumber][pointNumber].x; 
								newY += theOffset[imageNumber][pointNumber].y; 
						}
						corrList.point = cv::Point2d(newX, newY);
						corrList.corr = corr;
						// distance is calculated later because for this the epipolar lines are needed
						corrList.dist = 0.0;
						// store the local maximum values to the correlationList container
						correlationList[imageNumber][pointNumber].push_back(corrList);
						
					}
				}
			}
		}
		// sort the list by correlation value (highest to lowest)
		correlationList[imageNumber][pointNumber].sort(comparePointCorrelationByCorr());
		// only the best five results are needed
		while (correlationList[imageNumber][pointNumber].size() > 2)
			correlationList[imageNumber][pointNumber].pop_back();
	}
	else
	{
		PointCorrelation corrListEmpty;
		corrListEmpty.point = cv::Point2d(0,0);
		corrListEmpty.corr = 0;
		corrListEmpty.dist = 0;
		correlationList[imageNumber][pointNumber].push_back(corrListEmpty);
	}

}