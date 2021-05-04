#include "MainWindow.h"
#include "OverlayScene.h"
#include "SceneLabeling.h"
#include "MeshesDialog.h"
#include "XRayViewer.h"
#include <DICOMVisualizer.h>
#include "vtkInteractorStyleOnlyZoomWithViewAngle.h"
#include "SystemGeometryDefinitions.h"

#include "ui_UsageDialog.h"

#include <QFileDialog>
#include <QDockWidget>
#include <qsettings>
#include <QErrorMessage>
#include <qmessagebox>
#include <QInputDialog>
#include <QColorDialog>

#include <qinputdialog>

#include <vtkRenderWindow.h>
#include <vtkInteractorStyleRubberBand2D.h>
#include <vtkCamera.h>
#include <vtkRendererCollection.h>
#include <vtkDistanceWidget.h>
#include <vtkSmartPointer.h>

#include <QCloseEvent>
//#include <vtkImageShiftScale.h>

//#include <vtkProperty.h>
using namespace std;

MainWindow::MainWindow(QWidget *parent, int argc, char** argv) :
    QMainWindow(parent), theOverlayScene(NULL), theXRayViewer(NULL)
{	
	// disable warnings / error output to vtkOutputWindow
	vtkObject::GlobalWarningDisplayOff();
	
	getFrameGrabberConfiguration();

    setupUi(this);	

	setWindowTitle("3D-XGuide");
	enableMenus();

	initVTKWindows();
/////////////////////////
	/*theOverlayScene->getXRAYRenderWindow(0)->GetRenderers()->GetFirstRenderer()->ResetCamera();
	theOverlayScene->getXRAYRenderWindow(1)->GetRenderers()->GetFirstRenderer()->ResetCamera();
	theOverlayScene->getXRAYRenderWindow(2)->GetRenderers()->GetFirstRenderer()->ResetCamera();
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCamera();*/
//////////////////////////
	meshDialogDockWidget = new QDockWidget("Mesh settings", this);
	meshDialogDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
	meshDialogDockWidget->setFixedWidth(200);
	meshesDialog = new MeshesDialog(meshDialogDockWidget, theOverlayScene);
	meshDialogDockWidget->setWidget(meshesDialog);
	addDockWidget(Qt::LeftDockWidgetArea, meshDialogDockWidget);	

	update();
}

void MainWindow::getFrameGrabberConfiguration()
{
	QString ZERO_FRAME_GRABBER = "0 - No Framegrabber connected";
	QString ONE_FRAME_GRABBER = "1 - Frontal framegrabber only";
	QString TWO_FRAME_GRABBER = "2 - Frontal and lateral framegrabber";

	QStringList FrameGrabberOptions;
	FrameGrabberOptions << ZERO_FRAME_GRABBER << ONE_FRAME_GRABBER << TWO_FRAME_GRABBER;
	QString frameGrabber = QInputDialog::getItem(this, "Framegrabber configuration", "How many framegrabbers are connected", FrameGrabberOptions, 0, false);
	if (frameGrabber == ZERO_FRAME_GRABBER)
	{
		frameGrabberNumber = 0;
	}
	if (frameGrabber == ONE_FRAME_GRABBER)
	{
		frameGrabberNumber = 1;
	}
	if (frameGrabber == TWO_FRAME_GRABBER)
	{
		frameGrabberNumber = 2;
	}
}


void MainWindow::initVTKWindows()
{
	// create scene, if it was not created before
	if (theOverlayScene == NULL)
	{
		const int INPUT_CHANNELS = 2;	// 2 XRAY channels
		const int OUTPUT_WINDOWS = 3;	//3D view and 2 XRAY channels
		theOverlayScene = OverlayScene::New(INPUT_CHANNELS, OUTPUT_WINDOWS, frameGrabberNumber);
		
	}
	
	if (theXRayViewer == NULL)
	{
		theXRayViewer = XRayViewer::New(this, theOverlayScene);

	}

	vtkWidget_X->SetRenderWindow(theOverlayScene->GetRenderers2D(0)->GetRenderWindow());
	vtkWidget_Y->SetRenderWindow(theOverlayScene->GetRenderers2D(1)->GetRenderWindow());
	vtkWidget_Z->SetRenderWindow(theOverlayScene->GetRenderers2D(2)->GetRenderWindow());
	vtkWidget_3D->SetRenderWindow(theOverlayScene->GetRenderer3D()->GetRenderWindow());

	//disable complete rotation for 2D views, levelWindow is done via additional button in GUI
	vtkInteractorStyleOnlyZoomWithViewAngle* style1 = vtkInteractorStyleOnlyZoomWithViewAngle::New();
	vtkWidget_X->GetInteractor()->SetInteractorStyle(style1);
	vtkInteractorStyleOnlyZoomWithViewAngle* style2 = vtkInteractorStyleOnlyZoomWithViewAngle::New();
	vtkWidget_Y->GetInteractor()->SetInteractorStyle(style2);
	vtkInteractorStyleOnlyZoomWithViewAngle* style3 = vtkInteractorStyleOnlyZoomWithViewAngle::New();
	vtkWidget_Z->GetInteractor()->SetInteractorStyle(style3);

	distanceWidget = vtkSmartPointer< vtkDistanceWidget >::New();
	distanceWidget->SetInteractor(vtkWidget_X->GetInteractor());
	distanceWidget->CreateDefaultRepresentation();
	distanceWidget->SetEnabled(1);


	hideAxesCheckBox->setChecked(false);
	hideAxesCheckBox->setChecked(false);
	sliderLevelWindow->setDisabled(true);
	verticalSlider_X->setDisabled(true);
	verticalSlider_Y->setDisabled(true);
	verticalSlider_Z->setDisabled(true);

	disableUpdates = true;
	/*MRsequence = false;*/
	/*MeshOrientation = 0;*/

}

