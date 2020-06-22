#include "XRayViewer.h"
#include "OverlayScene.h"
#include "SceneLabeling.h"
#include "AlgorithmPropertiesDialog.h"
#include "vtkInteractorStyle2DRotateZoomWithViewAngle.h"

#include "vktInteractorStyleTrackballJoystickHybridActor.h"

#include "vtkInteractorStyleTrackballActor.h"
#include <vtkInteractorStyleImage.h>

#include <QFileDialog>
#include <qstring.h>
#include <qlist>
#include <qinputdialog>
#include <qmessagebox>
#include <qtoolbar>
#include <qstatusbar>
#include <QUndoStack>
#include <QColorDialog>


//////////////////////////
#include <QApplication>
#include <QKeyEvent>
#include <QGuiApplication>

//////////////////////////

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkCamera.h>
#include <vtkSmartPointer.h>

#include "MoveMeshInteractionCommand.h"

#include "vtkCommand.h"
#include "vtkCallbackCommand.h"

#include <vtkTextWidget.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTextRepresentation.h>
#include <vtkDistanceWidget.h>
#include <vtkLineWidget.h>
#include <vtkFocalPlanePointPlacer.h>
#include <vtkDistanceRepresentation2D.h>
#include "vtkHandleRepresentation.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkSeedRepresentation.h"

#include <vtkPropCollection.h>
#include <vtkAxisActor2D.h>
#include <vtkAxis.h>
#include <vtkProperty2D.h>

#include <vtkLineWidget2.h>
#include <vtkLineRepresentation.h>

#include <vector>
#include <string>
#include <math.h>


//////////////////
#include "opencv2/opencv.hpp"

using namespace cv;
/////////777///////


using namespace std;


XRayViewer* XRayViewer::theInstance = NULL;

XRayViewer* XRayViewer::New(QWidget *parent, OverlayScene* scene)
{
	if (!theInstance)
	{
		theInstance = new XRayViewer(parent, scene);
		return theInstance;
	}
	else
	{
		cout << "ERROR: XRayViewer::New should only be called once!" << endl;
		return NULL;
	}
}

