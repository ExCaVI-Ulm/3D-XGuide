#include "OverlayScene.h"
//=======================================================
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include <vtkImageViewer.h>
#include <vtkImageReader.h>
#include <vtkGDCMImageReader.h>
#include <vtkImageData.h>
#include <vtkDataSetMapper.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkDataObject.h>
#include <vtkDICOMWriter.h>
#include <vtkDICOMCTGenerator.h>
#include <vtkDICOMGenerator.h>
#include <vtkDICOMSCGenerator.h>
#include <vtkDICOMMetaData.h>
#include <vtkDICOMReader.h>
#include <vtkDICOMImageReader.h>

#include <vtkImageResize.h>
//
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
//________________________________________
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
//----------------------------------------
#include "vtkTimerLog.h"
#include "time.h"
#include <qfileinfo.h>
#include <qglobal.h>
//=======================================================
#include <vector>

#include "SystemGeometryDefinitions.h"
#include "SceneLabeling.h"

#include <DICOMVisualizer.h>
#include <XRAYReader.h>


#include <QFileDialog>
#include <qstring.h>
#include <qlist>
#include <qinputdialog>
#include <qmessagebox>
#include <qtoolbar>
#include <qstatusbar>
#include <QUndoStack>
#include <QColorDialog>

#include <vtkContextView.h>
#include <vtkContextActor.h>
#include <vtkContextScene.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>
#include <vtkChartXY.h>
#include <vtkPlot.h>
#include <vtkAxis.h>
#include <vtkImageMapper3D.h>
#include <vtkLandmarkTransform.h>
#include <vtkSmartPointer.h>

#include "vtkActor.h"
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkImageActor.h>
#include <vtkImageReslice.h>
#include <vtkImageMapper3D.h>
#include <vtkImageSliceMapper.h>
#include <vtkOpenGLImageSliceMapper.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkLineSource.h>
#include <vtkAlgorithm.h>
#include <vtkProp3DCollection.h>
#include <vtkTIFFWriter.h>
#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
//meshes
#include <vtkPolyDataReader.h>
#include <vtkTransformFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransform.h>
#include <vtkAssembly.h>
//motions
#include "MotionCorrectionFilter.h"
#include "BiplaneAlgorithm.h"
#include "SimpleBiplaneFilter.h"
#include "MotionCorrectionFilter.h"
#include "CrosscorrelationMotionCorrectionFilter.h"

// framegrabber
#include <Panel.h>
#include <vtkWin32VideoSource.h>
#include <vtkVideoSource.h>
#include <vtkJPEGReader.h>
#include <vtkDICOMImageReader.h>
#include <vtkDICOMWriter.h>
#include <vtkDICOMGenerator.h>
#include <vtkImageLuminance.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#include <vtkXMLPolyDataReader.h>
using namespace cv;
using namespace std;

// git test commit
OverlayScene* OverlayScene::theInstance = NULL;

OverlayScene* OverlayScene::New(int numberofChannels, int numberofWindows, int numberOfFramegrabbers)
{
	if(!theInstance)
	{
		theInstance = new OverlayScene(numberofChannels, numberofWindows, numberOfFramegrabbers);
		return theInstance;
	}
	else
	{
		cout << "ERROR: OverlayScene::New should only be called once!" << endl;
		return NULL;
	}
}

OverlayScene* OverlayScene::GetInstance()
{
	if(theInstance)
	{
		return theInstance;
	}
	else
	{
		cout << "ERROR: OverlayScene was not instantiated yet! Call New()" << endl;
		return NULL;
	}
}

OverlayScene::OverlayScene(int numberofChannels, int numberofWindows, int numberOfFramegrabbers)
	: line1(0), lineMapper1(0), lineActor1(0), line2(0), lineMapper2(0), lineActor2(0),
	theMRVisualizer(0), epiline1_1(0), epilineActor1_1(0), epilineMapper1_1(0), epiline2_1(0), epilineActor2_1(0), epilineMapper2_1(0),
	epiline1_2(0), epilineActor1_2(0), epilineMapper1_2(0), epiline2_2(0), epilineActor2_2(0), epilineMapper2_2(0), 
	isFramegrabber(0), isRecording(0), isLive(0), loadDefaultMeshPositionBool(true)
{
	activeFilterIsBiplane = false;

	inputChannels = numberofChannels;
	outputWindows = numberofWindows;
	frameGrabbersNumber = numberOfFramegrabbers;

	meshOpacity = 1.0;
	//VTK uses normalized color values so you must do these divisions:    r=double(re)/255.0;  g=double(ge)/255.0; b=double(be)/255.0;
	meshColor[0] = 0.8;
	meshColor[1] = 0.05;
	meshColor[2] = 0.05;

	primAngleMain = 0.0;
	secAngleMain = 0.0;

	primAngleRef[0] = primAngleRef[1] = 0.0;
	secAngleRef[0] = secAngleRef[1] = 0.0;

	readerMainXRAY = NULL;
	activate = false;

	alreadyConstructedPipeline[0] = alreadyConstructedPipeline[1] = alreadyConstructedPipeline[2] = false;
	alreadyConstructedPipelineDICOM[0] = alreadyConstructedPipelineDICOM[1] = alreadyConstructedPipelineDICOM[2] = false;

	setupPipeline();
}

void OverlayScene::SetRenderer3D(vtkRenderer* renderer)
{

	renderer3D = renderer;

}

void OverlayScene::SetRenderers2D(vtkRenderer* renderers[3])
{

	for (int i = 0; i < 3; i++)
	{
		renderers2D[i] = renderers[i];
	}

}

vtkRenderer* OverlayScene::GetRenderers2D(unsigned int number) const
{
	if (number < 0 || number >= 3) return NULL;

	return renderers2D[number];
}

vtkRenderWindow* OverlayScene::getXRAYRenderWindow(unsigned int number) const
{
	if (number < 0 || number >= 3) return NULL;

	if (number == 0 || number == 1)
	{
		return renWinXRAY[number];
	}
	if (number == 2)
	{
		return renWinMainXRAY;
	}

}

vtkRenderer* OverlayScene::getXRAYRenderer(unsigned int number) const
{
	if (number < 0 || number >= 3) return NULL;

	if (number == 0 || number == 1)
	{
		return rendererXRAY[number];
	}
	if (number == 2)
	{
		return rendererMainXRAY;
	}

}


DICOMVisualizer* OverlayScene::GetMRVisualizer()
{
	return theMRVisualizer;
}

void OverlayScene::SetMRVisualizer(string file)
{	

	theMRVisualizer->SetDICOMRenderers2D(renderers2D);
	theMRVisualizer->setMRInputFile(file);	

	renderer3D->ResetCamera();
	RenderAll();
	
}

void OverlayScene::SetCTVisualizer(string file, string folder, int meshOrientation)
{
	orientation = meshOrientation;
	theMRVisualizer->SetDICOMRenderers2D(renderers2D);
	theMRVisualizer->setCTInputFile(file, folder, orientation);

	renderer3D->ResetCamera();
	RenderAll();

}



void OverlayScene::SetAxesVisibility(bool visible)
{
	axesActor->SetVisibility(visible);	
	axesXRAY[0]->SetVisibility(visible);
	axesXRAY[1]->SetVisibility(visible);
	axesMainXRAY->SetVisibility(visible);

}

void OverlayScene::RenderAll()
{
	for (int i = 0; i < outputWindows; i++)
	{

		renderers2D[i]->ResetCamera();
		vtkCamera* cam = renderers2D[i]->GetActiveCamera();
		cam->Zoom(1.6);
		renWin[i]->Render();
	}

	renWin3D->Render();

}

void OverlayScene::initXRAYWindows()
{

}

void OverlayScene::setupPipeline()
{
	//================================================
	// Initialize Main Window with Renderers
	//================================================
	// Renderers/Windows for 3 orthogonal MRI slices
	for (int i = 0; i < outputWindows; i++)
	{
		renWin[i] = vtkRenderWindow::New();
		renderers2D[i] = vtkRenderer::New();
		renWin[i]->AddRenderer(renderers2D[i]);
		renderers2D[i]->SetBackground(0.5, 0.5, 0.5);
		renderers2D[i]->ResetCamera();
		renWin[i]->Render();

		// To plot ECG chart on a transparent backgroud a neu renderer for each XRAYrenderWindow [0,1] - ref1, ref2, and [2] - mainXRAY with a transparent background needs to be created
		renECG[i] = vtkRenderer::New();		
		renECG[i]->SetViewport(0.5, 0, 1, 0.35);
		renECG[i]->SetBackground(0.5, 0.5, 0.5);
		renECG[i]->SetLayer(1);
		// Set up the view
		view[i] = vtkContextActor::New();
		renECG[i]->AddActor(view[i]);
		// ECG Plot: Add multiple line plots, setting the colors etc
		chart[i] = vtkChartXY::New();

	}
	// Renderer/Window for 3D view
	renWin3D = vtkRenderWindow::New();
	renderer3D = vtkRenderer::New();
	renderer3D->SetBackground(0.5, 0.5, 0.5);	
	renWin3D->AddRenderer(renderer3D);

	axesActor = vtkAxesActor::New();
	axesActor->SetTotalLength(100, 100, 100);
	axesActor->SetVisibility(true);
	renderer3D->AddActor(axesActor);

	//================================================
	// Initialize XRAY Windows with Renderers/Actors
	//================================================
	// Renderers/Windows for 2 XRAY images
	for (int i = 0; i < inputChannels; i++)
	{				
		rendererXRAY[i] = vtkRenderer::New();		
		rendererXRAY[i]->SetBackground(0.5, 0.5, 0.5);
		rendererXRAY[i]->SetLayer(0);	
		//rendererXRAY[i]->SetInteractive(1);

		renWinXRAY[i] = vtkRenderWindow::New();
		renWinXRAY[i]->SetNumberOfLayers(2);
		renWinXRAY[i]->AddRenderer(rendererXRAY[i]);
		renWinXRAY[i]->AddRenderer(renECG[i]);

		XRAYReader* readerXRAY = XRAYReader::New();
		theXRAYReaders.push_back(readerXRAY);


		vtkImageReslice* resliceXRAY = vtkImageReslice::New();	
		resliceXRAY->SetOutputDimensionality(2);
		theXRAYReslicers.push_back(resliceXRAY);

		theXRAYActors.push_back(vtkImageActor::New());	
		theXRAYActors3D.push_back(vtkImageActor::New());

		rendererXRAY[i]->AddActor(theXRAYActors[i]);
		renderer3D->AddActor(theXRAYActors3D[i]);
		
		axesXRAY[i] = vtkAxesActor::New();
		axesXRAY[i]->SetTotalLength(100, 100, 100);
		axesXRAY[i]->SetVisibility(true);
		rendererXRAY[i]->AddActor(axesXRAY[i]);

	}

	renWinMainXRAY = vtkRenderWindow::New();
	rendererMainXRAY = vtkRenderer::New();
	renWinMainXRAY->AddRenderer(rendererMainXRAY);
	rendererMainXRAY->SetBackground(0.5, 0.5, 0.5);
	renWinMainXRAY->SetNumberOfLayers(2);
	rendererMainXRAY->SetLayer(0);

	renWinMainXRAY->AddRenderer(renECG[2]);

	
	resliceMainXRAY = vtkImageReslice::New();
	resliceMainXRAY->SetOutputDimensionality(2);

	actorMainXRAY = vtkImageActor::New();
	rendererMainXRAY->AddActor(actorMainXRAY);

	axesMainXRAY = vtkAxesActor::New();
	axesMainXRAY->SetTotalLength(100, 100, 100);
	axesMainXRAY->SetVisibility(true);
	rendererMainXRAY->AddActor(axesMainXRAY);

	//================================================
	// Add meshes : one assembly per output window
	//=================================================
		for (int i = 0; i < (outputWindows + 1); i++)
		{
			vtkAssembly* meshAssembly = vtkAssembly::New();
			theMeshAssemblies.push_back(meshAssembly);	//[0 1 2 3] - one for renWinXRAY[0], one for renWinXRAY[1], one for 3D window, and one for LIVE XRAY			
			meshAssembly->SetUserMatrix(vtkMatrix4x4::New());
			theRegistrationTransformers.push_back(vtkTransform::New());

			vtkAssembly* markerPointAssembly = vtkAssembly::New();
			markerPointAssembly->PickableOff();
			markerPointAssembly->SetPickable(false);
			theMarkerPointAssemblies.push_back(markerPointAssembly);
			markerPointAssembly->SetUserMatrix(vtkMatrix4x4::New());

			SceneLabeling* sceneLabeling = new SceneLabeling();
			theMarkerPoints.push_back(sceneLabeling);
			sceneLabeling->setAssembly(markerPointAssembly);
			if (i == 0 || i == 1)
				sceneLabeling->setRenderWindow(renWinXRAY[i]);
			if (i == 2)
				sceneLabeling->setRenderWindow(renWin3D);			

		}		

		renderer3D->AddActor(theMeshAssemblies[2]);	
		renderer3D->AddActor(theMarkerPointAssemblies[2]);

		rendererMainXRAY->AddActor(theMeshAssemblies[3]);
		rendererMainXRAY->ResetCamera();
		renWinMainXRAY->Render();
///////////////////////////
		/*renderer3D->ResetCamera();
		renWin3D->Render();*/

		for (int i = 0; i < inputChannels; i++)
		{
			
			rendererXRAY[i]->AddActor(theMeshAssemblies[i]);
			rendererXRAY[i]->AddActor(theMarkerPointAssemblies[i]);
			rendererXRAY[i]->ResetCamera();
			renWinXRAY[i]->Render();
		}
		theBiplaneFilter = BiplaneAlgorithm::New();
		//theBiplaneFilter = BiplaneFilterAlgorithmFactory::createDefaultAlgorithm();
		theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[0], true);
		theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[1], false);
		theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[0], true);
		theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[1], false);



		theTrackingPointAssembly = vtkAssembly::New();
		theTrackingPointAssembly->SetUserMatrix(vtkMatrix4x4::New());
		theTrackingPointAssembly->PickableOff();
		renderer3D->AddActor(theTrackingPointAssembly);
		//theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true);
		theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true, true);

		theMotionCorrections = FilterAlgorithmFactory::createDefaultAlgorithm();
		theMotionCorrections->addAssemblyToMove(theMeshAssemblies[3]); // work for motion comp in XRAY LIVE WIndow

		theRegistrationMarkers = new SceneLabeling();
		theRegistrationMarkers->setAssembly(theMeshAssemblies[2]);
		theRegistrationMarkers->setRenderWindow(renWin3D); // in 3D window
		double color[3] = { 0.0, 1.0, 1.0 };
		theRegistrationMarkers->setMarkerColor(color);

		theMRVisualizer = new DICOMVisualizer(theMeshAssemblies[2]);
		theBiplaneFilter->setVolumeVisualizerToUpdate(theMRVisualizer);
		theMotionCorrections->setVolumeVisualizerToUpdate(theMRVisualizer);

		line1 = vtkLineSource::New();
		lineMapper1 = vtkPolyDataMapper::New();
		lineMapper1->SetInputConnection(line1->GetOutputPort());
		lineActor1 = vtkActor::New();
		lineActor1->SetPickable(false);
		lineActor1->SetMapper(lineMapper1);
		lineActor1->GetProperty()->SetColor(1.0, 1.0, 0.0); // yellow
		//lineActor1->GetProperty()->SetColor(1.0, 0.0, 0.0); // red
		lineActor1->GetProperty()->SetLineWidth(3);
		renderer3D->AddActor(lineActor1);

		line2 = vtkLineSource::New();
		lineMapper2 = vtkPolyDataMapper::New();
		lineMapper2->SetInputConnection(line2->GetOutputPort());
		lineActor2 = vtkActor::New();
		lineActor2->SetPickable(false);
		lineActor2->SetMapper(lineMapper2);
		lineActor2->GetProperty()->SetColor(1.0, 1.0, 0.0);
		//lineActor2->GetProperty()->SetColor(1.0, 0.0, 0.0);
		lineActor2->GetProperty()->SetLineWidth(3);
		renderer3D->AddActor(lineActor2);
//////////////////////////////
		/*renderer3D->ResetCamera();
		renWin3D->Render();*/
//////////////////////
//================================================
// Initialize pipeline for playing videos
//================================================
		mustWrite[0] = mustWrite[1] = false;
		writer = vtkDICOMWriter::New();
		//vtkDICOMWriter* writer0 = vtkDICOMWriter::New();
		//vtkDICOMWriter* writer1 = vtkDICOMWriter::New();
		writer0 = vtkDICOMWriter::New();
		writer1 = vtkDICOMWriter::New();
		writerBiplane.push_back(writer0);
		writerBiplane.push_back(writer1);

		generator = vtkDICOMCTGenerator::New();
		generator0 = vtkDICOMCTGenerator::New();
		generator1 = vtkDICOMCTGenerator::New();
		generatorBiplane.push_back(generator0);
		generatorBiplane.push_back(generator1);

		DICOMreader = XRAYReader::New();
//-----------------------------------------------
		createPatientdirectory();
		biplaneSystem = false;
		
		//================================================
		// Initialize pipeline for getGeometry
		//================================================
		//-----------------------Framegrabber----------------------
		int initialized = 0;
		/*for (size_t i = 0; i < inputChannels; i++)*/
		for (size_t i = 0; i < frameGrabbersNumber; i++)
		{
			theFramegrabbers[i] = vtkWin32VideoSource::New();
			theFramegrabbers[i]->SetDataSpacing(SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[i], SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[i], 1);
			//theFramegrabbers[i]->SetOutputFormat(VTK_LUMINANCE); // funktioniert leider nicht in vtk8.2 -> luminancefilter
			// Initialize wird sonst automatisch bei erstem grab() aufgerufen, evtl ueber die GUI vearaenderbar machen?
			// theFramegrabbbers[0] wird gerendert
			theFramegrabbers[i]->Initialize();
			// test: is initialized?			
			initialized += theFramegrabbers[i]->GetInitialized(); // mindestens ein Framegrabber angeschlossen?
			
			luminanceFilter[i] = vtkImageLuminance::New();
			luminanceFilter[i]->SetInputConnection(theFramegrabbers[i]->GetOutputPort(0));
		}

		if (initialized == 1)
		{
			// Erkennung eines vom Framgrabber verursachten Shifts in horizontaler Richtung
			int shiftHorizontal0 = recognizeShift(0);

			// AdjustmentKorrektur eines vom Framgrabber verursachten Shifts in horizontaler Richtung
			// und Clipping der Namenszeile (hard gecodeter shift in verticaler Richtung zur Anonymisierung)
			clipFG(0, shiftHorizontal0, SystemGeometryDefinitions::CLIP_NAME);

			// initialize two panels with geometry values
			panelFront = new Panel(true, true, 0, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1200, 0);
			//panelLat = new Panel(true, true, 90, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1300, 0);

			// set frontal input to index 0; set lateral input to index 1
			testFramegrabberInitialization();
		}

		// falls ein Framegrabber angeschlossen ist, wird dieser zweimal initialisiert
		// hier wird geprueft, ob erfolgreich zwei framegrabber Objekte initialisiert wurden
		else if (initialized==2) {
			// Erkennung eines vom Framgrabber verursachten Shifts in horizontaler Richtung
			int shiftHorizontal0 = recognizeShift(0);
			int shiftHorizontal1 = recognizeShift(1);

			// AdjustmentKorrektur eines vom Framgrabber verursachten Shifts in horizontaler Richtung
			// und Clipping der Namenszeile (hard gecodeter shift in verticaler Richtung zur Anonymisierung)
			clipFG(0,shiftHorizontal0, SystemGeometryDefinitions::CLIP_NAME);
			clipFG(1, shiftHorizontal1, SystemGeometryDefinitions::CLIP_NAME);

			// initialize two panels with geometry values
			panelFront = new Panel(true, true, 0, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1200, 0);
			panelLat = new Panel(true, true, 90, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1300, 0);
			
			// set frontal input to index 0; set lateral input to index 1
			testFramegrabberInitialization();
		}
		else {
			canReadLayout[0] = canReadLayout[1] = false;
			cout << "WARNING: No framegrabber detected" << endl;
		}

		liveIndex = 0;
		ofstream f;
}

OverlayScene::~OverlayScene()
{
	releasePipeline(); 

}


bool OverlayScene::testInitializationFrontal(int index) {

	bool testErfolgreich = false;

	if (!theFramegrabbers[index]->GetInitialized()) {
		return testErfolgreich;
	}

	theFramegrabbers[index]->Grab();
	theFramegrabbers[index]->Update();
	luminanceFilter[index]->Update();

	vtkImageData* image = luminanceFilter[index]->GetOutput();
	//vtkImageData* image = theFramegrabbers[index]->GetOutput();
	//vtkImageData* image = DICOMreader->GetOutput(); // Test mit play
	int dimensions[3];
	image->GetDimensions(dimensions);

	// Get a pointer to the actual pixel data. 
	//unsigned char *pVtkPix = (unsigned char *)image->GetScalarPointer();

	int widthIm = dimensions[0];
	int heightIm = dimensions[1];

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());

	// Hoehe first
	cv::Rect Hoehe_FirstRect = cv::Rect(220, 1023 - 307, 19, 30);
	cv::Mat Hoehe_FirstDigit, Hoehe_FirstDigit_bin;
	Hoehe_FirstDigit = img(Hoehe_FirstRect);
	double minTest, maxTest;
	cv::minMaxLoc(Hoehe_FirstDigit, &minTest, &maxTest);
	cv::threshold(Hoehe_FirstDigit, Hoehe_FirstDigit_bin, minTest, 255, cv::THRESH_BINARY);
	int Hoehe;
	Hoehe = norm(Hoehe_FirstDigit_bin);

	if (Hoehe != 0) { // keinen Wert bei der Hoehe--> nicht frontal
		testErfolgreich = true;
	}

	return testErfolgreich;
}