void MainWindow::on_hideAxesCheckBox_stateChanged(int state)
{
	theOverlayScene->SetAxesVisibility(!hideAxesCheckBox->isChecked());
	//theOverlayScene->SetAxesVisibility((bool)state);
	vtkWidget_3D->GetRenderWindow()->Render();
	for (int i = 0; i < 3; i++)
	{
		theOverlayScene->getXRAYRenderWindow(i)->Render();
	}

}

void MainWindow::on_buttonSave3DScene_clicked()
{

	// save as image
	QString path = QFileDialog::getSaveFileName(this, "Save current 3D scene", ".");
	if (path.isNull()) return;

	//theOverlayScene->saveCurrent3DSceneImage(qPrintable(path.append(".tiff")));
	theOverlayScene->saveImage(qPrintable(path.append(".png")), theOverlayScene->GetRenderer3D()->GetRenderWindow());
	//theOverlayScene->saveImage(qPrintable(path.append(".tiff")), vtkWidget_3D->GetRenderWindow());


}

void MainWindow::on_hideMRvolumeCheckBox_stateChanged(int state)
{
	theOverlayScene->GetMRVisualizer()->setVisibility(!hideMRvolumeCheckBox->isChecked());
	vtkWidget_3D->GetRenderWindow()->Render();

}

void MainWindow::on_sliderLevelWindow_valueChanged(int value)
{
	if (disableUpdates) return;

	DICOMVisualizer* viewer = theOverlayScene->GetMRVisualizer();

	viewer->SetLevelWindow((double)value);
	
	vtkWidget_3D->GetRenderWindow()->Render();
	vtkWidget_X->GetRenderWindow()->Render();
	vtkWidget_Y->GetRenderWindow()->Render();
	vtkWidget_Z->GetRenderWindow()->Render();		
}

void MainWindow::on_verticalSlider_X_valueChanged(int value)
{
	updatePositionFromGui();
}

void MainWindow::on_verticalSlider_Y_valueChanged(int value)
{
	updatePositionFromGui();
}

void MainWindow::on_verticalSlider_Z_valueChanged(int value)
{
	updatePositionFromGui();
}


void MainWindow::updatePositionFromGui()
{
	if (disableUpdates) return;

	DICOMVisualizer* viewer = theOverlayScene->GetMRVisualizer();
	double spacing = viewer->getSliceSpacing();

	 //get slider values
	double pos[3];
	pos[0] = verticalSlider_X->value();
	pos[1] = verticalSlider_Y->value();
	pos[2] = verticalSlider_Z->value() * spacing;
	//cout << "slider_Z: " << (int)(verticalSlider_Z->value());
	//cout << "pos: " << (pos[2]);

	switch (MeshOrientation) {
	case 1:
		viewer->setMRSliceIntersectionPoint(pos);
		break;

	case 0:
	case 2:
	case 3:
		viewer->setCTSliceIntersectionPointLocal(pos);
		break;
	default:
		return;
	}

	/*if (MRsequence == true)
	{
		viewer->setMRSliceIntersectionPoint(pos);
	}
	else
		viewer->setCTSliceIntersectionPointLocal(pos);
	*/
	theOverlayScene->RenderAll();

}

void MainWindow::enableMenus()
{
	menuFile->setDisabled(false);
	menuDisplay->setDisabled(false);
	actionLoad3DMRIDataset->setEnabled(true);
}

void MainWindow::on_actionLoad3DMRIDataset_triggered()
{
	QString fileFormats("Image files (*.dcm);; All files (*)");
	QString FileName = QFileDialog::getOpenFileName(this,
		"Select 3D MRI Dataset", "b:/NAVIGATION/data", fileFormats);
	if (FileName.isNull()) return;

	theOverlayScene->SetMRVisualizer(qPrintable(FileName));
	MeshOrientation = OverlayScene::MR_ITK;
	reloadFile = qPrintable(FileName);
	meshesDialog->activateAddMeshButton(true);
	theOverlayScene->setMRInputFileForMesh(MeshOrientation);

	updateGUI();	

}


void MainWindow::on_actionLoad3DMRIDatasetPHILIPS_triggered()
{
	QString fileFormats("Image files (*.dcm);; All files (*)");
	QString FileName = QFileDialog::getOpenFileName(this,
		"Select 3D MRI Dataset", "b:/NAVIGATION/SVT ABLATION/", fileFormats);
	QDir d = QFileInfo(FileName).absoluteDir();
	QString absolute = d.absolutePath();
	if (FileName.isNull()) return;

	MeshOrientation = OverlayScene::MR_PHILIPS;
	theOverlayScene->SetCTVisualizer(qPrintable(FileName), qPrintable(absolute), MeshOrientation);
	reloadFile = qPrintable(FileName);
	reloadDir = qPrintable(absolute);
	meshesDialog->activateAddMeshButton(true);
	//theOverlayScene->setMRInputFileForMesh(MeshOrientation);

	updateGUI();

}