XRayViewer::XRayViewer(QWidget *parent, OverlayScene* scene) :
	QMainWindow(parent), previousMeshWindow(-1), txtActors{ NULL, NULL, NULL }/*, distanceWidget(0), inputIsFromFile(false)*/, run(0), isButtonPlayMainClickedFirst(0),
	isPause(false), recordMode(0), biplaneSystem(false), monoplaneView(false), startPlay(false), biplaneTemplateSet{ false, false }, biplaneStartAgain{ false, false, false, false, false, false, false, false,  false, false, false, false }
{
	lastDirectory = "B:\\NAVIGATION\\data";
	/*patientDirectory = "..\\Patientname";*/
	//patientDirectoryChar = "..\\Patientname\\";
	patientDirectory = scene->patientDir;
	patientDirectoryChar = scene->patientDir;
	setupUi(this);	

	setWindowTitle("XRay Viewer");

	this->scene = scene;

	vtkWidgetFirstStream->SetRenderWindow(scene->getXRAYRenderWindow(0));
	vtkWidgetSecondStream->SetRenderWindow(scene->getXRAYRenderWindow(1));
	vtkWidgetMainStream->SetRenderWindow(scene->getXRAYRenderWindow(2));

	//disable rotation for 2D views
	vtkInteractorStyle2DRotateZoomWithViewAngle* style1 = vtkInteractorStyle2DRotateZoomWithViewAngle::New();
	vtkWidgetFirstStream->GetInteractor()->SetInteractorStyle(style1);
	vtkInteractorStyle2DRotateZoomWithViewAngle* style2 = vtkInteractorStyle2DRotateZoomWithViewAngle::New();
	vtkWidgetSecondStream->GetInteractor()->SetInteractorStyle(style2);
	vtkInteractorStyle2DRotateZoomWithViewAngle* style3 = vtkInteractorStyle2DRotateZoomWithViewAngle::New();
	vtkWidgetMainStream->GetInteractor()->SetInteractorStyle(style3);


	vtkWidgetFirstStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera()->SetParallelProjection(false);
	vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera()->SetParallelProjection(false);

	
	//// Synchronize interaction in both 2D views by setting the same camera
	/// however update is only on play or mouse tip in the other window
	/// morover this leads to the fact that some XRAY images are not displayed in the first XRAY view if no mesh is loaded
	/*vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->SetActiveCamera(
		vtkWidgetFirstStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera()
		);
*/
	
	//vtkCamera* cam1 = vtkWidgetFirstStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	//vtkCamera* cam2 = vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();

	//vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->SetActiveCamera(
	//cam1);
	//cam1->Modified();

	//vtkWidgetFirstStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->Modified();
	//vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->Modified();
		
	vtkWidgetFirstStream->GetRenderWindow()->Render();
	vtkWidgetSecondStream->GetRenderWindow()->Render();
	/* or alternatively this code:
	scene->getXRAYRenderWindow(0)->Render();
	scene->getXRAYRenderWindow(1)->Render();*/

	//vtkWidgetFirstStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCamera();
	//vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCamera();

	// for template selection
	templateSelectorFirstStream = vtkInteractorStyleMy2D::New();
	templateSelectorFirstStream->SetDefaultRenderer(vtkWidgetFirstStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
	templateSelectorSecondStream = vtkInteractorStyleMy2D::New();
	templateSelectorSecondStream->SetDefaultRenderer(vtkWidgetSecondStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
	templateSelectorFirstStream->AddObserver(vtkCommand::SelectionChangedEvent, this);
	templateSelectorSecondStream->AddObserver(vtkCommand::SelectionChangedEvent, this);
	templateSelectorFirstStream->AddObserver(vtkCommand::RightButtonPressEvent, this);
	templateSelectorFirstStream->AddObserver(vtkCommand::RightButtonReleaseEvent, this);
	templateSelectorSecondStream->AddObserver(vtkCommand::RightButtonPressEvent, this);
	templateSelectorSecondStream->AddObserver(vtkCommand::RightButtonReleaseEvent, this);

	templateSelectorMainStream = vtkInteractorStyleMy2D::New();
	templateSelectorMainStream->SetDefaultRenderer(vtkWidgetMainStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
	templateSelectorMainStream->AddObserver(vtkCommand::SelectionChangedEvent, this);
	templateSelectorMainStream->AddObserver(vtkCommand::RightButtonPressEvent, this);
	templateSelectorMainStream->AddObserver(vtkCommand::RightButtonReleaseEvent, this);

	actionBiplaneSystem->setChecked(biplaneSystem);
	actionMonoplaneLiveView->setChecked(monoplaneView);
	// set all vertical splitters to middle (i.e. widgets weights are 50%/50% within each QFrame)
	QList<int> weights;
	weights.append(512);
	weights.append(512);
	splitter->setSizes(weights);
	splitter_2->setSizes(weights);
	splitter_3->setSizes(weights);

	//connect(splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitter_moved()));
	

	// to play saved streams
	streamPlayer = new QTimer(this);
	connect(streamPlayer, SIGNAL(timeout()), this, SLOT(streamPlayer_update()));

	streamPlayerSecondStream = new QTimer(this);
	connect(streamPlayerSecondStream, SIGNAL(timeout()), this, SLOT(streamPlayerSecondStream_update()));

	streamPlayerMainStream = new QTimer(this);
	connect(streamPlayerMainStream, SIGNAL(timeout()), this, SLOT(streamPlayerMainStream_update()));

	/*QToolBar* toolBar = addToolBar("Tool bar");
	toolBar->setAllowedAreas(Qt::TopToolBarArea);
	toolBar->setMovable(false);
	toolBar->setFloatable(false);

	toolBar->addAction(actionMoveObjects);
	toolBar->addAction(actionMoveScene);*/

	actionMoveScene->setChecked(true);
	toolBar->addSeparator();
	undoRedoStack = new QUndoStack();
	toolBar->addAction(undoRedoStack->createUndoAction(toolBar));
	toolBar->addAction(undoRedoStack->createRedoAction(toolBar));

	/*distanceWidget = vtkDistanceWidget::New();	
	distanceWidget->CreateDefaultRepresentation();	
	static_cast<vtkDistanceRepresentation *>(distanceWidget->GetRepresentation())->SetLabelFormat("%-#6.3g mm");*/

	update();

	disableUpdates = true;
	disableUpdatesMainStream = true;
	ECGLoad = false;
	ECGLoadSecondStream = false;
	ECGLoadMainStream = false;
}

void XRayViewer::on_actionReload_Pipeline_for_Framegrabber_triggered() {

	scene->frontalInput = scene->lateralInput = false;
	if (scene->frameGrabbersNumber > 0)
	{
		scene->frontalInput = true;
		if (scene->frameGrabbersNumber > 1)
		{
			scene->lateralInput = true;
		}
		
	}
	on_actionBiplaneSystem_toggled(false);
	scene->reloadPipelineFramegrabber();
	setupMainNew();
	//on_actionBiplaneSystem_toggled(false);
}

void XRayViewer::on_comboBoxSelectFramegrabber_currentIndexChanged() {

	int index;
	index = comboBoxSelectFramegrabber->currentIndex();

	if (!scene->canReadLayout[index]) {// can not read layout
		if(index == 0) comboBoxSelectFramegrabber->setCurrentIndex(1);
		if (index == 1) comboBoxSelectFramegrabber->setCurrentIndex(0);
		QMessageBox::warning(this, "View",
			"Problem with the input.\nCheck the command window output for further information.");
		return;
	}

	if (!scene->frontalInput) { // no frontal input-> no tableposition and no useful imagefusion
		cout << "ERROR: no frontal input" << endl;
		QMessageBox::warning(this, "View",
			"No frontal input.\nFrontal input is needed for the recognition of the table position.");
		return;
	}

	if (index == 1 && !scene->lateralInput) { // no frontal input-> no tableposition and no useful imagefusion
		if (index == 1) comboBoxSelectFramegrabber->setCurrentIndex(0);
		cout << "ERROR: no lateral input" << endl;
		QMessageBox::warning(this, "View",
			"No lateral input.");
		return;
	}

	setupMainNew(); 
	scene->liveIndex = comboBoxSelectFramegrabber->currentIndex();
	on_buttonLive_clicked();
}

void XRayViewer::setupMainNew() {

	streamPlayerMainStream->stop();

	// slider
	labelCurrentFrameMainStream->setText("1");
	//sliderBufferedFrameMainStream->setValue(1);
	sliderBufferedFrameMainStream->setMaximum(1000);
	labelNumberOfFramesMainStream->setText("1000");

	isButtonPlayMainClickedFirst = 0;
	isPause = false;

	// Pipeline
	scene->alreadyConstructedPipeline[2] = false;
	scene->visualizeActors(2, false);

	// GUI - Main
	buttonPlayMainStream->setEnabled(false);
	scene->isLive = false;
	buttonLive->setEnabled(true);
	radioButtonRecord->setChecked(false);
	buttonLoadECGMainStream->setEnabled(true);
	isStop = true;
	//buttonStopMainStream->setEnabled(false);
	doLoopButtonMainStream->setEnabled(true);
	buttonSaveImageMainStream->setEnabled(true);
	sliderBufferedFrameMainStream->setEnabled(false);

	if (biplaneSystem) {
		actionLoad_Run_Biplane->setEnabled(true);
		comboBoxSelectFramegrabber->setEnabled(false);
	}
	else {
		actionLoad_Run_Biplane->setEnabled(false);
		comboBoxSelectFramegrabber->setEnabled(true);
	}

}

void XRayViewer::setupNew(bool biplane) {

	streamPlayer->stop();
	streamPlayerSecondStream->stop();
	streamPlayerMainStream->stop();

	//if (!biplane)
	//{// slider
	//	labelCurrentFrameMainStream->setText("1");
	//	sliderBufferedFrameMainStream->setValue(1);
	//	sliderBufferedFrameMainStream->setMaximum(350);
	//	labelNumberOfFramesMainStream->setText("350");
	//}

	// Pipeline
	scene->alreadyConstructedPipeline[0] = scene->alreadyConstructedPipeline[1] = scene->alreadyConstructedPipeline[2] = false;
	scene->visualizeActors(0, false);
	scene->visualizeActors(1, false);
	scene->visualizeActors(2, false);

	//// Mesh 
	//scene->loadDefaultMeshPosition(OverlayScene::CT_PHILIPS);
	scene->loadDefaultMeshPositionBool = false;

	//// motion compensation
	//scene->unsetAllTemplates(0);
	//scene->unsetAllTemplates(1);
	//scene->unsetAllTemplates(2);

	// GUI - Main
	buttonPlayMainStream->setEnabled(false);
	scene->isLive = false;
	buttonLive->setEnabled(true);
	radioButtonRecord->setChecked(false);
	buttonLoadECGMainStream->setEnabled(true);
	isStop = true;
	//buttonStopMainStream->setEnabled(false);
	doLoopButtonMainStream->setEnabled(true);
	buttonSaveImageMainStream->setEnabled(true);
	sliderBufferedFrameMainStream->setEnabled(false);

	// slider
		labelCurrentFrameMainStream->setText("1");
		//sliderBufferedFrameMainStream->setValue(1);
		sliderBufferedFrameMainStream->setMaximum(1000);
		labelNumberOfFramesMainStream->setText("1000");
	
	isButtonPlayMainClickedFirst = 0;
	isPause = false;

	if (biplaneSystem) {
		actionLoad_Run_Biplane->setEnabled(true);
		comboBoxSelectFramegrabber->setEnabled(false);
	}
	else {
		// GUI - first reference
		buttonLoadECG->setEnabled(true);
		ButtonLoadRunFirst->setEnabled(true);
		buttonStop->setEnabled(false);
		buttonPlay->setEnabled(false);
		sliderBufferedFrame->setEnabled(false);

		// GUI - second reference
		buttonLoadECGSecondStream->setEnabled(true);
		ButtonLoadRunSecond->setEnabled(true);
		buttonStopSecondStream->setEnabled(false);
		buttonPlaySecondStream->setEnabled(false);
		sliderBufferedFrameSecondStream->setEnabled(false);

		actionLoad_Run_Biplane->setEnabled(false);
		comboBoxSelectFramegrabber->setEnabled(true);
	}
	
}


void XRayViewer::on_actionBiplaneSystem_toggled(bool checked)
{
	if (!scene->canReadLayout[0] || !scene->canReadLayout[1]) {// can not read layout
		actionBiplaneSystem->setChecked(false);
		QMessageBox::warning(this, "Biplane System",
			"Problem with the input.\nCheck the command window output for further information.");
		return;
	}

	if (!scene->frontalInput) { // no frontal input-> no tableposition and no useful imagefusion
		actionBiplaneSystem->setChecked(false);
		cout << "ERROR: no frontal input" << endl;
		QMessageBox::warning(this, "Biplane System",
			"No frontal input.\nFrontal input is needed for the recognition of the table position.");
		return;
	}

	if (!scene->lateralInput) { // no frontal input-> no tableposition and no useful imagefusion
		actionBiplaneSystem->setChecked(false);
		cout << "ERROR: no lateral input" << endl;
		QMessageBox::warning(this, "Biplane System",
			"No lateral input.");
		return;
	}


	//scene->isFramegrabber = 1;
	biplaneSystem = checked;
	scene->biplaneSystem = checked;
	setupNew(checked);
	if (checked) {
		// set all vertical splitters to bottom (i.e. widgets weights are 100%/0% within each QFrame)
		QList<int> weights;
		weights.append(1);
		weights.append(0);
		splitter->setSizes(weights);
		splitter_2->setSizes(weights);
		splitter_3->setSizes(weights);

		actionMonoplaneLiveView->setChecked(!checked);
		monoplaneView = !checked;

		disableUpdates = false;

		//if (!scene->isActiveFilterBiplane()) {
		//	QMessageBox::warning(this, "Biplane system is selected",
		//		"Currently monoplane filter is selected.\nPlease choose biplane filter.");
		//}
	}

	else {
		// set all vertical splitters to middle (i.e. widgets weights are 50%/50% within each QFrame)
		if (monoplaneView)
		{
			QList<int> weights;
			weights.append(0);
			weights.append(1);
			splitter->setSizes(weights);

		}
		
		else
		{
			QList<int> weights;
			weights.append(512);
			weights.append(512);
			splitter->setSizes(weights);
			splitter_2->setSizes(weights);
			splitter_3->setSizes(weights);
		}

		

		
	}
	on_buttonLive_clicked();
}

void XRayViewer::on_actionMonoplaneLiveView_toggled(bool checked)
{
	monoplaneView = checked;

	if (checked) {
		// set all vertical splitters to bottom (i.e. widgets weights are 100%/0% within each QFrame)
		QList<int> weights;
		weights.append(0);
		weights.append(1);
		splitter->setSizes(weights);

		actionBiplaneSystem->setChecked(!checked);
		biplaneSystem = !checked;
		scene->biplaneSystem = !checked;
		//setupNew(biplaneSystem);

		streamPlayer->stop();
		streamPlayerSecondStream->stop();
		//streamPlayerMainStream->stop();
		
		if (scene->isActiveFilterBiplane()) {
			QMessageBox::warning(this, "Monoplane view is selected",
				"Currently biplane filter is selected.\nPlease choose monoplane filter.");
		}

	}

	else {

		if (biplaneSystem)
		{
			QList<int> weights;
			weights.append(1);
			weights.append(0);
			splitter->setSizes(weights);
			splitter_2->setSizes(weights);
			splitter_3->setSizes(weights);

		}

		else
		{
			// set all vertical splitters to middle (i.e. widgets weights are 50%/50% within each QFrame)
			QList<int> weights;
			weights.append(512);
			weights.append(512);
			splitter->setSizes(weights);
			splitter_2->setSizes(weights);
			splitter_3->setSizes(weights);

		}		

	}

}

void XRayViewer::on_actionMoveObjects_triggered()
{

	vktInteractorStyleTrackballJoystickHybridActor* style1 = vktInteractorStyleTrackballJoystickHybridActor::New();
	//vtkInteractorStyleTrackballCamera* style1 = vtkInteractorStyleTrackballCamera::New()
	//vtkInteractorStyleTrackballActor* style1 = vtkInteractorStyleTrackballActor::New();
	//vtkInteractorStyleImage* style1 = vtkInteractorStyleImage::New();
	style1->AddObserver(vtkCommand::EndInteractionEvent, this);
	style1->AddObserver(vtkCommand::RightButtonPressEvent, this);
	style1->AddObserver(vtkCommand::RightButtonReleaseEvent, this);
	style1->AddObserver(vtkCommand::StartInteractionEvent, this);
	vtkWidgetFirstStream->GetInteractor()->SetInteractorStyle(style1);
	style1->Delete();

	vktInteractorStyleTrackballJoystickHybridActor* style2 = vktInteractorStyleTrackballJoystickHybridActor::New();
	style2->AddObserver(vtkCommand::EndInteractionEvent, this);
	style2->AddObserver(vtkCommand::RightButtonPressEvent, this);
	style2->AddObserver(vtkCommand::RightButtonReleaseEvent, this);
	style2->AddObserver(vtkCommand::StartInteractionEvent, this);
	vtkWidgetSecondStream->GetInteractor()->SetInteractorStyle(style2);
	style2->Delete();

	if (disableUpdatesMainStream)
	{
		vktInteractorStyleTrackballJoystickHybridActor* style3 = vktInteractorStyleTrackballJoystickHybridActor::New();
		style3->AddObserver(vtkCommand::EndInteractionEvent, this);
		//style3->AddObserver(vtkCommand::RightButtonPressEvent, this);
		//style3->AddObserver(vtkCommand::RightButtonReleaseEvent, this);
		vtkWidgetMainStream->GetInteractor()->SetInteractorStyle(style3);
		style3->Delete();
		vtkWidgetMainStream->setCursor(Qt::OpenHandCursor);
	}

	actionMoveObjects->setChecked(true);
	actionMoveScene->setChecked(false);	

	vtkWidgetFirstStream->setCursor(Qt::OpenHandCursor);
	vtkWidgetSecondStream->setCursor(Qt::OpenHandCursor);


}

void XRayViewer::on_actionMoveScene_triggered()
{
	//vtkInteractorStyleTrackballCamera* style1 = vtkInteractorStyleTrackballCamera::New();
	vtkInteractorStyle2DRotateZoomWithViewAngle* style1 = vtkInteractorStyle2DRotateZoomWithViewAngle::New();
	vtkWidgetFirstStream->GetInteractor()->SetInteractorStyle(style1);
	style1->Delete();

	//vtkInteractorStyleTrackballCamera* style2 = vtkInteractorStyleTrackballCamera::New();
	vtkInteractorStyle2DRotateZoomWithViewAngle* style2 = vtkInteractorStyle2DRotateZoomWithViewAngle::New();
	vtkWidgetSecondStream->GetInteractor()->SetInteractorStyle(style2);
	style2->Delete();

	//vtkInteractorStyleTrackballCamera* style3 = vtkInteractorStyleTrackballCamera::New();
	vtkInteractorStyle2DRotateZoomWithViewAngle* style3 = vtkInteractorStyle2DRotateZoomWithViewAngle::New();
	vtkWidgetMainStream->GetInteractor()->SetInteractorStyle(style3);	
	style3->Delete();

	actionMoveObjects->setChecked(false);
	actionMoveScene->setChecked(true);

	vtkWidgetFirstStream->setCursor(Qt::ArrowCursor);
	vtkWidgetSecondStream->setCursor(Qt::ArrowCursor);
	vtkWidgetMainStream->setCursor(Qt::ArrowCursor);

}
void XRayViewer::on_actionSetMarkerPoint_triggered()
{
	vtkWidgetFirstStream->GetInteractor()->SetInteractorStyle(templateSelectorFirstStream);
	vtkWidgetSecondStream->GetInteractor()->SetInteractorStyle(templateSelectorSecondStream);
	vtkWidgetMainStream->GetInteractor()->SetInteractorStyle(templateSelectorMainStream);

	vtkWidgetFirstStream->setCursor(Qt::CrossCursor);
	vtkWidgetSecondStream->setCursor(Qt::CrossCursor);
	vtkWidgetMainStream->setCursor(Qt::CrossCursor);
}

void XRayViewer::on_actionRemoveAll3dPoints_triggered()
{
	if (scene->getMarkerLabeling(2)->getNumberOfPoints() > 0)
	{
		scene->getMarkerLabeling(2)->deletePoints();
	}
	else
	{
		QMessageBox::information(this, "Remove 3D points", "There is no reconstructed 3D points.");
		return;
	}
}


void XRayViewer::on_actionReconstructAll3dPoints_triggered()
{
	int anz = min(
		scene->getMarkerLabeling(0)->getNumberOfPoints(),
		scene->getMarkerLabeling(1)->getNumberOfPoints()
		);
	if (anz == 0) {
		QMessageBox::information(this, "Reconstruct 3D point", "Please set points in the 2D windows first.");
		return;
	}

	if (scene->getMarkerLabeling(2)->getNumberOfPoints() > 0) {
		if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Reconstruct 3D point", "Should the points in the 3D window be deleted first?", QMessageBox::Yes | QMessageBox::No).exec())
		/*if (QMessageBox::question(this, "Reconstruct 3D point", "Should the points in the 3D window be deleted first?", "Delete", "Dont't delete"))*/
		{
			scene->getMarkerLabeling(2)->deletePoints();

		}
	}
	//scene->getMarkerLabeling(0)->getNumberOfPoints();
	double p1[3], p2[3], p3[3];
	QString label;
	for (int i = 0; i < anz; ++i) {
		scene->getMarkerLabeling(0)->getPointPosition(i, p1);
		scene->getMarkerLabeling(1)->getPointPosition(i, p2);

		// p3 is the result
		double distance = scene->reconstruct3dPointWithSkewLinesIntersection(p1, p2, p3);
		scene->getMarkerLabeling(2)->setStartPosition(p3);
		label = QString("%1").arg(i + 1);
		scene->getMarkerLabeling(2)->addPoint(qPrintable(label));
	}
}

void XRayViewer::on_actionReconstruct3dPoint_triggered()
{
	int anz = min(
		scene->getMarkerLabeling(0)->getNumberOfPoints(),
		scene->getMarkerLabeling(1)->getNumberOfPoints()
		);
	if (anz == 0) {
		QMessageBox::information(this, "Reconstruct 3D point", "Please set points in the 2D windows first.");
		return;
	}

	int num;
	if (anz == 1) { num = 1; }
	else {
		bool ok;
		num = QInputDialog::getInt(this, "Reconstruct 3D point", "Please select the point to reconstruct.", anz, 1, anz, 1, &ok);
		if (!ok) return;
	}

	double p1[3] = { 0,0,0 };
	scene->getMarkerLabeling(0)->getPointPosition(num - 1, p1);

	double p2[3] = { 0,0,0 };
	scene->getMarkerLabeling(1)->getPointPosition(num - 1, p2);

	// p3 is the result
	double p3[3] = { 0,0,0 };
	double distance = scene->reconstruct3dPointWithSkewLinesIntersection(p1, p2, p3);

	scene->getMarkerLabeling(2)->setStartPosition(p3);
	scene->getMarkerLabeling(2)->addPoint(qPrintable(QString("%1").arg(num)));
}

void XRayViewer::on_actionShowMarkerPositions_toggled(bool checked)
{
	for (unsigned int i = 0; i < 3; ++i) {
		scene->getMarkerLabeling(i)->setPointVisibility(checked);
	}

}

void XRayViewer::on_actionShowMarkerLabels_toggled(bool checked)
{
	for (unsigned int i = 0; i < 3; ++i) {
		scene->getMarkerLabeling(i)->setLabelVisibility(checked);
	}
}

void XRayViewer::on_actionSetMarkerColor_triggered()
{
	double c[3];
	scene->getMarkerLabeling(0)->getMarkerColor(c);
	QColor col((int)(c[0] * 255.0), (int)(c[1] * 255.0), (int)(c[2] * 255.0));

	QColor col2 = QColorDialog::getColor(col, this, "Select marker color");
	if (!col2.isValid()) return;

	int r, g, b;
	col2.getRgb(&r, &g, &b);
	c[0] = r / 255.0;
	c[1] = g / 255.0;
	c[2] = b / 255.0;

	for (int i = 0; i < 3; ++i) {
		scene->getMarkerLabeling(i)->setMarkerColor(c);
	}
}

void XRayViewer::on_actionSaveMarkerPositions_triggered()
{
	QString path = QFileDialog::getSaveFileName(this, "Save 3D position points", ".", "VTK files (*.vtk)");
	if (!path.isNull()) scene->getMarkerLabeling(2)->writePoints(qPrintable(path));

	path = QFileDialog::getSaveFileName(this, "Save 2D position points from Reference 1", ".", "VTK files (*.vtk)");
	if (!path.isNull()) scene->getMarkerLabeling(0)->writePoints(qPrintable(path));

	path = QFileDialog::getSaveFileName(this, "Save 2D position points from Reference 2", path.isNull() ? "." : path, "VTK files (*.vtk)");
	if (!path.isNull()) scene->getMarkerLabeling(1)->writePoints(qPrintable(path));
}

void XRayViewer::on_actionLoadMarkerPositions_triggered()
{
	QString positions3dPath = QFileDialog::getOpenFileName(this, "Load 3D position points", ".", "VTK files (*.vtk)");
	if (!positions3dPath.isNull()) scene->getMarkerLabeling(2)->loadPoints(qPrintable(positions3dPath));
	scene->getMarkerLabeling(2)->updateLabelPositions();

	QString positions2dPath1 = QFileDialog::getOpenFileName(this, "Load 2D position points from Reference 1", positions3dPath.isNull() ? "." : positions3dPath, "VTK files (*.vtk)");
	if (!positions2dPath1.isNull()) scene->getMarkerLabeling(0)->loadPoints(qPrintable(positions2dPath1));
	scene->getMarkerLabeling(0)->updateLabelPositions();

	QString positions2dPath2 = QFileDialog::getOpenFileName(this, "Load 2D position points from Reference 2", positions2dPath1.isNull() ? "." : positions2dPath1, "VTK files (*.vtk)");
	if (!positions2dPath2.isNull()) scene->getMarkerLabeling(1)->loadPoints(qPrintable(positions2dPath2));
	scene->getMarkerLabeling(1)->updateLabelPositions();
}

void XRayViewer::on_actionClearMarkerPositions_triggered()
{
	QMessageBox::StandardButton btn = QMessageBox::question(
		this,
		"Delete marker positions",
		"Delete all marked positions?",
		QMessageBox::Ok | QMessageBox::Cancel
		);
	if (btn != QMessageBox::Ok) return;

	scene->getMarkerLabeling(0)->deletePoints();
	scene->getMarkerLabeling(1)->deletePoints();
	scene->getMarkerLabeling(2)->deletePoints();
}

void XRayViewer::on_actionSelectFilter_triggered()
{
	if (!scene) return;

	vector<std::string> filterTypes;

	//filterTypes = scene->getMonoplaneFilterTypesList();
	filterTypes = scene->getFilterTypesList();

	QStringList types;
	for (unsigned int i = 0; i < filterTypes.size(); ++i) {
		types << filterTypes[i].c_str();
	}

	int currentFilterIndex = types.indexOf(scene->getCurrentFilterType().c_str());

	bool ok;
	/*QString filterType = QInputDialog::getItem(this, "Select filter type", "Please select the filter type", types, currentFilterIndex, false, &ok);*/
	filterType = QInputDialog::getItem(this, "Select filter type", "Please select the filter type", types, currentFilterIndex, false, &ok);
	if (!ok) return;

	scene->setFilterToType(qPrintable(filterType));
	//statusbar->showMessage(QString("Current filter: ").append(filterType));
}

void XRayViewer::on_actionShowFilterProperties_triggered()
{
	if (!scene) return;

	if (scene->isActiveFilterBiplane()) {
		QString prefix = "biplane";

		AlgorithmPropertiesDialog* dialog
			= new AlgorithmPropertiesDialog(this, scene->getBiplaneAlgorithmPropertiesGUI(), filterSettingsFile, prefix);
		dialog->exec();
		delete dialog;
	}
	else {

		QString prefix = "monoplane";

		AlgorithmPropertiesDialog* dialog
			= new AlgorithmPropertiesDialog(this, scene->getMonoplaneAlgorithmPropertiesGUI(), filterSettingsFile, prefix);
		dialog->exec();
		delete dialog;
	}
}

////////////////////////////////////////////////////////////////////////
// vtkCommand implementation
////////////////////////////////////////////////////////////////////////
void XRayViewer::Execute(vtkObject *caller, unsigned long eventId, void *callData)
{
	if (eventId == vtkCommand::RightButtonPressEvent || eventId == vtkCommand::StartInteractionEvent) {
		startRecordAgain = scene->isRecording;
		stopMain();
		if (isPause || !scene->isFramegrabber) {
		}
		else {
			scene->isLive = true;
		}
	}

	if (eventId == vtkCommand::SelectionChangedEvent) { //rubber band selection
		vtkInteractorStyleMy2D* style = dynamic_cast<vtkInteractorStyleMy2D*>(caller);
		if (!style) return; // cast failed
		startRecordAgain = scene->isRecording;
		// pos is in VTK world coordinates
		double* pos = reinterpret_cast<double*>(callData);
		
		unsigned int streamNumber;

		if (style == templateSelectorFirstStream)
		{
			streamNumber = 0;
		}
		else if (style == templateSelectorSecondStream)
		{
			streamNumber = 1;
		}
		else if (style == templateSelectorMainStream)
		{
			streamNumber = 2;
		}

		// show popup menu at cursor position
		QMenu* popupMenu = new QMenu();
		if (streamNumber == 0 || streamNumber == 1)
		{
			popupMenu->addAction("Add marker point");
			popupMenu->addAction("Remove marker point");
			popupMenu->addAction("Add biplane catheter tracking template");
			popupMenu->addAction("Add biplane motion compensation template");
			popupMenu->addAction("Use this tracking for CT volume");
			popupMenu->addAction("Remove biplane template");
			popupMenu->addAction("Remove all biplane templates");
			popupMenu->addAction("Manually set biplane motion compensation here");

		}
		else
		{
			//popupMenu->addAction("Add catheter tracking template");
			popupMenu->addAction("Add motion compensation template");
			popupMenu->addAction("Remove nearest template");
			popupMenu->addAction("Remove all templates");
			popupMenu->addAction("Manually set motion compensation here");
			
		}

		QAction* action = popupMenu->exec(QCursor::pos());
		int actionIndex = 0;
		startRecordAgain = scene->isRecording;
		if (action == 0) { // user cancelled
			delete popupMenu;
			return;
		}
			if (action->text() == "Add marker point") {
				scene->addMarkerPoint(streamNumber, pos);
				actionIndex = 1;
			}
			if (action->text() == "Remove marker point") {
				scene->removeNearestMarkerPoint(streamNumber, pos);
				actionIndex = 0;
			}

			if (action->text() == "Add biplane catheter tracking template") {
				const int TRACKING_TYPE = 0;
				scene->addTemplate(streamNumber, pos, TRACKING_TYPE);
				actionIndex = 2;
			}

			if (action->text() == "Add biplane motion compensation template") {
				const int MOTION_COMPENSATION_TYPE = 1;
				scene->addTemplate(streamNumber, pos, MOTION_COMPENSATION_TYPE);
				actionIndex = 3;
			}
			if (action->text() == "Use this tracking for CT volume") {
				scene->setTrackingPointIndexForMRVolume(streamNumber, pos);
				actionIndex = 4;
			}

			if (action->text() == "Remove biplane template") {
				scene->unsetNearestTemplate(streamNumber, pos);
				actionIndex = 0;
			}

			if (action->text() == "Remove all biplane templates") {
				scene->unsetAllTemplates(streamNumber);
				actionIndex = 0;
			}

			if (action->text() == "Manually set biplane motion compensation here") {
				scene->setTemplatePositionForMotionCompensation(streamNumber, pos);
				actionIndex = 5;
			}

			if (action->text() == "Add catheter tracking template") {
				const int TRACKING_TYPE = 0;
				scene->addTemplate(2, pos, TRACKING_TYPE);
				actionIndex = 0;
			}

			if (action->text() == "Add motion compensation template") {
				const int MOTION_COMPENSATION_TYPE = 1;
				scene->addTemplate(2, pos, MOTION_COMPENSATION_TYPE);
				actionIndex = 0;
			}

			if (action->text() == "Remove nearest template") {
				scene->unsetNearestTemplate(2, pos);
				actionIndex = 0;
			}

			if (action->text() == "Remove all templates") {
				scene->unsetAllTemplates(2);
				actionIndex = 0;
			}

			if (action->text() == "Manually set motion compensation here") {
				scene->setTemplatePositionForMotionCompensation(2, pos);
				actionIndex = 0;
			}

			biplaneStartAgain[actionIndex][streamNumber] = true;
			//// nicht schoen
			// nach stop-> starte play wieder, falls play oder nicht framegrabber
			if (isPause) {	//|| !scene->isFramegrabber 
				on_buttonPlayMainStream_clicked();
			}
			// nach stop-> falls kein Input vom Reader existiert-> live oder record
			else if (scene->canReadLayout[0] &&  scene->canReadLayout[1]) {
				// starte erst wieder, wenn in beiden referenzen ein template etc. gesetzt wurde
				if (biplaneStartAgain[actionIndex][0] && biplaneStartAgain[actionIndex][1] || actionIndex == 0) {
					if (startRecordAgain) {
						radioButtonRecord->setChecked(startRecordAgain);
						on_radioButtonRecord_clicked();
					}
					else {
						on_buttonLive_clicked();
					}
					biplaneStartAgain[actionIndex][0] = biplaneStartAgain[actionIndex][1] = false;
				}
			}
			//releaseKeyboard();
			//update();

		// do not delete the menu before using its actions (i.e. menu items)!
		delete popupMenu;
		return;
	}//SelectionChangedEvent

	if (eventId == vtkCommand::RightButtonReleaseEvent) {

		if (isPause || !scene->isFramegrabber) {
			on_buttonPlayMainStream_clicked();
		}
		else if (scene->canReadLayout[0] && scene->canReadLayout[1]) {
			if (startRecordAgain) {
				radioButtonRecord->setChecked(startRecordAgain);
				on_radioButtonRecord_clicked();
			}
			else {
				on_buttonLive_clicked();
			}
		}
	}

	if (eventId == vtkCommand::EndInteractionEvent) {
		QWidget* wgt = focusWidget();
		if (!wgt) return;
	
		// is not necessary now, since first command goes in the stack only when XRAY images are loaded
		// it means previousMeshWindow = 2 is called from selectXRAYReferenceImages
/*
		if (previousMeshWindow == -1)
		{
			previousMeshWindow = 2;
			scene->getUserMatrix(previousMeshWindow, previousMeshMatrix);

		}	*/		

		//scene->updateMeshPositionsFromWindow(windowNumber);

		int windowNumber = 0;
		if (wgt == vtkWidgetFirstStream) { windowNumber = 0; }
		if (wgt == vtkWidgetSecondStream) { windowNumber = 1; }
		if (wgt == vtkWidgetMainStream) { windowNumber = 3; }

		//scene->updateMeshPositionsFromWindow(windowNumber);


		//// generate undo/redo command
		//double matrix[4][4];
		//scene->getUserMatrix(windowNumber, matrix);

		if (windowNumber == 3)
		{
			if (disableUpdatesMainStream)
			{
				clearTextActors(2);
				double matrix[4][4];
				scene->getUserMatrix(windowNumber, matrix);
				double Yaw, Pitch, Roll;
				const double PI = 3.141592653589793;


				if (matrix[0][0] == 1.0f)
				{
					Yaw = (atan2f(matrix[0][2], matrix[2][3])) * 180 / PI;
					Pitch = 0;
					Roll = 0;

				}
				else if (matrix[0][0] == -1.0f)
				{
					Yaw = (atan2f(matrix[0][2], matrix[2][3])) * 180 / PI;
					Pitch = 0;
					Roll = 0;
				}
				else
				{
					if (scene->getMRInputFileForMesh() == 3 || scene->getMRInputFileForMesh() == 1)
					{
						/////here is the case for the ITK-SNAP mesh orientation
						Yaw = 180 - (atan2(-matrix[2][0], matrix[0][0])) * 180 / PI;
						Pitch = (asin(matrix[1][0])) * 180 / PI;
						Roll = (atan2(-matrix[1][2], matrix[1][1])) * 180 / PI + 90.0;
						displayAngleInWindow(2, Yaw, Roll);
					}

					if (scene->getMRInputFileForMesh() == 2)
					{
						Yaw = (atan2(-matrix[2][0], matrix[0][0])) * 180 / PI;
						Pitch = (asin(matrix[1][0])) * 180 / PI;
						Roll = (atan2(-matrix[1][2], matrix[1][1])) * 180 / PI + 90.0;
						displayAngleInWindow(2, -Yaw, Roll);
					}

					if (scene->getMRInputFileForMesh() == 0)
					{
						Yaw = (atan2(-matrix[2][0], matrix[0][0])) * 180 / PI;
						Pitch = (asin(matrix[1][0])) * 180 / PI;
						Roll = 180 + (atan2(-matrix[1][2], matrix[1][1])) * 180 / PI;
						displayAngleInWindow(2, -Yaw, Roll);
					}

				}
				
			
			}
	
				/*if (disableUpdatesMainStream)
				{
					clearTextActors(2);
					displayAngleInWindow(2, Yaw, Roll);
				}*/
			

		}
		

		if ((windowNumber == 0 || windowNumber == 1) && !disableUpdates)
		{ 
			if (disableUpdatesMainStream)
			{
				clearTextActors(2);
			}
			
			scene->updateMeshPositionsFromWindow(windowNumber);


			// generate undo/redo command
			double matrix[4][4];
			scene->getUserMatrix(windowNumber, matrix);

			MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, previousMeshWindow, previousMeshMatrix, windowNumber, matrix);
			undoRedoStack->push(cmd);

			// store actual values as previous (for next movement)
			previousMeshWindow = windowNumber;
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					previousMeshMatrix[i][j] = matrix[i][j];
				}
			}

			//scene->updateMeshPositionsFromWindow(windowNumber);

			/*if (disableUpdates)
			{
				clearTextActors(0);
				clearTextActors(1);


				displayAngleInWindow(0, Yaw, Roll);
				displayAngleInWindow(1, Yaw, Roll);

			}*/


			/*if (disableUpdatesMainStream)
			{
				clearTextActors(2);
				displayAngleInWindow(2, Yaw, Roll);
			}*/
		}
		

		//// nicht schoen
		if (isPause /*|| !scene->isFramegrabber*/) {
			on_buttonPlayMainStream_clicked();
		}
		else if (scene->biplaneSystem){
			if (scene->canReadLayout[0] && scene->canReadLayout[1]) {
				if (startRecordAgain) {
					radioButtonRecord->setChecked(startRecordAgain);
					on_radioButtonRecord_clicked();
				}
				else {
					on_buttonLive_clicked();
				}
			}
		}
		else if (scene->canReadLayout[0]) {
			if (startRecordAgain) {
				radioButtonRecord->setChecked(startRecordAgain);
				on_radioButtonRecord_clicked();
			}
			else {
				on_buttonLive_clicked();
			}
		}

	 }//EndInteractionEvent

}