bool OverlayScene::testInitializationLateral(int index) {
	bool testErfolgreich = false;

	if (!theFramegrabbers[index]->GetInitialized()) {
		return testErfolgreich;
	}

	theFramegrabbers[index]->Grab();
	theFramegrabbers[index]->Update();
	luminanceFilter[index]->Update();

	vtkImageData* image = luminanceFilter[index]->GetOutput();
	//vtkImageData* image = DICOMreader->GetOutput(); // Test mit play
	int dimensions[3];
	image->GetDimensions(dimensions);

	// Get a pointer to the actual pixel data. 
	//unsigned char *pVtkPix = (unsigned char *)image->GetScalarPointer();

	int widthIm = dimensions[0];
	int heightIm = dimensions[1];

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());

	// Hoehe first
	cv::Rect Hoehe_FirstRect = cv::Rect(220, 1023 - 307, 19, 30);
	cv::Mat Hoehe_FirstDigit, Hoehe_FirstDigit_bin;
	Hoehe_FirstDigit = img(Hoehe_FirstRect);
	double minTest, maxTest;
	cv::minMaxLoc(Hoehe_FirstDigit, &minTest, &maxTest);
	cv::threshold(Hoehe_FirstDigit, Hoehe_FirstDigit_bin, minTest, 255, cv::THRESH_BINARY);
	int Hoehe_FirstNorm = norm(Hoehe_FirstDigit_bin);


	if (Hoehe_FirstNorm == 0) { // kein Wert bei der Hoehe--> lateral
		testErfolgreich = true;
	}

	return testErfolgreich;
}


void OverlayScene::testFramegrabberInitialization() {
	
	if (frameGrabbersNumber == 1)
	{
		lateralInput = false;
		frontalInput = true;

		// index = 0 = frontal
		if (!theFramegrabbers[0]->GetInitialized()) {
			canReadLayout[0] = canReadLayout[1] = false;
			cout << "ERROR: Wrong or no initilization of the framegrabber(s)." << endl;
			frontalInput = lateralInput = false;
			return;
		}

		// check for lateral input
		if (!testInitializationFrontal(0)) {
			frontalInput = false;		//f�r test ohne 2-en framegrabber im biplane soll true sein
			cout << "WARNING: No frontal input." << endl;
		}
	}
	else if (frameGrabbersNumber == 2)
	{
		frontalInput = lateralInput = true;

		// index = 0 = frontal
		if (!theFramegrabbers[0]->GetInitialized() || !theFramegrabbers[1]->GetInitialized()) {
			canReadLayout[0] = canReadLayout[1] = false;
			cout << "ERROR: Wrong or no initilization of the framegrabber(s)." << endl;
			frontalInput = lateralInput = false;
			return;
		}

		// check for frontal input
		if (!testInitializationFrontal(0)) { // is Null not frontal 
			if (testInitializationFrontal(1)) { // is One frontal -> switch indexation
				// speichere zwischen
				// tausche framegrabber
				theFramegrabbersTemp = theFramegrabbers[0];
				theFramegrabbers[0] = theFramegrabbers[1];
				theFramegrabbers[1] = theFramegrabbersTemp;

				luminanceFilterTemp = luminanceFilter[0];
				luminanceFilter[0] = luminanceFilter[1];
				luminanceFilter[1] = luminanceFilterTemp;

				bool layoutTemp;
				layoutTemp = canReadLayout[0];
				canReadLayout[0] = canReadLayout[1];
				canReadLayout[1] = layoutTemp;
			}
			else {// is One not frontal 
				frontalInput = false;
				cout << "ERROR: no frontal input" << endl;
				return;
			}
		}

		// check for lateral input
		if (!testInitializationLateral(1)) { // is One not lateral 
			lateralInput = false;		//f�r test ohne 2-en framegrabber im biplane soll true sein
			cout << "WARNING: No lateral input." << endl;
		}
		// zum testen
		//frontalInput = lateralInput = true;
	}

}
int OverlayScene::recognizeShift(int index) {
	// checks wether there is a horizontal shift
	// using the position of the FD letters
	int shiftHorizontal = 0; // - <-> +

							 // ----------------------------------- ----------------------------------- 
							 // get frame/image 
							 // ----------------------------------- ----------------------------------- 
	if (!theFramegrabbers[index]->GetInitialized()) {
		canReadLayout[index] = false;
		cout << "WARNING: No Framegrabber is detected." << endl;
		return -1;
	}

	theFramegrabbers[index]->Grab();
	theFramegrabbers[index]->Update();
	luminanceFilter[index]->Update();


	vtkImageData* image = luminanceFilter[index]->GetOutput();
	//vtkImageData* image = theFramegrabbers[index]->GetOutput();
	int dimensions[3];
	image->GetDimensions(dimensions);

	// Get a pointer to the actual pixel data. 
	int widthIm = dimensions[0];
	int heightIm = dimensions[1];
	// ----------------------------------- ----------------------------------- 
	// test for dimension/resolution 
	// ----------------------------------- ----------------------------------- 
	if (widthIm != 1280 || heightIm != 1024) {
		canReadLayout[index] = false;
		cout << "ERROR: resolution for geometry recognition has to be 1280x1024." << endl;
		return -1;
	}

	// create OpenCV matrix of the vtkImageData without(!) copying the content
	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());

	int thresh = 20; // first non-black shift pixel in horizontal direction
	int pxlX = heightIm + SystemGeometryDefinitions::CLIP_NAME;
	shiftHorizontal = widthIm;
	int shiftHorizontal_limit = 20;
	while (shiftHorizontal > shiftHorizontal_limit && pxlX > 0)
	{
		for (int i = 0; i < img.cols; i++) {
			if ((int)img.at<uchar>(pxlX, i) > thresh) {	///gray pixel in the upper image region below patient name
				//cout << "horizontalShift = " << i << endl;
				shiftHorizontal = i;

				break;
			}
		}
		if (shiftHorizontal >= shiftHorizontal_limit)
		{
			pxlX -= 1;
		}
	}

	/*for (int i = 0; i < img.cols; i++) {
			cout << ((int)img.at<uchar>(pxlX, i)) << " ";
		
	}*/

	return shiftHorizontal;

}




//int OverlayScene::recognizeShift(int index) {
//	// checks wether there is a horizontal shift
//	// using the position of the FD letters
//	int shiftVertical = 0;
//	int shiftHorizontal = 0; // - <-> +
//
//	// ----------------------------------- ----------------------------------- 
//	// get frame/image 
//	// ----------------------------------- ----------------------------------- 
//	if (!theFramegrabbers[index]->GetInitialized()) {
//		canReadLayout[index] = false;
//		cout << "WARNING: No Framegrabber is detected." << endl;
//		return -1;
//	}
//
//	theFramegrabbers[index]->Grab();
//	theFramegrabbers[index]->Update();
//	luminanceFilter[index]->Update();
//
//	vtkImageData* image = luminanceFilter[index]->GetOutput();
//	//vtkImageData* image = theFramegrabbers[index]->GetOutput();
//	int dimensions[3];
//	image->GetDimensions(dimensions);
//
//	// Get a pointer to the actual pixel data. 
//	int widthIm = dimensions[0];
//	int heightIm = dimensions[1];
//	// ----------------------------------- ----------------------------------- 
//	// test for dimension/resolution 
//	// ----------------------------------- ----------------------------------- 
//	if (widthIm != 1280 || heightIm != 1024) {
//		canReadLayout[index] = false;
//		cout << "ERROR: resolution for geometry recognition has to be 1280x1024." << endl;
//		return -1;
//	}
//
//	// create OpenCV matrix of the vtkImageData without(!) copying the content
//	cv::Mat img(heightIm, widthIm, CV_8U, image->GetScalarPointer());
//	cv::Mat img_bin, dst1;
//	double min, max; // min = black, max = bright
//	cv::minMaxLoc(img, &min, &max);
//	cv::threshold(img, img_bin, min, 255, 0);
//	//cv::flip(img_bin, dst1, 0);     //segmentation fault shown
//	//namedWindow("Display window2", WINDOW_NORMAL);// Create a window for display.
//	//imshow("Display window2", dst1);
//
//	//-----------------------------------
//	// FD sample 
//	//-----------------------------------
//	// create datapath
//	char * datapath = "..\\Sample\\1.dcm";
//	char * H = ".dcm";
//	char * N = "\\"; //	char * N = "/";
//	char * bufferNew = new char[strlen(datapath) + 1];
//	strcpy(bufferNew, datapath);
//	// check for sample folder
//	QDir qdir = QDir(QString("..\\Sample"));
//	if (!qdir.exists())
//	{
//		cout << "Warning: Sample folder not found. Can't recognize shift." << endl;
//		return -1;
//	}
//	// read sample
//	vtkDICOMReader* DICOMreaderFD = vtkDICOMReader::New();
//	DICOMreaderFD->SetFileName(bufferNew);
//	DICOMreaderFD->Update();
//	vtkImageData* FDimage = DICOMreaderFD->GetOutput();
//	int dimensionsFD[3];
//	FDimage->GetDimensions(dimensionsFD);
//	int widthImFD = dimensionsFD[0];
//	int heightImFD = dimensionsFD[1];
//	cv::Mat FDMatrix(heightImFD, widthImFD, CV_8U, FDimage->GetScalarPointer());
//
//	cv::Rect FD_Rect = cv::Rect(7, 1023 - 400, 29,27); //upper edge of panel y = 168; panel width= 256-> take the middle 128
//															   //cv::Rect ROI_Rect = cv::Rect(120, 1023 - 168 -13, 16, 16); //upper edge of panel y = 168; panel width= 256-> take the middle 128
//	cv::Mat FDSample = FDMatrix(FD_Rect);
//	cv::Mat correlation(img.rows, img.cols, CV_8U);
//	matchTemplate(img, FDSample, correlation, CV_TM_CCORR_NORMED); //->maxPotision
//	//matchTemplate(img, FDSample, correlation, CV_TM_SQDIFF_NORMED); //->minPosition
//	
//	cv::Point minPosition, maxPosition;
//	double minValue, maxValue;
//	minMaxLoc(correlation, &minValue, &maxValue, &minPosition, &maxPosition);
//	shiftVertical = 1023 - 400 - maxPosition.y;
//	shiftHorizontal = maxPosition.x - 7;
//
//	cout << "shiftHorizontal: " << shiftHorizontal << endl;
//	cout << "shiftVertical: " << shiftVertical << endl;
//
//	// exeption for no FD found 
//	if (maxValue < 0.95 || maxPosition.y < 8) {
//		canReadLayout[index] = false;
//		cout << "ERROR: character recognition can not handle this layout. FD not found" << endl;
//		cout << "maxValue: " << maxValue << endl;
//		cout << "maxPosition.y: " << maxPosition.y << endl;
//		return -1;
//	}
//
//	// exeption for negative shift
//	if (shiftHorizontal < 0 || shiftVertical < 0) {
//		canReadLayout[index] = false;
//		cout << "ERROR: character recognition can not handle this layout." << endl;
//		return -1;
//	}
//
//	return shiftHorizontal;
//
//	//image->Delete();
//	//DICOMreaderSID->Delete();
//}


void OverlayScene::clipFG(int index, int shiftHorizontal, int shiftVertical) {
	
	// checks again resolution
	vtkImageData* image = luminanceFilter[index]->GetOutput();
	//vtkImageData* image = theFramegrabbers[index]->GetOutput();
	int dimensions[3];
	image->GetDimensions(dimensions);

	// Get a pointer to the actual pixel data. 
	int widthIm = dimensions[0];
	int heightIm = dimensions[1];
	// ----------------------------------- ----------------------------------- 
	// test for dimension/resolution 
	// ----------------------------------- ----------------------------------- 
	if (widthIm != 1280 || heightIm != 1024) {
		canReadLayout[index] = false;
		cout << "ERROR: resolution for geometry recognition has to be 1280x1024." << endl;
		return;
	}

	// ----------------------------------- -----------------------------------
	// adjustment
	// ----------------------------------- -----------------------------------
	theFramegrabbers[index]->SetClipRegion(
		shiftHorizontal,
		widthIm + shiftHorizontal,
		0,//shiftVertical,
		heightIm + shiftVertical,
		SystemGeometryDefinitions::CLIP_Z_MIN,
		SystemGeometryDefinitions::CLIP_Z_MAX
	);
	theFramegrabbers[index]->Update();
	luminanceFilter[index]->Update();
}

// Delete everything in reversed order of creation.
void OverlayScene::releasePipeline()
{

	theMotionCorrections->Delete(); //do this first, as it has access to some other parts of the pipeline
	theBiplaneFilter->Delete();

	while (!theMeshReaders.empty()) { removeOverlayMesh(); }

	delete theRegistrationMarkers;

	lineActor1->Delete();
	lineMapper1->Delete();
	line1->Delete();

	lineActor2->Delete();
	lineMapper2->Delete();
	line2->Delete();

	axesActor->Delete();
	axesMainXRAY->Delete();


	for (unsigned int i = 0; i < theTrackingPointSources.size(); ++i) {
		theTrackingPointAssembly->RemovePart(theTrackingPointActors[i]);
		theTrackingPointActors[i]->Delete();
		theTrackingPointTransformers[i]->Delete();
		theTrackingPointMappers[i]->Delete();
		theTrackingPointSources[i]->Delete();
	}
	theTrackingPointAssembly->Delete();	

	renWin3D->Delete();
	renderer3D->Delete();
	rendererMainXRAY->Delete();
	renWinMainXRAY->Delete();
	resliceMainXRAY->Delete();
	actorMainXRAY->Delete();


	for (unsigned int i = 0; i < theMeshAssemblies.size(); ++i) {
		theMeshAssemblies[i]->Delete();
		theMarkerPointAssemblies[i]->Delete();
		delete theMarkerPoints[i];
	}

	for (unsigned int j = 0; j < outputWindows; ++j) {
		renWin[j]->Delete();
		renderers2D[j]->Delete();
		view[j]->Delete();
		renECG[j]->Delete();
		chart[j]->Delete();
	}
	
	for (unsigned int i = 0; i < inputChannels; ++i)
	{
		rendererXRAY[i]->Delete();
		axesXRAY[i]->Delete();
		renWinXRAY[i]->Delete();
		
	}

	
	for (unsigned int i = 0; i < theXRAYActors.size(); ++i) {
		
		theXRAYReslicers[i]->Delete();
		theXRAYReaders[i]->Delete();
		theXRAYActors[i]->Delete();
		theXRAYActors3D[i]->Delete();		

	}
	for (int i = 0; i < frameGrabbersNumber; i++)
	{
		theFramegrabbers[i]->ReleaseSystemResources();
		theFramegrabbers[i]->Delete();
		luminanceFilter[i]->Delete();
	}	
	
	writer->Delete();
	writer0->Delete();
	writer1->Delete();
	writerBiplane.clear();
	
	generator->Delete();
	generator0->Delete();
	generator1->Delete();
	generatorBiplane.clear();

	DICOMreader->Delete();

	theMRVisualizer = NULL;
}


void OverlayScene::live()
{
	int index;
	bool updateGeometry = false;
	// frontal -> nur ein grab noetig
	if (liveIndex == 0) 
	{
		int index = liveIndex;
		
		if (!theFramegrabbers[index]->GetInitialized()) {
			cout << "WARNING: No Framegrabber is detected." << endl;
			return;
		}

		//int start = startClock("grab.txt");
			theFramegrabbers[index]->Grab();
		//stopClock(start);
		//int start1 = startClock("update.txt");
			theFramegrabbers[index]->Update();
		//stopClock(start1);
			luminanceFilter[index]->Update();
			
		if (!alreadyConstructedPipeline[2]) {
			visualizeActors(2, true);
			theMotionCorrections->SetInputConnection(luminanceFilter[index]->GetOutputPort());
			actorMainXRAY->GetMapper()->SetInputConnection(theMotionCorrections->GetOutputPort());
			setFilterToType("Crosscorrelation Motion Correction Filter");
			alreadyConstructedPipeline[2] = true;
			updateGeometry = true;
		}


		// panelFront ist nur aufgrund der initialwerte frontal, wichtig ist das �bergebene Bild
		panelFront->update(luminanceFilter[index]->GetOutput());

		if (updateGeometry || panelFront->getGeometryHasChanged())
		{
			setFramegrabberGeometryLive(index);//, SIDLive[index], primAngleLive[index], secAngleLive[index], LatLive[index], longLive[index], HoeheLive[index], (double)FDLive[index], false);
			mustWrite[index] = true;
		}
	}
	// lateral -> frontal muss auch gegrabbt werden (wegen der Tischposition)
	else {
		// grab and read frontal
		index = 0;
		if (!theFramegrabbers[index]->GetInitialized()) {
			cout << "WARNING: No Framegrabber is detected." << endl;
			return;
		}
		theFramegrabbers[index]->Grab();

		theFramegrabbers[index]->Update();
		luminanceFilter[index]->Update();
		// panelFront ist nur aufgrund der initialwerte frontal, wichtig ist das �bergebene Bild
		panelFront->update(luminanceFilter[index]->GetOutput());

		// grab, read lateral and replace tableposistion
		index = liveIndex;
		if (!theFramegrabbers[index]->GetInitialized()) {
			cout << "WARNING: No Framegrabber is detected." << endl;
			return;
		}
		theFramegrabbers[index]->Grab();
		theFramegrabbers[index]->Update();
		luminanceFilter[index]->Update();

		if (!alreadyConstructedPipeline[2]) {
			visualizeActors(2, true);
			theMotionCorrections->SetInputConnection(luminanceFilter[index]->GetOutputPort());
			actorMainXRAY->GetMapper()->SetInputConnection(theMotionCorrections->GetOutputPort());
			alreadyConstructedPipeline[2] = true;
			updateGeometry = true;
		}

		// panelFront ist nur aufgrund der initialwerte frontal, wichtig ist das �bergebene Bild
		panelLat->update(luminanceFilter[index]->GetOutput());

		// index = 1 -> lateral -> kein Lesen der Tischposition mgl. -> uebernehme tishcposition von frontal
		// Voraussetzung: liveBiplane(0) wurde aufgerufen
				
		if (updateGeometry || panelLat->getGeometryHasChanged() || panelFront->getTablePosHasChanged())
		{
			// (SID, RAOLAO, KRANKAUD, Lat, Long, hoehe, mmPerPxl, play?)
			setFramegrabberGeometryLive(index);//, SIDLive[index], primAngleLive[index], secAngleLive[index], LatLive[index], longLive[index], HoeheLive[index], (double)FDLive[index], false);
			mustWrite[index] = true;
		}
	
	}

	//renWinMainXRAY->Render();
	
}

void OverlayScene::liveBiplane(int index)
{
	bool updateGeometry = false;
	
	if (!alreadyConstructedPipeline[index]) {
		visualizeActors(index, true);
		theXRAYActors[index]->GetMapper()->SetInputConnection(luminanceFilter[index]->GetOutputPort());
		theXRAYActors3D[index]->GetMapper()->SetInputConnection(luminanceFilter[index]->GetOutputPort());
		setFilterToType("Crosscorrelation Biplane Filter");
		alreadyConstructedPipeline[index] = true;
		updateGeometry = true;
	}
	if (!theFramegrabbers[index]->GetInitialized()) {
		cout << "WARNING: No Framegrabber is detected." << endl;
		return;
	}
	theFramegrabbers[index]->Grab();
	theFramegrabbers[index]->Update();
	luminanceFilter[index]->Update();
	panelFront->update(luminanceFilter[0]->GetOutput());
	bool updateValues, updateTable;
	
	// index = 1 -> lateral -> kein Lesen der Tischposition mgl. -> uebernehme tishcposition von frontal
	// Voraussetzung: liveBiplane(0) wurde aufgerufen
	if (index == 1) {
		panelLat->update(luminanceFilter[1]->GetOutput());
		updateValues = panelLat->getGeometryHasChanged();
		updateTable = panelFront->getTablePosHasChanged();
		// liveBiplane(0); // mit diesem Aufruf wird die Voraussetzung gesichert, jedoch aufwendig durch erneuten grab und update
	}
	else {
		updateValues = panelFront->getGeometryHasChanged();
		updateTable = panelFront->getTablePosHasChanged();
	}

	// falls sich die Geometrie geaendert hat-> berechne Transformationen
	if (updateGeometry || updateValues || updateTable)
	{
		// (SID, RAOLAO, KRANKAUD, Lat, Long, hoehe, mmPerPxl, play?)
		setFramegrabberGeometryBiplaneLive(index);
		mustWrite[index] = true;
	}
	//renWin3D->Render();
}