void MainWindow::on_actionSet_slice_spacing_manually_triggered()
{
	if (reloadFile.empty()) return;

	double spacing = theOverlayScene->GetMRVisualizer()->getSliceSpacing();
	bool ok;
	double value = QInputDialog::getDouble(this, tr("QInputDialog::getDouble()"),
		tr("Slice spacing:"), spacing, 0.01, 10, 3, &ok);
	if (!ok) return;

	theOverlayScene->GetMRVisualizer()->setSliceSpacing(value);
	theOverlayScene->SetCTVisualizer(reloadFile, reloadDir, MeshOrientation);
	updateGUI();

}


void MainWindow::on_actionLoad3DCTDatasetPHILIPS_triggered()
{

	QString fileFormats("Image files (*.dcm);; All files (*)");
	QString FileName = QFileDialog::getOpenFileName(this,
		"Select 3D CT Dataset", "b:/NAVIGATION/data/TAVI", fileFormats);
	QDir d = QFileInfo(FileName).absoluteDir();
	QString absolute = d.absolutePath();

	if (FileName.isNull()) return;
	MeshOrientation = OverlayScene::CT_PHILIPS;
	theOverlayScene->SetCTVisualizer(qPrintable(FileName), qPrintable(absolute), MeshOrientation);

	reloadFile =  qPrintable(FileName);
	reloadDir = qPrintable(absolute);
	meshesDialog->activateAddMeshButton(true);
	//theOverlayScene->setMRInputFileForMesh(MeshOrientation);

	updateGUI();
}

void MainWindow::on_actionLoad3DCTDatasetITK_triggered()
{
	QString fileFormats("Image files (*.dcm);; All files (*)");
	QString FileName = QFileDialog::getOpenFileName(this,
		"Select 3D CT Dataset", "b:/NAVIGATION/data/TAVI", fileFormats);
	QDir d = QFileInfo(FileName).absoluteDir();
	QString absolute = d.absolutePath();
	if (FileName.isNull()) return;
	MeshOrientation = OverlayScene::CT_ITK;
	theOverlayScene->SetCTVisualizer(qPrintable(FileName), qPrintable(absolute), MeshOrientation);

	reloadFile = qPrintable(FileName);
	reloadDir = qPrintable(absolute);

	meshesDialog->activateAddMeshButton(true);
	//theOverlayScene->setMRInputFileForMesh(MeshOrientation);

	updateGUI();
}

void MainWindow::updateGUI()
{
	DICOMVisualizer* viewer = theOverlayScene->GetMRVisualizer();

	disableUpdates = true;

	double bounds[6];
	//viewer->getVolumeBounds(bounds);

	viewer->getRegisteredVolumeBounds(bounds);
	double spacing = viewer->getSliceSpacing();


	sliderLevelWindow->setEnabled(true);
	verticalSlider_X->setEnabled(true);
	verticalSlider_Y->setEnabled(true);
	verticalSlider_Z->setEnabled(true);

	// QSliders only have int values :-(
	verticalSlider_X->setRange((int)bounds[0], (int)bounds[1]);
	verticalSlider_Y->setRange((int)bounds[2], (int)bounds[3]);
	/*verticalSlider_Y->setRange((int)bounds[2] - (bounds[3] + reg[1]), (int)bounds[3] - (bounds[3] + reg[1]));*/
	verticalSlider_Z->setRange((int)(bounds[4] / spacing), (int)(bounds[5] / spacing));
	//cout << "begin: " << (int)(bounds[4] / spacing);
	//cout << "end: " << (int)(bounds[5] / spacing);

	sliderLevelWindow->setRange(0, 255);
	sliderLevelWindow->setSliderPosition((int)(viewer->GetLevelWindow()));

	double intersectionPoint[3];
	viewer->getSliceIntersectionPointLocal(intersectionPoint);
	// set current values
	verticalSlider_X->setSliderPosition((int)intersectionPoint[0]);
	verticalSlider_Y->setSliderPosition((int)intersectionPoint[1]);
	verticalSlider_Z->setSliderPosition((int)intersectionPoint[2]);

	disableUpdates = false;
}

void MainWindow::on_actionOpenXrayViewer_triggered()
{

	theXRayViewer->show();

	//theOverlayScene->initXRAYWindows();
}

void MainWindow::on_actionSaveScene_triggered()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save scene", "", "INI file (*.ini)");
	if (fileName.isNull()) return;

	saveScene(qPrintable(fileName));
}

void MainWindow::on_actionLoadScene_triggered()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Load scene", "", "INI file (*.ini)");
	if (fileName.isNull()) return;

	loadScene(qPrintable(fileName));

	//meshesDialog->activateAddMeshButton(true);
	meshesDialog->update();
	//updateGUI();
}

void MainWindow::on_actionViewFront_triggered()
{
	vtkCamera* cam = vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();

	cam->SetViewUp(0, 1, 0);
	double dist = cam->GetDistance();
	cam->SetPosition(0, 0, dist);
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCameraClippingRange();
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->Render();
	double* orient = cam->GetOrientation();
}

