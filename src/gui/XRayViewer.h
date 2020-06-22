#ifndef XRAYVIEWER_H
#define XRAYVIEWER_H

//#include <QMainWindow>
#include "ui_XRayViewer.h"
#include <qwidget.h>
#include <qtimer.h>


class MainWindow;
class QMenu;
class QUndoStack;

class MoveMeshInteractionCommand;
class OverlayScene;

class vtkHandleRepresentation;
class vtkDistanceRepresentation;
class vtkDistanceWidget;
class vtkLineWidget;
class vtkFocalPlanePointPlacer;


#include <vtkSmartPointer.h>
#include "vtkCommand.h"
#include "vtkInteractorStyleMy2D.h"
#include <vtkTextActor.h>

#include <vtkLineWidget2.h>


class XRayViewer : public QMainWindow, protected Ui::XRayViewer, public vtkCommand
{
    Q_OBJECT

public:

	static XRayViewer* New(QWidget *parent, OverlayScene* scene);
    ~XRayViewer();

	/// handlesVTK popup menu events
	void Execute(vtkObject *caller, unsigned long eventId, void *callData);
	void displayAngleInWindow(int index, double primAngle, double secAngle);
	void generateUndoRedoCommandStack(MoveMeshInteractionCommand* cmd, int prevWindow, double prevMatrix[4][4]);
	//void getPreviousCommand(OverlayScene* scene, int prevWindow, double prevMatrix[4][4]);
	void stackClear();
	void stackSetClean();
	

	int getSliderValue(int index);
	void setSliderValue(int index, int value);
	void setUpdates(int index, bool disable);
	void activateGui(int framenumber, char* dir, int index);
	void clearTextActors(int index);
	void enableMarkerMenu();

	//std::vector<int> GeometryLiveNew;
	//std::vector<int> GeometryLive;

signals:

	private slots :

	void on_actionReload_Pipeline_for_Framegrabber_triggered();

	void on_actionSelectXRayReferenceFirst_triggered();
	void on_actionSelectXRayReferenceSecond_triggered();
	void on_actionSelectXRaySequence_triggered();
	void on_actionExitXRAY_triggered();
	//====================================================
	void on_actionLive_triggered();
	void on_radioButtonRecord_clicked();
	void on_actionLoad_Run_Biplane_triggered();
	void stopMain();
	//====================================================

	void on_actionLoadDefaultMeshPosition_triggered();
	void on_actionLoadSavedMeshPosition_triggered();
	void on_actionSaveMeshPosition_triggered();

	void on_actionBiplaneSystem_toggled(bool checked);
	void on_actionMonoplaneLiveView_toggled(bool checked);

	void on_actionMoveObjects_triggered();
	void on_actionMoveScene_triggered();

	void on_actionSetMarkerPoint_triggered();
	void on_actionReconstruct3dPoint_triggered();
	void on_actionReconstructAll3dPoints_triggered();
	void on_actionRemoveAll3dPoints_triggered();
	void on_actionShowMarkerPositions_toggled(bool checked);
	void on_actionShowMarkerLabels_toggled(bool checked);
	void on_actionSetMarkerColor_triggered();

	void on_actionSaveMarkerPositions_triggered();
	void on_actionLoadMarkerPositions_triggered();
	void on_actionClearMarkerPositions_triggered();

	void on_actionSelectFilter_triggered();
	void on_actionShowFilterProperties_triggered();

	void on_actionNextFrame_triggered();
	void on_actionPreviousFrame_triggered();
	void on_actionRemoveECG_triggered();
	void on_actionMeasureDistance_toggled(bool checked);


	void on_sliderBufferedFrame_valueChanged(int value);
	void on_sliderBufferedFrameSecondStream_valueChanged(int value);
	void on_sliderBufferedFrameMainStream_valueChanged(int value);
	void on_buttonPlay_clicked();
	void on_buttonPlaySecondStream_clicked();
	void on_buttonPlayMainStream_clicked();
	void on_buttonStop_clicked();
	void on_buttonStopSecondStream_clicked();
	//void on_buttonStopMainStream_clicked();
	void on_buttonSaveImageMainStream_clicked();
	void streamPlayer_update();
	void streamPlayerSecondStream_update();
	void streamPlayerMainStream_update();

	void on_buttonLoadECG_clicked();
	void on_buttonLoadECGSecondStream_clicked();
	void on_buttonLoadECGMainStream_clicked();

	void on_buttonLive_clicked();
	void on_ButtonLoadRunSecond_clicked();
	void on_ButtonLoadRunFirst_clicked();

	void on_comboBoxSelectFramegrabber_currentIndexChanged();

private:
	void Seed(bool checked);
	void AddDistanceMeasurementToView(bool checked);
	void setupNew(bool biplane);
	//void setupNew();
	void setupMainNew();

	XRayViewer(QWidget *parent, OverlayScene* scene);
	static XRayViewer* theInstance;
	QUndoStack* undoRedoStack;

	double previousMeshMatrix[4][4];
	int previousMeshWindow;

	/// handles for template selection
	vtkInteractorStyleMy2D* templateSelectorFirstStream;
	vtkInteractorStyleMy2D* templateSelectorSecondStream;
	vtkInteractorStyleMy2D* templateSelectorMainStream;

	bool ECGLoad;
	bool ECGLoadSecondStream;
	bool ECGLoadMainStream;
	bool disableUpdates;
	bool disableUpdatesMainStream;
	OverlayScene* scene;
	QString lastDirectory;
	//===========================================================
	QString patientDirectory;
	char* patientDirectoryChar;
	bool isButtonPlayMainClickedFirst;
	bool startPlay;
	bool isPause;
	bool startRecordAgain;
	int recordMode;
	ofstream f;
	QString filterType;
	bool isStop;
	//===========================================================

	/// file for filter settings
	QString filterSettingsFile;

	bool biplaneSystem;
	bool monoplaneView;
	bool inputIsFromFile; /// input is from file or from framegrabber
	bool biplaneTemplateSet[2];
	bool biplaneStartAgain[6][2]; // six cases, two streams

	QTimer* streamPlayer; /// to play saved streams
	QTimer* streamPlayerSecondStream; /// to play saved streams
	QTimer* streamPlayerMainStream; /// to play saved streams
	//vtkTextActor* textActor;
	vtkTextActor* txtActors[3];

	vtkSmartPointer<vtkDistanceWidget> distanceWidget;
	vtkSmartPointer<vtkLineWidget2> lineWidget;
	//===========================================================
	int run;
	/*const char* fileString;*/
	QByteArray dir;
	QByteArray dirSecondReference;
	QByteArray dirFirstReference;
	QByteArray dirReferenceBiplane;

	int startClock(char* txt);
	void stopClock(int startTime);
	//===========================================================
};

#endif // XRAYVIEWER_H