void XRayViewer::on_actionSelectXRaySequence_triggered()
{
	QByteArray file;
	buttonStop->click();
	buttonStopSecondStream->click();
	stopMain();

	scene->isFramegrabber = false;
	scene->isLive = false;
	buttonLive->setEnabled(true);


	QString fileFormats("Image files (*.dcm);; All files (*)");

	QString fileName = QFileDialog::getOpenFileName(this,
		"Select first XRAY Image file", lastDirectory, fileFormats);
	if (fileName.isNull()) return;
	file.push_back(qPrintable(fileName));

	int pos = fileName.lastIndexOf('/');
	lastDirectory = fileName.left(pos);

	const char* fileString;
	fileString = file; // implicit conversion
	

	scene->setMainXRAYInputToFile(fileString);
	buttonLoadECGMainStream->setEnabled(true);
	
	//buttonStopMainStream->click();
	labelCurrentFrameMainStream->setText("1");
	sliderBufferedFrameMainStream->setValue(1);
	sliderBufferedFrameMainStream->setMaximum(scene->getNumberOfFrames(2));
	labelNumberOfFramesMainStream->setText(QString("%1").arg(scene->getNumberOfFrames(2)));

	clearTextActors(2);

	double primAngle, secAngle;
	scene->getDICOMAnglesToWindow(primAngle, secAngle);
	displayAngleInWindow(2, primAngle, secAngle);

	disableUpdatesMainStream = false;
	//actionMoveScene->changed();
	actionMoveScene->setChecked(true);
	actionMoveObjects->setChecked(false);
	buttonPlayMainStream->setEnabled(true);

	double defaultNullDouble = 0.0;
	int defaultNullInt = 0;
}

