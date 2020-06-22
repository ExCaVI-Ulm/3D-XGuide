#include "SimpleBiplaneFilter.h"

#include "CrosscorrelationFilterWidget.h"
#include "OverlayScene.h"
#include "highgui.h"
#include <opencv2/imgproc.hpp>

using namespace std;

SimpleBiplaneFilter::SimpleBiplaneFilter()
:CrosscorrelationFilter(2), PointListBasedFilter(2)
{
	motionPositionSetManually = false;
	pointUsedForManualPosition[0] = pointUsedForManualPosition[1] = -1;
}

SimpleBiplaneFilter::~SimpleBiplaneFilter()
{
}
//fstream f;
void SimpleBiplaneFilter::ComputeOnBiplaneImages(vtkImageData* image1, vtkImageData* image2, vtkImageData* image1forDraw, vtkImageData* image2forDraw, double detectedMotion[3], vector<double>& detectedPositionsX, vector<double>& detectedPositionsY, vector<double>& detectedPositionsZ)
{
	// no templates set
	if (correlationTemplates[0].empty() && correlationTemplates[1].empty()) return;

	int dimensions[3];

	//get dimensions of the first image
	image1->GetDimensions(dimensions);
	int width = dimensions[0];
	int height = dimensions[1];
	// create OpenCV matrix of the vtkImageData image1 without(!) copying the content
	cv::Mat img1(height, width, CV_8U, image1->GetScalarPointer());
	cv::Mat img1forDraw(height, width, CV_8U, image1forDraw->GetScalarPointer());

	//get dimensions of the second image
	image2->GetDimensions(dimensions);
	width = dimensions[0];
	height = dimensions[1];
	// create OpenCV matrix of the vtkImageData image2 without(!) copying the content
	cv::Mat img2(height, width, CV_8U, image2->GetScalarPointer());
	cv::Mat img2forDraw(height, width, CV_8U, image2forDraw->GetScalarPointer());

	if (UseUnsharpMasking == true){		
		cv::Mat blurred1;
		cv::Mat blurred1forDraw;
		/*double sigma = 1, threshold = 5, amount = 1;*/
		double sigma = MaskingSigma, threshold = 5, amount = MaskingWeight;
		GaussianBlur(img1, blurred1, cv::Size(), sigma, sigma);
		GaussianBlur(img1forDraw, blurred1forDraw, cv::Size(), sigma, sigma);
		cv::Mat lowConstrastMask1 = abs(img1 - blurred1) < threshold;
		cv::Mat lowConstrastMask1forDraw = abs(img1forDraw - blurred1forDraw) < threshold;
		img1 = img1*(1+amount) + blurred1*(-amount);
		img1forDraw = img1forDraw*(1+amount) + blurred1forDraw*(-amount);
		img1.copyTo(img1, lowConstrastMask1);
		img1forDraw.copyTo(img1forDraw, lowConstrastMask1forDraw);

		cv::Mat blurred2;
		cv::Mat blurred2forDraw;
		GaussianBlur(img2, blurred2, cv::Size(), sigma, sigma);
		GaussianBlur(img2forDraw, blurred2forDraw, cv::Size(), sigma, sigma);
		cv::Mat lowConstrastMask2 = abs(img2 - blurred2) < threshold;
		cv::Mat lowConstrastMask2forDraw = abs(img2forDraw - blurred2forDraw) < threshold;
		img2 = img2*(1+amount) + blurred2*(-amount);
		img2forDraw = img2forDraw*(1+amount) + blurred2forDraw*(-amount);
		img2.copyTo(img2, lowConstrastMask2);
		img2forDraw.copyTo(img2forDraw, lowConstrastMask2forDraw);
	}

	if (UseSobelFilter == true) {
		cv::GaussianBlur(img1, img1, cv::Size(KSizeSobel, KSizeSobel), 0, 0, 4); //m0:sinnvoll das mit dem ksizeSobel zu koppeln? GaussianBlur Wird oben schon verwendet
		cv::GaussianBlur(img1forDraw, img1forDraw, cv::Size(KSizeSobel, KSizeSobel), 0, 0, 4);

		cv::Mat grad_x1;
		cv::Sobel(img1, grad_x1, CV_16S, 1, 0, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::Sobel(img1forDraw, grad_x1, CV_16S, 1, 0, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::convertScaleAbs(grad_x1, grad_x1);

		cv::Mat grad_y1;
		cv::Sobel(img1, grad_y1, CV_16S, 0, 1, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::Sobel(img1forDraw, grad_y1, CV_16S, 0, 1, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::convertScaleAbs(grad_y1, grad_y1);
		addWeighted(grad_x1, 0.5, grad_y1, 0.5, 0, img1);
		addWeighted(grad_x1, 0.5, grad_y1, 0.5, 0, img1forDraw);

		cv::GaussianBlur(img2, img2, cv::Size(KSizeSobel, KSizeSobel), 0, 0, 4); //m0:sinnvoll das mit dem ksizeSobel zu koppeln? GaussianBlur Wird oben schon verwendet
		cv::GaussianBlur(img2forDraw, img2forDraw, cv::Size(KSizeSobel, KSizeSobel), 0, 0, 4);
		cv::Mat grad_x2;
		cv::Sobel(img2, grad_x2, CV_16S, 1, 0, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::Sobel(img2forDraw, grad_x2, CV_16S, 1, 0, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::convertScaleAbs(grad_x2, grad_x2);

		cv::Mat grad_y2;
		cv::Sobel(img2, grad_y2, CV_16S, 0, 1, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::Sobel(img2forDraw, grad_y2, CV_16S, 0, 1, KSizeSobel, ScaleSobel, DeltaSobel, 4);
		cv::convertScaleAbs(grad_y2, grad_y2);
		addWeighted(grad_x2, 0.5, grad_y2, 0.5, 0, img2);
		addWeighted(grad_x2, 0.5, grad_y2, 0.5, 0, img2forDraw);
	}

	if (UseScharrFilter == true) {
		cv::GaussianBlur(img1, img1, cv::Size(3, 3), 0, 0, 4);
		cv::GaussianBlur(img1forDraw, img1forDraw, cv::Size(3, 3), 0, 0, 4);

		cv::Mat grad_x1;
		cv::Scharr(img1, grad_x1, CV_16S, 1, 0, ScaleScharr, DeltaScharr, 4);
		cv::Scharr(img1forDraw, grad_x1, CV_16S, 1, 0, ScaleScharr, DeltaScharr, 4);
		cv::convertScaleAbs(grad_x1, grad_x1);

		cv::Mat grad_y1;
		cv::Scharr(img1, grad_y1, CV_16S, 0, 1, ScaleScharr, DeltaScharr, 4);
		cv::Scharr(img1forDraw, grad_y1, CV_16S, 0, 1, ScaleScharr, DeltaScharr, 4);
		cv::convertScaleAbs(grad_y1, grad_y1);
		addWeighted(grad_x1, 0.5, grad_y1, 0.5, 0, img1);
		addWeighted(grad_x1, 0.5, grad_y1, 0.5, 0, img1forDraw);

		cv::GaussianBlur(img2, img2, cv::Size(3, 3), 0, 0, 4);
		cv::GaussianBlur(img2forDraw, img2forDraw, cv::Size(3, 3), 0, 0, 4);

		cv::Mat grad_x2;
		cv::Scharr(img2, grad_x2, CV_16S, 1, 0, ScaleScharr, DeltaScharr, 4);
		cv::Scharr(img2forDraw, grad_x2, CV_16S, 1, 0, ScaleScharr, DeltaScharr, 4);
		cv::convertScaleAbs(grad_x2, grad_x2);

		cv::Mat grad_y2;
		cv::Scharr(img2, grad_y2, CV_16S, 0, 1, ScaleScharr, DeltaScharr, 4);
		cv::Scharr(img2forDraw, grad_y2, CV_16S, 0, 1, ScaleScharr, DeltaScharr, 4);
		cv::convertScaleAbs(grad_y2, grad_y2);
		addWeighted(grad_x2, 0.5, grad_y2, 0.5, 0, img2);
		addWeighted(grad_x2, 0.5, grad_y2, 0.5, 0, img2forDraw);
	}



	cv::Point matchPositionForMotionCorrection;
	// to find the template used for motion correction
	const int pointTypeForMotionCorrection = 1;
	

	PointVector pointList;
	cv::Point matchPosition;
	cv::Point matchPositionLeft;
	cv::Point matchPositionRight;
	// for Mat correlation
	cv::Mat matchImg;
	int x = 0;
	if(correlationTemplates[0].size() == 0 && correlationTemplates[1].size() > 0){
		x = 1;
	}

	for(unsigned int j = 0; j < correlationTemplates[x].size(); ++j)
	{
		for(unsigned int i = 0; i < correlationTemplates.size(); ++i) 
		{
			pointList = getPointList(i);
			if(!isInitialization && (correlationTemplates[0].size() == correlationTemplates[1].size()))
			{
				// adjust template if necessary
				if (tplSettingsModified == true){
					if(i == 0)
					{
						setTemplates(img1, lastPosition[i][j], pointList[j].type, i, j);
					}
					if(i == 1)
					{
						setTemplates(img2, lastPosition[i][j], pointList[j].type, i, j);
					}
				}
				

				if (useROI) {
					// set/adjust roi(region of interest)
					if(i == 0) //left Image
					{
						setROI(lastPosition[i][j], img1, i, j);
					}
					if(i == 1) //right Image
					{
						setROI(lastPosition[i][j], img2, i, j);
					}

					// settings for Mat correlation
					width = templatesRoi[i][j].cols - correlationTemplates[i][j].cols +1;
					height = templatesRoi[i][j].rows - correlationTemplates[i][j].rows +1;
					matchImg = templatesRoi[i][j];
				} else {
					// settings for Mat correlation
					if(i == 0){
						width = img1.cols;
						height = img1.rows;
						matchImg = img1; 
					}
					if(i == 1){
						width = img2.cols;
						height = img2.rows;
						matchImg = img2; 
					}
				}

				// apply normalized crosscorrelation
				cv::Mat correlation(height, width, CV_8U);

				if ((useBasicTpl == true || useModelBasedTpl == true) && numberOfRotations > 1 && pointList[j].type == 0) {
					matchPosition = getBestMaxLoc(correlation, i, j);	
				} else {
					if(pointList[j].type == pointTypeForMotionCorrection){
						trackMotionCorrectionPoint(matchImg, correlationTemplates[i][j], matchPosition, width, height);
						findLocalMaxima(correlation, i, j, false, pointList[j].type);
						matchPosition.x = matchPosition.x + (correlationTemplates[i][j].cols/2) + theOffset[i][j].x;
						matchPosition.y = matchPosition.y + (correlationTemplates[i][j].rows/2) + theOffset[i][j].y;
						if (markersVisible) {
						// draw all markers
							if(i == 0) {
								drawMarkers(img1forDraw, img2forDraw, pointList[j].type, matchPosition, 0, j);								
							}
							else {
								drawMarkers(img2forDraw, img1forDraw, pointList[j].type, matchPosition, 1, j);
							}
							/*if (i == 0) {
								drawMarkers(img1forDraw, img1forDraw, pointList[j].type, matchPosition, 0, j);
							}
							else {
								drawMarkers(img2forDraw, img2forDraw, pointList[j].type, matchPosition, 1, j);
							}*/
						}
						lastPosition[i][j] = cv::Point2d(matchPosition.x, matchPosition.y);
					}
					else {
						matchTemplate(matchImg, correlationTemplates[i][j], correlation, CV_TM_CCOEFF_NORMED);
						bool addOffset = (useROI && !(pointList[j].type == pointTypeForMotionCorrection && motionPositionSetManually));
						// find the maximum
						minMaxLoc(correlation, NULL, NULL, NULL, &matchPosition);
						matchPosition.x = matchPosition.x + (correlationTemplates[i][j].cols/2);
						matchPosition.y = matchPosition.y + (correlationTemplates[i][j].rows/2);

						// roi used -> adjust the matchPosition
						// Do not change with manual position, because this is always absolute.
						if (addOffset) {
							matchPosition.x += theOffset[i][j].x; 
							matchPosition.y += theOffset[i][j].y; 
						}
					
						// find the 2 best maxima
						findLocalMaxima(correlation, i, j, addOffset, pointList[j].type);
					}
				}

				// manually overwrite motion correction match, if the user has set the position: changed by INA
				if (motionPositionSetManually
					&& pointList[j].type == pointTypeForMotionCorrection) {
					matchPosition.x = manualMotionPosition[motionSetManuallyStreamNumber][0];
					matchPosition.y = manualMotionPosition[motionSetManuallyStreamNumber][1];

					// generate template of the manually set region
					if (motionSetManuallyStreamNumber == 0) {
						setTemplates(img1, matchPosition, pointList[j].type, 0, j);
						// remember position
						lastPosition[0][j] = cv::Point2f(matchPosition.x, matchPosition.y);
						if (markersVisible) {
							drawMarkers(img1forDraw, img2forDraw, pointList[j].type, lastPosition[0][j], 0, j);
							//drawMarkers(img1forDraw, img1forDraw, pointList[j].type, lastPosition[0][j], 0, j);
						
						}

					}
					if (motionSetManuallyStreamNumber == 1) {
						setTemplates(img2, matchPosition, pointList[j].type, 1, j);
						lastPosition[1][j] = cv::Point2f(matchPosition.x, matchPosition.y);	
						if (markersVisible) {
							drawMarkers(img2forDraw, img1forDraw, pointList[j].type, lastPosition[1][j], 1, j);
							//drawMarkers(img2forDraw, img2forDraw, pointList[j].type, lastPosition[1][j], 1, j);
						}

					}

						motionPositionSetManually = false;
				}

				//setGoodFeaturesToTrack(matchImg, i, j);
				
				if (i == 1 && pointList[j].type != pointTypeForMotionCorrection) {
				
					//select the point with the best correlation:
					//matchPositionLeft = correlationList[0][j].front().point;
					//matchPositionRight = correlationList[1][j].front().point;
					
					//select the two points with the min distance:
					getBestMatch(j, matchPositionLeft, matchPositionRight);
					
					//double p1[2] = { matchPositionLeft.x, matchPositionLeft.y }; // left match
					//double p2[2] = { matchPositionRight.x, matchPositionRight.y }; // right match
					//double p3[3];
					//biplaneGeometry->reconstruct3dPointFromImageCoordinates(p1, p2, p3);
					//f.open("trackingresultsSimpleBi3d.txt",  ios::out|ios::app);
					//f << "x:	" << p3[0] << "		;y:		" << p3[1] << "		;z:		" << p3[2]  << endl;
					//f.close();


					//getBestMatchWithInterpolation(i, j, matchPositionLeft, matchPositionRight);
					//getBestMatchWithHalfInterpolation(i, j, matchPositionLeft, matchPositionRight);
					// compute the epilines
					computeEpilines(matchPositionLeft, 0, j);
					computeEpilines(matchPositionRight, 1, j);
			
					if (markersVisible) {
						// draw all markers
						drawMarkers(img1forDraw, img2forDraw, pointList[j].type, matchPositionLeft, 0, j);
						drawMarkers(img2forDraw, img1forDraw, pointList[j].type, matchPositionRight, 1, j);
						/*drawMarkers(img1forDraw, img1forDraw, pointList[j].type, matchPositionLeft, 0, j);
						drawMarkers(img2forDraw, img2forDraw, pointList[j].type, matchPositionRight, 1, j);*/

		
						// drawing may cause problems for the next matching -> restore image to initial state
						/*image1->Initialize();	
						image2->Initialize();*/
					}
					lastPosition[0][j] = cv::Point2d(matchPositionLeft.x, matchPositionLeft.y);
					lastPosition[1][j] = cv::Point2d(matchPositionRight.x, matchPositionRight.y);
				}				
			}



			if (isInitialization) {			
				// draw all markers
				if (i == 0 && correlationTemplates[0].size() > j) {
					drawMarkers(img1forDraw, img2forDraw, pointList[j].type, lastPosition[0][j], i, j);
					//drawMarkers(img1forDraw, img1forDraw, pointList[j].type, lastPosition[0][j], i, j);
					
				}
				if (i == 1 && correlationTemplates[1].size() > j) {
					drawMarkers(img2forDraw, img1forDraw, pointList[j].type, lastPosition[1][j], i, j);
					//drawMarkers(img2forDraw, img2forDraw, pointList[j].type, lastPosition[1][j], i, j);
					
				}
			}
		}
	}

	int pointUsedForMotionCorrection = -1;

	// reconstruct matches
	for(unsigned int i = 0; i < min(lastPosition[0].size(), lastPosition[1].size()); ++i) {

		double p1[2] = { lastPosition[0][i].x, lastPosition[0][i].y }; // left match
		double p2[2] = { lastPosition[1][i].x, lastPosition[1][i].y }; // right match
		double p3[3];
		
		biplaneGeometry->reconstruct3dPointFromImageCoordinates(p1, p2, p3);
		
		detectedPositionsX.push_back(p3[0]);
		detectedPositionsY.push_back(p3[1]);
		detectedPositionsZ.push_back(p3[2]);

		
		
		// find point used for motion correction
		if (i < getPointList(0).size() && i < getPointList(1).size()
			&& getPointList(0)[i].type == pointTypeForMotionCorrection
			&& getPointList(1)[i].type == pointTypeForMotionCorrection) {
			pointUsedForMotionCorrection = i;
		}
	}

	if(pointUsedForMotionCorrection >= 0) {
		// reconstruct original position
		double p1[2] = { getPointList(0)[pointUsedForMotionCorrection].x, getPointList(0)[pointUsedForMotionCorrection].y };
		double p2[2] = { getPointList(1)[pointUsedForMotionCorrection].x, getPointList(1)[pointUsedForMotionCorrection].y };
		double p4[3];
		biplaneGeometry->reconstruct3dPointFromImageCoordinates(p1, p2, p4);

		// motion vector
		detectedMotion[0] = detectedPositionsX[pointUsedForMotionCorrection] - p4[0];
		detectedMotion[1] = detectedPositionsY[pointUsedForMotionCorrection] - p4[1];
		detectedMotion[2] = detectedPositionsZ[pointUsedForMotionCorrection] - p4[2];
	}

	motionPositionSetManually = false;
	isInitialization = false;

	// Settings modifications are adapted now.
	tplSettingsModified = false;
}

void SimpleBiplaneFilter::pointWasAdded(unsigned int listNumber, unsigned int pointNumber)
{
	Annotated2dPoint point = getPointList(listNumber)[pointNumber];

	if (motionPositionSetManually)
	{
		vector<cv::Point2d>::iterator toDeleteLastPosition = lastPosition[listNumber].begin();
		toDeleteLastPosition += pointNumber;
		lastPosition[listNumber].erase(toDeleteLastPosition);
		pointUsedForManualPosition[listNumber] = pointNumber;
		//lastPosition[listNumber][pointNumber] = cv::Point2d(point.x, point.y);
	}


	lastPosition[listNumber].push_back(cv::Point2d(point.x, point.y));

	vtkImageData* image;

	if (listNumber == 0)
		image = GetOutputImage(0);
	if (listNumber == 1)
		image = GetOutputImage(1);
			
	int dimensions[3];
	image->GetDimensions(dimensions);

	const int width = dimensions[0];
	const int heigth = dimensions[1];

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(heigth, width, CV_8U, image->GetScalarPointer());

	// if necessary set roi to represent the region of interest for template matching
	if (useROI == true)
		setROI(lastPosition[listNumber][pointNumber], img, listNumber, pointNumber); 

	setTemplates(img, cv::Point(point.x, point.y), point.type, listNumber, pointNumber);
	computeEpilines(cv::Point2d(point.x, point.y), listNumber, pointNumber);
	//setGoodFeaturesToTrack(img, listNumber, pointNumber);

	isInitialization = true;

	Modified();
}

void SimpleBiplaneFilter::pointWasDeleted(unsigned int listNumber, unsigned int pointNumber)
{
	cleanUp(listNumber, pointNumber);

	isInitialization = true;

	if (lastPosition[listNumber].size() == 0)
	{
		MotionX = 0.0;	// otherwise mesh is shifted when play,
		MotionY = 0.0;	//since computeMotion is called with motionX, motionY =/ 0
		MotionZ = 0.0;

	}


	Modified();
}

vector<AbstractFilterWidget*> SimpleBiplaneFilter::getPropertiesGUI()
{
	vector<AbstractFilterWidget*> res = BiplaneAlgorithm::getPropertiesGUI();
	res.push_back(new CrosscorrelationFilterWidget(dynamic_cast<BiplaneAlgorithm*>(this)));

	return res;
}

void SimpleBiplaneFilter::settingsChanged()
{
	isInitialization = true;
	Modified();
}

void SimpleBiplaneFilter::computeEpilines(cv::Point2d point, int imageNumber, int pointNumber)
{
	cv::Point2d myPoint;
	vector<cv::Point2d> epilinePoints;
	vector<cv::Vec3f> corrLine;
	cv::Mat myFundamentalMatrix;
	// set coords for tranformation
	myPoint.x = point.x;
	myPoint.y = point.y;
	// transform coords depending on the image
	if (imageNumber == 0)
		biplaneGeometry->firstSystem.transformFromImageToDetectorCoordinates(myPoint.x, myPoint.y);
	if (imageNumber == 1)
		biplaneGeometry->secondSystem.transformFromImageToDetectorCoordinates(myPoint.x, myPoint.y);
	// get fundamental matrix
	biplaneGeometry->getFundamentalMatrix(myFundamentalMatrix);
	// get corresponding epiline 
	corrLine = getCorrespondingEpiline(myFundamentalMatrix, myPoint, imageNumber);
	// get points for drawing the epiline
	epilinePoints = getEpiLinePoints(corrLine, imageNumber);
	// set the corresponsing points
	setEpilinePoints(epilinePoints, imageNumber, pointNumber);
}

void SimpleBiplaneFilter::getBestMatch(unsigned int pointNumber, cv::Point& bestMatchLeft, cv::Point& bestMatchRight)
{
	double minDistance = 100;
	double minDistanceTmp;
	double res[3];
	double p1[2];
	double p2[2];
	
	double corrPoint1[2];
	double corrPoint2[2];
	double corrPoint3[2];
	double corrPoint4[2];
	double bestPoint1[2];
	double bestPoint2[2];
	int iter1 = 0;
	int iter2 = 0;

	for(list<PointCorrelation>::const_iterator ci0 = correlationList[0][pointNumber].begin(); 
		ci0 != correlationList[0][pointNumber].end(); ++ci0)
	{
		if(iter1 == 0)
		{
			corrPoint1[0] = ci0->point.x;
			corrPoint1[1] = ci0->point.y;
		}
		else if(iter1 == 1)
		{
			corrPoint2[0] = ci0->point.x;
			corrPoint2[1] = ci0->point.y;
		}
		iter1++;

		for(list<PointCorrelation>::const_iterator ci1 = correlationList[1][pointNumber].begin();
			ci1 != correlationList[1][pointNumber].end(); ++ci1)
		{
			if(iter2 == 0)
			{
				corrPoint3[0] = ci1->point.x;
				corrPoint3[1] = ci1->point.y;
			}
			else if(iter2 == 1)
			{
				corrPoint4[0] = ci1->point.x;
				corrPoint4[1] = ci1->point.y;
			}
			iter2++;

			p1[0] = ci0->point.x;
			p1[1] = ci0->point.y;
			p2[0] = ci1->point.x;
			p2[1] = ci1->point.y;

			minDistanceTmp = biplaneGeometry->reconstruct3dPointFromImageCoordinates(p1, p2, res);

			if (minDistanceTmp < minDistance )
			{
				bestMatchLeft.x = p1[0];
				bestMatchLeft.y = p1[1];
				bestMatchRight.x = p2[0];
				bestMatchRight.y = p2[1];
				minDistance = minDistanceTmp;
				cout << "minDistance: " << minDistance << endl;
			}
		}
	}
	
	bestPoint1[0] = bestMatchLeft.x;
	bestPoint1[1] = bestMatchLeft.y;
	bestPoint2[0] = bestMatchRight.x;
	bestPoint2[1] = bestMatchRight.y;

	//OverlayScene::GetInstance()->addEpipolarLinesIn3dWindow(corrPoint1, corrPoint2, corrPoint3, corrPoint4, bestPoint1, bestPoint2);
}

void SimpleBiplaneFilter::getBestMatchWithInterpolation(unsigned int imageNumber, unsigned int pointNumber, cv::Point& bestMatchLeft, cv::Point& bestMatchRight)
{
	double minDistance1 = 100;
	double minDistance2 = 100;
	double minDistanceTmp1;
	double minDistanceTmp2;
	double res[3];
	double p1[2];
	double p2[2];
	double p3[2];
	double p4[2];
	
	double corrPoint1[2];
	double corrPoint2[2];
	double corrPoint3[2];
	double corrPoint4[2];
	double bestPoint1[2];
	double bestPoint2[2];
	int iter1 = 0;
	int iter2 = 0;
	
	for(list<PointCorrelation>::const_iterator ci0 = correlationList[0][pointNumber].begin(); 
		ci0 != correlationList[0][pointNumber].end(); ++ci0)
	{
		if(iter1 == 0)
		{
			corrPoint1[0] = ci0->point.x;
			corrPoint1[1] = ci0->point.y;
		}
		else if(iter1 == 1)
		{
			corrPoint2[0] = ci0->point.x;
			corrPoint2[1] = ci0->point.y;
		}
		iter1++;

		for(list<PointCorrelation>::const_iterator ci1 = correlationList[1][pointNumber].begin();
			ci1 != correlationList[1][pointNumber].end(); ++ci1)
		{	
			if(iter2 == 0)
			{
				corrPoint3[0] = ci1->point.x;
				corrPoint3[1] = ci1->point.y;
			}
			else if(iter2 == 1)
			{
				corrPoint4[0] = ci1->point.x;
				corrPoint4[1] = ci1->point.y;
			}
			iter2++;

			p1[0] = ci0->point.x;
			p1[1] = ci0->point.y;
			p2[0] = lastPosition[1][pointNumber].x + ((ci1->point.x - lastPosition[1][pointNumber].x)/2);
			p2[1] = lastPosition[1][pointNumber].y + ((ci1->point.y - lastPosition[1][pointNumber].y)/2);
			p3[0] = ci0->point.x + ((ci0->point.x - lastPosition[0][pointNumber].x)/2);
			p3[1] = ci0->point.y + ((ci0->point.y - lastPosition[0][pointNumber].y)/2);
			p4[0] = ci1->point.x;
			p4[1] = ci1->point.y;

			minDistanceTmp1 = biplaneGeometry->reconstruct3dPointFromImageCoordinates(p1, p2, res);
			minDistanceTmp2 = biplaneGeometry->reconstruct3dPointFromImageCoordinates(p3, p4, res);

			if (minDistanceTmp1 < minDistance1 )
			{
				bestMatchLeft.x = p1[0];
				bestMatchLeft.y = p1[1];
				//bestMatchRight.x = p2[0];
				//bestMatchRight.y = p2[1];
				minDistance1 = minDistanceTmp1;
				cout << "minDistance1: " << minDistance1 << endl;
			}
			if (minDistanceTmp2 < minDistance2 )
			{
				//bestMatchLeft.x = p1[0];
				//bestMatchLeft.y = p1[1];
				bestMatchRight.x = p4[0];
				bestMatchRight.y = p4[1];
				minDistance2 = minDistanceTmp2;
				cout << "minDistance2: " << minDistance2 << endl;
			}
		}
	}
	bestPoint1[0] = bestMatchLeft.x;
	bestPoint1[1] = bestMatchLeft.y;
	bestPoint2[0] = bestMatchRight.x;
	bestPoint2[1] = bestMatchRight.y;

	OverlayScene::GetInstance()->addEpipolarLinesIn3dWindow(corrPoint1, corrPoint2, corrPoint3, corrPoint4, bestPoint1, bestPoint2);
}

void SimpleBiplaneFilter::getBestMatchWithHalfInterpolation(unsigned int imageNumber, unsigned int pointNumber, cv::Point& bestMatchLeft, cv::Point& bestMatchRight)
{
	double minDistance1 = 100;
	double minDistanceTmp1;
	double res[3];
	double p1[2];
	double p2[2];
	
	double corrPoint1[2];
	double corrPoint2[2];
	double corrPoint3[2];
	double corrPoint4[2];
	double bestPoint1[2];
	double bestPoint2[2];
	int iter1 = 0;
	int iter2 = 0;
	
	for(list<PointCorrelation>::const_iterator ci0 = correlationList[0][pointNumber].begin(); 
		ci0 != correlationList[0][pointNumber].end(); ++ci0)
	{
		if(iter1 == 0)
		{
			corrPoint1[0] = ci0->point.x;
			corrPoint1[1] = ci0->point.y;
		}
		else if(iter1 == 1)
		{
			corrPoint2[0] = ci0->point.x;
			corrPoint2[1] = ci0->point.y;
		}
		iter1++;

		for(list<PointCorrelation>::const_iterator ci1 = correlationList[1][pointNumber].begin();
			ci1 != correlationList[1][pointNumber].end(); ++ci1)
		{	
			if(iter2 == 0)
			{
				corrPoint3[0] = ci1->point.x;
				corrPoint3[1] = ci1->point.y;
			}
			else if(iter2 == 1)
			{
				corrPoint4[0] = ci1->point.x;
				corrPoint4[1] = ci1->point.y;
			}
			iter2++;

			p1[0] = ci0->point.x;
			p1[1] = ci0->point.y;
			p2[0] = lastPosition[1][pointNumber].x + ((ci1->point.x - lastPosition[1][pointNumber].x)/2);
			p2[1] = lastPosition[1][pointNumber].y + ((ci1->point.y - lastPosition[1][pointNumber].y)/2);
			
			minDistanceTmp1 = biplaneGeometry->reconstruct3dPointFromImageCoordinates(p1, p2, res);

			if (minDistanceTmp1 < minDistance1 )
			{
				bestMatchLeft.x = p1[0];
				bestMatchLeft.y = p1[1];
				bestMatchRight.x = p2[0];
				bestMatchRight.y = p2[1];
				minDistance1 = minDistanceTmp1;
				cout << "minDistance1: " << minDistance1 << endl;
			}
		}
	}

	bestPoint1[0] = bestMatchLeft.x;
	bestPoint1[1] = bestMatchLeft.y;
	bestPoint2[0] = bestMatchRight.x;
	bestPoint2[1] = bestMatchRight.y;

	OverlayScene::GetInstance()->addEpipolarLinesIn3dWindow(corrPoint1, corrPoint2, corrPoint3, corrPoint4, bestPoint1, bestPoint2);
}

void SimpleBiplaneFilter::overwriteMotionCorrectionMatchPosition(const int streamNumber, const double pos[2])
{
	manualMotionPosition[streamNumber][0] = pos[0];
	manualMotionPosition[streamNumber][1] = pos[1];

	motionPositionSetManually = true;
	motionSetManuallyStreamNumber = streamNumber;

	Modified();
}