void OverlayScene::startRecord(int run)
{
	if (!isFramegrabber) return; // nothing to record, because stream is from file
	QDir qdir = QDir(QString(patientDir));
	if (!qdir.exists())
	{
		cout << "WARNING: Patientdirectory did not exist." << endl;
		CreateDirectory(patientDir,0);

	}

	char* N = patientDir;
	char int2char[10], int2charRAO[10];
	sprintf(int2char, "%d", run);
	char* Angle;
	bool rao;
	int laorao;
	//panelFront->update(...);??
	panelFront->getFirstAngle(rao,laorao);
	(rao) ?  Angle = "_RAO" : Angle = "_LAO";
	sprintf(int2charRAO, "%d", abs(laorao)); // immer frontal angle
	char* movieDirectory = new char[strlen(N) + strlen(int2char) + strlen(Angle) + strlen(int2charRAO) + 1];
	strcpy(movieDirectory, N);
	strcat(movieDirectory, int2char);
	strcat(movieDirectory, Angle);
	strcat(movieDirectory, int2charRAO);
	QDir mdir = QDir(QString(movieDirectory));
	if (mdir.exists())
	{
		cout << "WARNING: Directory already exists. Files may be overwritten." << endl;
	}
	CreateDirectory(movieDirectory,0);


	curRunDir = movieDirectory;
	writer->SetInputConnection(luminanceFilter[liveIndex]->GetOutputPort());
	//writer->SetInputConnection(theFramegrabbers[liveIndex]->GetOutputPort(0));
	writer->SetGenerator(generator);
	generator->MultiFrameOn();
	writer->SetFileDimensionality(2);	
	countOverwrittenframes = 0;
}

void OverlayScene::startRecordBiplane(int run, int index)
{
	if (!isFramegrabber) return; // nothing to record, because stream is from file
	QDir qdir = QDir(QString(patientDir));
	if (!qdir.exists())
	{
		cout << "WARNING: Patientdirectory did not exist." << endl;
		CreateDirectory(patientDir, 0);

	}

	bool raoFront, raoLat;
	int laoraoFront, laoraoLat;
	//panelFront->update(...);??
	panelFront->getFirstAngle(raoFront, laoraoFront);
	panelLat->getFirstAngle(raoLat, laoraoLat);
	// directory for each time recordbutton ispressed
	//const char * N = "../Patientname/run"; //directory "../patientname" wir din setuppipeline erstellt
	char* N = patientDir;
	char* slash = "\\";
	char int2charRun[10];
	sprintf(int2charRun, "%d", run);
	//char* front = "_frontal";
	//char* lat = "_lateral";
	char int2charRAOfront[10];
	char int2charRAOlat[10];
	sprintf(int2charRAOfront, "%d", laoraoFront);
	sprintf(int2charRAOlat, "%d", laoraoLat);
	char* RAOboolfront;
	char* RAOboollat;
	(raoFront) ? RAOboolfront = "_RAO" : RAOboolfront = "_LAO";
	(raoLat) ? RAOboollat = "_RAO" : RAOboollat = "_LAO";
	char* ref;
	(index == 0) ? ref = "frontal" : ref = "lateral";
	// movieDirectory illustration: Patientname/run_frontalRAO30_lateralLAO90/... 
	char* movieDirectory = new char[strlen(N) + strlen(int2charRun)
		/*+ strlen(front)*/ + strlen(RAOboolfront) + strlen(int2charRAOfront)
		/*+ strlen(lat)*/ + strlen(RAOboollat) + strlen(int2charRAOlat)
		+ strlen(slash) + strlen(ref) + 1];
	//char* movieDirectory = new char[strlen(N) + strlen(ref) + strlen(slash) + strlen(int2charRun) + 1];
	strcpy(movieDirectory, N);
	strcat(movieDirectory, int2charRun);
	/*strcat(movieDirectory, front);*/
	strcat(movieDirectory, RAOboolfront);
	strcat(movieDirectory, int2charRAOfront);
	/*strcat(movieDirectory, lat);*/
	strcat(movieDirectory, RAOboollat);
	strcat(movieDirectory, int2charRAOlat);
	CreateDirectory(movieDirectory, 0); // es kann jeweils nur eine Ordnerebene mit CreateDirectory erstellt werden
	strcat(movieDirectory, slash);
	strcat(movieDirectory, ref);
	//strcat(movieDirectory, int2charIndex);
	QDir mdir = QDir(QString(movieDirectory));
	if (mdir.exists())
	{
		cout << "WARNING: Directory already exists. Files may be overwritten." << endl;
	}
	CreateDirectory(movieDirectory, 0);


	curRunDirBiplane[index] = movieDirectory;
	writerBiplane[index]->SetInputConnection(luminanceFilter[index]->GetOutputPort());
	//writerBiplane[index]->SetInputConnection(theFramegrabbers[index]->GetOutputPort(0));
	writerBiplane[index]->SetGenerator(generatorBiplane[index]);
	generatorBiplane[index]->MultiFrameOn();
	writerBiplane[index]->SetFileDimensionality(2);
	countOverwrittenframes = 0;
}

void OverlayScene::record(int value)
{

	// ---------meta-data fuer header----------------------------------
	vtkSmartPointer <vtkDICOMMetaData> meta =
		vtkSmartPointer <vtkDICOMMetaData>::New();

	int index = liveIndex;
	bool rao, kaud;
	int raoLao, kaudKran, lon, lat, vert, sid, fd;
	if (index == 0)
	{
		panelFront->getDisplayedValues(rao, kaud, raoLao, kaudKran, lon, lat, vert, sid, fd);
	}
	else
	{
		panelLat->getDisplayedValues(rao, kaud, raoLao, kaudKran, lon, lat, vert, sid, fd);
		panelFront->getTablePos(lon, lat, vert);
	}

	char primAngle[10], secAngle[10], tabTopVertPos[10], FDchar[10], latChar[10], longChar[10], SPD[10];
	sprintf(primAngle, "%d", raoLao);
	sprintf(secAngle, "%d", kaudKran);
	sprintf(tabTopVertPos, "%d", vert); // table position only frontal
	sprintf(FDchar, "%d", fd);
	sprintf(latChar, "%d", lat); // table position only frontal
	sprintf(longChar, "%d", lon); // table position only frontal
	sprintf(SPD, "%d", SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[index]);


	meta->SetAttributeValue(DC::DistanceSourceToDetector, sid);
	meta->SetAttributeValue(DC::StudyID, tabTopVertPos); // TableTopVerticalPosition
	meta->SetAttributeValue(DC::SeriesNumber, longChar); // TableTopLongitudinalPosition
	meta->SetAttributeValue(DC::AcquisitionNumber, latChar); //TableTopLateralPosition
	meta->SetAttributeValue(DC::PositionReferenceIndicator, FDchar); // FD
	meta->SetAttributeValue(DC::StudyDate, primAngle); //PositionerPrimaryAngle
	meta->SetAttributeValue(DC::StudyTime, secAngle); // PositionerSecondaryAngle
	meta->SetAttributeValue(DC::KVP, SPD); //Source Patient Distance
	
	writer->SetMetaData(meta);

	//writer->SetInputDataObject(theFramegrabbers->GetOutputDataObject(0));
	//-----------------------------------------------------------------
	//vtkDICOMTag tag;
	//tag = meta->ResolvePrivateTag()
	//	.ResolvePrivateTag(tag, creator)
	//	if (!data.HasCreator(tag, creator))
	//		data.AddCreator(tag, creator))
	//		data.Set(tag, value)
	
	
	// save each frame as .dcm-image
	//writer->SetFilePrefix("../movies/");

	char int2char[10];
	sprintf(int2char, "%d", value-countOverwrittenframes);
	const char * H = ".dcm";
	char * N = curRunDir;
	char * slash = "\\";
	char * bufferNew = new char[strlen(N) + strlen(slash) + strlen(H) + strlen(int2char) + 1];
	strcpy(bufferNew, N);
	strcat(bufferNew, slash);
	strcat(bufferNew, int2char);
	strcat(bufferNew, H);
	//writer->SetFilePattern(bufferNew);
	writer->SetFileName(bufferNew);


	if (mustWrite[index]) {
		//int start1 = startClock("write.txt");
			writer->Write();
			//writer->Update();
			//writer->Write();
		//stopClock(start1);
			countOverwrittenframes += 1;
			mustWrite[index] = false;
	}
	else {
		//int start2 = startClock("updateWriter.txt");
			writer->Update();
		//stopClock(start2);
	}

}



void OverlayScene::recordBiplane(int value, int index, int currentRun)
{
	// get displayed data
	bool raoFront, kaudFront, raoLat, kaudLat;
	int raoLaoFront, kaudKranFront, lonFront, latFront, vertFront, sidFront, fdFront;
	int raoLaoLat, kaudKranLat, lonLat, latLat, vertLat, sidLat, fdLat;

	panelFront->getDisplayedValues(raoFront, kaudFront, raoLaoFront, kaudKranFront, 
									lonFront, latFront, vertFront, sidFront, fdFront);
	panelLat->getDisplayedValues(raoLat, kaudLat, raoLaoLat, kaudKranLat, 
									lonLat, latLat, vertLat, sidLat, fdLat);

	// ---------meta-data fuer header----------------------------------
	vtkSmartPointer <vtkDICOMMetaData> meta0 =
		vtkSmartPointer <vtkDICOMMetaData>::New();
	vtkSmartPointer <vtkDICOMMetaData> meta1 =
		vtkSmartPointer <vtkDICOMMetaData>::New();

	char primAngle[10], secAngle[10], tabTopVertPos[10], FDchar[10], latChar[10], longChar[10], SPD[10];
	char primAngle1[10], secAngle1[10], tabTopVertPos1[10], FDchar1[10], latChar1[10], longChar1[10], SPD1[10];
	// convertion
	sprintf(primAngle, "%d", (int) raoLaoFront);
	sprintf(secAngle, "%d", (int) kaudKranFront);
	sprintf(tabTopVertPos, "%d", vertFront);
	sprintf(FDchar, "%d", fdFront);
	sprintf(latChar, "%d", latFront);
	sprintf(longChar, "%d", lonFront);
	sprintf(SPD, "%d", SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[0]);
	
	sprintf(primAngle1, "%d", (int) raoLaoLat);
	sprintf(secAngle1, "%d", (int) kaudKranLat);
	//sprintf(tabTopVertPos1, "%d", HoeheLive[0]); // table position only frontal
	sprintf(FDchar1, "%d", fdLat);
	//sprintf(latChar1, "%d", LatLive[0]);// table position only frontal
	//sprintf(longChar1, "%d", longLive[0]); // table position only frontal
	sprintf(SPD1, "%d", SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[1]);

	// allocate to tags, WARNING: tag name does not match tag values
	meta0->SetAttributeValue(DC::DistanceSourceToDetector, sidFront);
	meta0->SetAttributeValue(DC::StudyID, tabTopVertPos); // TableTopVerticalPosition
	meta0->SetAttributeValue(DC::SeriesNumber, longChar); // TableTopLongitudinalPosition
	meta0->SetAttributeValue(DC::AcquisitionNumber, latChar); //TableTopLateralPosition
	meta0->SetAttributeValue(DC::PositionReferenceIndicator, FDchar); // FD
	meta0->SetAttributeValue(DC::StudyDate, primAngle); //PositionerPrimaryAngle
	meta0->SetAttributeValue(DC::StudyTime, secAngle); // PositionerSecondaryAngle
	meta0->SetAttributeValue(DC::KVP, SPD); //Source Patient Distance

	meta1->SetAttributeValue(DC::DistanceSourceToDetector, sidLat);
	meta1->SetAttributeValue(DC::StudyID, tabTopVertPos); // TableTopVerticalPosition
	meta1->SetAttributeValue(DC::SeriesNumber, longChar); // TableTopLongitudinalPosition
	meta1->SetAttributeValue(DC::AcquisitionNumber, latChar); //TableTopLateralPosition
	meta1->SetAttributeValue(DC::PositionReferenceIndicator, FDchar1); // FD
	meta1->SetAttributeValue(DC::StudyDate, primAngle1); //PositionerPrimaryAngle
	meta1->SetAttributeValue(DC::StudyTime, secAngle1); // PositionerSecondaryAngle
	meta1->SetAttributeValue(DC::KVP, SPD1); //Source Patient Distance


	writerBiplane[0]->SetMetaData(meta0);
	writerBiplane[1]->SetMetaData(meta1);

	// grab, connectpipeline, render are not necessary, record is pressed the same time than live 

	// save each frame as .dcm-image
	//writer->SetFilePrefix("../movies/");

	// patientdir\\index\\run\\value.dcm
	char * N = curRunDirBiplane[0];
	char int2charValue[10];
	sprintf(int2charValue, "%d", value-countOverwrittenframes);
	const char * H = ".dcm";
	char * slash = "\\";
	char * bufferNew = new char[strlen(N) + strlen(slash) + strlen(int2charValue) + strlen(H) + 1];
	strcpy(bufferNew, N);
	strcat(bufferNew, slash);
	strcat(bufferNew, int2charValue);
	strcat(bufferNew, H);

	char * N1 = curRunDirBiplane[1];
	char * bufferNew1 = new char[strlen(N1) + strlen(slash) + strlen(int2charValue) + strlen(H) + 1];
	strcpy(bufferNew1, N1);
	strcat(bufferNew1, slash);
	strcat(bufferNew1, int2charValue);
	strcat(bufferNew1, H);

	writerBiplane[0]->SetFileName(bufferNew);
	writerBiplane[1]->SetFileName(bufferNew1);

	if (mustWrite[0] || mustWrite[1]) {

		writerBiplane[0]->Write(); 
		writerBiplane[1]->Write();
		countOverwrittenframes += 1;
		mustWrite[0] = mustWrite[1] = false;
	}
	else {
		// update image data
		writerBiplane[0]->Update();
		writerBiplane[1]->Update();
	}

}

void OverlayScene::stopRecord() 
{
	if (!isFramegrabber) {
		return; // playing of files is managed from outside this class
	}

	else {	
		for (int i = 0; i < frameGrabbersNumber; i++)
		{
			theFramegrabbers[i]->Stop();
		}

	}
}


void OverlayScene::play(int framenumber, char* dir)
{
	if (!isFramegrabber) return;

	// create datapath
	char int2char[10];
	sprintf(int2char, "%d", framenumber);
	char * H = ".dcm";
	char * N = "\\"; //	char * N = "/";
	char * bufferNew = new char[strlen(dir) + strlen(N) + strlen(H) + strlen(int2char) + 1];
	strcpy(bufferNew, dir);
	strcat(bufferNew, N);
	strcat(bufferNew, int2char);
	strcat(bufferNew, H);

	QDir qdir = QDir(QString(dir));
	if (!qdir.exists())
	{
		cout << "ERROR: Nothing to play." << endl;
		return;
	}

	// conntect to Pipeline
	DICOMreader->setInputFile(bufferNew);
	DICOMreader->Update();
	
	// read Geometry:
	DICOMreader->ReadGeometryFramegrabber(primAnglePlayNew, secAnglePlayNew, longPlayNew, latPlayNew, HoehePlayNew, SIDPlayNew, mmPerPxlPlayNew, FDPlayNew, SODPlayNew);
	
	// Anpassung nur, falls sich die Geometrie geaendert hat
	if (primAnglePlayNew != primAnglePlay || secAnglePlayNew != secAnglePlay || longPlayNew != longPlay || latPlayNew != latPlay || HoehePlayNew != HoehePlay|| SIDPlayNew != SIDPlay|| mmPerPxlPlayNew != mmPerPxlPlay || FDPlayNew != FDPlay || SODPlayNew != SODPlay) {
		setNewGeometryPlay(primAnglePlayNew, secAnglePlayNew, longPlayNew, latPlayNew, HoehePlayNew, SIDPlayNew, mmPerPxlPlayNew, FDPlayNew, SODPlayNew);
	
		setFramegrabberGeometry(0,SIDPlay, primAnglePlay, secAnglePlay, latPlay, longPlay, HoehePlay, mmPerPxlPlay, 1);
	}
	
	////==============================test-getGeometry===================//
	////{ LAORAO, KAUDKRAN, Long, Lat, Hoehe, SID, FD, RAO, KAUD} 
	//std::vector<int> Geometry = getGeometryFramegrabber();
	////______________________________END______________________________//

	//================ this requires the geometry load==============//
	theMotionCorrections->SetInputConnection(DICOMreader->GetOutputPort());
	//theMotionCorrections->Update();
	actorMainXRAY->GetMapper()->SetInputConnection(theMotionCorrections->GetOutputPort());
	//================ otherwise ==============//
	//actorMainXRAY->GetMapper()->SetInputConnection(DICOMreader->GetOutputPort());
	//renWinMainXRAY->Render();
	alreadyConstructedPipeline[2] = false;
}

void OverlayScene::setNewGeometryPlay(double& primAnglePlayNew, double& secAnglePlayNew, int& longPlayNew, int& latPlayNew, int& HoehePlayNew, int& SIDPlayNew, double& mmPerPxlPlayNew, int& FDPlayNew, int& SODPlayNew) {
	primAnglePlay = primAnglePlayNew;
	secAnglePlay = secAnglePlayNew;
	longPlay = longPlayNew;
	latPlay = latPlayNew;
	HoehePlay = HoehePlayNew;
	SIDPlay = SIDPlayNew;
	mmPerPxlPlay = mmPerPxlPlayNew;
	FDPlay = FDPlayNew;
	SODPlay = SODPlayNew;
}

void OverlayScene::playReferenceStream(int framenumber, char* dir, int index)
{
	// Pipeline
	if (!alreadyConstructedPipeline[index]) {
		visualizeActors(index, true);
		theXRAYActors[index]->GetMapper()->SetInputConnection(theXRAYReaders[index]->GetOutputPort());
		theXRAYActors3D[index]->GetMapper()->SetInputConnection(theXRAYReaders[index]->GetOutputPort());
		if(biplaneSystem) setFilterToType("Crosscorrelation Biplane Filter");
		alreadyConstructedPipeline[index] = true;

		// default mesh 
		if (alreadyConstructedPipeline[0] + alreadyConstructedPipeline[1] == 1 && loadDefaultMeshPositionBool)
		{
			if (orientation == 0)	loadDefaultMeshPosition(MR_PHILIPS);
			if (orientation == 1)	loadDefaultMeshPosition(MR_ITK);	// correct orientation AP is needed for correct calculation and display of the angles
			if (orientation == 2)	loadDefaultMeshPosition(CT_PHILIPS);	// correct orientation AP is needed for correct calculation and display of the angles
			if (orientation == 3)	loadDefaultMeshPosition(CT_ITK);	// correct orientation AP is needed for correct calculation and display of the angles				

			/*theXRayViewer->stackClear();

			double previousMeshMatrix[4][4];
			int previousMeshWindow;
			previousMeshWindow = 2;
			this->getUserMatrix(previousMeshWindow, previousMeshMatrix);
			MoveMeshInteractionCommand* cmd = new MoveMeshInteractionCommand(this, previousMeshWindow, previousMeshMatrix, previousMeshWindow, previousMeshMatrix);

			cmd->setText("default position");
			theXRayViewer->generateUndoRedoCommandStack(cmd, previousMeshWindow, previousMeshMatrix);*/

		}
	}

	// create datapath
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

	QFileInfo qfile  = QString(bufferNew);
	if (!qfile.exists() || !qfile.isFile())
	{
		cout << "ERROR: Nothing to play." << endl;
		return;
	}

	theXRAYReaders[index]->setInputFile(bufferNew);
	theXRAYReaders[index]->Update();
	
	// 	return{ primAngleRead, secAngleRead, longRead, latRead, HoeheRead, SIDRead, mmPerPxlRead };
	theXRAYReaders[index]->ReadGeometryFramegrabber(primAngleRefNew[index], secAngleRefNew[index], longRefNew[index], latRefNew[index], HoeheRefNew[index], SIDRefNew[index], mmPerPxlRefNew[index], FDRefNew[index], SODRefNew[index]);
	if (SODRefNew[index] == SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[1])
	{
		HoeheRefNew[index] += SystemGeometryDefinitions::ISOCENTER_SHIFT_Z;
	}

	if (primAngleRef[index]!=primAngleRefNew[index] || secAngleRef[index] != secAngleRefNew[index] || longRef[index] != longRefNew[index] || latRef[index] != latRefNew[index] || HoeheRef[index] != HoeheRefNew[index] || SIDRef[index] != SIDRefNew[index] || mmPerPxlRef[index] != mmPerPxlRefNew[index] || FDRef[index] != FDRefNew[index] || SODRef[index] != SODRefNew[index]) {
		setNewGeometryRef(index, primAngleRefNew[index], secAngleRefNew[index], longRefNew[index], latRefNew[index], HoeheRefNew[index], SIDRefNew[index], mmPerPxlRefNew[index], FDRefNew[index], SODRefNew[index]);

		setGeometryFromFramegrabber(index, SIDRef[index], primAngleRef[index], secAngleRef[index], latRef[index], longRef[index], HoeheRef[index], mmPerPxlRef[index]);
	}

	
	/*renWinXRAY[index]->Render();*/
	renWin3D->Render();
	
}

void OverlayScene::setNewGeometryRef(int index, double& primAngleRead, double& secAngleRead, int& longRead, int& latRead, int& HoeheRead, int& SIDRead, double& mmPerPxlRead, int& FDRead, int& SODRead)
{
	primAngleRef[index] = primAngleRead;
	secAngleRef[index] = secAngleRead;
	longRef[index] = longRead;
	latRef[index] = latRead;
	HoeheRef[index] = HoeheRead;
	SIDRef[index] = SIDRead;
	mmPerPxlRef[index] = mmPerPxlRead;
	FDRef[index] = FDRead;
	SODRef[index] = SODRead;
}

vector<string> OverlayScene::getMonoplaneFilterTypesList()
{
	return FilterAlgorithmFactory::getFilterTypes();
}