void MainWindow::on_actionViewSide_triggered()
{
	vtkCamera* cam = vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();

	cam->SetViewUp(0, 1, 0);
	double dist = cam->GetDistance();
	cam->SetPosition(dist, 0, 0);
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCameraClippingRange();
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->Render();
	double* orient  = cam->GetOrientation();
}

void MainWindow::on_actionViewDown_triggered()
{
	vtkCamera* cam = vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();

	cam->SetViewUp(0, 0, 1);
	double dist = cam->GetDistance();
	cam->SetPosition(0, dist, 0);
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->ResetCameraClippingRange();
	vtkWidget_3D->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->Render();
	double* orient = cam->GetOrientation();
}

void MainWindow::on_actionShowXRAYin3DScene_toggled(bool checked)
{
	theOverlayScene->setXRayVisibilityIn3dWindow(checked);
}

void MainWindow::on_actionFirstXRAYdirection_triggered()
{
	theOverlayScene->viewXRayDirection(0);
}

void MainWindow::on_actionSecondXRAYdirection_triggered()
{
	theOverlayScene->viewXRayDirection(1);
}

void MainWindow::saveScene(const char* file)
{
	
	// INI file to save
	QSettings settings(QString(file), QSettings::IniFormat);
	settings.clear(); // make sure that there is nothing old saved, when the file was already existing

	QDir dir = QDir(QString(file)); // for relative paths
	dir.cdUp(); // do "cd .." because the file name in file* is interpreted as diretory	

	if (theOverlayScene->isFramegrabber)
	{
		settings.setValue("USE_FRAMEGRABBER", 1);
	}
	else
	{
		settings.setValue("USE_FRAMEGRABBER", 0);
	}


	BiplaneGeometry* geometry = theOverlayScene->getCurrentBiplaneGeometry();

	

	vector<const char*> XRAYFiles = theOverlayScene->getXRAYFileNames();
	QString input("XRAY_%1");
	QString frame("CURRENT_FRAME_%1");
		for (unsigned int i = 0; i < XRAYFiles.size(); ++i) {
		settings.setValue(input.arg(i), QString(XRAYFiles[i]));
		settings.setValue(frame.arg(i), theXRayViewer->getSliderValue(i));	
			
	}	
		
	/*QString saveFile = file;

	if (XRAYFiles.size() != 0)
	{
		saveFile.append(".geometry_1.txt");
		settings.setValue("GEOMETRY_FIRST_STREAM", saveFile);
		geometry->firstSystem.saveParametersAsAnglesAndDistancesFile(qPrintable(saveFile));

		saveFile = file;
		saveFile.append(".geometry_2.txt");
		settings.setValue("GEOMETRY_SECOND_STREAM", saveFile);
		geometry->secondSystem.saveParametersAsAnglesAndDistancesFile(qPrintable(saveFile));

		settings.setValue("SID_FIRST", geometry->firstSystem.getSourceDetectorDistance());
		settings.setValue("SID_SECOND", geometry->secondSystem.getSourceDetectorDistance());

	}*/
	
	const char* XRAYMainFile = theOverlayScene->getXRAYMainFileName();
	if (XRAYMainFile != NULL && XRAYMainFile[0] != '\0')
	{
		settings.setValue("XRAY_MAIN", QString(XRAYMainFile));
		settings.setValue("CURRENT_FRAME_MAIN", theXRayViewer->getSliderValue(2));
		/*saveFile = file;
		saveFile.append(".geometry_main.txt");
		settings.setValue("GEOMETRY_MAIN_STREAM", saveFile);
		geometry->mainSystem.saveParametersAsAnglesAndDistancesFile(qPrintable(saveFile));
		settings.setValue("SID_MAIN", geometry->mainSystem.getSourceDetectorDistance());*/

	}		

	vector<std::string> meshFiles = theOverlayScene->getMeshFileNames();
	settings.setValue("MESH_SIZE", meshFiles.size());
	QString param("MESH_%1");
	for (unsigned int i = 0; i < meshFiles.size(); ++i) {
		settings.setValue(param.arg(i), QString(meshFiles[i].c_str()));

		double c[3];
		theOverlayScene->getMeshColor(c, i);
		QColor col;
		col.setRgbF(c[0], c[1], c[2]);
		settings.setValue(QString("MESH_COLOR_%1").arg(i), col.name());

		settings.setValue(QString("MESH_OPACITY_%1").arg(i), theOverlayScene->getMeshOpacity(i));
	}

	/*settings.setValue("MR_INPUT_YES", theOverlayScene->getMRInputFileForMesh());*/
	settings.setValue("MESH_ORIENTATION", MeshOrientation);
	settings.setValue("MR_VOLUME", theOverlayScene->GetMRVisualizer()->getInputFile());

	OverlayScene::MR_ITK;

	QString save = file;
	save.append(".marker_3d.vtk");
	theOverlayScene->getMarkerLabeling(2)->writePoints(qPrintable(save));
	settings.setValue("MARKER_3D", dir.relativeFilePath(save));
	double cMarker[3], cRegistration[3];
	theOverlayScene->getMarkerLabeling(2)->getMarkerColor(cMarker);
	QColor colMarker, colRegistration;
	colMarker.setRgbF(cMarker[0], cMarker[1], cMarker[2]);
	settings.setValue(QString("MARKER_3D_COLOR"), colMarker.name());

	save = file;
	save.append(".marker_registration.vtk");
	theOverlayScene->getRegistrationMarkers()->writePoints(qPrintable(save));
	settings.setValue("MARKER_REGISTRATION", dir.relativeFilePath(save));
	theOverlayScene->getRegistrationMarkers()->getMarkerColor(cRegistration);

	colRegistration.setRgbF(cRegistration[0], cRegistration[1], cRegistration[2]);
	settings.setValue(QString("MARKER_REGISTRATION_COLOR"), colRegistration.name());

	save = file;
	save.append(".marker_2d_1.vtk");
	theOverlayScene->getMarkerLabeling(0)->writePoints(qPrintable(save));
	settings.setValue("MARKER_2D_FIRST_STREAM", dir.relativeFilePath(save));

	save = file;
	save.append(".marker_2d_2.vtk");
	theOverlayScene->getMarkerLabeling(1)->writePoints(qPrintable(save));
	settings.setValue("MARKER_2D_SECOND_STREAM", dir.relativeFilePath(save));

	save = file;
	save.append(".meshposition.txt");
	const unsigned int streamNumber = 2;
	theOverlayScene->saveUserMatrix(streamNumber, qPrintable(save));
	settings.setValue("MESH_POSITION_STREAM_2", dir.relativeFilePath(save));
	// mesh positions for other views are updated in OverlayScene accordingly,
	// so they don't need to be saved.

	// QSettings automatically writes to file here, because it is destroyed.
}