int XRayViewer::getSliderValue(int index)
{
	if (index == 0)
	{
		return sliderBufferedFrame->value();
	}
	if (index == 1)
	{
		return sliderBufferedFrameSecondStream->value();
	}
	if (index == 2)
	{
		return sliderBufferedFrameMainStream->value();
	}
		
}
//
//int XRayViewer::getECGValue(int index)
//{
//	if (ECGLoad)
//	{
//		return sliderBufferedFrame->value();
//	}
//	if (index == 1)
//	{
//		return sliderBufferedFrameSecondStream->value();
//	}
//	
//}

void XRayViewer::setSliderValue(int index, int value)
{
	QString QValue = QString::number(value);	
	
	if (index == 0)
	{
		sliderBufferedFrame->setValue(value);
		labelCurrentFrame->setText(QValue);
		sliderBufferedFrame->setMaximum(scene->getNumberOfFrames(index));
		labelNumberOfFrames->setText(QString("%1").arg(scene->getNumberOfFrames(index)));	

	}
	if (index == 1)
	{
		sliderBufferedFrameSecondStream->setValue(value);
		labelCurrentFrameSecondStream->setText(QValue);
		sliderBufferedFrameSecondStream->setMaximum(scene->getNumberOfFrames(index));
		labelNumberOfFramesSecondStream->setText(QString("%1").arg(scene->getNumberOfFrames(index)));

	}

	if (index == 2)
	{
		sliderBufferedFrameMainStream->setValue(value);
		labelCurrentFrameMainStream->setText(QValue);
		sliderBufferedFrameMainStream->setMaximum(scene->getNumberOfFrames(index));
		labelNumberOfFramesMainStream->setText(QString("%1").arg(scene->getNumberOfFrames(index)));
	}

}

void XRayViewer::setUpdates(int index, bool disable)
{

	if (index == 0 || index == 1)
	{
		disableUpdates = disable;
	}

	if (index == 2)
	{
		disableUpdatesMainStream = disable;
	}

}

void XRayViewer::activateGui(int framenumber, char* dir, int index)
{
	QString QValue = QString::number(framenumber);

	if (scene->isFramegrabber)
	{
		char int2char[10];
		sprintf(int2char, "%d", framenumber);
		char * H = ".dcm";
		char * N = "\\";
		//char * N = "/";
		char * bufferNew = new char[strlen(dir) + strlen(N) + strlen(H) + strlen(int2char) + 1];
		strcpy(bufferNew, dir);
		strcat(bufferNew, N);
		strcat(bufferNew, int2char);
		strcat(bufferNew, H);

		QFileInfo qfile = QString(bufferNew);
		if (!qfile.exists() || !qfile.isFile())
		{
			cout << "ERROR: Nothing to play." << endl;
			return;
		}


		QDir qdir = dir;		

		// values
		int total_frames;
		total_frames = qdir.count() - 2;

		if (index == 0)
		{
			dirFirstReference.clear();
			dirFirstReference.push_back(qPrintable(dir));

			buttonPlay->setEnabled(true);
			buttonStop->setEnabled(false);
			buttonLoadECG->setEnabled(false);
			sliderBufferedFrame->setEnabled(true);


			labelCurrentFrame->setText(QValue);
			sliderBufferedFrame->setValue(framenumber);
			sliderBufferedFrame->setMaximum(total_frames);
			labelNumberOfFrames->setText(QString("%1").arg(total_frames));

		}
		else if (index == 1)
		{
			dirSecondReference.clear();
			dirSecondReference.push_back(qPrintable(dir));

			buttonPlaySecondStream->setEnabled(true);
			buttonStopSecondStream->setEnabled(false);
			buttonLoadECGSecondStream->setEnabled(false);
			sliderBufferedFrameSecondStream->setEnabled(true);

			labelCurrentFrameSecondStream->setText(QValue);
			sliderBufferedFrameSecondStream->setValue(framenumber);
			sliderBufferedFrameSecondStream->setMaximum(total_frames);
			labelNumberOfFramesSecondStream->setText(QString("%1").arg(total_frames));

		}

	}
	else
	{
		if (index == 0)
		{
			buttonPlay->setEnabled(true);
			sliderBufferedFrame->setEnabled(true);
			buttonLoadECG->setEnabled(true);
			ButtonLoadRunFirst->setEnabled(false);

			labelCurrentFrame->setText(QValue);
			sliderBufferedFrame->setValue(framenumber);
			sliderBufferedFrame->setMaximum(scene->getNumberOfFrames(index));
			labelNumberOfFrames->setText(QString("%1").arg(scene->getNumberOfFrames(index)));


		}
		else if (index == 1)
		{
			buttonPlaySecondStream->setEnabled(true);
			sliderBufferedFrameSecondStream->setEnabled(true);
			buttonLoadECGSecondStream->setEnabled(true);
			ButtonLoadRunSecond->setEnabled(false);

			labelCurrentFrameSecondStream->setText(QValue);
			sliderBufferedFrameSecondStream->setValue(framenumber);
			sliderBufferedFrameSecondStream->setMaximum(scene->getNumberOfFrames(index));
			labelNumberOfFramesSecondStream->setText(QString("%1").arg(scene->getNumberOfFrames(index)));
		}

		else if (index == 2)
		{
			buttonPlayMainStream->setEnabled(true);
			buttonLoadECGMainStream->setEnabled(true);
			sliderBufferedFrameMainStream->setEnabled(true);

			streamPlayerMainStream->stop();

			labelCurrentFrameMainStream->setText(QValue);
			sliderBufferedFrameMainStream->setValue(framenumber);
			sliderBufferedFrameMainStream->setMaximum(scene->getNumberOfFrames(index));
			labelNumberOfFramesMainStream->setText(QString("%1").arg(scene->getNumberOfFrames(index)));
		}

	}

	if (index == 0 || index == 1)
	{
		disableUpdates = false;
	}

	if (index == 2)
	{
		disableUpdatesMainStream = false;
	}


}