string OverlayScene::getCurrentFilterType() {
	if (activeFilterIsBiplane) return BiplaneFilterAlgorithmFactory::getFilterTypeLastCreated();
	else return FilterAlgorithmFactory::getFilterTypeLastCreated();
}

vector<string> OverlayScene::getFilterTypesList()
{
	vector<string> result = FilterAlgorithmFactory::getFilterTypes();
	vector<string> biplane = BiplaneFilterAlgorithmFactory::getFilterTypes();

	result.insert(result.end(), biplane.begin(), biplane.end());

	return result;
}

//string OverlayScene::getCurrentFilterType()
//{
//	return FilterAlgorithmFactory::getFilterTypeLastCreated();
//}

void OverlayScene::setFilterToType(string type)
{
	if (BiplaneFilterAlgorithmFactory::hasFilterType(type)) {
		setBiplaneFilterToType(type);

	}
	else {
		setMonoplaneFilterToType(type);
		
	}

	setMotionCorrectionScalingAccordingToGeometry();

}

void OverlayScene::setMotionCorrectionScalingAccordingToGeometry()
{
	double SD = theBiplaneGeometry.mainSystem.getSourceDetectorDistance();
	double SM = theBiplaneGeometry.mainSystem.getIsoCenterToSourceDistance();
	double f = SM / SD;
	theMotionCorrections->SetScaleX(f);
	theMotionCorrections->SetScaleY(f);
	
}

void OverlayScene::setMonoplaneFilterToType(string type)
{
	// first, disconnect filter from pipeline, and then delete them
		theMotionCorrections->SetInput(NULL);		
		theMotionCorrections->Delete();


	// now create new filters of the requested type and connect them

		theMotionCorrections = FilterAlgorithmFactory::createAlgorithm(type);
		theMotionCorrections->addAssemblyToMove(theMeshAssemblies[3]); //<------
		if (isFramegrabber)
		{
			if (isLive) {
				theMotionCorrections->SetInputConnection(luminanceFilter[liveIndex]->GetOutputPort());
			}
			else {
				theMotionCorrections->SetInputConnection(DICOMreader->GetOutputPort());
			}
		}
		else
		{
			theMotionCorrections->SetInputConnection(resliceMainXRAY->GetOutputPort(0));
		}
		

	actorMainXRAY->GetMapper()->SetInputConnection(theMotionCorrections->GetOutputPort(0)); // this is correct connection of the pipeline

	theMotionCorrections->SetImageToWorldCoordinatesScaling(theBiplaneGeometry.mainSystem.getPixelToMillimeterScaling());
	theMotionCorrections->setGeometry(&theBiplaneGeometry.mainSystem);
	theMotionCorrections->setCurrentAssemblyPositionAsNullPositions();

	// disconnect the biplane part of the pipeline to save performance
	theBiplaneFilter->SetInputImage1(NULL);
	theBiplaneFilter->SetInputImage2(NULL);

	activeFilterIsBiplane = false;

}

void OverlayScene::setBiplaneFilterToType(string type)
{
	// create new filter
	BiplaneAlgorithm* newfilter = BiplaneFilterAlgorithmFactory::createAlgorithm(type);

	if (isFramegrabber)
	{
		if (isLive && biplaneSystem) {
			newfilter->SetInputImage1(luminanceFilter[0]->GetOutput());
			newfilter->SetInputImage2(luminanceFilter[1]->GetOutput());

			newfilter->setVolumeVisualizerToUpdate(theMRVisualizer);

			for (unsigned int i = 0; i < theXRAYActors.size(); ++i)
			{
				//theXRAYActors[i]->SetInputData(newfilter->GetOutputImage(i));	// is wrong, the images are not displayed
				theXRAYActors[i]->GetMapper()->SetInputConnection(newfilter->GetOutputPort(i)); // this is correct connection of the pipeline
			}

			// delete and swap
			theBiplaneFilter->Delete();
			theBiplaneFilter = newfilter;

			theBiplaneFilter->setBiplaneGeometry(&theBiplaneGeometry);

			theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[0], true);
			theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[1], false);
			theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[0], true);
			theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[1], false);
			//theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true);
			theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true, true);

			theBiplaneFilter->setCurrentAssemblyPositionAsNullPositions();	
			theMotionCorrections->SetInput(NULL);

			activeFilterIsBiplane = true;

		}
		else if(alreadyConstructedPipeline[0]== true && alreadyConstructedPipeline[1] == true){
			newfilter->SetInputImage1(theXRAYReaders[0]->GetOutput());
			newfilter->SetInputImage2(theXRAYReaders[1]->GetOutput());

			newfilter->setVolumeVisualizerToUpdate(theMRVisualizer);

			for (unsigned int i = 0; i < theXRAYActors.size(); ++i)
			{
				theXRAYActors[i]->GetMapper()->SetInputConnection(newfilter->GetOutputPort(i)); // this is correct connection of the pipeline
			}

			// delete and swap
			theBiplaneFilter->Delete();
			theBiplaneFilter = newfilter;

			theBiplaneFilter->setBiplaneGeometry(&theBiplaneGeometry);

			theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[0], true);
			theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[1], false);
			theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[0], true);
			theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[1], false);
			//theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true);
			theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true, true);

			theBiplaneFilter->setCurrentAssemblyPositionAsNullPositions();	
			theMotionCorrections->SetInput(NULL);

			activeFilterIsBiplane = true;
		}
	}
	else
	{
		// re-connect to new filter
		newfilter->SetInputImage1(theXRAYReslicers[0]->GetOutput());
		newfilter->SetInputImage2(theXRAYReslicers[1]->GetOutput());

		newfilter->setVolumeVisualizerToUpdate(theMRVisualizer);

		for (unsigned int i = 0; i < theXRAYActors.size(); ++i)
		{
			theXRAYActors[i]->GetMapper()->SetInputConnection(newfilter->GetOutputPort(i)); // this is correct connection of the pipeline
		}

		// delete and swap
		theBiplaneFilter->Delete();
		theBiplaneFilter = newfilter;

		theBiplaneFilter->setBiplaneGeometry(&theBiplaneGeometry);

		theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[0], true);
		theBiplaneFilter->addAssemblyToMove(theMeshAssemblies[1], false);
		theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[0], true);
		theBiplaneFilter->addAssemblyToMove(theMarkerPointAssemblies[1], false);
		//theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true);
		theBiplaneFilter->addAssemblyToMove(theTrackingPointAssembly, true, true);

		theBiplaneFilter->setCurrentAssemblyPositionAsNullPositions();
		theMotionCorrections->SetInput(NULL);

		activeFilterIsBiplane = true;

	}
	activeFilterIsBiplane = true;


}

void OverlayScene::reconnectTrackingTransformations(vector<vtkTransform*> newTransforms)
{
	// delete old pipeline
	for (unsigned int i = 0; i < theTrackingPointTransformers.size(); ++i) {
		theTrackingPointAssembly->RemovePart(theTrackingPointActors[i]);
		theTrackingPointActors[i]->Delete();
		theTrackingPointMappers[i]->Delete();
		theTrackingPointTransformers[i]->Delete();
		theTrackingPointSources[i]->Delete();
	}
	

	theTrackingPointActors.clear();
	theTrackingPointMappers.clear();
	theTrackingPointTransformers.clear();
	theTrackingPointSources.clear();

	// create new pipeline
	for (unsigned int i = 0; i < newTransforms.size(); ++i) {
		vtkSphereSource* source = vtkSphereSource::New();
		theTrackingPointSources.push_back(source);
		source->SetRadius(2);

		vtkTransformFilter* trans = vtkTransformFilter::New();
		theTrackingPointTransformers.push_back(trans);
		trans->SetInputConnection(source->GetOutputPort());
		trans->SetTransform(newTransforms[i]);

		vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
		theTrackingPointMappers.push_back(mapper);
		mapper->SetInputConnection(trans->GetOutputPort());

		vtkActor* actor = vtkActor::New();
		theTrackingPointActors.push_back(actor);
		actor->SetMapper(mapper);
		actor->GetProperty()->SetColor(0.0, 0.0, 1.0);
		actor->GetProperty()->SetOpacity(0.7);

		theTrackingPointAssembly->AddPart(actor);
		theTrackingPointAssembly->Modified();
		renWin3D->Render();
	}

}

fstream f1;
void OverlayScene::addTemplate(unsigned int streamNumber, double worldCoords[2], int pointType)
{
	double imageCoords[2];
	transformFromWorldToImageCoordinates(streamNumber, worldCoords, imageCoords);

	////save selected coordinates:
	//f1.open("coordinates.txt", ios::out | ios::app);
	//f1 << "x: " << imageCoords[0] << " ; y: " << imageCoords[1] << endl;
	//f1.close();

	if (activeFilterIsBiplane) {
		if (streamNumber == 2)
		{
			cout << "Please select monoplane Filter!" << endl;
			return;

		}
		else
		{
			PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theBiplaneFilter);
			if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing

			filter->addPoint(imageCoords, pointType, streamNumber); // theBiplaneFilter must know on which stream the point was set

			vector<vtkTransform*> transforms = theBiplaneFilter->setNumberOfTransformations(filter->getPointList().size());
			reconnectTrackingTransformations(transforms);
		}


	}
	else {
		if (streamNumber != 2)
		{
			cout << "Please select biplane Filter!" << endl;
			return;

		}
		else {
			// select the monoplane filter of the main stream
			PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theMotionCorrections);
			if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing

			filter->addPoint(imageCoords, pointType);

		}

	}

}

void OverlayScene::unsetNearestTemplate(unsigned int streamNumber, double worldCoords[2])
{
	double imageCoords[2];
	transformFromWorldToImageCoordinates(streamNumber, worldCoords, imageCoords);

	if (activeFilterIsBiplane) {
		PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theBiplaneFilter);
		if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing

							 // theBiplaneFilter must know on which stream the point was set
		int index = filter->indexOfNearestPoint(imageCoords, streamNumber);
		if (index >= 0) filter->removePoint(index, streamNumber);

		reconnectTrackingTransformations(theBiplaneFilter->getTransforms());
		removeEpipolarLinesIn3dWindow();
	}
	else {
		// select the monoplane filter of the main stream
		PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theMotionCorrections);
		if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing

		int index = filter->indexOfNearestPoint(imageCoords);
		if (index >= 0) filter->removePoint(index);

	}
}

void OverlayScene::unsetAllTemplates(unsigned int streamNumber)
{
	if (activeFilterIsBiplane) {
		PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theBiplaneFilter);
		if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing



		filter->removeAllPoints(streamNumber);

		reconnectTrackingTransformations(theBiplaneFilter->getTransforms());
		removeEpipolarLinesIn3dWindow();
	}
	else {
		// select the monoplane filter of the correct stream
		PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theMotionCorrections);
		if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing

		filter->removeAllPoints();

	}
}

void OverlayScene::setTemplatePositionForMotionCompensation(unsigned int streamNumber, double worldCoords[2]) {
	if (activeFilterIsBiplane) {
		// select the monoplane filter of the correct stream
		SimpleBiplaneFilter* filter = dynamic_cast<SimpleBiplaneFilter*>(theBiplaneFilter);
		if (!filter) {
			cout << "Manual motion compensation not implemented for this filter type!" << endl;
			return;
		}

		double imageCoords[2];
		transformFromWorldToImageCoordinates(streamNumber, worldCoords, imageCoords);
		filter->overwriteMotionCorrectionMatchPosition(streamNumber, imageCoords);
		filter->overwritePoint(imageCoords, 1, streamNumber);
		filter->UpdateWholeExtent();

	}
	else {
		// select the monoplane filter of the correct stream
		CrosscorrelationMotionCorrectionFilter* filter = dynamic_cast<CrosscorrelationMotionCorrectionFilter*>(theMotionCorrections);
		if (!filter) {
			cout << "Manual motion compensation not implemented for this filter type!" << endl;
			return;
		}

		double imageCoords[2];
		transformFromWorldToImageCoordinates(streamNumber, worldCoords, imageCoords);
		
		filter->overwriteMotionCorrectionMatchPosition(imageCoords);
		filter->overwritePoint(imageCoords, 1);

		filter->UpdateWholeExtent();
		
	}
}

void OverlayScene::setTrackingPointIndexForMRVolume(unsigned int streamNumber, double worldCoords[2])
{
	double imageCoords[2];
	transformFromWorldToImageCoordinates(streamNumber, worldCoords, imageCoords);

	if (activeFilterIsBiplane) {
		PointListBasedFilter* filter = dynamic_cast<PointListBasedFilter*>(theBiplaneFilter);
		if (!filter) return; // the current filter is not derived from PointListBasedFilter --> do nothing

		int index = filter->indexOfNearestPoint(imageCoords, streamNumber);
		if (index < 0) return; // no template set
		theBiplaneFilter->setTrackingPointIndexToUseForVolumeVisualizer(index);
	}

}

bool OverlayScene::isActiveFilterBiplane()
{
	return activeFilterIsBiplane;
}

vector<AbstractFilterWidget*> OverlayScene::getBiplaneAlgorithmPropertiesGUI()
{
	return theBiplaneFilter->getPropertiesGUI();
}

vector<AbstractFilterWidget*> OverlayScene::getMonoplaneAlgorithmPropertiesGUI()
{
	return theMotionCorrections->getPropertiesGUI();
}

void OverlayScene::addEpipolarLinesIn3dWindow(double p1_1[2], double p1_2[2], double p2_1[2], double p2_2[2], double bestPoint1[2], double bestPoint2[2])
{
	double source1[3], source2[3];
	double p1_1Line[3], p2_1Line[3];
	double p1_2Line[3], p2_2Line[3];

	//first Epipolarline:
	epiline1_1 = vtkLineSource::New();
	epilineMapper1_1 = vtkPolyDataMapper::New();
	epilineActor1_1 = vtkActor::New();
	epiline1_2 = vtkLineSource::New();
	epilineMapper1_2 = vtkPolyDataMapper::New();
	epilineActor1_2 = vtkActor::New();

	if (bestPoint1[0] == p1_1[0] && bestPoint1[1] == p1_1[1])
	{
		p1_1Line[0] = p1_1[0];
		p1_1Line[1] = p1_1[1];
		p1_2Line[0] = p1_2[0];
		p1_2Line[1] = p1_2[1];
	}
	else
	{
		p1_1Line[0] = p1_2[0];
		p1_1Line[1] = p1_2[1];
		p1_2Line[0] = p1_1[0];
		p1_2Line[1] = p1_1[1];
	}
	theBiplaneGeometry.firstSystem.transformFromImageToPatientCoordinates(p1_1Line);
	theBiplaneGeometry.firstSystem.transformFromImageToPatientCoordinates(p1_2Line);

	theBiplaneGeometry.firstSystem.getSourcePosition(source1[0], source1[1], source1[2]);
	epiline1_1->SetPoint1(source1);
	epiline1_1->SetPoint2(p1_1Line);
	epiline1_2->SetPoint1(source1);
	epiline1_2->SetPoint2(p1_2Line);

	epilineMapper1_1->SetInputConnection(epiline1_1->GetOutputPort());
	epilineActor1_1->SetPickable(false);
	epilineActor1_1->SetMapper(epilineMapper1_1);
	epilineActor1_1->GetProperty()->SetColor(255, 0, 0);
	renderer3D->AddActor(epilineActor1_1);

	epilineMapper1_2->SetInputConnection(epiline1_2->GetOutputPort());
	epilineActor1_2->SetPickable(false);
	epilineActor1_2->SetMapper(epilineMapper1_2);
	epilineActor1_2->GetProperty()->SetColor(0, 0, 255);
	renderer3D->AddActor(epilineActor1_2);


	//second Epipolarline:
	epiline2_1 = vtkLineSource::New();
	epilineMapper2_1 = vtkPolyDataMapper::New();
	epilineActor2_1 = vtkActor::New();
	epiline2_2 = vtkLineSource::New();
	epilineMapper2_2 = vtkPolyDataMapper::New();
	epilineActor2_2 = vtkActor::New();

	if (bestPoint2[0] == p2_1[0] && bestPoint2[1] == p2_1[1])
	{
		p2_1Line[0] = p2_1[0];
		p2_1Line[1] = p2_1[1];
		p2_2Line[0] = p2_2[0];
		p2_2Line[1] = p2_2[1];
	}
	else
	{
		p2_1Line[0] = p2_2[0];
		p2_1Line[1] = p2_2[1];
		p2_2Line[0] = p2_1[0];
		p2_2Line[1] = p2_1[1];
	}

	theBiplaneGeometry.secondSystem.transformFromImageToPatientCoordinates(p2_1Line);
	theBiplaneGeometry.secondSystem.transformFromImageToPatientCoordinates(p2_2Line);

	theBiplaneGeometry.secondSystem.getSourcePosition(source2[0], source2[1], source2[2]);
	epiline2_1->SetPoint1(source2);
	epiline2_1->SetPoint2(p2_1Line);
	epiline2_2->SetPoint1(source2);
	epiline2_2->SetPoint2(p2_2Line);

	epilineMapper2_1->SetInputConnection(epiline2_1->GetOutputPort());
	epilineActor2_1->SetPickable(false);
	epilineActor2_1->SetMapper(epilineMapper2_1);
	epilineActor2_1->GetProperty()->SetColor(255, 0, 0);
	renderer3D->AddActor(epilineActor2_1);

	epilineMapper2_2->SetInputConnection(epiline2_2->GetOutputPort());
	epilineActor2_2->SetPickable(false);
	epilineActor2_2->SetMapper(epilineMapper2_2);
	epilineActor2_2->GetProperty()->SetColor(0, 0, 255);
	renderer3D->AddActor(epilineActor2_2);
}

void OverlayScene::removeEpipolarLinesIn3dWindow()
{
	if (epilineActor1_1 != NULL)
	{
		renderer3D->RemoveActor(epilineActor1_1);
		epilineActor1_1->Delete();
		epilineMapper1_1->Delete();
		epiline1_1->Delete();
		epiline1_1 = 0;
		epilineMapper1_1 = 0;
		epilineActor1_1 = 0;
		renderer3D->RemoveActor(epilineActor2_1);
		epilineActor2_1->Delete();
		epilineMapper2_1->Delete();
		epiline2_1->Delete();
		epiline2_1 = 0;
		epilineMapper2_1 = 0;
		epilineActor2_1 = 0;

		renderer3D->RemoveActor(epilineActor1_2);
		epilineActor1_2->Delete();
		epilineMapper1_2->Delete();
		epiline1_2->Delete();
		epiline1_2 = 0;
		epilineMapper1_2 = 0;
		epilineActor1_2 = 0;
		renderer3D->RemoveActor(epilineActor2_2);
		epilineActor2_2->Delete();
		epilineMapper2_2->Delete();
		epiline2_2->Delete();
		epiline2_2 = 0;
		epilineMapper2_2 = 0;
		epilineActor2_2 = 0;

	}
	renWin3D->Render();


}

void OverlayScene::transformFromWorldToImageCoordinates(unsigned int streamNumber, const double worldCoords[2], double imageCoords[2])
{
	bool ok[1];
	double mmPerPixel, bounds[6];

	if (streamNumber == 0 || streamNumber == 1)
	{
		if (streamNumber >= theXRAYActors.size()) return;

		if (isFramegrabber)
		{
			if (isLive && biplaneSystem) {
				if (streamNumber == 0) {
					XRayGeometry* geometry = &theBiplaneGeometry.firstSystem;
					mmPerPixel = geometry->getPixelToMillimeterScaling();					
				}
				else {
					XRayGeometry* geometry = &theBiplaneGeometry.secondSystem;
					mmPerPixel = geometry->getPixelToMillimeterScaling();
				}

				//theFramegrabbers[streamNumber]->GetOutput()->GetBounds(bounds);
				luminanceFilter[streamNumber]->GetOutput()->GetBounds(bounds);

			}
			else {
				theXRAYReaders[streamNumber]->ReadGeometryFramegrabbermmPerPxl(mmPerPixel);
				theXRAYReaders[streamNumber]->GetOutput()->GetBounds(bounds);
				
			}
		}
		else
		{
			mmPerPixel = theXRAYReaders[streamNumber]->GetMillimeterPerPixelScaling(&ok[0]);
			theXRAYReaders[streamNumber]->GetOutput()->GetBounds(bounds);		
			
		}	


		// translation
		double* imgPos = theXRAYActors[streamNumber]->GetPosition();
		imageCoords[0] = worldCoords[0] - imgPos[0] - bounds[0];
		imageCoords[1] = worldCoords[1] - imgPos[1] - bounds[2];

		
		// scaling
		//imageCoords[0] /= theXRAYActors[streamNumber]->GetScale()[0];
		//imageCoords[1] /= theXRAYActors[streamNumber]->GetScale()[0];
		imageCoords[0] /= mmPerPixel;
		imageCoords[1] /= mmPerPixel;
	}

	else
	{
		if (isFramegrabber)
		{

			XRayGeometry* geometry = &theBiplaneGeometry.mainSystem;
			mmPerPixel = geometry->getPixelToMillimeterScaling();
			bounds[0] = bounds[2] = 0;
	

		}
		else
		{ 
			readerMainXRAY->GetOutput()->GetBounds(bounds);
			mmPerPixel = readerMainXRAY->GetMillimeterPerPixelScaling(&ok[0]);

			
		}

		// translation
		double* imgPos = actorMainXRAY->GetPosition();
		imageCoords[0] = worldCoords[0] - imgPos[0] - bounds[0];
		imageCoords[1] = worldCoords[1] - imgPos[1] - bounds[2];

		// scaling
		imageCoords[0] /= mmPerPixel;
		imageCoords[1] /= mmPerPixel;
		

	}	

}