void MainWindow::loadScene(const char* file)
{
	// load the INI file
	if (!QFile::exists(file)) cout << "Warning: invalid ini file '" << file << "', trying to load default values" << endl;
	QSettings settings(QString(file), QSettings::IniFormat);

	QDir dir = QDir(QString(file)); // for relative paths
	dir.cdUp(); // do "cd .." because the file name in file* is interpreted as diretory

	cout << "setting system geometry... " << endl;

	const int inputChannels = 2;
	const int outputWindows = 3;

	// -- SETUP THE WHOLE PIPELINE (OverlayScene instance is created)
	cout << "initializing from '" << file << "'..." << endl;
	initVTKWindows();

	QString frameGrab("USE_FRAMEGRABBER");
	bool frameGrabber;
	frameGrabber = settings.value("USE_FRAMEGRABBER").toInt();
	theOverlayScene->isFramegrabber = frameGrabber;


	/* To pass vector<const char*> from QStrings, the strings have to be existent
	when using their const char*. Thus, we hold the corresponding QByteArray.
	If you don't, you access a bad pointer and the application crashes.
	*/

	QString input("XRAY_%1");
	QString frame("CURRENT_FRAME_%1");
	int value;
	vector<QByteArray> RAWs;
	vector<string> strings;
	vector<string> stringsNew;
	for (int i = 0; i < (inputChannels); ++i) {
		QString input_xray = settings.value(input.arg(i)).toString();
		if (input_xray.startsWith(".")) {
			RAWs.push_back(qPrintable(dir.absoluteFilePath(input_xray)));
			strings.push_back(qPrintable(dir.absoluteFilePath(input_xray)));
		}
		else {
			RAWs.push_back(qPrintable(input_xray));
			strings.push_back(qPrintable(input_xray));
		}
		//RAWs.push_back(qPrintable(settings.value(input.arg(i)).toString()));
		//strings.push_back(qPrintable(settings.value(input.arg(i)).toString()));
	}

	vector<const char*> RAWstrings;
	//const char* RAWstringMain = RAWs[RAWs.size() - 1];
	for (unsigned int i = 0; i < RAWs.size(); ++i)
	{
		RAWstrings.push_back(RAWs[i]);

		if (!QFile::exists(RAWs[i])) {
			cout << "Warning: raw file '" << file << "' does not exist" << endl;
		}
	}
		
	if (!frameGrabber)
	{
		theOverlayScene->alreadyConstructedPipelineDICOM[0] = theOverlayScene->alreadyConstructedPipelineDICOM[1] = false; // to update 3D reconstruction depending on initial table pos

		if (!RAWstrings.empty() && (RAWstrings[0] != NULL && RAWstrings[0][0] != '\0'))
			cout << "loading XRAYs... " << endl;
			{
							
				for (int i = 0; i < inputChannels; i++)
				{
					double primAngle, secAngle;

					value = settings.value(frame.arg(i)).toInt();
					//theXRayViewer->setSliderValue(i, value);	
					theXRayViewer->resetCounters(true); // 0 - for FG; 1 - for system
					theOverlayScene->setInputToFile(RAWstrings[i], i);	//HERE correct
					theXRayViewer->activateGui(value, (char*)strings[i].c_str(), i);
					theOverlayScene->getDICOMAnglesToWindowRef(i, primAngle, secAngle);
					//theXRayViewer->setUpdates(i, false);
					theXRayViewer->clearTextActors(i);
					theXRayViewer->displayAngleInWindow(i, primAngle, secAngle);
				}

			}
		}
		else
		{
			theOverlayScene->alreadyConstructedPipeline[0] = theOverlayScene->alreadyConstructedPipeline[1] = false; // to update 3D reconstruction depending on initial table pos
				
			for (int i = 0; i < inputChannels; i++)
			{
				double primAngle, secAngle;					
					
				stringsNew.push_back(strings[i].substr(0, strings[i].find("\\", 0)));
										
				value = settings.value(frame.arg(i)).toInt();
				theXRayViewer->resetCounters(false); // 0 - for FG; 1 - for system				
				//theXRayViewer->setSliderValue(i, value);				
				theOverlayScene->playReferenceStream(value, (char*)stringsNew[i].c_str(), i);
				theXRayViewer->activateGui(value, (char*)stringsNew[i].c_str(), i);
				theXRayViewer->clearTextActors(i);
				theOverlayScene->getDICOMAnglesToWindowRef(i, primAngle, secAngle);
				theXRayViewer->displayAngleInWindow(i, primAngle, secAngle);
					
			}

		}	


	

	theXRayViewer->show();
	//const char* 
	
	QString RAWstringMain = settings.value("XRAY_MAIN").toString();
	
	string RAWMain = qPrintable(RAWstringMain);
	if (RAWstringMain.startsWith(".")) {
		RAWMain = qPrintable(dir.absoluteFilePath(RAWstringMain));
	}
		
	if (RAWstringMain != QString(""))
	//if (RAWstringMain != NULL && RAWstringMain[0] != '\0')
	{
		cout << "loading Main Stream XRAY... " << endl;

		if (!frameGrabber)
		{
			double primAngle, secAngle;			

			value = settings.value("CURRENT_FRAME_MAIN").toInt();
			theOverlayScene->setMainXRAYInputToFile(RAWMain.c_str());
			theXRayViewer->activateGui(value, (char*)RAWMain.c_str(), 2);
			/*theXRayViewer->setSliderValue(2, settings.value("CURRENT_FRAME_MAIN").toInt());
			theXRayViewer->setUpdates(2, false);*/
			theXRayViewer->clearTextActors(2);
			theOverlayScene->getDICOMAnglesToWindow(primAngle, secAngle);
			theXRayViewer->displayAngleInWindow(2, primAngle, secAngle);
			//theXRayViewer->show();
		}
		/*else
		{

		}*/
	}


	cout << "setting biplane geometry..." << endl;
	
	/*
	QString MRInputFileYes = settings.value("MR_INPUT_YES").toString();*/
	int orientation = settings.value("MESH_ORIENTATION").toInt();
	
	QString MRVolumeFile = settings.value("MR_VOLUME").toString();
	string MRVolumeFileString = qPrintable(MRVolumeFile);


	QDir d = QFileInfo(MRVolumeFile).absoluteDir();
	QString absoluteDir = d.absolutePath();
	string MRVolumeDirString = qPrintable(absoluteDir);	


	if (MRVolumeFile.startsWith(".")) {
		MRVolumeFileString = qPrintable(dir.absoluteFilePath(MRVolumeFile));
		MRVolumeDirString = qPrintable(dir.absoluteFilePath(MRVolumeFile).section("/", 0, -2));

	}

	if (MRVolumeFile != QString("")) {
		cout << "loading MR volume data from '" << MRVolumeFileString << "'..." << endl;
		switch (orientation)
		{
		case 0:
			theOverlayScene->SetCTVisualizer(MRVolumeFileString, MRVolumeDirString, orientation);
			reloadFile = MRVolumeFileString;
			reloadDir = MRVolumeDirString;
			MeshOrientation = 0;
			break;
		case 1:
			theOverlayScene->SetMRVisualizer(MRVolumeFileString);
			theOverlayScene->setMRInputFileForMesh(1);
			reloadFile = MRVolumeFileString;
			MeshOrientation = 1;
			break;
		case 2:
			theOverlayScene->SetCTVisualizer(MRVolumeFileString, MRVolumeDirString, orientation);
			reloadFile = MRVolumeFileString;
			reloadDir = MRVolumeDirString;
			MeshOrientation = 2;
			break;
		case 3:
			theOverlayScene->SetCTVisualizer(MRVolumeFileString, MRVolumeDirString, orientation);
			reloadFile = MRVolumeFileString;
			reloadDir = MRVolumeDirString;
			MeshOrientation = 3;
			break;
		default:
				return;

		}

		/*if (MRInputFileYes == "true")		
		{
			theOverlayScene->SetMRVisualizer(qPrintable(MRVolumeFile));
			theOverlayScene->setMRInputFileForMesh(1);
			MRsequence = true;

		}
		else
		{
			theOverlayScene->SetCTVisualizer(qPrintable(MRVolumeFile));
			theOverlayScene->setMRInputFileForMesh(0);
			MRsequence = false;
		}*/

		meshesDialog->activateAddMeshButton(true);
		updateGUI();

	}

	// remove old meshes
	while (theOverlayScene->getNumberOfMeshes() > 0) { theOverlayScene->removeOverlayMesh(); }

	// set default mesh opacity value
	double defaultOpacity = settings.value("DEFAULT_OPACITY", 1.0).toDouble();
	theOverlayScene->giveMeshOpacityToScene(defaultOpacity);

	int mriSize = settings.value("MESH_SIZE", 0).toInt();
	if (mriSize > 0) {
		QString input("MESH_%1");
		cout << "loading Meshes... " << endl;
		for (int i = 0; i < mriSize; ++i) {
			
			QString mesh_file = settings.value(input.arg(i)).toString();			
			if (mesh_file.startsWith(".")) {
				theOverlayScene->addOverlayMesh(qPrintable(dir.absoluteFilePath(mesh_file)));
			}
			else {
				theOverlayScene->addOverlayMesh(qPrintable(mesh_file));
			}
			//theOverlayScene->addOverlayMesh(qPrintable(settings.value(input.arg(i)).toString()));

			QColor col(settings.value(QString("MESH_COLOR_%1").arg(i), "#ff0000").toString());
			double r, g, b;
			col.getRgbF(&r, &g, &b);
			double c[3] = { r, g, b };
			theOverlayScene->setMeshColor(c, i);

			double opacity = settings.value(QString("MESH_OPACITY_%1").arg(i), defaultOpacity).toDouble();
			theOverlayScene->setMeshOpacity(opacity, i);
		}
	}
	
	if (defaultOpacity == 0.0 && mriSize > 0 ) {
		QMessageBox::warning(this, "Load scene", "The mesh opacity equals 0.0, so you will not see the meshes.");
	}


	theXRayViewer->stackClear();

	double previousMeshMatrix[4][4];
	int previousMeshWindow;

	QString meshPositionFile = settings.value("MESH_POSITION_STREAM_2").toString();
	if (meshPositionFile != QString("")) {
		cout << "setting mesh position from '" << qPrintable(meshPositionFile) << "'" << endl;
		const unsigned int streamNumber = 2;
		theOverlayScene->loadUserMatrix(streamNumber, qPrintable(dir.absoluteFilePath(meshPositionFile)));
		
		theOverlayScene->getUserMatrix(streamNumber, previousMeshMatrix);
		previousMeshWindow = streamNumber;
		MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(theOverlayScene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);
		cmd->setText("saved position");
		theXRayViewer->generateUndoRedoCommandStack(cmd, previousMeshWindow, previousMeshMatrix);
		
		// mesh positions for other views are updated in OverlayScene accordingly,
		// so they don't need to be saved.
	}
	else if (mriSize > 0 ) {
		cout << "setting default mesh position" << endl;

		QStringList MESH;
		//MESH << "PHILIPS MRI" << "ITK-SNAP for MRI" << "PHILIPS CT" << "ITK-SNAP for CT"; // not supported: << "saggital";
		MESH << "PHILIPS MRI" << "PHILIPS CT";
		QString orientation = QInputDialog::getItem(this, "Load default mesh position", "For which volume mesh was exported", MESH, 0, false);
		if (orientation == "PHILIPS MRI") theOverlayScene->loadDefaultMeshPosition(OverlayScene::MR_PHILIPS);
		if (orientation == "ITK-SNAP for MRI") theOverlayScene->loadDefaultMeshPosition(OverlayScene::MR_ITK);
		if (orientation == "PHILIPS CT") theOverlayScene->loadDefaultMeshPosition(OverlayScene::CT_PHILIPS);
		if (orientation == "ITK-SNAP for CT") theOverlayScene->loadDefaultMeshPosition(OverlayScene::CT_ITK);
		
		previousMeshWindow = 2;
		theOverlayScene->getUserMatrix(previousMeshWindow, previousMeshMatrix);
		MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(theOverlayScene, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);		
		
		cmd->setText("default position");
		theXRayViewer->generateUndoRedoCommandStack(cmd, previousMeshWindow, previousMeshMatrix);

	}	
	//MESH_POSITION

	//theXRayViewer->stackSetClean();


	QString markerFile3d = settings.value("MARKER_3D").toString();
	if (markerFile3d != QString("")) {
		cout << "loading 3d markers... " << endl;
		theOverlayScene->getMarkerLabeling(2)->loadPoints(qPrintable(dir.absoluteFilePath(markerFile3d)));
		theOverlayScene->getMarkerLabeling(2)->updateLabelPositions();

		QColor col(settings.value(QString("MARKER_3D_COLOR"), "#ff0000").toString());
		double r, g, b;
		col.getRgbF(&r, &g, &b);
		double c[3] = { r, g, b };
		for (unsigned int i = 0; i < 3; ++i) {
			theOverlayScene->getMarkerLabeling(i)->setMarkerColor(c);
		}
		

	}//MARKER_3D

	QString markerFileRegistration = settings.value("MARKER_REGISTRATION").toString();
	if (markerFileRegistration != QString("")) {
		cout << "loading registration markers... " << endl;
		theOverlayScene->getRegistrationMarkers()->loadPoints(qPrintable(dir.absoluteFilePath(markerFileRegistration)));
		theOverlayScene->getRegistrationMarkers()->updateLabelPositions();

		QColor col(settings.value(QString("MARKER_REGISTRATION_COLOR"), "#ff0000").toString());
		double r, g, b;
		col.getRgbF(&r, &g, &b);
		double c[3] = { r, g, b };
		theOverlayScene->getRegistrationMarkers()->setMarkerColor(c);
		changeColorButton->setPalette(col);

		updateGuiRegistrationPointList();

	}//MARKER_REGISTRATION

	QString markerFile2dFirst = settings.value("MARKER_2D_FIRST_STREAM").toString();
	if (markerFile2dFirst != QString("")) {
		cout << "loading 2d markers for first stream... " << endl;
		theOverlayScene->getMarkerLabeling(0)->loadPoints(qPrintable(dir.absoluteFilePath(markerFile2dFirst)));
		theOverlayScene->getMarkerLabeling(0)->updateLabelPositions();
	}//MARKER_2D_FIRST_STREAM

	QString markerFile2dSecond = settings.value("MARKER_2D_SECOND_STREAM").toString();
	if (markerFile2dSecond != QString("")) {
		cout << "loading 2d markers for second stream... " << endl;
		theOverlayScene->getMarkerLabeling(1)->loadPoints(qPrintable(dir.absoluteFilePath(markerFile2dSecond)));
		theOverlayScene->getMarkerLabeling(1)->updateLabelPositions();
	}//MARKER_2D_SECOND_STREAM

	enableMenus();
	// no labels
	theXRayViewer->enableMarkerMenu();
	// not necessarily, are always on when first open the mainWindow
	//on_actionShowXRAYin3DScene_toggled(true);
	//on_actionShowMeshSettings_toggled(true);

	cout << endl;

}