void XRayViewer::displayAngleInWindow(int index, double primAngle, double secAngle)
{

	string inputString;
	string angle1, angle2;
	if ((primAngle >= 0) && (primAngle <= 180))
	{
		angle1 = "LAO";
		primAngle = primAngle;
	}
	else if (primAngle < 0)
	{
		angle1 = "RAO";
		primAngle = -primAngle;
	}
	else if (primAngle > 180)
	{
		angle1 = "RAO";
		primAngle = 360-primAngle;
	}
	if ((secAngle >= 0) && (secAngle <= 180))
	{
		angle2 = "Cran";
		secAngle = secAngle;
	}
	else if (secAngle < 0)
	{
		angle2 = "Caud";
		secAngle = -secAngle;
	}

	else if (secAngle > 180)
	{
		angle2 = "Caud";
		secAngle = 360 - secAngle;
	}

	//// Create the widget
	/*vtkSmartPointer<vtkTextActor> textActor =
		vtkSmartPointer<vtkTextActor>::New();*/

	//Create the widget
	vtkTextActor* textActor = vtkTextActor::New();


	inputString.append(angle1);
	inputString.append(" ");
	inputString.append(to_string((int)primAngle));
	inputString.append("\n");
	//inputString += (angle2);
	inputString.append(angle2);
	inputString.append(" ");
	inputString.append(to_string((int)secAngle));

	

	textActor->SetInput(inputString.c_str());
	textActor->GetTextProperty()->SetFontSize(16);
	textActor->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
	textActor->SetDisplayPosition(0, 0);
	txtActors[index] = textActor;

	scene->getXRAYRenderer(index)->AddActor2D(txtActors[index]);
	if(!scene->isFramegrabber) scene->getXRAYRenderWindow(index)->Render();
	//textActor->Delete();
	

	//vtkSmartPointer<vtkTextWidget> textWidget =
	//	vtkSmartPointer<vtkTextWidget>::New();

	//*vtkSmartPointer<vtkTextRepresentation> textRepresentation =
	//vtkSmartPointer<vtkTextRepresentation>::New();
	//textRepresentation->GetPositionCoordinate()->SetValue(.15, .15);
	//textRepresentation->GetPosition2Coordinate()->SetValue(.7, .2);
	//textWidget->SetRepresentation(textRepresentation);*/
	//
	//textWidget->SetInteractor(scene->getXRAYRenderWindow(index)->GetInteractor());
	//textWidget->SetTextActor(textActor);
	//textWidget->SelectableOff();

	////vtkWidgetMainStream->GetInteractor()->Initialize();
	////scene->getXRAYRenderWindow(index)->GetInteractor()->EnableRenderOn();
	////scene->getXRAYRenderWindow(index)->Render();
	//textWidget->On();	
	//scene->getXRAYRenderWindow(index)->Render();
	////vtkWidgetMainStream->GetInteractor()->Start();



}

void XRayViewer::on_actionSelectXRayReferenceFirst_triggered()
{
	if (biplaneSystem) {
		cout << "Please select Monoplane System." << endl;
		return;
	}

	//stopMain();
	if (scene->isFramegrabber) {
		/*-----------------------------TODO------------------------------------------------------------------------------*/
		// stop play aus aktuellem thread
		// value sliderbuffer
		scene->isLive = false;
		buttonLive->setEnabled(true);
		//if (isButtonPlayMainClickedFirst || isPause) buttonPlayMainStream->setEnabled(true);
		buttonLoadECGMainStream->setEnabled(false);
		radioButtonRecord->setChecked(0);
		scene->isRecording = 0;
		sliderBufferedFrameMainStream->setEnabled(true);
		/*---------------------------------------------------------------------------------------------------------------*/
		streamPlayerMainStream->stop();
		scene->alreadyConstructedPipelineDICOM[1] = false;
		scene->loadDefaultMeshPositionBool = true;
	}
	scene->isFramegrabber = false;
	QByteArray file;
	//vector<string> files;

	QString fileFormats("All files (*);; Image files (*.dcm)");

	QString fileName1 = QFileDialog::getOpenFileName(this,
		"Select first XRAY Image file", lastDirectory, fileFormats);
	if (fileName1.isNull()) return;
	file =qPrintable(fileName1);
	int pos = fileName1.lastIndexOf('/');
	lastDirectory = fileName1.left(pos);		
	const char* fileString = file;

	buttonStop->click();
	buttonLoadECG->setEnabled(true);

	scene->setInputToFile(fileString,0);

	buttonPlay->setEnabled(true);
	sliderBufferedFrame->setEnabled(true);
	//scene->setMeshVisibility(scene->getMeshVisibility());

	labelCurrentFrame->setText("1");
	sliderBufferedFrame->setValue(1);
	sliderBufferedFrame->setMaximum(scene->getNumberOfFrames(0));
	labelNumberOfFrames->setText(QString("%1").arg(scene->getNumberOfFrames(0)));

	clearTextActors(0);
	//clearTextActors(2);

	double primAngle, secAngle;
	scene->getDICOMAnglesToWindowRef(0, primAngle, secAngle);
	displayAngleInWindow(0, primAngle, secAngle);

	//________________________
	// | | | | | |    ???
	// v v v v v v   

	undoRedoStack->clear();

	previousMeshWindow = 2;
	scene->getUserMatrix(previousMeshWindow, previousMeshMatrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);

	cmd->setText("default position");
	undoRedoStack->push(cmd);

	disableUpdates = false;
}


void XRayViewer::on_actionSelectXRayReferenceSecond_triggered()
{
	if (biplaneSystem) {
		cout << "Please select Monoplane System." << endl;
		return;
	}

	//stopMain();
	if (scene->isFramegrabber) {
		/*-----------------------------TODO------------------------------------------------------------------------------*/
		// stop play aus aktuellem thread
		// value sliderbuffer
		scene->isLive = false;
		buttonLive->setEnabled(true);
		//if (isButtonPlayMainClickedFirst || isPause) buttonPlayMainStream->setEnabled(true);
		buttonLoadECGMainStream->setEnabled(false);
		radioButtonRecord->setChecked(0);
		scene->isRecording = 0;
		sliderBufferedFrameMainStream->setEnabled(true);
		/*---------------------------------------------------------------------------------------------------------------*/
		streamPlayerMainStream->stop();
		scene->alreadyConstructedPipelineDICOM[0] = false;
		scene->loadDefaultMeshPositionBool = true;
	}
	scene->isFramegrabber = false;
	QByteArray file;

	QString fileFormats("All files (*);; Image files (*.dcm)");

	QString fileName2 = QFileDialog::getOpenFileName(this,
		"Select second XRAY Image file", lastDirectory, fileFormats);
	if (fileName2.isNull()) return;
	file = qPrintable(fileName2);
	int pos = fileName2.lastIndexOf('/');
	lastDirectory = fileName2.left(pos);
	const char* fileString = file;

	buttonStopSecondStream->click();
	buttonLoadECGSecondStream->setEnabled(true);

	scene->setInputToFile(fileString, 1);

	buttonPlaySecondStream->setEnabled(true);
	sliderBufferedFrameSecondStream->setEnabled(true);

	//scene->setMeshVisibility(scene->getMeshVisibility());

	labelCurrentFrameSecondStream->setText("1");
	sliderBufferedFrameSecondStream->setValue(1);
	sliderBufferedFrameSecondStream->setMaximum(scene->getNumberOfFrames(1));
	labelNumberOfFramesSecondStream->setText(QString("%1").arg(scene->getNumberOfFrames(1)));

	clearTextActors(1);
	//clearTextActors(2);

	double primAngle, secAngle;
	scene->getDICOMAnglesToWindowRef(1, primAngle, secAngle);
	displayAngleInWindow(1, primAngle, secAngle);

	//________________________
	// | | | | | |    ???
	// v v v v v v   
	undoRedoStack->clear();

	previousMeshWindow = 2;
	scene->getUserMatrix(previousMeshWindow, previousMeshMatrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);

	cmd->setText("default position");
	undoRedoStack->push(cmd);

	disableUpdates = false;
}





void XRayViewer::clearTextActors(int index)
{
	if (txtActors[index] != NULL)
	{
		scene->getXRAYRenderer(index)->RemoveActor2D(txtActors[index]);
		//scene->getXRAYRenderWindow(index)->Render();
	}
}


void XRayViewer::enableMarkerMenu()
{
	actionShowMarkerPositions->setChecked(true);
	actionShowMarkerLabels->setChecked(true);
}


void XRayViewer::stackClear()
{
	undoRedoStack->clear();

}

void XRayViewer::stackSetClean()
{
	undoRedoStack->setClean();
}

void XRayViewer::on_actionExitXRAY_triggered()
{
	close();

}

void XRayViewer::generateUndoRedoCommandStack(MoveMeshInteractionCommand* cmd, int prevWindow, double prevMatrix[4][4])
{
	undoRedoStack->push(cmd);
	previousMeshWindow = prevWindow;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			previousMeshMatrix[i][j] = prevMatrix[i][j];
		}
	}

}
//
//void XRayViewer::getPreviousCommand(OverlayScene* scene, int prevWindow, double prevMatrix[4][4])
//{
//	this->scene = scene;
//	prevWindow = previousMeshWindow;
//	for (int i = 0; i < 4; i++) {
//		for (int j = 0; j < 4; j++) {
//			prevMatrix[i][j] = previousMeshMatrix[i][j];
//		}
//	}
//
//}

void XRayViewer::on_actionLoadSavedMeshPosition_triggered()
{
	QString path = QFileDialog::getOpenFileName(this, "Open saved mesh position", ".", "Mesh position file (*.txt)");
	if (path.isNull()) return;

	undoRedoStack->clear();

	unsigned int streamNumber = 2;
	double matrix[4][4];
	scene->loadUserMatrix(streamNumber, qPrintable(path));
	scene->getUserMatrix(streamNumber, matrix);

	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, streamNumber, matrix, streamNumber, matrix);
	cmd->setText("saved position");
	undoRedoStack->push(cmd);

	previousMeshWindow = streamNumber;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			previousMeshMatrix[i][j] = matrix[i][j];
		}
	}


	if (disableUpdatesMainStream)
	{
		clearTextActors(2);
	}
}

void XRayViewer::on_actionSaveMeshPosition_triggered()
{
	QString path = QFileDialog::getSaveFileName(this, "Save mesh position", ".", "Mesh position file (*.txt)");
	if (path.isNull()) return;

	unsigned int streamNumber = 2;
	scene->saveUserMatrix(streamNumber, qPrintable(path));
}