void OverlayScene::setBiplaneGeometry(unsigned int streamNumber, int SID, int sourcePatDist, double primAngle, double secAngle,
	int lateralPos, int longitudinalPos, int verticalPos, double mmPerPxl, int imageSizeX, int imageSizeY)
{
	//printf("setting geometry with first SID: %.0f and second SID: %.0f\n", SID[0], SID[1]);
	/* compute geometry */
	double angles[2] = { primAngle, secAngle };
	XRayGeometry* geometry;

	if (streamNumber == 0)
	{
		geometry = &theBiplaneGeometry.firstSystem;
	}
	else if(streamNumber == 1)
	{
		geometry = &theBiplaneGeometry.secondSystem;
	}

	geometry->setImageDimension(
		imageSizeX, 
		imageSizeY
		);
	geometry->setIsFramegrabber(isFramegrabber);
	geometry->setTablePosition((double)lateralPos, (double)longitudinalPos, (double)verticalPos);
	geometry->setParameters(angles, SID, sourcePatDist);
	geometry->setMillimeterToPixelScaling(mmPerPxl);
	if (alreadyConstructedPipelineDICOM[0] + alreadyConstructedPipelineDICOM[1] == 1 && loadDefaultMeshPositionBool)
	{
		SystemGeometryDefinitions::TABLE_POSITION_X = lateralPos; // lat.
		SystemGeometryDefinitions::TABLE_POSITION_Y = longitudinalPos; // long.
		SystemGeometryDefinitions::TABLE_POSITION_Z = verticalPos; //vert. = h�he

	}
		
	setDICOMAnglesToWindowRef(streamNumber, primAngle, secAngle);


	const char* filename1;
	const char* filename2;
	filename1 = theXRAYReaders[0]->getInputFile();
	filename2 = theXRAYReaders[1]->getInputFile();
	if (filename1 != NULL && filename1[0] != '\0' && filename1 != NULL && filename2[0] != '\0')
	{
		theBiplaneGeometry.computeParameters();
		setupBiplaneGeometry();
	}
	else
	{
		setupBiplaneGeometry(streamNumber);
		updateMeshPositionsFromWindow(2);
	}

}


void OverlayScene::setGeometryFromFramegrabber(unsigned int streamNumber, int SID, double primAngle, double secAngle, int lateralPos, int longitudinalPos, int verticalPos, double mmPerPxl)
{
	double angles[2] = { primAngle, secAngle };
	if (streamNumber == 0)
	{
		theBiplaneGeometry.firstSystem.setImageDimension(
			SystemGeometryDefinitions::CROPPED_SIZE_Y,
			SystemGeometryDefinitions::CROPPED_SIZE_Y

		);
		theBiplaneGeometry.firstSystem.setIsFramegrabber(isFramegrabber);
		theBiplaneGeometry.firstSystem.setTablePosition((double) lateralPos, (double) longitudinalPos, (double) verticalPos);		
		theBiplaneGeometry.firstSystem.setMillimeterToPixelScaling(mmPerPxl);
		theBiplaneGeometry.firstSystem.setParameters(angles, SID, SODRefNew[streamNumber]);
	}
	else if (streamNumber == 1)
	{
		theBiplaneGeometry.secondSystem.setImageDimension(
			SystemGeometryDefinitions::CROPPED_SIZE_Y,
			SystemGeometryDefinitions::CROPPED_SIZE_Y

		);
		theBiplaneGeometry.secondSystem.setIsFramegrabber(isFramegrabber);
		theBiplaneGeometry.secondSystem.setTablePosition((double)lateralPos, (double)longitudinalPos, (double)verticalPos);
		theBiplaneGeometry.secondSystem.setMillimeterToPixelScaling(mmPerPxl);
		theBiplaneGeometry.secondSystem.setParameters(angles, SID, SODRefNew[streamNumber]);
	}
	if (alreadyConstructedPipeline[0] + alreadyConstructedPipeline[1] == 1 && loadDefaultMeshPositionBool)
	{
		SystemGeometryDefinitions::TABLE_POSITION_X = lateralPos; // lat.
		SystemGeometryDefinitions::TABLE_POSITION_Y = longitudinalPos; // long.
		SystemGeometryDefinitions::TABLE_POSITION_Z = 0; //vert. = nicht die h�he, die soll 0 bleiben

	}
	setDICOMAnglesToWindowRef(streamNumber, primAngle, secAngle);

	const char* filename1;
	const char* filename2;
	filename1 = theXRAYReaders[0]->getInputFile();
	filename2 = theXRAYReaders[1]->getInputFile();
	if (filename1 != NULL && filename1[0] != '\0' && filename1 != NULL && filename2[0] != '\0')
	{
		theBiplaneGeometry.computeParameters();
		setupBiplaneGeometryFromFramegrabber();

	}
	else
	{
		setupGeometryFromFramegrabber(streamNumber);
		updateMeshPositionsFromWindow(2);
	}
}



void OverlayScene::setupBiplaneGeometryFromFramegrabber()
{
	/*
	This method sets all visualization according to the geometry
	by overwriting many of the properties set in setupPipeline().
	*/

	// update scene according to geometry
	setupGeometryFromFramegrabber(0);
	setupGeometryFromFramegrabber(1);

	/* 3D view: camera position */
	double pos[3], pos2[3];
	theBiplaneGeometry.firstSystem.getSourcePosition(pos[0], pos[1], pos[2]);
	theBiplaneGeometry.secondSystem.getSourcePosition(pos2[0], pos2[1], pos2[2]);

	// compute the middle between first and second
	pos[0] += pos2[0]; pos[0] /= 2;
	pos[1] += pos2[1]; pos[1] /= 2;
	pos[2] += pos2[2]; pos[2] /= 2;
	renderer3D->GetActiveCamera()->SetPosition(pos);

	// Trigger updating mesh positions according to updated geometry.
	updateMeshPositionsFromWindow(2);
	// set motion correction scaling
	//set2DWindowCameraToDefinedPosition();


}

void OverlayScene::setupGeometryFromFramegrabber(int index)
{
	XRayGeometry* geometry;


	vtkLineSource* line;
	switch (index) {
	case 0:
		geometry = &theBiplaneGeometry.firstSystem;
		line = line1;
		break;
	case 1:
		geometry = &theBiplaneGeometry.secondSystem;
		line = line2;
		break;
	}

	int width, height;
	bool ok[1];
	double mmPerPixel, scaleFactor, bounds[6], detector[3], detectorCentered[3], angle[2], source[3];
	double isoCenterToDetectorDistance, isoCenterToSourceDistance;
	theXRAYReaders[index]->GetOutput()->GetBounds(bounds);
	/* bring the the detector to the correct position */
	geometry->getImageDimension(width, height);
	scaleFactor = geometry->getPixelToMillimeterScaling();
	geometry->getPositionerAngles(angle[0], angle[1]);
	geometry->getSourcePosition(source[0], source[1], source[2]);
	geometry->getDetectorPosition(detector[0], detector[1], detector[2]);	
	isoCenterToDetectorDistance = geometry->getIsoCenterToDetectorDistance();
	isoCenterToSourceDistance = geometry->getIsoCenterToSourceDistance();

	detectorCentered[0] = detector[0] - bounds[0] - (width / 2.0)* scaleFactor - SystemGeometryDefinitions::CLIP_X_MIN* scaleFactor;
	detectorCentered[1] = detector[1] - bounds[2] - (height / 2.0)* scaleFactor;
	detectorCentered[2] = detector[2] - bounds[4];

	theXRAYActors3D[index]->SetOrigin(
			bounds[0] + width / 2.0* scaleFactor + SystemGeometryDefinitions::CLIP_X_MIN* scaleFactor,
			bounds[2] + height / 2.0*scaleFactor,
			bounds[4]
			);
		// be aware of the different angle sequence
	//theXRAYActors3D[index]->SetOrientation(-angle[1], angle[0], 0.0); // does not work
	theXRAYActors3D[index]->SetOrientation(0.0, 0.0, 0.0);
	theXRAYActors3D[index]->RotateY(angle[0]);
	theXRAYActors3D[index]->RotateX(-angle[1]);
	theXRAYActors3D[index]->SetPosition(detectorCentered);

		///* projection line */
	line->SetPoint1(source);
	line->SetPoint2(detector);

		/* 2D overlay view */
	theXRAYActors[index]->SetPosition(
			-(bounds[0] + width / 2.0* scaleFactor + SystemGeometryDefinitions::CLIP_X_MIN* scaleFactor),
			-(bounds[2] + height / 2.0*scaleFactor),
			-bounds[4] + geometry->getIsoCenterToDetectorDistance()
		);

	theXRAYActors[index]->SetPickable(0);

	theMarkerPointAssemblies[index]->GetUserMatrix()->SetElement(0, 3, 0);
	theMarkerPointAssemblies[index]->GetUserMatrix()->SetElement(1, 3, 0);
	theMarkerPointAssemblies[index]->GetUserMatrix()->SetElement(2, 3, geometry->getIsoCenterToDetectorDistance());

	rendererXRAY[index]->GetActiveCamera()->SetPosition(0, 0, -isoCenterToSourceDistance);
	rendererXRAY[index]->GetActiveCamera()->SetFocalPoint(0, 0, isoCenterToDetectorDistance);
	rendererXRAY[index]->GetActiveCamera()->SetViewUp(0, 1, 0);
	rendererXRAY[index]->ResetCameraClippingRange(-300, 300, -300, 300, 0, 2000);	//evtl. braucht man
	//renWinXRAY[index]->Render();	

}


void OverlayScene::setMainGeometryByDICOM(int SID, int sourcePatDist, double primAngle, double secAngle, int lateralPos, int longitudinalPos, int verticalPos, double mmPerPixel, int imageSizeX, int imageSizeY)
{

	theBiplaneGeometry.mainSystem.setIsFramegrabber(isFramegrabber);
	theBiplaneGeometry.mainSystem.setImageDimension(
		imageSizeX,
		imageSizeY
		);

	double angles[2] = { primAngle, secAngle};


	theBiplaneGeometry.mainSystem.setMillimeterToPixelScaling(mmPerPixel);	
	theBiplaneGeometry.mainSystem.setTablePosition((double)lateralPos, (double)longitudinalPos, (double)verticalPos);	
	theBiplaneGeometry.mainSystem.setParameters(angles, SID, sourcePatDist);

	setDICOMAnglesToWindow(primAngle, secAngle);
	setupBiplaneGeometry(2);	
	updateMeshPositionsFromWindow(2);
	setMotionCorrectionScalingAccordingToGeometry();
	
}


void OverlayScene::setFramegrabberGeometryBiplaneLive(int index)
{

	bool rao, kaud, fdHasChanged;
	int primAngle, secAngle, lateralPos, longitudinalPos, verticalPos, sid, fd;

	if (index == 0)
	{
		panelFront->getDisplayedValues(rao, kaud, primAngle, secAngle, longitudinalPos, lateralPos, verticalPos, sid, fd);
		fdHasChanged = panelFront->getFDHasChanged();
	}
	else if (index == 1)
	{
		panelLat->getDisplayedValues(rao, kaud, primAngle, secAngle, longitudinalPos, lateralPos, verticalPos, sid, fd);
		fdHasChanged = panelLat->getFDHasChanged();
		panelFront->getTablePos(longitudinalPos, lateralPos, verticalPos);
	}

	double angles[2] = { (double)primAngle, (double)secAngle };
	double mmPerPxl, mmPerPxlOld;

	// nur bei wenn sich FD geaendert hat und nur bei LIVE, bei play wird aus mmPerPxl ausgelesen
	if (fdHasChanged) {

		switch (fd) {
		case 15: // frontal // lateral <- fehlt evtl
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.110726;
			break;
		case 19: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.132902;
			break;
		case 20: // lateral
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.1323;
			break;
		case 22: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.15631;
			break;
		case 25: // lateral
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.17498;
			break;
		case 27: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.187775;
			break;
		case 31: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.22025;
			break;
		case 37: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.259644;
			break;
		case 42: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.292908;
			break;
		case 48: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.3795;
			break;
		}
	}

	theFramegrabbers[index]->SetDataSpacing(SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index], SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index], 1);
	theFramegrabbers[index]->Update();
	luminanceFilter[index]->Update();

	mmPerPxl = SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index];
	

	if (index == 0) {
		theBiplaneGeometry.firstSystem.setImageDimension(
			SystemGeometryDefinitions::CROPPED_SIZE_Y,
			SystemGeometryDefinitions::CROPPED_SIZE_Y
		);

		theBiplaneGeometry.firstSystem.setIsFramegrabber(isFramegrabber);
		mmPerPxlOld = theBiplaneGeometry.firstSystem.getMillimeterToPixelScalingOld();
		theBiplaneGeometry.firstSystem.setTablePosition((double)lateralPos, (double)longitudinalPos, (double)verticalPos);
		theBiplaneGeometry.firstSystem.setMillimeterToPixelScaling(mmPerPxl);
		theBiplaneGeometry.firstSystem.setParameters(angles, sid, SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[index]);

		setDICOMAnglesToWindowRef(index, (double)primAngle, (double)secAngle);

		double factor = 1.0 / (mmPerPxl / mmPerPxlOld);
		theBiplaneGeometry.firstSystem.setZoomFactor(factor);

	}
	else if (index == 1) {
		theBiplaneGeometry.secondSystem.setImageDimension(
			SystemGeometryDefinitions::CROPPED_SIZE_Y,
			SystemGeometryDefinitions::CROPPED_SIZE_Y

		);
		theBiplaneGeometry.secondSystem.setIsFramegrabber(isFramegrabber);
		mmPerPxlOld = theBiplaneGeometry.secondSystem.getMillimeterToPixelScalingOld();
		verticalPos += SystemGeometryDefinitions::ISOCENTER_SHIFT_Z;
		theBiplaneGeometry.secondSystem.setTablePosition((double)lateralPos, (double)longitudinalPos, (double)verticalPos);
		theBiplaneGeometry.secondSystem.setMillimeterToPixelScaling(mmPerPxl);
		theBiplaneGeometry.secondSystem.setParameters(angles, sid, SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[index]);

		setDICOMAnglesToWindowRef(index, (double)primAngle, (double)secAngle);

		double factor = 1.0 / (mmPerPxl / mmPerPxlOld);
		theBiplaneGeometry.secondSystem.setZoomFactor(factor);

	}


	theBiplaneGeometry.computeParameters();
	setupGeometryFromFramegrabber(index);
	updateMeshPositionsFromWindow(2);

}


void OverlayScene::setFramegrabberGeometry(int index, int SID, double primAngle, double secAngle, int lateralPos, int longitudinalPos, int verticalPos, double FDoderMMPerPxl, bool play)
{

	theBiplaneGeometry.mainSystem.setImageDimension(
		SystemGeometryDefinitions::CROPPED_SIZE_Y,
		SystemGeometryDefinitions::CROPPED_SIZE_Y

	);
	double mmPerPxl, mmPerPxlOld;
	int SOD;
	if (play == true)
	{
		mmPerPxl = FDoderMMPerPxl;
		SOD = SODPlay;
	}
	else
	{
		// nur bei wenn sich FD geaendert hat und nur bei LIVE, bei play wird aus mmPerPxl ausgelesen
		int FD = (int)FDoderMMPerPxl;
		if (FD != FDalt[index]) {//FDmain

			switch (FD) {
			case 15: // frontal // lateral <- fehlt evtl.
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.110726;
				break;
			case 19: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.132902;
				break;
			case 20: // lateral
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.1323;
				break;
			case 22: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.15631;
				break;
			case 25: // lateral
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.17498;
				break;
			case 27: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.187775;
				break;
			case 31: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.22025;
				break;
			case 37: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.259644;
				break;
			case 42: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.292908;
				break;
			case 48: // frontal
				SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.3795;
				break;
			}

			FDalt[index] = FD;
			//FDmain = FD;

			theFramegrabbers[index]->SetDataSpacing(SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index], SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index], 1);
			theFramegrabbers[index]->Update();
			luminanceFilter[index]->Update();
		}
		mmPerPxl = SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index];
		SOD = SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[index];
	}

	double angles[2] = { primAngle, secAngle };
	theBiplaneGeometry.mainSystem.setIsFramegrabber(isFramegrabber);
	mmPerPxlOld = theBiplaneGeometry.mainSystem.getMillimeterToPixelScalingOld();

	if (index == 1)
	{
		verticalPos += SystemGeometryDefinitions::ISOCENTER_SHIFT_Z;
	}

	theBiplaneGeometry.mainSystem.setTablePosition((double) lateralPos, (double) longitudinalPos, (double) verticalPos);
	theBiplaneGeometry.mainSystem.setMillimeterToPixelScaling(mmPerPxl);
	theBiplaneGeometry.mainSystem.setParameters(angles, SID, SOD);

	setDICOMAnglesToWindow(primAngle, secAngle);

	double factor = 1.0 / (mmPerPxl / mmPerPxlOld);
	theBiplaneGeometry.mainSystem.setZoomFactor(factor);

	setupFramegrabberGeometry();
	updateMeshPositionsFromWindow(2);
	setMotionCorrectionScalingAccordingToGeometry();

}

void OverlayScene::setFramegrabberGeometryLive(int index)
{
	bool rao, kaud, fdHasChanged;
	int primAngle, secAngle, lateralPos, longitudinalPos, verticalPos, sid, fd;

	if (index == 0)
	{
		panelFront->getDisplayedValues(rao, kaud, primAngle, secAngle, longitudinalPos, lateralPos, verticalPos, sid, fd);
		fdHasChanged= panelFront->getFDHasChanged();
	}
	else if(index == 1)
	{
		panelLat->getDisplayedValues(rao, kaud, primAngle, secAngle, longitudinalPos, lateralPos, verticalPos, sid, fd);
		fdHasChanged = panelLat->getFDHasChanged();
		panelFront->getTablePos(longitudinalPos, lateralPos, verticalPos);
	}
		
	theBiplaneGeometry.mainSystem.setImageDimension(
		SystemGeometryDefinitions::CROPPED_SIZE_Y,
		SystemGeometryDefinitions::CROPPED_SIZE_Y

	);
	double mmPerPxl, mmPerPxlOld;
	int SOD;

	// nur bei wenn sich FD geaendert hat und nur bei LIVE
	if (fdHasChanged) {//FDmain

		switch (fd) {
		case 15: // frontal // lateral <- fehlt evtl.
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.110726;
			break;
		case 19: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.132902;
			break;
		case 20: // lateral
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.1323;
			break;
		case 22: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.15631;
			break;
		case 25: // lateral
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.17498;
			break;
		case 27: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.187775;
			break;
		case 31: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.22025;
			break;
		case 37: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.259644;
			break;
		case 42: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.292908;
			break;
		case 48: // frontal
			SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index] = 0.3795;
			break;
		}

		theFramegrabbers[index]->SetDataSpacing(SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index], SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index], 1);
		theFramegrabbers[index]->Update();
		luminanceFilter[index]->Update();
	}
	mmPerPxl = SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[index];
	SOD = SystemGeometryDefinitions::DISTANCE_SOURCE_TO_PATIENT[index];

	double angles[2] = { (double) primAngle, (double) secAngle };
	theBiplaneGeometry.mainSystem.setIsFramegrabber(isFramegrabber);
	mmPerPxlOld = theBiplaneGeometry.mainSystem.getMillimeterToPixelScalingOld();

	if (index == 1)
	{
		verticalPos += SystemGeometryDefinitions::ISOCENTER_SHIFT_Z;
	}

	theBiplaneGeometry.mainSystem.setTablePosition((double)lateralPos, (double)longitudinalPos, (double)verticalPos);
	theBiplaneGeometry.mainSystem.setMillimeterToPixelScaling(mmPerPxl);
	theBiplaneGeometry.mainSystem.setParameters(angles, sid, SOD);

	setDICOMAnglesToWindow((double)primAngle, (double)secAngle);

	double factor = 1.0 / (mmPerPxl / mmPerPxlOld);
	theBiplaneGeometry.mainSystem.setZoomFactor(factor);

	setupFramegrabberGeometry();
	updateMeshPositionsFromWindow(2);
	setMotionCorrectionScalingAccordingToGeometry();

}

void OverlayScene::setupFramegrabberGeometry()
{
	XRayGeometry* geometry = &theBiplaneGeometry.mainSystem;

	int width, height;
	double scaleFactor, bounds[6], detector[3], angle[2], source[3];
	double verticalPos, longitudinalPos, lateralPos;
	double isoCenterToDetectorDistance, isoCenterToSourceDistance;

	/* bring the the detector to the correct position */
	geometry->getImageDimension(width, height);
	scaleFactor = geometry->getPixelToMillimeterScaling();
	geometry->getPositionerAngles(angle[0], angle[1]);
	geometry->getSourcePosition(source[0], source[1], source[2]);
	geometry->getDetectorPosition(detector[0], detector[1], detector[2]);
	isoCenterToDetectorDistance = geometry->getIsoCenterToDetectorDistance();
	isoCenterToSourceDistance = geometry->getIsoCenterToSourceDistance();

	theMotionCorrections->SetImageToWorldCoordinatesScaling(scaleFactor);
	theMotionCorrections->setGeometry(geometry);

	actorMainXRAY->SetPosition(
		-(width / 2.0* scaleFactor + SystemGeometryDefinitions::CLIP_X_MIN* scaleFactor),
		-(height / 2.0*scaleFactor),
		geometry->getIsoCenterToDetectorDistance()
		);

	actorMainXRAY->SetPickable(0);

	rendererMainXRAY->GetActiveCamera()->SetPosition(0, 0, -isoCenterToSourceDistance);
	rendererMainXRAY->GetActiveCamera()->SetFocalPoint(0, 0, isoCenterToDetectorDistance);
	rendererMainXRAY->GetActiveCamera()->SetViewUp(0, 1, 0);

	double factor = theBiplaneGeometry.mainSystem.getZoomFactor();
	
	rendererMainXRAY->GetActiveCamera()->Zoom(factor);

}

