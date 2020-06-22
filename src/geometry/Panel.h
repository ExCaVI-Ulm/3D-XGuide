#ifndef PANEL_H
#define PANEL_H

#include <cv.h>
#include <vtkImageLuminance.h>
#include <vtkImageData.h>

//using namespace cv;

/** Class for displayed 3D geometry information.
  */
class Panel {
public:

	Panel(bool rao, bool kaud, int raoLao, int kaudKran, int lon, int lat, int vert, int sid, int fd);
	//~Panel();

	void update(vtkImageData* image);
	bool getGeometryHasChanged();
	bool getFDHasChanged();
	bool getTablePosHasChanged();
	void getDisplayedValues(bool &rao, bool &kaud, int &raoLao, int &kaudKran, int &lon, int &lat, int &vert, int &sid, int &fd);
	void getTablePos(int &lon, int &lat, int &vert);
	void getFirstAngle(bool &rao, int &laorao);

private:
	void setupPattern();
	void recognizeDisplayedValues(vtkImageData* image);
	int CR(int characterNorm);
	void setDisplayedValues(bool rao, bool kaud, bool angulation, int raoLao, int kaudKran, int lon, int lat, int vert, int sid, int fd);
	void setGeometryHasChanged(bool geometryHasChanged);
	void setFDHasChanged(bool fdHasChanged);
	void setTablePosHasChanged(bool tablePosHasChanged);
	//==============================getgeometry-Variables========================
	// RAOLAO
	cv::Rect LAORAO_FirstRect, LAORAO_SecRect;
	// KAUDKRAN 
	cv::Rect KAUDKRAN_FirstRect, KAUDKRAN_SecRect, KAUDKRAN_ThirdRect;
	// Position
	cv::Point DEGREESIGN, FONTINK_RAOLAO, FONTINK_KAUDKRAN;

	//--------------------------Hoehe-----------------------
	// Hoehe first
	cv::Rect Hoehe_FirstRect, Hoehe_SecRect, Hoehe_ThirdRect;

	//--------------------------SID-----------------------
	// SID first
	cv::Rect SID_FirstRect, SID_SecRect, SID_ThirdRect;

	//--------------------------FD-----------------------
	// FD first
	cv::Rect FD_FirstRect, FD_SecRect;

	bool rao_; 
	bool kaud_; 
	int raoLao_; 
	int kaudKran_;
	int lon_; 
	int lat_; 
	int vert_;
	int sid_; 
	int fd_;

	bool geometryHasChanged_;
	bool fdHasChanged_;
	bool tablePosHasChanged_;
};


#endif