void XRayViewer::on_actionLoadDefaultMeshPosition_triggered()
{
	QStringList MESH;
	//MESH << "PHILIPS MRI" << "ITK-SNAP for MRI" << "PHILIPS CT" << "ITK-SNAP for CT"; // not supported: << "saggital";
	MESH << "PHILIPS MRI" << "PHILIPS CT";
	QString orientation = QInputDialog::getItem(this, "Load default mesh position", "For which volume mesh was exported", MESH, 0, false);
	if (orientation == "PHILIPS MRI") scene->loadDefaultMeshPosition(OverlayScene::MR_PHILIPS);
	if (orientation == "ITK-SNAP for MRI") scene->loadDefaultMeshPosition(OverlayScene::MR_ITK);
	if (orientation == "PHILIPS CT") scene->loadDefaultMeshPosition(OverlayScene::CT_PHILIPS);
	if (orientation == "ITK-SNAP for CT") scene->loadDefaultMeshPosition(OverlayScene::CT_ITK);
	
	//// generate new undo/redo command
	undoRedoStack->clear();
	previousMeshWindow = 2;
	scene->getUserMatrix(previousMeshWindow, previousMeshMatrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);

	cmd->setText("default position");
	undoRedoStack->push(cmd);

	if (disableUpdatesMainStream)
	{
		clearTextActors(2);
	}
	/*if (disableUpdates)
	{
		for (int i = 0; i < 2; i++)
		{
			clearTextActors(i);

		}
	}*/
	

}

void XRayViewer::on_buttonStop_clicked()
{
	if (disableUpdates) return;

	if (scene->isFramegrabber)
	{

		buttonStop->setEnabled(false);
		buttonPlay->setEnabled(true);
		buttonLoadECG->setEnabled(false);
		//buttonSaveImage->setEnabled(true);
		sliderBufferedFrame->setEnabled(true);

		streamPlayer->stop();

	}

	else
	{
		buttonStop->setEnabled(false);
		buttonPlay->setEnabled(true);
		buttonLoadECG->setEnabled(true);
		//buttonSaveImage->setEnabled(true);
		sliderBufferedFrame->setEnabled(true);

		streamPlayer->stop();
		sliderBufferedFrame->setMaximum(scene->getNumberOfFrames(0));
		labelNumberOfFrames->setText(QString("%1").arg(scene->getNumberOfFrames(0)));
	}

	
}

void XRayViewer::on_buttonStopSecondStream_clicked()
{
	if (disableUpdates) return;

	if (scene->isFramegrabber)
	{
			
		buttonStopSecondStream->setEnabled(false);
		buttonPlaySecondStream->setEnabled(true);
		buttonLoadECGSecondStream->setEnabled(false);
		//buttonSaveImageSecondStream->setEnabled(true);
		sliderBufferedFrameSecondStream->setEnabled(true);
			
		streamPlayerSecondStream->stop();
	
	}
	else
	{ 
		buttonStopSecondStream->setEnabled(false);
		buttonPlaySecondStream->setEnabled(true);
		buttonLoadECGSecondStream->setEnabled(true);
		//buttonSaveImageSecondStream->setEnabled(true);
		sliderBufferedFrameSecondStream->setEnabled(true);

		streamPlayerSecondStream->stop();
		sliderBufferedFrameSecondStream->setMaximum(scene->getNumberOfFrames(1));
		labelNumberOfFramesSecondStream->setText(QString("%1").arg(scene->getNumberOfFrames(1)));
	}
}

void XRayViewer::stopMain()
{
	//vtkWidgetMainStream->GetInteractor()->SetInteractorStyle(templateSelectorMainStream);

	if (disableUpdatesMainStream) return;
	isStop = true;
	//buttonStopMainStream->setEnabled(false);	
	
	if (scene->isFramegrabber) {
		/*-----------------------------TODO------------------------------------------------------------------------------*/
		// stop play aus aktuellem thread
		// value sliderbuffer
		scene->isLive = false;
		buttonLive->setEnabled(true);
		//if (isButtonPlayMainClickedFirst|| isPause) buttonPlayMainStream->setEnabled(true);
		buttonLoadECGMainStream->setEnabled(false);
		radioButtonRecord->setChecked(0);
		scene->isRecording = 0;
		sliderBufferedFrameMainStream->setEnabled(true);
		/*---------------------------------------------------------------------------------------------------------------*/
		streamPlayerMainStream->stop();
	}
	else
	{
		buttonPlayMainStream->setEnabled(true);
		buttonLoadECGMainStream->setEnabled(true);
		//buttonSaveImageSecondStream->setEnabled(true);
		sliderBufferedFrameMainStream->setEnabled(true);

		streamPlayerMainStream->stop();

		sliderBufferedFrameMainStream->setMaximum(scene->getNumberOfFrames(2));
		labelNumberOfFramesMainStream->setText(QString("%1").arg(scene->getNumberOfFrames(2)));
	}



}

void XRayViewer::on_buttonSaveImageMainStream_clicked()
{
		// save as image
		QString Qpath = QFileDialog::getSaveFileName(this, "Save current X-ray images", ".");
		if (Qpath.isNull()) return;


		//scene->saveCurrentXRayImages(qPrintable(path.append("_channel%d.tif")));

		vtkRenderWindow* renWin;
		for (int i = 0; i < 3; i++)
		{
			string path = qPrintable(Qpath);

			renWin = scene->getXRAYRenderWindow(i);
			path.append("_reference");
			path.append(to_string(i));
			path.append(".png");

			scene->saveImage(path.c_str(), renWin);
		}

		//statusbar->showMessage("Images saved.", 2000);

}


void XRayViewer::on_actionNextFrame_triggered()
{
	int frame1, frame2, mainFrame;

	frame1 = sliderBufferedFrame->value();
	sliderBufferedFrame->setValue(++frame1);

	frame2 = sliderBufferedFrameSecondStream->value();
	sliderBufferedFrameSecondStream->setValue(++frame2);


}

void XRayViewer::on_actionPreviousFrame_triggered()
{
	int frame1, frame2, mainFrame;

	frame1 = sliderBufferedFrame->value();
	sliderBufferedFrame->setValue(--frame1);

	frame2 = sliderBufferedFrameSecondStream->value();
	sliderBufferedFrameSecondStream->setValue(--frame2);

}

void XRayViewer::on_actionRemoveECG_triggered()
{
	for (int i = 0; i < 3; i++)
	{
		scene->disableECG(i);
	}
	
}

void XRayViewer::on_sliderBufferedFrame_valueChanged(int value)
{
	if (disableUpdates) return;

	QString lbl = QString("%1").arg(value);
	labelCurrentFrame->setText(lbl);

	double primAngle, secAngle;
	if (scene->isFramegrabber)
	{
		//if (biplaneSystem && !buttonLive->isEnabled()) {

		//	buttonPlay->setDown(false);
		//	scene->liveBiplane(0);

		//	clearTextActors(0);
		//	double primAngle, secAngle;
		//	scene->getDICOMAnglesToWindow(primAngle, secAngle);
		//	displayAngleInWindow(0, primAngle, secAngle);
		//}
		//else {
		//	// play each frame
		//	const char* fileString;
		//	fileString = dirFirstReference; // implicit conversion
		//	//scene->setGeometryFromFramegrabber(0, SID, sourcePatDist, primAngle, secAngle);
		//	clearTextActors(0);
		//	scene->playReferenceStream(value, (char*)fileString, 0);
		//	scene->getDICOMAnglesToWindowRef(0, primAngle, secAngle);
		//	displayAngleInWindow(0, primAngle, secAngle);
		//}

		// play each frame
		const char* fileString;
		fileString = dirFirstReference; // implicit conversion
										//scene->setGeometryFromFramegrabber(0, SID, sourcePatDist, primAngle, secAngle);
		clearTextActors(0);
		scene->playReferenceStream(value, (char*)fileString, 0);
		scene->getDICOMAnglesToWindowRef(0, primAngle, secAngle);
		displayAngleInWindow(0, primAngle, secAngle);
	}
	else
	{
		scene->showFrame(0, (value - 1));
		if (ECGLoad)
		{
			scene->setECGFrame(0, (value));
		}
	}

	scene->renderRef(0);
}

void XRayViewer::on_sliderBufferedFrameSecondStream_valueChanged(int value)
{
	if (disableUpdates) return;	

	QString lbl1 = QString("%1").arg(value);
	labelCurrentFrameSecondStream->setText(lbl1);

	double primAngle, secAngle;
	if (scene->isFramegrabber)
	{
		// play each frame
		const char* fileString;
		fileString = dirSecondReference; // implicit conversion
		clearTextActors(1);
		scene->playReferenceStream(value, (char*)fileString, 1);	
		scene->getDICOMAnglesToWindowRef(1, primAngle, secAngle);
		displayAngleInWindow(1, primAngle, secAngle);
	}
	else
	{

		scene->showFrame(1, (value - 1));
		if (ECGLoadSecondStream)
		{
			scene->setECGFrame(1, (value));
		}

		int frameSecond, frame;
		if (ECGLoad & ECGLoadSecondStream)
		{
			scene->getFrameForPeak(1, frameSecond);

			if (frameSecond == 1)
			{

				scene->getFrameForPeak(0, frame);
				value = value + frame - 1;
			}
			else
			{
				value = value - frameSecond + 1;
				if (value < 1)
					value = 1;
				if (value > sliderBufferedFrame->maximum())
					value = sliderBufferedFrame->maximum();

			}

			QString lbl = QString("%1").arg(value);
			labelCurrentFrame->setText(lbl);
			scene->showFrame(0, (value - 1));
			scene->setECGFrame(0, (value));

			sliderBufferedFrame->setValue(value);
		}
	}
	scene->renderRef(1);
}

void XRayViewer::on_sliderBufferedFrameMainStream_valueChanged(int value)
{
	if (disableUpdatesMainStream) return;

	QString lbl = QString("%1").arg(value);
	labelCurrentFrameMainStream->setText(lbl);

	if (scene->isFramegrabber && !biplaneSystem)
	{		
	// Monoplane, Framegrabber
		bool liveEnabled = buttonLive->isEnabled();
		bool startplay = buttonPlayMainStream->isDown();
	
		// play each frame
		//if ((!buttonPlayMainStream->isEnabled()) && liveEnabled)
		//if ((!buttonPlayMainStream->isEnabled() || !buttonStopMainStream->isEnabled()) && liveEnabled)
		if ((!buttonPlayMainStream->isEnabled() || isStop) && liveEnabled)
		{
			const char* fileString;
			fileString = dir; // implicit conversion

			scene->play(value, (char*)fileString);

			clearTextActors(2);
			double primAngle, secAngle;
			scene->getDICOMAnglesToWindow(primAngle, secAngle);
			displayAngleInWindow(2, primAngle, secAngle);
		}

		// grab each frame
		if (!buttonLive->isEnabled())
		{
			buttonPlayMainStream->setDown(false);

			//int start1 = startClock("live.txt");
				scene->live();
			//stopClock(start1);

			clearTextActors(2);
			double primAngle, secAngle;
			scene->getDICOMAnglesToWindow(primAngle, secAngle);
			displayAngleInWindow(2, primAngle, secAngle);
		}

		// record each frame
		if (radioButtonRecord->isChecked())
		{
			scene->record(value);
		
			/*clock_t time5, time6;
			double duration3;
			f.open("recordTotal.txt", ios::out | ios::app);			
			time5 = clock();

			time6 = clock();
			duration3 = (double)(time6 - time5);
			f << duration3 << endl;
			f.close();*/

		}
		//int start2 = startClock("renderMain.txt");
			scene->renderMain();
		//stopClock(start2);
	}
	else if (scene->isFramegrabber && biplaneSystem) {
	// Biplane, Framegrabber
		bool liveEnabled = buttonLive->isEnabled();
		bool startplay = buttonPlayMainStream->isDown();

		// play each frame
		//if ((!buttonPlayMainStream->isEnabled()) && liveEnabled)
		//if ((!buttonPlayMainStream->isEnabled() || !buttonStopMainStream->isEnabled()) && liveEnabled)
		if ((!buttonPlayMainStream->isEnabled() || isStop) && liveEnabled)
		{
			// play each frame using load run
			const char* fileString;
			fileString = dirReferenceBiplane; // implicit conversion
			double primAngle0, secAngle0, primAngle1, secAngle1;
			
			if (recordMode == 1) { // biplane
				char* ref0 = "\\frontal";
				char* pathRef0 = new char[strlen(fileString) + strlen(ref0) + 1];
				strcpy(pathRef0, fileString);
				strcat(pathRef0, ref0);
				clearTextActors(0);
				scene->playReferenceStream(value, (char*)pathRef0, 0);
				scene->getDICOMAnglesToWindowRef(0, primAngle0, secAngle0);

				char* ref1 = "\\lateral";
				char* pathRef1 = new char[strlen(fileString) + strlen(ref1) + 1];
				strcpy(pathRef1, fileString);
				strcat(pathRef1, ref1);
				clearTextActors(1);
				scene->playReferenceStream(value, (char*)pathRef1, 1);
				scene->getDICOMAnglesToWindowRef(1, primAngle1, secAngle1);
			}
			else { // recorded in monoplaneMode
				clearTextActors(0);
				scene->playReferenceStream(value, (char*)fileString, 0);
				scene->getDICOMAnglesToWindowRef(0, primAngle0, secAngle0);

				clearTextActors(1);
				scene->playReferenceStream(value, (char*)fileString, 1);
				scene->getDICOMAnglesToWindowRef(1, primAngle1, secAngle1);
			
			}

			displayAngleInWindow(0, primAngle0, secAngle0);
			displayAngleInWindow(1, primAngle1, secAngle1);			
		}

		// grab each frame
		if (!buttonLive->isEnabled())
		{
			buttonPlayMainStream->setDown(false);

			double primAngle[2], secAngle[2];
			//double primAngle, secAngle;

			scene->liveBiplane(0); // liveBiplane(0) muss vor liveBiplane(1) aufgerufen werden, wegen der Tischposition!!!
			clearTextActors(0);
			scene->getDICOMAnglesToWindowRef(0, primAngle[0], secAngle[0]);
			displayAngleInWindow(0, primAngle[0], secAngle[0]);

			scene->liveBiplane(1);
			clearTextActors(1);
			scene->getDICOMAnglesToWindowRef(1, primAngle[1], secAngle[1]);
			displayAngleInWindow(1, primAngle[1], secAngle[1]);
			//displayAngleInWindow(0, primAngle[0], secAngle[0]);
			//displayAngleInWindow(1, primAngle[1], secAngle[1]);
		}

		// record each frame
		if (radioButtonRecord->isChecked())
		{
			scene->recordBiplane(value, 0, run);
//			scene->recordBiplane(value, 1, run);
		}
		scene->renderRef(0);
		scene->renderRef(1);
	}
	else {
	// kein Framegrabber	
		scene->showFrame(2, (value - 1));
		if (ECGLoadMainStream)
		{
			scene->setECGFrame(2, (value));
		}
		scene->renderMain();
	}
	
}


