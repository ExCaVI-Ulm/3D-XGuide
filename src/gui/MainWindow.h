#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// QT includes
#include <QMainWindow>
#include <qslider.h>
#include "ui_MainWindow.h"

//own classes
class DICOMVisualizer;
class MeshesDialog;
class XRayViewer;
class OverlayScene;
class XRayViewer;

class vtkDistanceWidget;

#include <QCloseEvent>
#include <vtkSmartPointer.h>

#include <string>
#include <vector>
using namespace std;

/*** The MainWindow is the GUI for this project.
*/
class MainWindow : public QMainWindow, protected Ui::MainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, int argc = 0, char** argv = 0);
    ~MainWindow();

	void updateGUI();
	void updateGuiRegistrationPointList();

	void do3d3dRegistration();


private slots:
	//menu "file"
	void on_action3DMRI_coronal_triggered();
	void on_action3DMRI_axial_triggered();
	void on_actionSet_slice_spacing_manually_triggered();
	void on_actionLoad3DMRIDataset_triggered();
	/*void on_action3DMRI_coronal_triggered();
	void on_action3DMRI_axial_triggered();*/
	void on_actionLoad3DCTDatasetPHILIPS_triggered();
	void on_actionLoad3DCTDatasetITK_triggered();
	void on_actionOpenXrayViewer_triggered();

	void on_actionSaveScene_triggered();
	void on_actionLoadScene_triggered();

	void on_actionShowMeshSettings_toggled(bool checked);

	void closeEvent(QCloseEvent *event);

	//menu "display"
	void on_actionViewFront_triggered();
	void on_actionViewSide_triggered();
	void on_actionViewDown_triggered();

	void on_actionShowXRAYin3DScene_toggled(bool checked);

	void on_actionFirstXRAYdirection_triggered();
	void on_actionSecondXRAYdirection_triggered();

	// menu "help"
	void on_actionHowToUse_triggered();

	// 
	void on_hideAxesCheckBox_stateChanged(int state);
	void on_hideMRvolumeCheckBox_stateChanged(int state);

	void on_verticalSlider_X_valueChanged(int value);
	void on_verticalSlider_Y_valueChanged(int value);
	void on_verticalSlider_Z_valueChanged(int value);

	void on_sliderLevelWindow_valueChanged(int value);
	void on_buttonSave3DScene_clicked();

	

	// registration window
	void on_addPointButton_clicked();
	void on_removePointButton_clicked();
	void on_showPointsCheckBox_toggled(bool checked);
	void on_showPointLabelsCheckBox_toggled(bool checked);
	void on_changeColorButton_clicked();
	void on_registrationButton_clicked();

private:	

	void initVTKWindows();
	void updatePositionFromGui();
	void enableMenus();	
	/// load/save all parameters defining a scene
	void loadScene(const char* file);
	void saveScene(const char* file);
	void getFrameGrabberConfiguration();

	QDockWidget* meshDialogDockWidget;
	MeshesDialog* meshesDialog;

	OverlayScene* theOverlayScene; /// VTK pipeline
	XRayViewer* theXRayViewer;
	DICOMVisualizer* viewer;

	//vtkSmartPointer<vtkGDCMImageReader> XrayDICOMReader[2];	
	
	bool disableUpdates;
	/*bool MRsequence;*/
	int MeshOrientation;
	string reloadFile;
	string reloadDir;
	vtkSmartPointer< vtkDistanceWidget > distanceWidget;
	int frameGrabberNumber;

};

#endif // MAINWINDOW_H