void MainWindow::on_addPointButton_clicked()
{
	double pos[3];
	DICOMVisualizer* DICOMvisualizer = theOverlayScene->GetMRVisualizer();
	DICOMvisualizer->getSliceIntersectionPointLocal(pos);

	SceneLabeling* markers = theOverlayScene->getRegistrationMarkers();
	markers->setStartPosition(pos);
	markers->addPoint();
	markers->updateLabelPositions();

	updateGuiRegistrationPointList();
}

void MainWindow::on_changeColorButton_clicked()
{
	QColor col1 = changeColorButton->palette().color(QPalette::Button);
	QColor col2 = QColorDialog::getColor(col1, this, "Select registration marker point color");
	if (!col2.isValid()) return; // user cancelled

	qreal r, g, b;
	col2.getRgbF(&r, &g, &b);
	double color[3] = { r, g, b };

	SceneLabeling* markers = theOverlayScene->getRegistrationMarkers();
	markers->setMarkerColor(color);

	changeColorButton->setPalette(col2);
}

void MainWindow::updateGuiRegistrationPointList()
{
	pointList->clear();
	SceneLabeling* markers = theOverlayScene->getRegistrationMarkers();
	for (int i = 0; i < markers->getNumberOfPoints(); ++i) {
		pointList->addItem(QString("Point %1").arg(i + 1));
	}
}