void XRayViewer::on_actionLoad_Run_Biplane_triggered()
{
	scene->isFramegrabber = 1;
	isPause = true;
	// filter pipeline
	scene->isLive = false;
	//scene->setFilterToType(qPrintable(filterType));

	streamPlayer->stop();
	streamPlayerSecondStream->stop();
	streamPlayerMainStream->stop();

	// Variante: waehle einen run aus und rendere beide referenzen
	// select directory
	// the directory of the run or of the reference can be selected
	QString path;
	if (startPlay) {
		// Pfad zu zuletzt aufgenommenem Run erstellen
		char int2char[10];
		sprintf(int2char, "%d", run);
		char* currentRunDirectory = new char[strlen(patientDirectoryChar) + strlen(int2char) + 1];
		strcpy(currentRunDirectory, patientDirectoryChar);
		strcat(currentRunDirectory, int2char);

		path = currentRunDirectory;
		startPlay = false;
		//isButtonPlayMainClickedFirst = false;
	}
	else {
		// Pfad ueber Dialogfeld waehlen
		path = QFileDialog::getExistingDirectory(this, "Select run directory", patientDirectory);
	}

	QDir qdir; // path to .dicom files
	
	if (path.isNull()) return;
	if (path.endsWith("frontal") || path.endsWith("lateral")) {
		// to count the frames of the selected run
		qdir = path;
		
		// to get the path to the run (not to the reference folder)
		path.remove("/frontal");
		path.remove("/lateral");
		path.remove("\\frontal");
		path.remove("\\lateral");
		dirReferenceBiplane.clear();
		dirReferenceBiplane.push_back(qPrintable(path));

		recordMode = 1;
	}
	else {
		////hier wird davvon ausgegangen, dass biplanar recorded wurde
		//// to get the path to the run (not to the reference folder)
		//dirReferenceBiplane.clear();
		//dirReferenceBiplane.push_back(qPrintable(path));
		//
		//// to count the frames of the selected run
		//QString x = "\\Reference0";
		//path.append(x);
		//qdir = path;

		// to get the path to the run (not to the reference folder)
		dirReferenceBiplane.clear();
		dirReferenceBiplane.push_back(qPrintable(path));

		// to count the frames of the selected run -> get path to .dicom files
		QString x = "\\frontal";
		path.append(x);
		QDir test;
		test = path;
		if (!test.exists()) { // no reference folder exits -> monoplane recorded
			path.remove("\\frontal");
			qdir = path; // run folder contains .dicom files
			recordMode = 0; // monoplane (0 = monoplane, 1 = biplane);
		}
		else {
			qdir = path;
			recordMode = 1; // biplane;
		}
	}

	// exception directory is empty
	if (qdir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries).count() == 0)
	{
		cout << "ERROR: Directory is empty." << endl;
		return;
	}

	// GUI
	buttonPlayMainStream->setEnabled(false);
	buttonLive->setEnabled(true);
	radioButtonRecord->setChecked(false);
	isStop = false;
	//buttonStopMainStream->setEnabled(true);
	buttonLoadECGMainStream->setEnabled(false);
	//buttonSaveImageSecondStream->setEnabled(false);
	sliderBufferedFrameMainStream->setEnabled(false);

	// values
	int total_frames;
	total_frames = qdir.count() - 2;
	labelCurrentFrameMainStream->setText("1");
	sliderBufferedFrameMainStream->setValue(1);
	sliderBufferedFrameMainStream->setMaximum(total_frames);
	labelNumberOfFramesMainStream->setText(QString("%1").arg(total_frames));

	scene->alreadyConstructedPipeline[0] = scene->alreadyConstructedPipeline[1] = false;

	streamPlayerMainStream->start(1000 / 15);

	disableUpdatesMainStream = false;
}


void XRayViewer::on_ButtonLoadRunFirst_clicked()
{
	//=============================================================================================
	scene->isFramegrabber = 1; // zum Testen ohne Geraet
	//=============================================================================================

	// select directory
	QString path = QFileDialog::getExistingDirectory(this, "Select run directory", patientDirectory);
	if (path.isNull()) return;
	QDir qdir = path;
	dirFirstReference.clear();
	dirFirstReference.push_back(qPrintable(path));

	// exception directory is empty
	if (qdir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries).count() == 0)
	{
		cout << "ERROR: Directory is empty." << endl;
		return;
	}

	// GUI
	//buttonStop->click();	
	buttonPlay->setEnabled(false);
	buttonStop->setEnabled(true);
	buttonLoadECG->setEnabled(false);
	//buttonSaveImageSecondStream->setEnabled(false);
	sliderBufferedFrame->setEnabled(false);

	// values
	int total_frames;
	total_frames = qdir.count() - 2;
	labelCurrentFrame->setText("1");
	sliderBufferedFrame->setValue(1);
	sliderBufferedFrame->setMaximum(total_frames);
	labelNumberOfFrames->setText(QString("%1").arg(total_frames));

	//scene->loadDefaultMeshPosition(OverlayScene::CT_PHILIPS);
	streamPlayer->start(1000 / 15);	
	//scene->update2DXRAYWindows(0);
	/*const char* fileString;
	fileString = dirFirstReference;
	scene->setInputFromFramegrabber(0, (char*)fileString);
*/
	disableUpdates = false;

	undoRedoStack->clear();
	previousMeshWindow = 2;
	scene->getUserMatrix(previousMeshWindow, previousMeshMatrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);

	cmd->setText("default position");
	undoRedoStack->push(cmd);
}

void XRayViewer::on_ButtonLoadRunSecond_clicked()
{
	//=============================================================================================
	scene->isFramegrabber = 1; // zum Testen ohne Geraet
	//=============================================================================================

	// select directory
	QString path = QFileDialog::getExistingDirectory(this, "Select run directory", patientDirectory);
	if (path.isNull()) return;
	QDir qdir = path;
	dirSecondReference.clear();
	dirSecondReference.push_back(qPrintable(path));

	// exception directory is empty
	if (qdir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries).count() == 0)
	{
		cout << "ERROR: Directory is empty." << endl;
		return;
	}

	// GUI
	buttonPlaySecondStream->setEnabled(false);
	buttonStopSecondStream->setEnabled(true);
	buttonLoadECGSecondStream->setEnabled(false);
	//buttonSaveImageSecondStream->setEnabled(false);
	sliderBufferedFrameSecondStream->setEnabled(false);

	// values
	int total_frames;
	total_frames = qdir.count() - 2;
	labelCurrentFrameSecondStream->setText("1");
	sliderBufferedFrameSecondStream->setValue(1);
	sliderBufferedFrameSecondStream->setMaximum(total_frames);
	labelNumberOfFramesSecondStream->setText(QString("%1").arg(total_frames));


	streamPlayerSecondStream->start(1000 / 15);
	//scene->update2DXRAYWindows(1);
	disableUpdates = false;

	undoRedoStack->clear();
	previousMeshWindow = 2;
	scene->getUserMatrix(previousMeshWindow, previousMeshMatrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(scene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);

	cmd->setText("default position");
	undoRedoStack->push(cmd);
}

void XRayViewer::on_buttonPlay_clicked()
{
	if (disableUpdates) return;
	//=============================================================================================
	//scene->isFramegrabber = 1; // zum Testen ohne Geraet
	//=============================================================================================
	if (scene->isFramegrabber)
	{
		// GUI
		buttonPlay->setEnabled(false);
		buttonStop->setEnabled(true);
		buttonLoadECG->setEnabled(false);
		sliderBufferedFrame->setEnabled(false);

		//// renew Geometry
		//double defaultNullDouble = 0.0;
		//int defaultNullInt = 0;
		//scene->setNewGeometryPlay(defaultNullDouble, defaultNullDouble, defaultNullInt, defaultNullInt, defaultNullInt, defaultNullInt, defaultNullDouble, defaultNullInt);

		streamPlayer->start(1000 / 5);
	}
	else
	{

		buttonPlay->setEnabled(false);
		buttonStop->setEnabled(true);
		buttonLoadECG->setEnabled(false);
		//buttonSaveImage->setEnabled(true);
		sliderBufferedFrame->setEnabled(false);

		streamPlayer->start(1000 / 13);
	}
}


void XRayViewer::on_buttonPlaySecondStream_clicked()
{
	if (disableUpdates) return;

	if (scene->isFramegrabber)
	{
		// GUI
		buttonPlaySecondStream->setEnabled(false);
		buttonStopSecondStream->setEnabled(true);
		buttonLoadECGSecondStream->setEnabled(false);
		//buttonSaveImageSecondStream->setEnabled(false);
		sliderBufferedFrameSecondStream->setEnabled(false);

		streamPlayerSecondStream->start(1000 / 5);
	}
	else
	{ 

		buttonPlaySecondStream->setEnabled(false);
		buttonStopSecondStream->setEnabled(true);
		buttonLoadECGSecondStream->setEnabled(false);
		//buttonSaveImageSecondStream->setEnabled(false);
		sliderBufferedFrameSecondStream->setEnabled(false);

		streamPlayerSecondStream->start(1000 / 13);
	}
}