void OverlayScene::setupBiplaneGeometry()
{
	/*
	This method sets all visualization according to the geometry
	by overwriting many of the properties set in setupPipeline().
	*/

	// update scene according to geometry
	setupBiplaneGeometry(0);
	setupBiplaneGeometry(1);

	/* 3D view: camera position */
	double pos[3], pos2[3];
	/*theBiplaneGeometry.firstSystem.getDetectorPosition(pos[0], pos[1], pos[2]);
	theBiplaneGeometry.secondSystem.getDetectorPosition(pos2[0], pos2[1], pos2[2]);*/

	theBiplaneGeometry.firstSystem.getSourcePosition(pos[0], pos[1], pos[2]);
	theBiplaneGeometry.secondSystem.getSourcePosition(pos2[0], pos2[1], pos2[2]);

	// compute the middle between first and second
	pos[0] += pos2[0]; pos[0] /= 2;
	pos[1] += pos2[1]; pos[1] /= 2;
	pos[2] += pos2[2]; pos[2] /= 2;
	renderer3D->GetActiveCamera()->SetPosition(pos);

	// Trigger updating mesh positions according to updated geometry.
	updateMeshPositionsFromWindow(2);
}

void OverlayScene::set2DWindowCameraToDefinedPosition()
{
	double pos[3];
	theBiplaneGeometry.firstSystem.getSourcePosition(pos[0], pos[1], pos[2]);
	rendererXRAY[0]->GetActiveCamera()->SetPosition(0, 0, pos[2]);	

	theBiplaneGeometry.secondSystem.getSourcePosition(pos[0], pos[1], pos[2]);
	rendererXRAY[1]->GetActiveCamera()->SetPosition(0, 0, pos[2]);

	rendererXRAY[0]->ResetCameraClippingRange(-300, 300, -300, 300, 0, 2000);
	rendererXRAY[1]->ResetCameraClippingRange(-300, 300, -300, 300, 0, 2000);

}


void OverlayScene::setupBiplaneGeometry(int index)
{
	XRayGeometry* geometry;


	vtkLineSource* line;
	switch (index) {
	case 0:
		geometry = &theBiplaneGeometry.firstSystem;
		line = line1;
		break;
	case 1:
		geometry = &theBiplaneGeometry.secondSystem;
		line = line2;
		break;
	case 2:
		geometry = &theBiplaneGeometry.mainSystem;
		break;
	}

	int width, height;
	bool ok[1];
	double mmPerPixel, scaleFactor, bounds[6], detector[3], detectorCentered[3], angle[2], source[3];
	double verticalPos, longitudinalPos, lateralPos;
	double isoCenterToDetectorDistance, isoCenterToSourceDistance;
	if (index == 0 || index == 1)
	{
		theXRAYReaders[index]->GetOutput()->GetBounds(bounds);

	}
	else if (index == 2)
	{
		readerMainXRAY->GetOutput()->GetBounds(bounds);
	}		

	/* bring the the detector to the correct position */
	geometry->getImageDimension(width, height);	
	scaleFactor = geometry->getPixelToMillimeterScaling();
	geometry->getPositionerAngles(angle[0], angle[1]);
	geometry->getSourcePosition(source[0], source[1], source[2]);

	geometry->getDetectorPosition(detector[0], detector[1], detector[2]);	

	isoCenterToDetectorDistance = geometry->getIsoCenterToDetectorDistance();
	isoCenterToSourceDistance = geometry->getIsoCenterToSourceDistance();

	detectorCentered[0] = detector[0] - bounds[0] - (width/2.0)* scaleFactor;
	detectorCentered[1] = detector[1] - bounds[2] - (height / 2.0)* scaleFactor;
	detectorCentered[2] = detector[2] - bounds[4];

	if (index == 0 || index == 1)
	{
		theXRAYActors3D[index]->SetOrigin(
			bounds[0] + width / 2.0* scaleFactor,
			bounds[2] + height / 2.0*scaleFactor,
			bounds[4]
			);
		// be aware of the different angle sequence
		//theXRAYActors3D[index]->SetOrientation(-angle[1], angle[0], 0.0);
		theXRAYActors3D[index]->SetOrientation(0.0, 0.0, 0.0);
		theXRAYActors3D[index]->RotateY(angle[0]);
		theXRAYActors3D[index]->RotateX(-angle[1]);
		theXRAYActors3D[index]->SetPosition(detectorCentered);

		///* projection line */
		line->SetPoint1(source);
		line->SetPoint2(detector);

		/* 2D overlay view */
		theXRAYActors[index]->SetPosition(
			-(bounds[0] + width / 2.0* scaleFactor),
			-(bounds[2] + height / 2.0*scaleFactor),
			-bounds[4] + geometry->getIsoCenterToDetectorDistance()
			);

		theXRAYActors[index]->SetPickable(0);

		theMarkerPointAssemblies[index]->GetUserMatrix()->SetElement(0, 3, 0);
		theMarkerPointAssemblies[index]->GetUserMatrix()->SetElement(1, 3, 0);
		theMarkerPointAssemblies[index]->GetUserMatrix()->SetElement(2, 3, geometry->getIsoCenterToDetectorDistance());

		rendererXRAY[index]->GetActiveCamera()->SetPosition(0, 0, -isoCenterToSourceDistance);
		rendererXRAY[index]->GetActiveCamera()->SetFocalPoint(0, 0, isoCenterToDetectorDistance);
		rendererXRAY[index]->GetActiveCamera()->SetViewUp(0, 1, 0);
		rendererXRAY[index]->ResetCameraClippingRange(-300, 300, -300, 300, 0, 2000);
		//renWinXRAY[index]->Render();

	}

	else if (index == 2)
	{
		theMotionCorrections->SetImageToWorldCoordinatesScaling(scaleFactor);
		theMotionCorrections->setGeometry(geometry);

		actorMainXRAY->SetPosition(
			-(bounds[0] + width / 2.0* scaleFactor),
			-(bounds[2] + height / 2.0*scaleFactor),
			-bounds[4] + geometry->getIsoCenterToDetectorDistance()
			);

		actorMainXRAY->SetPickable(0);
		

		rendererMainXRAY->GetActiveCamera()->SetPosition(0, 0, -isoCenterToSourceDistance);
		rendererMainXRAY->GetActiveCamera()->SetFocalPoint(0, 0, isoCenterToDetectorDistance);
		rendererMainXRAY->GetActiveCamera()->SetViewUp(0, 1, 0);
		rendererMainXRAY->ResetCameraClippingRange(-300, 300, -300, 300, 0, 2000);
		//renWinMainXRAY->Render();
	}
	

}

void OverlayScene::viewXRayDirection(int index)
{
	XRayGeometry* geometry;

	switch (index) {
	case 0:
		geometry = &theBiplaneGeometry.firstSystem;
		break;
	case 1:
		geometry = &theBiplaneGeometry.secondSystem;
		break;

	}

	double pos[3];
	geometry->getSourcePosition(pos[0], pos[1], pos[2]);
	renderer3D->GetActiveCamera()->SetPosition(pos);

	double pos2[3];
	geometry->getDetectorPosition(pos2[0], pos2[1], pos2[2]);
	renderer3D->GetActiveCamera()->SetFocalPoint(pos2);

	renderer3D->GetActiveCamera()->SetViewUp(0, 1, 0);
	renderer3D->ResetCameraClippingRange();

	renWin3D->Render();
}

int OverlayScene::getNumberOfFrames(int index)
{
	/*int res = INT_MAX;
	for (unsigned int i = 0; i < theXRAYReaders.size(); ++i)
	{
		res = min(res, theXRAYReaders[i]->getNumberOfFrames());
	}

	return res;*/

	int res;

	if (isFramegrabber)
	{
		res = 1000;
	}
	else
	{
		if (index == 0 || index == 1)
		{
			res = theXRAYReaders[index]->getNumberOfFrames();
		}
		else if (index == 2)
		{
			res = readerMainXRAY->getNumberOfFrames();
		}

	}

	

	return res;
}


void OverlayScene::showFrame(int index, int frame)
{
	if (frame < 0) 
		frame = 0; // ensure non-negative value
	

	if (index == 0 || index == 1)
	{
		int numberOfFrames = theXRAYReaders[index]->getNumberOfFrames();

		// the frame is selected by selecting a slice in the xy-plane
		// of the video stream (which is handled as an "image volume")		
		double bounds[6];
		theXRAYReaders[index]->GetOutput()->GetBounds(bounds);
		double z = frame * (bounds[5] - bounds[4]) / numberOfFrames;
		theXRAYReslicers[index]->SetResliceAxesOrigin(0.0, 0.0, z);
		theXRAYReslicers[index]->Update();

		//renWinXRAY[index]->Render();

		renWin3D->Render();
	}
	if (index == 2)
	{
		int numberOfFrames = readerMainXRAY->getNumberOfFrames();

		// the frame is selected by selecting a slice in the xy-plane
		// of the video stream (which is handled as an "image volume")		
		double bounds[6];
		readerMainXRAY->GetOutput()->GetBounds(bounds);
		double z = frame * (bounds[5] - bounds[4]) / numberOfFrames;
		resliceMainXRAY->SetResliceAxesOrigin(0.0, 0.0, z);
		resliceMainXRAY->Update();

	}
	
}

void OverlayScene::setMainXRAYInputToFile(const char* file)
{
	int SID, sourcePatDist, imageSizeX, imageSizeY;
	double primAngle, secAngle, mmPerPixel;
	int lateralPos, longitudinalPos, verticalPos;
	bool ok[10];


	readerMainXRAY = XRAYReader::New();
	readerMainXRAY->setInputFile(file);
	readerMainXRAY->Update();

	resliceMainXRAY->SetInputData(readerMainXRAY->GetOutput());

	resliceMainXRAY->SetResliceAxesOrigin(0.0, 0.0, 1.0);
	resliceMainXRAY->Update();
	theMotionCorrections->SetInputConnection(resliceMainXRAY->GetOutputPort(0));
	actorMainXRAY->GetMapper()->SetInputConnection(theMotionCorrections->GetOutputPort());

	this->getDicomGeometryValues(SID, sourcePatDist, primAngle, secAngle, lateralPos, longitudinalPos, verticalPos, mmPerPixel, imageSizeX, imageSizeY, ok);
	setMainGeometryByDICOM(SID, sourcePatDist, primAngle, secAngle, lateralPos, longitudinalPos, verticalPos, mmPerPixel, imageSizeX, imageSizeY);

	//rendererMainXRAY->ResetCamera();
	//renWinMainXRAY->Render();

	alreadyConstructedPipeline[2] = false;

}

void OverlayScene::disableECG(unsigned int streamNumber)
{

	view[streamNumber]->GetScene()->RemoveItem(chart[streamNumber]);

	if (streamNumber == 0 || streamNumber == 1)
	{
		renWinXRAY[streamNumber]->Render();
	}
	else if (streamNumber == 2)
	{
		renWinMainXRAY->Render();

	}
}

void OverlayScene::setFrameForPeak(unsigned int streamNumber)
{
	int frame = std::max(frameNumberForPeak[0], frameNumberForPeak[1]) - std::min(frameNumberForPeak[0], frameNumberForPeak[1]) + 1;
	if (frameNumberForPeak[0] > frameNumberForPeak[1])
	{
		setECGFrame(0, frame);
		setECGFrame(1, 1);
	}
	else
	{
		setECGFrame(1, frame);
		setECGFrame(0, 1);
	}

}
	

void OverlayScene::getFrameForPeak(unsigned int streamNumber, int& frame)
{
	if (frameNumberForPeak[0] > frameNumberForPeak[1])
	{
		if (streamNumber == 0)
		{
			frame = std::max(frameNumberForPeak[0], frameNumberForPeak[1]) - std::min(frameNumberForPeak[0], frameNumberForPeak[1]) + 1;
		}
		else
		{
			frame = 1;
		}
	}
		
	else
	{
		if (streamNumber == 0)
		{
			frame = 1;
		}
		else
		{
			frame = std::max(frameNumberForPeak[0], frameNumberForPeak[1]) - std::min(frameNumberForPeak[0], frameNumberForPeak[1]) + 1;
		}
	}
	
}

void OverlayScene::loadECGFile(unsigned int streamNumber, const char* filename, int frameNumber)
{
	chart[streamNumber]->ClearPlots();
	view[streamNumber]->GetScene()->RemoveItem(chart[streamNumber]);

	int numLines = 0, peakLoc = 0;
	ifstream myfile(filename);
	string sline;
	vector<int> list;

	while (myfile)
	{
		myfile >> sline;
		if (sline.front() != '/')
		{
			list.push_back(stoi(sline));
			++numLines;
		}
		else
		{
			std::size_t pos = sline.find("/");
			std::string str = sline.substr(pos + 1);
			peakLoc = stoi(str);
			break;
		}

	}

	int beatFrame;
	double cineFrame;

	if (streamNumber == 0 || streamNumber == 1)
	{
		beatFrame = theXRAYReaders[streamNumber]->GetECGBeatFrame();
		cineFrame = theXRAYReaders[streamNumber]->GetCineFrame();
	}
	else if (streamNumber == 2)
	{
		beatFrame = readerMainXRAY->GetECGBeatFrame();
		cineFrame = readerMainXRAY->GetCineFrame();
	}

	frameNumberForPeak[streamNumber] = (int)peakLoc / (beatFrame / cineFrame);
	int mod = peakLoc % (int)(beatFrame / cineFrame);
	if (mod > 0)
	{
		frameNumberForPeak[streamNumber] += 1;
	}

	int* ECG = &list[0];

	// Create a table with some points in it
	vtkSmartPointer<vtkTable> table =
		vtkSmartPointer<vtkTable>::New();

	vtkSmartPointer<vtkFloatArray> arrX =
		vtkSmartPointer<vtkFloatArray>::New();
	arrX->SetName("Time");
	table->AddColumn(arrX);

	vtkSmartPointer<vtkFloatArray> arrY =
		vtkSmartPointer<vtkFloatArray>::New();
	arrY->SetName("Signal");
	table->AddColumn(arrY);

	table->SetNumberOfRows(numLines);
	for (int i = 0; i < numLines; ++i)
	{
		table->SetValue(i, 0, i);
		table->SetValue(i, 1, ECG[i]);

	}

	view[streamNumber]->GetScene()->AddItem(chart[streamNumber]);

	vtkPlot *line = chart[streamNumber]->AddPlot(vtkChart::LINE);
	chart[streamNumber]->SetForceAxesToBounds(true);
	chart[streamNumber]->SetForceAxesToBounds(true);

	chart[streamNumber]->GetAxis(vtkAxis::LEFT)->SetGridVisible(false);
	chart[streamNumber]->GetAxis(vtkAxis::BOTTOM)->SetGridVisible(false);
	chart[streamNumber]->GetAxis(0)->SetTicksVisible(false);
	chart[streamNumber]->GetAxis(1)->SetTicksVisible(false);
	chart[streamNumber]->GetAxis(0)->SetLabelsVisible(false);
	chart[streamNumber]->GetAxis(1)->SetLabelsVisible(false);
	chart[streamNumber]->GetAxis(0)->SetAxisVisible(false);
	chart[streamNumber]->GetAxis(1)->SetAxisVisible(false);

#if VTK_MAJOR_VERSION <= 5
	line->SetInput(table, 0, 1);
#else
	line->SetInputData(table, 0, 1);
	//line->Update();
	line->Modified();
#endif
	line->SetColor(0, 255, 0, 255);
	line->SetWidth(1.0);

	setECGFrame(streamNumber, frameNumber);

	if (streamNumber == 0 || streamNumber == 1)
	{
		renWinXRAY[streamNumber]->Render();
	}
	else if (streamNumber == 2)
	{
		renWinMainXRAY->Render();

	}

}

void OverlayScene::setECGFrame(unsigned int streamNumber, int frameNumber)
{

	vtkSmartPointer<vtkTable> table =
		vtkSmartPointer<vtkTable>::New();

	vtkSmartPointer<vtkFloatArray> arrP =
		vtkSmartPointer<vtkFloatArray>::New();
	arrP->SetName("Point");
	table->AddColumn(arrP);

	vtkSmartPointer<vtkFloatArray> arrS =
		vtkSmartPointer<vtkFloatArray>::New();
	arrS->SetName("Cursor");
	table->AddColumn(arrS);

	int beatFrame;
	double cineFrame;

	if (streamNumber == 0 || streamNumber == 1)
	{
		beatFrame = theXRAYReaders[streamNumber]->GetECGBeatFrame();
		cineFrame = theXRAYReaders[streamNumber]->GetCineFrame();
	}
	else if (streamNumber == 2)
	{
		beatFrame = readerMainXRAY->GetECGBeatFrame();
		cineFrame = readerMainXRAY->GetCineFrame();
	}


	table->SetNumberOfRows(2);
	table->SetValue(0, 0, (frameNumber* (int)(beatFrame / cineFrame) - (int)(beatFrame / cineFrame)/2 ) );	//300 beats/s / 15 frames/s (read from DICOM header) => 20 beats per frame
	table->SetValue(1, 0, (frameNumber* (int)(beatFrame / cineFrame) - (int)(beatFrame / cineFrame) / 2));
	table->SetValue(0, 1, 10000);
	table->SetValue(1, 1, 40000);	

	chart[streamNumber]->RemovePlot(1);

	vtkPlot *line = chart[streamNumber]->AddPlot(vtkChart::LINE);
	line->SetInputData(table, 0, 1);
	//line->Update();
	line->Modified();
	line->SetColor(255, 0, 0, 255);

	line->SetWidth(1.5);

	if (streamNumber == 0 || streamNumber == 1)
	{
		renWinXRAY[streamNumber]->Render();
	}
	else if (streamNumber == 2)
	{
		renWinMainXRAY->Render();

	}

}

void OverlayScene::setInputToFile(const char* file, int streamNumber)
{

	if (file ==NULL) {
		cout << "Problem with file selection" << endl;
		return;
	}

	alreadyConstructedPipeline[streamNumber] = false;	

	double defaultNullDouble = 0.0;
	int defaultNullInt = 0;
	setNewGeometryRef(streamNumber, defaultNullDouble, defaultNullDouble, defaultNullInt, defaultNullInt, defaultNullInt, defaultNullInt, defaultNullDouble, defaultNullInt, defaultNullInt);

	theXRAYReaders[streamNumber]->Delete();

	int SID, sourcePatDist, imageSizeX, imageSizeY;
	double primAngle, secAngle, mmPerPixel;
	int lateralPos, longitudinalPos, verticalPos;
	bool ok[10];

	XRAYReader* readerXRAY = XRAYReader::New();
	readerXRAY->setInputFile(file);
	readerXRAY->Update();
	theXRAYReaders[streamNumber] = readerXRAY;
	theXRAYReaders[streamNumber]->Update();
	theXRAYReslicers[streamNumber]->SetInputData(readerXRAY->GetOutput());

	theXRAYReslicers[streamNumber]->Update();
	theXRAYActors[streamNumber]->GetMapper()->SetInputConnection(theXRAYReslicers[streamNumber]->GetOutputPort());
	theXRAYActors3D[streamNumber]->GetMapper()->SetInputConnection(theXRAYReslicers[streamNumber]->GetOutputPort());
	theXRAYReslicers[streamNumber]->SetResliceAxesOrigin(0.0, 0.0, 0.0); // to display the first frame, even if there is only one image in a stream

	this->getDicomReferenceGeometryValues(streamNumber, SID, sourcePatDist, primAngle, secAngle, lateralPos, longitudinalPos, verticalPos, mmPerPixel, imageSizeX, imageSizeY, ok);
	// berechnung fehlt!!!!!!!!!!!!!!!!!
	if (!alreadyConstructedPipelineDICOM[streamNumber]) {

		alreadyConstructedPipelineDICOM[streamNumber] = true;
		// default mesh 
		if (alreadyConstructedPipelineDICOM[0] + alreadyConstructedPipelineDICOM[1] == 1)
		{
			if (orientation == 0)	loadDefaultMeshPosition(MR_PHILIPS);
			if (orientation == 1)	loadDefaultMeshPosition(MR_ITK);	// correct orientation AP is needed for correct calculation and display of the angles
			if (orientation == 2)	loadDefaultMeshPosition(CT_PHILIPS);	// correct orientation AP is needed for correct calculation and display of the angles
			if (orientation == 3)	loadDefaultMeshPosition(CT_ITK);	// correct orientation AP is needed for correct calculation and display of the angles				


		}
	}
	setBiplaneGeometry(streamNumber, SID, sourcePatDist, primAngle, secAngle, lateralPos, longitudinalPos, verticalPos, mmPerPixel, imageSizeX, imageSizeY);

	renderer3D->ResetCamera();
	renWin3D->Render();
	
}