void MainWindow::on_removePointButton_clicked()
{
	if (pointList->currentRow() < 0) return; // nothing selected

	SceneLabeling* markers = theOverlayScene->getRegistrationMarkers();
	markers->deletePoint(pointList->currentRow());

	updateGuiRegistrationPointList();
}

void MainWindow::on_showPointsCheckBox_toggled(bool checked)
{
	SceneLabeling* markers = theOverlayScene->getRegistrationMarkers();
	markers->setPointVisibility(checked);
}

void MainWindow::on_showPointLabelsCheckBox_toggled(bool checked)
{
	SceneLabeling* markers = theOverlayScene->getRegistrationMarkers();
	markers->setLabelVisibility(checked);
}

void MainWindow::on_registrationButton_clicked()
{
	if (theOverlayScene->getRegistrationMarkers()->getNumberOfPoints()
		!= theOverlayScene->getMarkerLabeling(2)->getNumberOfPoints()
		)
	{
		QMessageBox::information(this, "3D-3D-Registration",
			"Please generate an equal number of reconstruced and manually set 3D markers");
		return;
	}

	do3d3dRegistration();
}

void MainWindow::do3d3dRegistration()
{
	// get the matrix before registration and save its state for undo/redo stack
	double prevMatrix[4][4];
	theOverlayScene->getUserMatrix(2, prevMatrix);

	theOverlayScene->do3d3dRegistration();

	// get the matrix after registration and save its state for undo/redo stack
	double matrix[4][4];
	theOverlayScene->getUserMatrix(2, matrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(theOverlayScene,
		2, prevMatrix, 2, matrix);
	
	cmd->setText("Mesh registration");
	
	/*double prevMatrix[4][4];
	int prevWindow;

	theXRayViewer->getPreviousCommand(theOverlayScene, prevWindow, prevMatrix);
	double matrix[4][4];
	theOverlayScene->getUserMatrix(2, matrix);
	MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(theOverlayScene,
		prevWindow, prevMatrix, 2, matrix);

	cmd->setText("Mesh registration");*/

	theXRayViewer->generateUndoRedoCommandStack(cmd, 2, matrix);
	
}

void MainWindow::on_actionShowMeshSettings_toggled(bool checked)
{
	if (checked) {
		meshDialogDockWidget->show();
	}
	else {
		meshDialogDockWidget->hide();
	}
}

void MainWindow::on_actionHowToUse_triggered()
{
	QDialog* dialog = new QDialog(this);
	Ui::UsageDialog ui;
	ui.setupUi(dialog);
	dialog->exec();
	delete dialog;
}


void MainWindow::closeEvent(QCloseEvent *event)
{
	close();
	theOverlayScene->~OverlayScene();
}

MainWindow::~MainWindow()
{

}