void XRayViewer::on_buttonPlayMainStream_clicked()
{
	// first init
	if (disableUpdatesMainStream) return;

	
	if (scene->isFramegrabber)
	{	// spiele zuletzt gespeicherten run ab

		// automatisches beenden von record, da sonst ueberschneidungen bei slider value bzw. imagenamen
		radioButtonRecord->setChecked(0); 
		scene->isRecording = 0;
		streamPlayerMainStream->stop();
		
		if (!biplaneSystem) {
			// Pfad zu zuletzt aufgenommenem Run erstellen
			char int2char[10];
			sprintf(int2char, "%d", run);
			char* currentRunDirectory = new char[strlen(patientDirectoryChar) + strlen(int2char) + 1];
			strcpy(currentRunDirectory, patientDirectoryChar);
			strcat(currentRunDirectory, int2char);

			// globale Variable fuer abzuspielenden Run uebergeben
			dir.clear();
			dir.push_back(currentRunDirectory);
			if (dir.isNull()) return;

			// frames des Runs zaehlen
			QDir qdir = QDir(QString(currentRunDirectory));
			int total_frames;
			total_frames = qdir.count() - 2;

			sliderBufferedFrameMainStream->setMaximum(total_frames);
			labelNumberOfFramesMainStream->setText(QString("%1").arg(total_frames));
		}

		// GUI
		buttonPlayMainStream->setEnabled(false);
		scene->isLive = false;
		buttonLive->setEnabled(true);
		isStop = false;
		isPause = true;
		//buttonStopMainStream->setEnabled(true);
		buttonLoadECGMainStream->setEnabled(false);
		//buttonSaveImageSecondStream->setEnabled(false);
		sliderBufferedFrameMainStream->setEnabled(false);
	
		if (isButtonPlayMainClickedFirst) // without load run
		{
			if (biplaneSystem) 
			{ 
				startPlay = true;
				on_actionLoad_Run_Biplane_triggered(); 
			}
			
			labelCurrentFrameMainStream->setText("1");
			sliderBufferedFrameMainStream->setValue(1);
			isButtonPlayMainClickedFirst = 0;		

			streamPlayerMainStream->stop(); // da sonst doppelt gestartet in on_actionLoad_Run_Biplane_triggered()
		}

		streamPlayerMainStream->start(1000 / 15);
	}
	else 
	{
		buttonPlayMainStream->setEnabled(false);
		isStop = false;
		//buttonStopMainStream->setEnabled(true);
		buttonLoadECGMainStream->setEnabled(false);
		sliderBufferedFrameMainStream->setEnabled(false);

		streamPlayerMainStream->start(1000/13);
	}

}

void XRayViewer::streamPlayerSecondStream_update()
{
	int value = sliderBufferedFrameSecondStream->value();
	++value;

	//if (value == sliderBufferedFrameSecondStream->maximum() && !doLoopButtonSecondStream->isChecked()) {
	//	buttonStopSecondStream->click();
	//}

	value = value % sliderBufferedFrameSecondStream->maximum();
	sliderBufferedFrameSecondStream->setValue(value);
}

void XRayViewer::streamPlayerMainStream_update()
{
	int value = sliderBufferedFrameMainStream->value();
	++value;

	if (value == sliderBufferedFrameMainStream->maximum() && !doLoopButtonMainStream->isChecked()) {
		//buttonStopMainStream->click();
	}

	value = value % sliderBufferedFrameMainStream->maximum();
	sliderBufferedFrameMainStream->setValue(value);

	//scene->live();
	
}

void XRayViewer::streamPlayer_update()
{
	int value = sliderBufferedFrame->value();
	++value;

	//if (value == sliderBufferedFrame->maximum() && !doLoopButton->isChecked()) {
	//	buttonStop->click();
	//}

	value = value % sliderBufferedFrame->maximum();
	sliderBufferedFrame->setValue(value);
}

void XRayViewer::on_buttonLive_clicked()
{	
	if (!scene->canReadLayout[0]) {// can not read layout
		QMessageBox::warning(this, "Live",
			"Problem with the input.\nCheck the command window output for further information.");
		return;
	}

	if (!scene->frontalInput) { // no frontal input-> no tableposition and no useful imagefusion
		cout << "ERROR: no frontal input" << endl;
		QMessageBox::warning(this, "Live",
		"No frontal input.\nFrontal input is needed for the recognition of the table position.");
		return;
	}
	scene->isFramegrabber = 1;
	
	if (biplaneSystem && sliderBufferedFrameMainStream->sliderPosition() ==1)
	{
		scene->alreadyConstructedPipeline[0] = scene->alreadyConstructedPipeline[1] = false;
		//scene->setFilterToType(qPrintable(filterType));
	}


	disableUpdatesMainStream = false;
	//on_actionMoveScene_triggered();
	if (actionMoveObjects->isChecked()) {
		vtkWidgetMainStream->GetInteractor()->SetInteractorStyle(NULL);
		vtkWidgetMainStream->setCursor(Qt::ArrowCursor);
	}
		//actionMoveScene->changed();
	//actionMoveObjects->setChecked(false);
	//actionMoveScene->setChecked(true);


	sliderBufferedFrameMainStream->setEnabled(false);
	scene->isLive = 1;
	buttonLive->setEnabled(false);
	buttonLoadECGMainStream->setEnabled(false);
	isPause = false;
	isStop = false;
	//buttonStopMainStream->setEnabled(true);
	buttonPlayMainStream->setEnabled(false);
	//buttonSaveImageMainStream->setEnabled(false);

	labelCurrentFrameMainStream->setText("1");
	sliderBufferedFrameMainStream->setValue(1);
	sliderBufferedFrameMainStream->setMaximum(1000);
	labelNumberOfFramesMainStream->setText("1000");

	
	streamPlayerMainStream->start(1000 / 10);
}

void XRayViewer::on_actionLive_triggered()
{
	on_buttonLive_clicked();
}

void XRayViewer::on_radioButtonRecord_clicked()
{
	if (!scene) return;
	if (!scene->isFramegrabber) return;

	if (!scene->canReadLayout[0]) {// can not read layout
		QMessageBox::warning(this, "Record",
			"Problem with the input.\nCheck the command window output for further information.");
		return;
	}

	if (!scene->frontalInput) { // no frontal input-> no tableposition and no useful imagefusion
		cout << "ERROR: no frontal input" << endl;
		QMessageBox::warning(this, "Record",
			"No frontal input.\nFrontal input is needed for the recognition of the table position.");
		return;
	}

	if (radioButtonRecord->isChecked())
	{	
		// start record
		buttonLive->setEnabled(false);
		//buttonPlayMainStream->setEnabled(true);
		isStop = false;
		//buttonStopMainStream->setEnabled(true);
		//buttonSaveImageMainStream->setEnabled(false);
		sliderBufferedFrameMainStream->setEnabled(false);

		// new folder
		run += 1;
		if (biplaneSystem) {
			if (sliderBufferedFrameMainStream->sliderPosition() == 1)
			{
				scene->alreadyConstructedPipeline[0] = scene->alreadyConstructedPipeline[1] = false;
				//if(!scene->isLive) scene->setFilterToType(qPrintable(filterType));
			}
			scene->startRecordBiplane(run, 0);
			scene->startRecordBiplane(run, 1);
		}
		else {
			scene->startRecord(run);
		}
		scene->isRecording = 1;
		scene->isLive = 1;
		isButtonPlayMainClickedFirst = 1;
		//isPause = true;

		// slider
		labelCurrentFrameMainStream->setText("1");
		sliderBufferedFrameMainStream->setValue(1);
		sliderBufferedFrameMainStream->setMaximum(1000);
		labelNumberOfFramesMainStream->setText("1000");

		streamPlayerMainStream->start(1000 / 10);
		
	}
	else 
	{	// stop record
		scene->isRecording = 0;
		scene->stopRecord();

		//buttonPlayMainStream->setEnabled(true);
	}

}


void XRayViewer::on_buttonLoadECG_clicked()
{
	if (disableUpdates) return;

	QString path = QFileDialog::getOpenFileName(this, "Select ECG file for the first XRAY reference", lastDirectory, "ECG Text file (*.txt)");
	if (path.isNull()) return;

	QByteArray file;
	file.push_back(qPrintable(path));

	int pos = file.lastIndexOf('/');
	lastDirectory = file.left(pos);

	const char* fileString;
	fileString = file; // implicit conversion
	
	scene->loadECGFile(0, fileString, sliderBufferedFrame->value());
	if (ECGLoadSecondStream)
	{
		scene->setFrameForPeak(0);
		int frame, frameSecond;
		scene->getFrameForPeak(0, frame);
		sliderBufferedFrame->setValue(frame);	
		scene->getFrameForPeak(1, frameSecond);
		sliderBufferedFrameSecondStream->setValue(frameSecond);

	}

	ECGLoad = true;
	
}

void XRayViewer::on_buttonLoadECGSecondStream_clicked()
{
	if (disableUpdates) return;

	QString path = QFileDialog::getOpenFileName(this, "Select ECG file for the second XRAY reference", lastDirectory, "ECG Text file (*.txt)");
	if (path.isNull()) return;

	QByteArray file;
	file.push_back(qPrintable(path));

	int pos = file.lastIndexOf('/');
	lastDirectory = file.left(pos);

	const char* fileString;
	fileString = file; // implicit conversion

	scene->loadECGFile(1, fileString, sliderBufferedFrameSecondStream->value());
	if (ECGLoad)
	{
		
		scene->setFrameForPeak(1);
		int frameSecond, frame;
		scene->getFrameForPeak(1, frameSecond);
		sliderBufferedFrameSecondStream->setValue(frameSecond);
		scene->getFrameForPeak(0, frame);
		sliderBufferedFrame->setValue(frame);

	
	}
	
	ECGLoadSecondStream = true;

}


void XRayViewer::on_buttonLoadECGMainStream_clicked()
{
	if (disableUpdatesMainStream) return;

	QString path = QFileDialog::getOpenFileName(this, "Select ECG file for the actual XRAY run", lastDirectory, "ECG Text file (*.txt)");
	if (path.isNull()) return;

	QByteArray file;
	file.push_back(qPrintable(path));

	int pos = file.lastIndexOf('/');
	lastDirectory = file.left(pos);

	const char* fileString;
	fileString = file; // implicit conversion

	scene->loadECGFile(2, fileString, sliderBufferedFrameMainStream->value());

	ECGLoadMainStream = true;

}


void XRayViewer::on_actionMeasureDistance_toggled(bool checked)
{
	double pos1[3], pos2[3];
	this->AddDistanceMeasurementToView(checked);
}



void XRayViewer::AddDistanceMeasurementToView(bool checked)
{
	if (lineWidget)
		   {
			   lineWidget->SetEnabled(0);
			   lineWidget = NULL;
		   }	
	
	/* Line widget*/
	lineWidget = vtkLineWidget2::New();
	lineWidget->SetInteractor(vtkWidgetMainStream->GetInteractor());

	/* Create the representation*/
	vtkSmartPointer<vtkLineRepresentation> rep =
		vtkSmartPointer<vtkLineRepresentation>::New();
	lineWidget->SetRepresentation(rep);

	static_cast<vtkLineRepresentation*>(lineWidget->GetRepresentation())->DistanceAnnotationVisibilityOn();
	static_cast<vtkLineRepresentation*>(lineWidget->GetRepresentation())->SetDistanceAnnotationFormat("%-#6.3g mm");
	static_cast<vtkLineRepresentation*>(lineWidget->GetRepresentation())->SetDistanceAnnotationScale(10, 10, 10);
	
	vtkWidgetMainStream->GetInteractor()->Start();
	lineWidget->SetEnabled(checked);
	vtkWidgetMainStream->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera()->SetParallelProjection(checked);
	vtkWidgetMainStream->GetRenderWindow()->Render();

}


XRayViewer::~XRayViewer()
{
 
}



int XRayViewer::startClock(char* txt) {
	clock_t time1, time2;
	//double duration1;
	char* PD = scene->patientDir;
	//char* txt = "grab.txt";
	char* file = new char[strlen(PD) + strlen(txt) + 1];
	strcpy(file, PD);
	strcat(file, txt);
	f.open(file, ios::out | ios::app);
	time1 = clock();

	return time1;
}

void XRayViewer::stopClock(int startTime) {
	clock_t time2 = clock();
	double duration = (double)(time2 - startTime);
	f << duration << endl;
	f.close();
}