void OverlayScene::getDicomReferenceGeometryValues(int streamNumber, int& SID, int& sourcePatDist, double& primAngle, double& secAngle, 
	int& lateralPos, int& longitudinalPos, int& verticalPos, double& mmPerPixel, int& imageSizeX, int& imageSizeY, bool ok[10])
{
	SID = theXRAYReaders[streamNumber]->GetSID(&ok[0]);
	sourcePatDist = theXRAYReaders[streamNumber]->GetSourcePatientDistance(&ok[1]);
	primAngle = theXRAYReaders[streamNumber]->GetPrimaryAngle(&ok[2]);
	secAngle = theXRAYReaders[streamNumber]->GetSecondaryAngle(&ok[3]);
	lateralPos = theXRAYReaders[streamNumber]->GetTableTopLateralPosition(&ok[4]);
	longitudinalPos = theXRAYReaders[streamNumber]->GetTableTopLongitudinalPosition(&ok[5]);
	verticalPos = theXRAYReaders[streamNumber]->GetTableTopVerticalPosition(&ok[6]);
	mmPerPixel = theXRAYReaders[streamNumber]->GetMillimeterPerPixelScaling(&ok[7]);
	imageSizeX = theXRAYReaders[streamNumber]->GetImageSizeX(&ok[8]);
	imageSizeY = theXRAYReaders[streamNumber]->GetImageSizeY(&ok[9]);
}


void OverlayScene::getDicomGeometryValues(int& SID, int& sourcePatDist, double& primAngle, double& secAngle,
	int& lateralPos, int& longitudinalPos, int& verticalPos, double& mmPerPixel, int& imageSizeX, int&imageSizeY, bool ok[10])
{
	SID = readerMainXRAY->GetSID(&ok[0]);	
	sourcePatDist = readerMainXRAY->GetSourcePatientDistance(&ok[1]);
	primAngle = readerMainXRAY->GetPrimaryAngle(&ok[2]);
	secAngle = readerMainXRAY->GetSecondaryAngle(&ok[3]);
	lateralPos = readerMainXRAY->GetTableTopLateralPosition(&ok[4]);
	longitudinalPos = readerMainXRAY->GetTableTopLongitudinalPosition(&ok[5]);
	verticalPos = readerMainXRAY->GetTableTopVerticalPosition(&ok[6]);
	mmPerPixel = readerMainXRAY->GetMillimeterPerPixelScaling(&ok[7]);
	imageSizeX = readerMainXRAY->GetImageSizeX(&ok[8]);
	imageSizeY = readerMainXRAY->GetImageSizeY(&ok[9]);
}


void OverlayScene::setDICOMAnglesToWindow(double primAngle, double secAngle)
{
	primAngleMain = primAngle;
	secAngleMain = secAngle;
}


void OverlayScene::setDICOMAnglesToWindowRef(int index, double primAngle, double secAngle)
{
	primAngleRef[index] = primAngle;
	secAngleRef[index] = secAngle;
}


void OverlayScene::getDICOMAnglesToWindow(double& primAngle, double& secAngle)
{
	primAngle = primAngleMain;
	secAngle = secAngleMain;
}


void OverlayScene::getDICOMAnglesToWindowRef(int index, double& primAngle, double& secAngle)
{
	primAngle = primAngleRef[index];
	secAngle = secAngleRef[index];
	
}


void OverlayScene::setUserMatrix(unsigned int window, double matrix[4][4])
{
	vtkMatrix4x4* m = theMeshAssemblies[window]->GetUserMatrix();
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m->SetElement(i, j, matrix[i][j]);
		}
	}

	updateMeshPositionsFromWindow(window);

}

void OverlayScene::getUserMatrix(unsigned int window, double matrix[4][4])
{

	vtkMatrix4x4* m = theMeshAssemblies[window]->GetUserMatrix();

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			matrix[i][j] = m->GetElement(i, j);
		}
	}


}

void OverlayScene::saveUserMatrix(unsigned int streamNumber, const char* filename)
{
	if (streamNumber >= theMeshAssemblies.size()) return;

	// all assemblies reference the same user matrix
	vtkMatrix4x4 *mat = vtkMatrix4x4::New();
	mat->DeepCopy(theMeshAssemblies[streamNumber]->GetUserMatrix());

	mat->Element[0][3] -= SystemGeometryDefinitions::TABLE_POSITION_X; // lateralPos;
	mat->Element[1][3] -= SystemGeometryDefinitions::TABLE_POSITION_Y; // longitudinalPos;
	mat->Element[2][3] -= SystemGeometryDefinitions::TABLE_POSITION_Z; // verticalPos;

	ofstream file(filename);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			file << mat->GetElement(i, j);
			if (j < 3) file << "\t";
		}
		file << "\n";
	}
	file.close();
}

void OverlayScene::loadUserMatrix(unsigned int streamNumber, const char* filename)
{
	if (streamNumber >= theMeshAssemblies.size()) return;

	ifstream file(filename);
	vtkMatrix4x4* mat = vtkMatrix4x4::New();
	double buf;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			file >> buf;
			mat->SetElement(i, j, buf);
		}
	}
	file.close();

	mat->Element[0][3] += SystemGeometryDefinitions::TABLE_POSITION_X; // lateralPos;
	mat->Element[1][3] += SystemGeometryDefinitions::TABLE_POSITION_Y; // longitudinalPos;
	mat->Element[2][3] += SystemGeometryDefinitions::TABLE_POSITION_Z; // verticalPos;

	theMeshAssemblies[streamNumber]->SetUserMatrix(mat);

	mat->Delete();

	// trigger position update for other views
	updateMeshPositionsFromWindow(streamNumber);
}


void OverlayScene::loadDefaultMeshPosition(MESH defaultOrientation)
{
	vtkTransform* t = vtkTransform::New();
	double x = (26-40), y = (574-525), z = (855-867);

	switch (defaultOrientation) {
	case MR_ITK:
		/* The patient lies head-first in the CT, on his/her back.
		In X-Ray, the patient is standing upwards, facing
		the X-Ray sources (at least in the 3D view of this program).
		Thus, for registration of MR to X-Ray, the patient must
		'stand up' (then his back faces the X-Ray sources)
		and then 'turn around.'*/
		t->RotateX(-90); // turn patient head-up to correlate CT and XRAy CS
		t->RotateZ(180); // make patient turn around because x-axis is flipped in DICOM Visualizer		
		break;

	case CT_PHILIPS:
		/// here is the case with converted XML
		t->RotateX(-90); // for CT and 1.5T			
		break;


	case MR_PHILIPS:
		/// here is the case with converted XML
		t->RotateX(180);
		break;

	case CT_ITK:		
		/////here is the case for the ITK-SNAP mesh
		t->RotateX(-90);
		t->RotateY(180);
		t->RotateX(180);
		break;	

	default:
		cerr << "Undefined parameter in loadDefaultMeshPosition!" << endl;
	}
		

	theMeshAssemblies[2]->SetUserMatrix(t->GetMatrix());
	t->Delete();

	updateMeshPositionsFromWindow(2);

}

void OverlayScene::setMRInputFileForMesh(int meshOrientation)
{
	orientation = meshOrientation;
}

int OverlayScene::getMRInputFileForMesh()
{
	return orientation;
}

void OverlayScene::addOverlayMesh(string file)
{
	
	vtkPolyDataReader* meshReader = vtkPolyDataReader::New();
	theMeshReaders.push_back(meshReader);
	meshReader->SetFileName(file.c_str());
	//meshReader->Print(cout);
	meshReader->Update();
	vtkPolyData* data = meshReader->GetOutput();


		double reg[3];
		theMRVisualizer->getImagePositionPatient(reg);

		double bounds[6], position[3];
		theMRVisualizer->getVolumeBounds(bounds);
		double spacing = theMRVisualizer->getSliceSpacing();

		position[0] = reg[0] + (bounds[1] - bounds[0]) / 2.0;
		position[1] = reg[1] + (bounds[3] - bounds[2]) / 2.0;
		position[2] = -reg[2] - spacing*(bounds[5] - bounds[4]) / 2.0;

		for (int i = 0; i < (outputWindows+1); ++i) { // for each window

		vtkTransformFilter* filter = vtkTransformFilter::New();
		theMeshTransformFilters[i].push_back(filter);
		filter->SetInputConnection(meshReader->GetOutputPort());
		filter->SetTransform(theRegistrationTransformers[i]);

		vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
		theMeshMappers[i].push_back(mapper);
		mapper->SetInputConnection(filter->GetOutputPort());

		vtkActor* actor = vtkActor::New();
		theMeshActors[i].push_back(actor);
		actor->SetMapper(mapper);
		actor->GetProperty()->SetColor(meshColor);
		actor->GetProperty()->SetOpacity(meshOpacity);

			if (orientation == 3)
			{
				actor->SetPosition(position);	// shifted origin+reg
			}

			theMeshAssemblies[i]->AddPart(actor);
		}	

		if (theMeshReaders.size() == 1)
		{
			if (orientation == 0)
			{
				loadDefaultMeshPosition(MR_PHILIPS);	// correct orientation AP is needed for correct calculation and display of the angles
			}

			if (orientation == 1)
			{
				loadDefaultMeshPosition(MR_ITK);	// correct orientation AP is needed for correct calculation and display of the angles
			}
			if (orientation == 2)
			{
				loadDefaultMeshPosition(CT_PHILIPS);	// correct orientation AP is needed for correct calculation and display of the angles
			}
			if (orientation == 3)
			{
				loadDefaultMeshPosition(CT_ITK);	// correct orientation AP is needed for correct calculation and display of the angles
			}
			
		}
		

	/*	vtkTransform* t = vtkTransform::New();
		t->Translate(-position[0], -position[1], -position[2]);
		vtkMatrix4x4* matrix = vtkMatrix4x4::New();
		theMeshAssemblies[2]->SetUserMatrix(t->GetMatrix());*/

	renWin3D->Render();
	for (int i = 0; i < inputChannels; ++i) {
		renWinXRAY[i]->Render();
	}

	renWinMainXRAY->Render();
}



void OverlayScene::updateMeshPositionsFromWindow(int windowNumber) {
	/* This method applies the user transformation for the given window
	to the other two windows, using the known BiplaneGeometry.
	It is assumed that theMeshAssemblies[i]->GetMatrix() is Identity, i.e.
	Orientation, Position and Origin of the assemblies are set to the default values.
	*/
	vtkMatrix4x4* sourceUserMatrixMain;
	vtkMatrix4x4* sourceUserMatrix;
	vtkMatrix4x4* transformationMatrix;
	vtkMatrix4x4* resultMatrix;
	vtkMatrix4x4* resultMatrixMain;
	vtkMatrix4x4* sourceUserMatrixUpperRef;
	vtkMatrix4x4* sourceUserMatrixLowerRef;

	double RefX, RefY, RefZ;
	double x, y, z;
	double RefX0, RefY0, RefZ0, RefX1, RefY1, RefZ1;

	switch (windowNumber) {
	case 0: // update from left XRAY view
			// make a copy of sourceUserMatrix , because some elements are changed
		sourceUserMatrix = vtkMatrix4x4::New();
		sourceUserMatrix->DeepCopy(theMeshAssemblies[0]->GetUserMatrix());

		sourceUserMatrix->Element[2][3] = sourceUserMatrix->Element[2][3] + theBiplaneGeometry.firstSystem.getIsoCenterToSourceDistance();
	
		resultMatrix = vtkMatrix4x4::New();
		resultMatrixMain = vtkMatrix4x4::New();

		// from first source coordinate system to patient coordinate system
		transformationMatrix = theBiplaneGeometry.firstSystem.getSourceToPatientMatrix();
		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrix, resultMatrix);

			theBiplaneGeometry.firstSystem.getTablePositionInWC(x, y, z);

			resultMatrix->Element[0][3] = resultMatrix->Element[0][3] - x;
			resultMatrix->Element[1][3] = resultMatrix->Element[1][3] - y;
			resultMatrix->Element[2][3] = resultMatrix->Element[2][3] - z;

		transformationMatrix->Delete();

		sourceUserMatrixMain = vtkMatrix4x4::New();
		sourceUserMatrixMain->DeepCopy(resultMatrix);
	
			theBiplaneGeometry.mainSystem.getTablePositionInWC(x, y, z);

			sourceUserMatrixMain->Element[0][3] = sourceUserMatrixMain->Element[0][3] + x;
			sourceUserMatrixMain->Element[1][3] = sourceUserMatrixMain->Element[1][3] + y;
			sourceUserMatrixMain->Element[2][3] = sourceUserMatrixMain->Element[2][3] + z;

		// from patient coordinate system to Main Stream coordinate system
		transformationMatrix = theBiplaneGeometry.mainSystem.getPatientToSourceMatrix();
		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrixMain, resultMatrixMain);
		sourceUserMatrixMain->Delete();
		transformationMatrix->Delete();

		// from patient coordinate system to second source coordinate system
		transformationMatrix = theBiplaneGeometry.secondSystem.getPatientToSourceMatrix();

			theBiplaneGeometry.secondSystem.getTablePositionInWC(x, y, z);

			resultMatrix->Element[0][3] = resultMatrix->Element[0][3] + x;
			resultMatrix->Element[1][3] = resultMatrix->Element[1][3] + y;
			resultMatrix->Element[2][3] = resultMatrix->Element[2][3] + z;
		//}
		vtkMatrix4x4::Multiply4x4(transformationMatrix, resultMatrix, resultMatrix);
		transformationMatrix->Delete();
		
		// z is now distance from source, but we need distance from iso center
		resultMatrix->Element[2][3] = (resultMatrix->Element[2][3] - theBiplaneGeometry.secondSystem.getIsoCenterToSourceDistance());

		// set right view
		theMeshAssemblies[1]->PokeMatrix(resultMatrix);
		resultMatrix->Delete();

		// z is distance from source, but we need distance from iso center
		resultMatrixMain->Element[2][3] = (resultMatrixMain->Element[2][3] - theBiplaneGeometry.mainSystem.getIsoCenterToSourceDistance());

		// set Main Stream view
		theMeshAssemblies[3]->PokeMatrix(resultMatrixMain);
		resultMatrixMain->Delete();

		// update 3D view:
		// here, transformation is source -> patient coordinate transformation
		transformationMatrix = theBiplaneGeometry.firstSystem.getSourceToPatientMatrix();
		resultMatrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrix, resultMatrix);

			theBiplaneGeometry.firstSystem.getTablePositionInWC(x, y, z);

			resultMatrix->Element[0][3] = resultMatrix->Element[0][3] - x;
			resultMatrix->Element[1][3] = resultMatrix->Element[1][3] - y;
			resultMatrix->Element[2][3] = resultMatrix->Element[2][3] - z;

		sourceUserMatrix->Delete(); // delete the copy created above

		theMeshAssemblies[2]->PokeMatrix(resultMatrix);
		transformationMatrix->Delete();
		resultMatrix->Delete();

		rendererXRAY[0]->ResetCameraClippingRange();
		break;
	case 1: // update from right XRAY view
			// make a copy of sourceUserMatrix , because some elements are changed
		sourceUserMatrix = vtkMatrix4x4::New();
		sourceUserMatrix->DeepCopy(theMeshAssemblies[1]->GetUserMatrix());

		sourceUserMatrix->Element[2][3] = sourceUserMatrix->Element[2][3] + theBiplaneGeometry.secondSystem.getIsoCenterToSourceDistance();
		

		resultMatrix = vtkMatrix4x4::New();
		sourceUserMatrixMain = vtkMatrix4x4::New();
		resultMatrixMain = vtkMatrix4x4::New();

		// from second source coordinate system to patient coordinate system
		transformationMatrix = theBiplaneGeometry.secondSystem.getSourceToPatientMatrix();
		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrix, resultMatrix);

			theBiplaneGeometry.secondSystem.getTablePositionInWC(x, y, z);

			resultMatrix->Element[0][3] = resultMatrix->Element[0][3] - x;
			resultMatrix->Element[1][3] = resultMatrix->Element[1][3] - y;
			resultMatrix->Element[2][3] = resultMatrix->Element[2][3] - z;

		transformationMatrix->Delete();

		sourceUserMatrixMain = vtkMatrix4x4::New();
		sourceUserMatrixMain->DeepCopy(resultMatrix);
	
			theBiplaneGeometry.mainSystem.getTablePositionInWC(x, y, z);

			sourceUserMatrixMain->Element[0][3] = sourceUserMatrixMain->Element[0][3] + x;
			sourceUserMatrixMain->Element[1][3] = sourceUserMatrixMain->Element[1][3] + y;
			sourceUserMatrixMain->Element[2][3] = sourceUserMatrixMain->Element[2][3] + z;
	
		// from patient coordinate system to Main Stream coordinate system
		transformationMatrix = theBiplaneGeometry.mainSystem.getPatientToSourceMatrix();
		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrixMain, resultMatrixMain);
		sourceUserMatrixMain->Delete();
		transformationMatrix->Delete();

		// from patient coordinate system to first source coordinate system
		transformationMatrix = theBiplaneGeometry.firstSystem.getPatientToSourceMatrix();

			theBiplaneGeometry.firstSystem.getTablePositionInWC(x, y, z);

			resultMatrix->Element[0][3] = resultMatrix->Element[0][3] + x;
			resultMatrix->Element[1][3] = resultMatrix->Element[1][3] + y;
			resultMatrix->Element[2][3] = resultMatrix->Element[2][3] + z;

		vtkMatrix4x4::Multiply4x4(transformationMatrix, resultMatrix, resultMatrix);
		transformationMatrix->Delete();		

		// z is now distance from source, but we need distance from iso center
		resultMatrix->Element[2][3] = (resultMatrix->Element[2][3] - theBiplaneGeometry.firstSystem.getIsoCenterToSourceDistance());

		// set left view
		theMeshAssemblies[0]->PokeMatrix(resultMatrix);
		resultMatrix->Delete();

		// z is distance from source, but we need distance from iso center
		resultMatrixMain->Element[2][3] = (resultMatrixMain->Element[2][3] - theBiplaneGeometry.mainSystem.getIsoCenterToSourceDistance());

		// set Main Stream view
		theMeshAssemblies[3]->PokeMatrix(resultMatrixMain);
		resultMatrixMain->Delete();

		// update 3D view:
		// here, transformation is source -> patient coordinate transformation
		transformationMatrix = theBiplaneGeometry.secondSystem.getSourceToPatientMatrix();
		resultMatrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrix, resultMatrix);
		
			theBiplaneGeometry.secondSystem.getTablePositionInWC(x, y, z);

			resultMatrix->Element[0][3] = resultMatrix->Element[0][3] - x;
			resultMatrix->Element[1][3] = resultMatrix->Element[1][3] - y;
			resultMatrix->Element[2][3] = resultMatrix->Element[2][3] - z;

		sourceUserMatrix->Delete(); // delete the copy created above

		theMeshAssemblies[2]->PokeMatrix(resultMatrix);
		transformationMatrix->Delete();
		resultMatrix->Delete();

		rendererXRAY[1]->ResetCameraClippingRange();		
		break;

	case 2: // update from 3D view
		sourceUserMatrix = vtkMatrix4x4::New();
		sourceUserMatrix->DeepCopy(theMeshAssemblies[2]->GetUserMatrix());

		// left reference view:
		transformationMatrix = theBiplaneGeometry.firstSystem.getPatientToSourceMatrix();

		resultMatrix = vtkMatrix4x4::New();
		sourceUserMatrixUpperRef = vtkMatrix4x4::New();
		sourceUserMatrixUpperRef->DeepCopy(sourceUserMatrix);

			theBiplaneGeometry.firstSystem.getTablePositionInWC(x, y, z);

			sourceUserMatrixUpperRef->Element[0][3] = sourceUserMatrixUpperRef->Element[0][3] + x;
			sourceUserMatrixUpperRef->Element[1][3] = sourceUserMatrixUpperRef->Element[1][3] + y;
			sourceUserMatrixUpperRef->Element[2][3] = sourceUserMatrixUpperRef->Element[2][3] + z;

		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrixUpperRef, resultMatrix);
				
		// z is distance from source, but we need distance from iso center
		resultMatrix->Element[2][3] = (resultMatrix->Element[2][3] - theBiplaneGeometry.firstSystem.getIsoCenterToSourceDistance());
		
		theMeshAssemblies[0]->PokeMatrix(resultMatrix);

		transformationMatrix->Delete();
		resultMatrix->Delete();
		sourceUserMatrixUpperRef->Delete();

		// right reference view:
		transformationMatrix = theBiplaneGeometry.secondSystem.getPatientToSourceMatrix();	

		resultMatrix = vtkMatrix4x4::New();

		sourceUserMatrixLowerRef = vtkMatrix4x4::New();
		sourceUserMatrixLowerRef->DeepCopy(sourceUserMatrix);

			theBiplaneGeometry.secondSystem.getTablePositionInWC(x, y, z);

			sourceUserMatrixLowerRef->Element[0][3] = sourceUserMatrixLowerRef->Element[0][3] + x;
			sourceUserMatrixLowerRef->Element[1][3] = sourceUserMatrixLowerRef->Element[1][3] + y;
			sourceUserMatrixLowerRef->Element[2][3] = sourceUserMatrixLowerRef->Element[2][3] + z;

		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrixLowerRef, resultMatrix);

		// z is distance from source, but we need distance from iso center
		resultMatrix->Element[2][3] = (resultMatrix->Element[2][3] - theBiplaneGeometry.secondSystem.getIsoCenterToSourceDistance());

		theMeshAssemblies[1]->PokeMatrix(resultMatrix);
		transformationMatrix->Delete();
		resultMatrix->Delete();
		sourceUserMatrixLowerRef->Delete();

		// update Main Stream XRAY view
		transformationMatrix = theBiplaneGeometry.mainSystem.getPatientToSourceMatrix();

		resultMatrix = vtkMatrix4x4::New();

			theBiplaneGeometry.mainSystem.getTablePositionInWC(x, y, z);

			sourceUserMatrix->Element[0][3] = sourceUserMatrix->Element[0][3] + x;
			sourceUserMatrix->Element[1][3] = sourceUserMatrix->Element[1][3] + y;
			sourceUserMatrix->Element[2][3] = sourceUserMatrix->Element[2][3] + z;

		vtkMatrix4x4::Multiply4x4(transformationMatrix, sourceUserMatrix, resultMatrix);
		
		
		// z is distance from source, but we need distance from iso center
		resultMatrix->Element[2][3] = (resultMatrix->Element[2][3] - theBiplaneGeometry.mainSystem.getIsoCenterToSourceDistance());

		theMeshAssemblies[3]->PokeMatrix(resultMatrix);
		transformationMatrix->Delete();
		sourceUserMatrix->Delete();
		resultMatrix->Delete();		

		rendererXRAY[0]->ResetCameraClippingRange();
		rendererXRAY[1]->ResetCameraClippingRange();
		rendererMainXRAY->ResetCameraClippingRange();
		break;
	
	default:
		cout << "OverlayScene::updateMeshPositionsFromWindow(...) called with invalid window number: " << windowNumber << endl;
	}

	if (activeFilterIsBiplane) {
		theBiplaneFilter->setCurrentAssemblyPositionAsNullPositions();
	}
	else {

			theMotionCorrections->setCurrentAssemblyPositionAsNullPositions();		

	}
	theRegistrationMarkers->updateLabelPositions(); // because those are relative to the mesh position
	
	if (!isLive|| ! isFramegrabber) {
		renWin3D->Render();
		renWinMainXRAY->Render();
	}
	for (int i = 0; i < inputChannels; ++i) {
		renWinXRAY[i]->Render();
	}
	
}


void OverlayScene::removeOverlayMesh(int meshNumber)
{
	if (theMeshReaders.empty()) return; //nothing to remove

	if (meshNumber == -1) meshNumber = theMeshActors[0].size() - 1; // -1 is a flag to delete the mesh which was added last
	if (meshNumber < 0 || meshNumber >= theMeshActors[0].size()) return; // invalid number


	vtkPolyDataReader* reader = theMeshReaders[meshNumber];
	reader->Delete();

	vector<vtkPolyDataReader*>::iterator itReader = theMeshReaders.begin();
	itReader += meshNumber;
	theMeshReaders.erase(itReader);

	for (unsigned int i = 0; i < (outputWindows+1); ++i) {

		theMeshAssemblies[i]->RemovePart(theMeshActors[i][meshNumber]);

		theMeshActors[i][meshNumber]->Delete();
		theMeshMappers[i][meshNumber]->Delete();
		theMeshTransformFilters[i][meshNumber]->Delete();

		// Erase the objects from the vectors
		vector< vtkActor*>::iterator itActor = theMeshActors[i].begin();
		itActor += meshNumber;
		theMeshActors[i].erase(itActor);

		vector< vtkPolyDataMapper*>::iterator itMapper = theMeshMappers[i].begin();
		itMapper += meshNumber;
		theMeshMappers[i].erase(itMapper);

		vector<vtkTransformFilter*>::iterator itTranformer = theMeshTransformFilters[i].begin();
		itTranformer += meshNumber;
		theMeshTransformFilters[i].erase(itTranformer);

	}

	renWin3D->Render();

	for (int i = 0; i < inputChannels; i++)
	{
		renWinXRAY[i]->Render();
	}
	renWinMainXRAY->Render();
}

vector<const char*> OverlayScene::getMeshFileNames()
{
	vector<const char*> fileNames;
	for (unsigned int i = 0; i < theMeshReaders.size(); ++i) {
		fileNames.push_back(theMeshReaders[i]->GetFileName());
	}
	return fileNames;
}


vector<const char*> OverlayScene::getXRAYFileNames()
{
	vector<const char*> fileNames;
	for (unsigned int i = 0; i < theXRAYReaders.size(); ++i) {
		const char* file = theXRAYReaders[i]->getInputFile();
		fileNames.push_back(file);
	}

	return fileNames;
}

const char* OverlayScene::getXRAYMainFileName()
{
	if (readerMainXRAY != NULL)
	{
		return readerMainXRAY->getInputFile();
	}
	else
		return 0;

}

void OverlayScene::saveImage(const char* path, vtkRenderWindow* renWin)
{

	// Screenshot  
	vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter =
		vtkSmartPointer<vtkWindowToImageFilter>::New();
	// ----------------------------- vtk 6.2.0
	//windowToImageFilter->SetMagnification(3); //set the resolution of the output image (3 times the current resolution of vtk render window)
	// ----------------------------- END
	// ----------------------------- vtk 8.2.0
	windowToImageFilter->SetScale(3); //set the resolution of the output image (3 times the current resolution of vtk render window)
	// ----------------------------- END
	windowToImageFilter->SetInputBufferTypeToRGBA(); //also record the alpha (transparency) channel
	windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer


	/*vtkTIFFWriter *writer = vtkTIFFWriter::New();
	writer->SetCompressionToNoCompression();*/
	vtkPNGWriter *writer = vtkPNGWriter::New();

	char buf[1024];

	windowToImageFilter->SetInput(renWin);
	windowToImageFilter->Update();
	writer->SetInputConnection(windowToImageFilter->GetOutputPort());
	sprintf(buf, path);
	writer->SetFileName(buf);
	writer->Write();

	writer->Delete();
}


void OverlayScene::saveCurrentXRayImages(const char* pathpattern)
{

	// Screenshot  
	vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter =
		vtkSmartPointer<vtkWindowToImageFilter>::New();
	// ----------------------------- vtk 6.2.0
	//windowToImageFilter->SetMagnification(3); //set the resolution of the output image (3 times the current resolution of vtk render window)
	// ----------------------------- END
	// ----------------------------- vtk 8.2.0
	windowToImageFilter->SetScale(3); //set the resolution of the output image (3 times the current resolution of vtk render window)
	// ----------------------------- END
	windowToImageFilter->SetInputBufferTypeToRGBA(); //also record the alpha (transparency) channel
	windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer


	/*vtkTIFFWriter *writer = vtkTIFFWriter::New();
	writer->SetCompressionToNoCompression();*/
	vtkPNGWriter *writer = vtkPNGWriter::New();

	char buf[1024];

	int i = 0;

	for (vector<XRAYReader*>::iterator it = theXRAYReaders.begin(); it != theXRAYReaders.end(); ++it)
	{

		windowToImageFilter->SetInput(renWinXRAY[i]);
		windowToImageFilter->Update();
		writer->SetInputConnection(windowToImageFilter->GetOutputPort());
		sprintf(buf, pathpattern, i);
		writer->SetFileName(buf);
		writer->Write();
		++i;
	}	

	if (readerMainXRAY != NULL)
	{
		windowToImageFilter->SetInput(renWinMainXRAY);
		//windowToImageFilter->Update();
		writer->SetInputConnection(windowToImageFilter->GetOutputPort());
		sprintf(buf, pathpattern, 2);
		writer->SetFileName(buf);
		writer->Write();
	}

	writer->Delete();
}

void OverlayScene::setMeshColor(double color[3], int index)
{	
	for (unsigned int j = 0; j < (outputWindows+1); ++j) {
		theMeshActors[j][index]->GetProperty()->SetColor(color);
	}

	meshColor[0] = color[0];
	meshColor[1] = color[1];
	meshColor[2] = color[2];

	renWin3D->Render();
	for (int i = 0; i < inputChannels; i++)
	{
		renWinXRAY[i]->Render();
	}
	renWinMainXRAY->Render();
}

void OverlayScene::getMeshColor(double color[3], int index)
{
	
	theMeshActors[0][index]->GetProperty()->GetColor(color);
	
}

void OverlayScene::giveMeshColorToScene(double color[3])
{

	meshColor[0] = color[0];
	meshColor[1] = color[1];
	meshColor[2] = color[2];

}

void OverlayScene::giveMeshOpacityToScene(double opacity)
{

	meshOpacity = opacity;

}

void OverlayScene::setMeshOpacity(double opacity, int index)
{
	for (unsigned int j = 0; j < (outputWindows+1); ++j) {
		theMeshActors[j][index]->GetProperty()->SetOpacity(opacity);
	}

	meshOpacity = opacity;

	renWin3D->Render();
	for (int i = 0; i < inputChannels; i++)
	{
		renWinXRAY[i]->Render();
	}
	renWinMainXRAY->Render();
}

double OverlayScene::getMeshOpacity(int index)
{	
	
	return theMeshActors[0][index]->GetProperty()->GetOpacity();	

}

void OverlayScene::setMeshVisibility(bool visible)
{

	for (unsigned int i = 0; i < (outputWindows+1); ++i) {
		for (unsigned int j = 0; j < theMeshActors[i].size(); ++j) {
			theMeshActors[i][j]->SetVisibility(visible);
		}
	}

	renWin3D->Render();
	for (int i = 0; i < inputChannels; i++)
	{
		renWinXRAY[i]->Render();
	}
	renWinMainXRAY->Render();
}

bool OverlayScene::getMeshVisibility()
{
	bool visible;
	visible = theMeshActors[0][0]->GetVisibility();
	return visible;
}

void OverlayScene::setXRayVisibilityIn3dWindow(bool visible)
{
	for (unsigned int i = 0; i < theXRAYActors3D.size(); ++i) {
		// Do not use SetVisibility(...) here, because visibility=true
		// is required for a working VTK pipeline update mechanism.
		if (visible) theXRAYActors3D[i]->SetOpacity(1.0);
		else theXRAYActors3D[i]->SetOpacity(0.0);
	}

	// Also hide/show projection lines.
	lineActor1->SetVisibility(visible);
	lineActor2->SetVisibility(visible);

	renWin3D->Render();
}

void OverlayScene::addMarkerPoint(unsigned int streamNumber, double pos[3])
{
	// z-position must be on the detector
	pos[2] = theXRAYActors[streamNumber]->GetPosition()[2];

	// get position relative to current motion correction
	for (int i = 0; i < 3; ++i) {
		pos[i] -= theMarkerPointAssemblies[streamNumber]->GetUserMatrix()->GetElement(i, 3);
	}

	theMarkerPoints[streamNumber]->setStartPosition(pos);
	theMarkerPoints[streamNumber]->addPoint();
}

void OverlayScene::removeNearestMarkerPoint(unsigned int streamNumber, double pos[3])
{
	// z-position must be on the detector
	pos[2] = theXRAYActors[streamNumber]->GetPosition()[2];

	theMarkerPoints[streamNumber]->deletePointNextTo(pos);
}

SceneLabeling* OverlayScene::getMarkerLabeling(unsigned int streamNumber)
{
	return theMarkerPoints[streamNumber];
}

double OverlayScene::reconstruct3dPointWithSkewLinesIntersection(const double p1[2], const double p2[2], double res[3])
{
	double distance = theBiplaneGeometry.reconstruct3dPointWithSkewLinesIntersection(p1, p2, res);
	return distance;
}

SceneLabeling* OverlayScene::getRegistrationMarkers()
{
	return theRegistrationMarkers;
}

void OverlayScene::do3d3dRegistration()
{

	if (theMarkerPoints[2]->getNumberOfPoints() != theRegistrationMarkers->getNumberOfPoints()) return;

	// Make mesh assembly to id, because the registration marker
	// positions are relative to the assembly.
	// This avoids a coordination system transformation.
	theMeshAssemblies[2]->GetMatrix()->Identity();

	double pos[3];

	// source points: set by the user in the mesh/MR volume
	vtkPoints* sourcePoints = vtkPoints::New();
	for (int i = 0; i < theRegistrationMarkers->getNumberOfPoints(); ++i) {
		theRegistrationMarkers->getPointPosition(i, pos);
		sourcePoints->InsertNextPoint(pos);
	}

	// target points: reconstructed 3D points from markers in X-ray
	vtkPoints* targetPoints = vtkPoints::New();
	for (int i = 0; i < theMarkerPoints[2]->getNumberOfPoints(); ++i) {
		theMarkerPoints[2]->getPointPosition(i, pos);
		targetPoints->InsertNextPoint(pos);
	}

	vtkLandmarkTransform* transform = vtkLandmarkTransform::New();
	transform->SetSourceLandmarks(sourcePoints);
	transform->SetTargetLandmarks(targetPoints);
	transform->SetModeToRigidBody();
	transform->Update();


	double p1[3];
	for (int i = 0; i < theRegistrationMarkers->getNumberOfPoints(); ++i) {
		sourcePoints->GetPoint(i, p1);
		double pos1[4] = {
			p1[0],
			p1[1],
			p1[2],
			1
		};	

	}

	// set in 3D window and update other views
	theMeshAssemblies[2]->PokeMatrix(transform->GetMatrix());
	updateMeshPositionsFromWindow(2);

	transform->Delete();
	sourcePoints->Delete();
	targetPoints->Delete();
}


void OverlayScene::renderMain() {
	renWinMainXRAY->Render();
}

void OverlayScene::renderRef(int index) {
	renWinXRAY[index]->Render();
}

void OverlayScene::clearRenderer(int index) {
	if (index == 0 || index == 1) {
		theXRAYActors[index]->SetVisibility(false);
		//rendererXRAY[index]->Clear();
		rendererXRAY[index]->Render();
	}
	else if (index == 2) {
		rendererMainXRAY->Clear();
		rendererMainXRAY->Render();
	}

}

void OverlayScene::visualizeActors(int index, bool visualize) {
	if (index == 0 || index == 1) {
		theXRAYActors[index]->SetVisibility(visualize);
		//rendererXRAY[index]->Render();
	}
	else if (index == 2) {
		actorMainXRAY->SetVisibility(visualize);
		rendererMainXRAY->Render();
	}

}

void OverlayScene::createPatientdirectory() {
	// current time for Foldername
	time_t t;
	struct tm *ts;
	t = time(NULL);
	ts = localtime(&t);

	int	Y = ts->tm_year + 1900;
	int	M = ts->tm_mon + 1;
	int	D = ts->tm_mday;
	int	h = ts->tm_hour;
	int	m = ts->tm_min;
	int s = ts->tm_sec;

	// int2char
	char space[] = "0";
	char Year[10];
	sprintf(Year, "%d", Y);
	if(M < 10 ) strcat(Year, space);
	char Month[10];
	sprintf(Month, "%d", M);
	if (D < 10) strcat(Month, space);
	char Day[10];
	sprintf(Day, "%d", D);
	if (h < 10) strcat(Day, space);
	char hour[10];
	sprintf(hour, "%d", h);
	if (m < 10) strcat(hour, space);
	char min[10];
	sprintf(min, "%d", m);
	if (s < 10) strcat(min, space);
	char sec[10];
	sprintf(sec, "%d", s);
	char start[] = "..\\";
	char end[] = "\\";



	// connect chars
	patientDir = new char[strlen(start) + strlen(Year) + strlen(Month) + strlen(Day) + strlen(hour) + strlen(min) + strlen(sec) + strlen(end) + 1];
	strcpy(patientDir, start);
	strcat(patientDir, Year);
	strcat(patientDir, Month);
	strcat(patientDir, Day);
	strcat(patientDir, hour);
	strcat(patientDir, min);
	strcat(patientDir, sec);
	strcat(patientDir, end);

	CreateDirectory(patientDir, 0);
}

int OverlayScene::startClock(char* txt) {
	clock_t time1, time2;
	//double duration1;
	char* PD = patientDir;
	//char* txt = "grab.txt";
	char* file = new char[strlen(PD) + strlen(txt) + 1];
	strcpy(file, PD);
	strcat(file, txt);
	g.open(file, ios::out | ios::app);
	time1 = clock();

	return time1;
}

void OverlayScene::stopClock(int startTime) {
	clock_t time2 = clock();
	double duration = (double)(time2 - startTime);
	g << duration << endl;
	g.close();
}

void OverlayScene::reloadPipelineFramegrabber() {

	//================================================
	// Initialize pipeline for playing videos
	//================================================
	writer->Delete();
	writer0->Delete();
	writer1->Delete();
	writerBiplane.clear();


	generator->Delete();
	generator0->Delete();
	generator1->Delete();
	generatorBiplane.clear();

	DICOMreader->Delete();

	//================================================
	// Initialize pipeline for getGeometry
	//================================================
	//-----------------------Framegrabber----------------------

	/*for (size_t i = 0; i < inputChannels; i++)*/
	for (size_t i = 0; i < frameGrabbersNumber; i++)
	{
		theFramegrabbers[i]->ReleaseSystemResources();
		theFramegrabbers[i]->Delete();
		luminanceFilter[i]->Delete();
	}
	//delete panelFront;
	//delete panelLat;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//================================================
	// Initialize pipeline for playing videos
	//================================================
	mustWrite[0] = mustWrite[1] = false;
	writer = vtkDICOMWriter::New();
	//vtkDICOMWriter* writer0 = vtkDICOMWriter::New();
	//vtkDICOMWriter* writer1 = vtkDICOMWriter::New();
	writer0 = vtkDICOMWriter::New();
	writer1 = vtkDICOMWriter::New();
	writerBiplane.push_back(writer0);
	writerBiplane.push_back(writer1);

	generator = vtkDICOMCTGenerator::New();
	generator0 = vtkDICOMCTGenerator::New();
	generator1 = vtkDICOMCTGenerator::New();
	generatorBiplane.push_back(generator0);
	generatorBiplane.push_back(generator1);

	DICOMreader = XRAYReader::New();
	//-----------------------------------------------
	createPatientdirectory();
	biplaneSystem = false;


	//================================================
	// Initialize pipeline for getGeometry
	//================================================
	if (frameGrabbersNumber > 0)
	{
		delete panelFront;
		if (frameGrabbersNumber > 1)
		{
			delete panelLat;
		}
	}	

	//-----------------------Framegrabber----------------------
	int initialized = 0;
	/*for (size_t i = 0; i < inputChannels; i++)*/
	for (size_t i = 0; i < frameGrabbersNumber; i++)
	{
		theFramegrabbers[i] = vtkWin32VideoSource::New();
		theFramegrabbers[i]->SetDataSpacing(SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[i], SystemGeometryDefinitions::MILLIMETER_PER_PIXEL[i], 1);
		//theFramegrabbers[i]->SetOutputFormat(VTK_LUMINANCE); // funktioniert leider nicht in vtk8.2 -> luminancefilter
															 // Initialize wird sonst automatisch bei erstem grab() aufgerufen, evtl ueber die GUI vearaenderbar machen?
															 // theFramegrabbbers[0] wird gerendert
		theFramegrabbers[i]->Initialize();
		// test: is initialized?			
		initialized += theFramegrabbers[i]->GetInitialized(); // mindestens ein Framegrabber angeschlossen?

		luminanceFilter[i] = vtkImageLuminance::New();
		luminanceFilter[i]->SetInputConnection(theFramegrabbers[i]->GetOutputPort(0));
	}

	if (initialized == 1)
	{
		// Erkennung eines vom Framgrabber verursachten Shifts in horizontaler Richtung
		int shiftHorizontal0 = recognizeShift(0);

		// AdjustmentKorrektur eines vom Framgrabber verursachten Shifts in horizontaler Richtung
		// und Clipping der Namenszeile (hard gecodeter shift in verticaler Richtung zur Anonymisierung)
		clipFG(0, shiftHorizontal0, SystemGeometryDefinitions::CLIP_NAME);

		// initialize two panels with geometry values
		panelFront = new Panel(true, true, 0, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1200, 0);
		//panelLat = new Panel(true, true, 90, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1300, 0);

		// set frontal input to index 0; set lateral input to index 1
		testFramegrabberInitialization();
	}
	else if (initialized == 2) {
		// Erkennung eines vom Framgrabber verursachten Shifts in horizontaler Richtung
		int shiftHorizontal0 = recognizeShift(0);
		int shiftHorizontal1 = recognizeShift(1);

		// AdjustmentKorrektur eines vom Framgrabber verursachten Shifts in horizontaler Richtung
		// und Clipping der Namenszeile (hard gecodeter shift in verticaler Richtung zur Anonymisierung)
		clipFG(0, shiftHorizontal0, SystemGeometryDefinitions::CLIP_NAME);
		clipFG(1, shiftHorizontal1, SystemGeometryDefinitions::CLIP_NAME);

		// initialize two panels with geometry values
		panelFront = new Panel(true, true, 0, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1200, 0);
		panelLat = new Panel(true, true, 90, 0, SystemGeometryDefinitions::TABLE_POSITION_Y, SystemGeometryDefinitions::TABLE_POSITION_X, SystemGeometryDefinitions::TABLE_POSITION_Z, 1300, 0);

		// set frontal input to index 0; set lateral input to index 1
		testFramegrabberInitialization();
	}
	else {
		canReadLayout[0] = canReadLayout[1] = false;
		cout << "WARNING: No framegrabber detected" << endl;
	}

	liveIndex = 0;
	ofstream f;
}