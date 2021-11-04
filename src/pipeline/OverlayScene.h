#ifndef OVERLAYSCENE_H
#define OVERLAYSCENE_H


// VTK includes
#include <vtkSmartPointer.h>
class vtkImageResize;
// VTK classes
class vtkPolyDataReader;
class vtkSTLReader;
class vtkPolyData;
class vtkTransformFilter;
class vtkPolyDataMapper;
class vtkPolyDataNormals;
class vtkTransform;
class vtkAssembly;
class vtkRenderer;
class vtkRenderWindow;
class vtkImageActor;
class vtkImageReslice;
class vtkSphereSource;
class vtkImageMapper3D;
class vtkOpenGLImageSliceMapper;
class vtkImageSliceMapper;
class vtkImageShiftScale;
class vtkAxesActor;
class vtkActor;
class vtkLineSource;
class vtkChartXY;
class vtkContextActor;
class vtkWin32VideoSource;
class vtkImageLuminance;

class vtkXMLPolyDataReader;

//own includes
class DICOMVisualizer;
class MeshesDialog;
class XRAYReader;
class XRayViewer;
class SceneLabeling;
class MotionCorrectionFilter;
class BiplaneAlgorithm;

class AbstractFilterWidget;
//========================================================
//class vtkWin32VideoSourceModification;

class vtkImageViewer;
class vtkImageReader;
class vtkGDCMImageReader;
//class vtkImageData;
//class vtkDataSetMapper;
//class vtkActor;
//class vtkMapper;
//class vtkRenderer;
//class VideoCapture;
class vtkRenderWindowInteractor;
class vtkDICOMWriter;
class vtkDICOMCTGenerator;
class vtkDICOMReader;
class vtkDICOMMetaData;
class Panel;

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/core/types.hpp>
//========================================================
#include "BiplaneGeometry.h"
#include "MoveMeshInteractionCommand.h"
#include "FilterAlgorithmFactory.h"
#include "BiplaneFilterAlgorithmFactory.h"

#include <vector>
#include <string>

#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
using namespace std;
//using namespace std;

/** This class manages the VTK visualization pipeline
 *   and provides access to several vtkRenderWindow instances of (nearly) the same scene.
 *   Two inputs are expected.
 *
 *   An example call for the singleton use (puts a tube into the 3D window):
 *   \code
 *   OverlayScene::GetInstance()->getRenderWindow(2)->
 *      GetRenderers()->GetFirstRenderer()->AddActor(tubeActor);
 *   \endcode
 *
 */
class OverlayScene {
public:

	static OverlayScene* New(int numberofChannels, int numberofWindows, int numberOfFramegrabbers);
	static OverlayScene* GetInstance();

	~OverlayScene();
	enum MESH { MR_PHILIPS, MR_ITK, CT_PHILIPS, CT_ITK, MR_PHILIPS_ax};

public:

	void reloadPipelineFramegrabber();

	void setBiplaneGeometry(unsigned int streamNumber, int SID, int sourcePatDist, double primAngle, double secAngle, 
		int lateralPos, int longitudinalPos, int verticalPos, double mmPerPxl, int imageSizeX, int imageSizeY);
	void setMainGeometryByDICOM(int SID, int sourcePatDist, double primAngle, double secAngle, int lateralPos, int longitudinalPos, int verticalPos, double mmPerPixel, int imageSizeX, int imageSizeY);
	void setFramegrabberGeometry(int index, int SID, double primAngle, double secAngle, int lateralPos, int longitudinalPos, int verticalPos, double FDoderMMPerPxl, bool play);
	void setFramegrabberGeometryLive(int index);
	void setFramegrabberGeometryBiplaneLive(int index);
	//void setInputFromFramegrabber(unsigned int streamNumber, const char* file);
	void setGeometryFromFramegrabber(unsigned int streamNumber, int SID, double primAngle, double secAngle, int lateralPos, int longitudinalPos, int verticalPos, double mmPerPxl);
	
	void viewXRayDirection(int index);

	/// This method transforms the 2d(!) coordinates using the current XR actor in 2d view.
	void transformFromWorldToImageCoordinates(unsigned int streamNumber, const double worldCoords[2], double imageCoords[2]);

	void loadDefaultMeshPosition(MESH defaultOrientation);
	bool loadDefaultMeshPositionBool;
	/// Apply the user's mesh transformation to the other windows, according to the geomerty.
	void updateMeshPositionsFromWindow(int windowNumber);
	/// Set the 2D Window to the position expected by registration.
	void set2DWindowCameraToDefinedPosition();
	void setUserMatrix(unsigned int window, double matrix[4][4]); /// Set the matrix directly
	void getUserMatrix(unsigned int window, double matrix[4][4]); /// Get the matrix directly
	void saveUserMatrix(unsigned int streamNumber, const char* filename); /// Save the assembly's UserMatrix.
	void loadUserMatrix(unsigned int streamNumber, const char* filename);/// Set the assembly's UserMatrix from the given file.

	void SetMRVisualizer(string file);
	void SetCTVisualizer(string file, string folder, int meshOrientation);
	DICOMVisualizer* GetMRVisualizer();
	//XRayViewer* GetXRayViewer() {return theXRayViewer};
	void SetRenderer3D(vtkRenderer* renderer);
	void SetRenderers2D(vtkRenderer* renderers[3]);
	vtkRenderer* const GetRenderer3D() { return renderer3D; };
	vtkRenderer* GetRenderers2D(unsigned int number) const; //o, 1, 2 for respectively X, Y, Z planes of MR images
	void SetAxesVisibility(bool visible);

	vtkRenderWindow* getXRAYRenderWindow(unsigned int number) const;
	vtkRenderer* getXRAYRenderer(unsigned int number) const;
	void initXRAYWindows();
	void setMainXRAYInputToFile(const char* file);
	void setInputToFile(const char* files, int streamNumber);
	void disableECG(unsigned int streamNumber);
	void loadECGFile(unsigned int streamNumber, const char* filename, int frameNumber);
	void setFrameForPeak(unsigned int streamNumber);
	void getFrameForPeak(unsigned int streamNumber, int& frame);
	void setECGFrame(unsigned int streamNumber, int frameNumber);
	vector<const char*> getXRAYFileNames();
	const char* getXRAYMainFileName();
	int getNumberOfFrames(int index); // get the number of frames for respective theXRAYReaders[0 - FirstStream, [1] - Second stream]
	void showFrame(int index, int frame); /// show respective frame for the given Stream [index [0] - FirstStream, index [1] - Second stream) with the given frame index

	void generateUndoRedoCommandStack(MoveMeshInteractionCommand* cmd);

	/// pathpattern must contain one "%d" (for sprintf),
	/// as one image is written for each input stream+output
	void saveCurrentXRayImages(const char* pathpattern);
	// general save image function for all images 3D scene, and xray views independent on if any image is available
	void saveImage(const char* path, vtkRenderWindow* renWin);

	//////////////////////////////////////////////////////////////////
	//void startPreview(); // live direkt von framegrabber
	//void stopPreview();
	void live();// live direkt von framegrabber
	void startRecord(int run); /// start recording
	void record(int value); ///record for each frame
	void stopRecord(); /// stop recording/playing
	void play(int framenumber, char* dir); /// play the buffer content
	void playReferenceStream(int framenumber, char* dir, int index); /// play content of chosen folder

	bool isFramegrabber;
	bool switchSystems_FG;
	bool switchSystems_sys;
	bool alreadyConstructedPipeline[3];
	bool alreadyConstructedPipelineDICOM[3];
	bool isRecording;
	bool isLive;
	bool mustWrite[2];
	bool canReadLayout[2];
	bool frontalInput;
	bool lateralInput;
	//-------------biplanar-------------------
	void liveBiplane(int index);
	void startRecordBiplane(int run, int index); /// start recording
	void recordBiplane(int value, int index, int currentRun); ///record for each frame

	//////////////////////////////////////////////////////////////////

	vector<string> getMonoplaneFilterTypesList();
	string getCurrentFilterType();
	vector<string> getFilterTypesList();
	void setFilterToType(string type);

	bool isActiveFilterBiplane();
	vector<AbstractFilterWidget*> getBiplaneAlgorithmPropertiesGUI();
	vector<AbstractFilterWidget*> getMonoplaneAlgorithmPropertiesGUI();


	/// Add/remove templates
	void addTemplate(unsigned int streamNumber, double worldCoords[2], int pointType);
	void unsetNearestTemplate(unsigned int streamNumber, double worldCoords[2]);
	void unsetAllTemplates(unsigned int streamNumber);
	/// Manually set the motions compensation position (this can be used if the tracking goes bad).
	void setTemplatePositionForMotionCompensation(unsigned int streamNumber, double worldCoords[2]);
	void setReferenceFrameForMotionCompensation(unsigned int streamNumber, bool set);
	void resetReferenceFrameForMotionCompensation(unsigned int streamNumber, bool set);
	void removeReferenceFrameForMotionCompensation(unsigned int streamNumber, bool set);

	void setDICOMAnglesToWindow(double primAngle, double secAngle);
	void getDICOMAnglesToWindow(double& primAngle, double& secAngle);
	void setDICOMAnglesToWindowRef(int index, double primAngle, double secAngle);
	void getDICOMAnglesToWindowRef(int index, double& primAngle, double& secAngle);
	void getDicomReferenceGeometryValues(int streamNumber, int& SID, int& sourcePatDist, double& primAngle, double& secAngle, 
		int& lateralPos, int& longitudinalPos, int& verticalPos, double& mmPerPixel, int& imageSizeX, int& imageSizeY, bool ok[10]);
	void getDicomGeometryValues(int& SID, int& sourcePatDist, double& primAngle, double& secAngle,
		int& lateralPos, int& longitudinalPos, int& verticalPos, double& mmPerPixel, int& imageSizeX, int& imageSizeY, bool ok[10]);
	/// set/get input file (CT or MR) in order to calculate correct mesh position in OverlayScene->AddOverlayMesh(), because for CT file with wrong spacing the mesh position has to be recalculated
	void setMRInputFileForMesh(int meshOrientation);
	int getMRInputFileForMesh();
	vector<std::string> getMeshFileNames();	/// Returns the filenames of the shown mesh files
	void setMeshVisibility(bool visible);
	bool getMeshVisibility();
	void setMeshColor(double color[3], int index);
	void giveMeshColorToScene(double color[3]);
	void getMeshColor(double color[3], int index);
	void setMeshOpacity(double opacity, int index);
	void giveMeshOpacityToScene(double opacity);
	double getMeshOpacity(int index);
	void addOverlayMesh(string file);
	void removeOverlayMesh(int meshNumber = -1); /// Removes the mesh, by default the one which was added last.
	int getNumberOfMeshes() { return theMeshActors[0].size(); } /// Returns the number of meshes.
	
	void setXRayVisibilityIn3dWindow(bool visible);

	void setNewGeometryRef(int index, double& primAngleRead, double& secAngleRead, int& longRead, int& latRead, int& HoeheRead, int& SIDRead, double& mmPerPxlRead, int& FDRead, int& SODRead);
	void setNewGeometryPlay(double& primAngleRead, double& secAngleRead, int& longRead, int& latRead, int& HoeheRead, int& SIDRead, double& mmPerPxlRead, int& FDRead, int& SODRead);

	void RenderAll();

	/// Returns the biplane geometry.
	BiplaneGeometry* getCurrentBiplaneGeometry() { return &theBiplaneGeometry; }

	/// @param pos: in world coordinates
	void addMarkerPoint(unsigned int streamNumber, double pos[3]);
	void removeNearestMarkerPoint(unsigned int streamNumber, double pos[3]);
	SceneLabeling* getMarkerLabeling(unsigned int streamNumber);
	/// Get the nearest template and use it's 3D position to set the MR volume sclices.
	void setTrackingPointIndexForMRVolume(unsigned int streamNumber, double worldCoords[2]);

	double reconstruct3dPointWithSkewLinesIntersection(const double p1[2], const double p2[2], double res[3]);

	/// SceneLabeling markers set by the user (not reconstructed) for 3D-3D-Registration
	SceneLabeling* getRegistrationMarkers();
	void do3d3dRegistration();

	void addEpipolarLinesIn3dWindow(double p1_1[2], double p1_2[2], double p2_1[2], double p2_2[2], double bestPoint1[2], double bestPoint2[2]);
	void removeEpipolarLinesIn3dWindow();
	//======================================================

	//std::vector<int> GeometryPlayNew;
	//std::vector<int> GeometryPlay;
	//std::vector<int> GeometryLiveNew;
	//std::vector<int> GeometryLive;
	int FDalt[2];
	int FDmain;
	void renderMain();
	void renderRef(int index);
	void clearRenderer(int index);
	void visualizeActors(int index, bool visualize);
	char* patientDir;
	int liveIndex;
	bool biplaneSystem;
	int frameGrabbersNumber;
	//======================================================
	
protected:
	/// create and connect the pipline objects
	void setupPipeline();
	void setupBiplaneGeometry();
	/// @param index: 0 = first/upper image, 1 = second/lower image
	void setupBiplaneGeometry(int index);
	void setupFramegrabberGeometry();
	void setupGeometryFromFramegrabber(int index);
	void setupBiplaneGeometryFromFramegrabber();
	/// delete all pipline objects
	void releasePipeline();

	void setMonoplaneFilterToType(string type);
	void setBiplaneFilterToType(string type);

	void reconnectTrackingTransformations(vector<vtkTransform*> newTransforms);
	void setMotionCorrectionScalingAccordingToGeometry();

	void createPatientdirectory();

	//======================================================
	//bool testInitializationFrontal();
	bool testInitializationFrontal(int index);
	//bool testInitializationLateral();
	bool testInitializationLateral(int index);
	void testFramegrabberInitialization();
	int recognizeShift(int index);
	void clipFG(int index, int shiftHorizontal, int shiftVertical);
	//std::vector<int>  readGeometry(vtkDICOMMetaData* metaRead);

	int startClock(char* txt);
	void stopClock(int startTime);
	//======================================================
	bool hasEnding(string const file, string const ending);

private:
	OverlayScene(int numberofChannels, int numberofWindows, int numberOfFramegrabbers);
	static OverlayScene* theInstance;

	BiplaneGeometry theBiplaneGeometry;

	vtkLineSource* line1;
	vtkPolyDataMapper* lineMapper1;
	vtkActor* lineActor1;
	vtkLineSource* line2;
	vtkPolyDataMapper* lineMapper2;
	vtkActor* lineActor2;
	vtkLineSource* line3;
	vtkPolyDataMapper* lineMapper3;
	vtkActor* lineActor3;

	// One extra X-ray plane in the 3D window's center.
	// Whithout this, the 3D scene is not re-rendered correctly
	// when the user zooms in too much.
	vtkImageActor* theEnforceCorrectRenderingIn3DWindowActor;

	//XRayViewer* theXRayViewer;
	DICOMVisualizer* theMRVisualizer;
	SceneLabeling* theRegistrationMarkers; // set by the user in 3D

	vtkRenderWindow* renWin[3];
	vtkRenderWindow* renWin3D;
	vtkRenderWindow* renWinXRAY[2];
	vtkRenderer* renderer3D;
	vtkRenderer* renderers2D[3];
	vtkRenderer* rendererXRAY[2];
	vtkRenderer* renECG[3];
	vtkContextActor* view[3];
	vtkAxesActor* axesActor;
	vtkAxesActor* axesXRAY[2];
	vtkAxesActor* axesMainXRAY;
	vtkRenderWindow* renWinMainXRAY;
	vtkRenderer* rendererMainXRAY;
	vtkRenderWindowInteractor* iren;

	vtkImageViewer* viewer;
	vtkImageReader* reader;
	vtkGDCMImageReader* GDCMreader;

	/// one for each template
	vector<vtkSphereSource*> theTrackingPointSources;
	vector<vtkTransformFilter*> theTrackingPointTransformers;
	vector<vtkPolyDataMapper*> theTrackingPointMappers;
	vector<vtkActor*> theTrackingPointActors;

	/// one at all
	vtkAssembly* theTrackingPointAssembly;


	BiplaneAlgorithm* theBiplaneFilter;

	/// one per output window
	vector<vtkAssembly*> theMeshAssemblies;
	vector<vtkTransform*> theRegistrationTransformers;

	/*vector<MotionCorrectionFilter*> theMotionCorrections;*/
	MotionCorrectionFilter* theMotionCorrections;

	vector<vtkAssembly*> theMarkerPointAssemblies;
	vector<SceneLabeling*> theMarkerPoints;
	

	//for each extra mesh	
	vector<vtkSmartPointer<vtkPolyData>> theMeshes;
	
	/// one per ouput window (first index) and per mesh (second index)
	vector<vtkTransformFilter*> theMeshTransformFilters[4];
	vector<vtkPolyDataMapper*> theMeshMappers[4];
	vector<vtkActor*> theMeshActors[4];
	

	vector <vtkImageReslice*> theXRAYReslicers;

	vector<vtkImageActor*> theXRAYActors;	//For 2D view: index [0] - upper image, index [1] - lower image
	vector<vtkImageActor*> theXRAYActors3D;	//For 3D view: index [0] represent theXRAYActors[0], index [1] - theXRAYActors[1]
	//XRAYReader* readerXRAY[2];
	vector<XRAYReader*> theXRAYReaders;
	XRAYReader* readerXRAY;

	XRAYReader* readerMainXRAY;
	vtkImageReslice* resliceMainXRAY;
	vtkImageActor* actorMainXRAY;

	//========================================================
	//XRAYReader* readerMain;
	//vtkImageReslice* resliceMain;
	//vtkImageActor* actorMain;
	//vtkImageData* ImageData;
	//vtkDataSetMapper* mapper;
	//vtkActor* actor;
	//vtkRenderer* renderer;
	//CvCapture* cap;
	//vtkRenderWindow* renWinMain;
	//vtkDataObject* imageObject;
	//_________________
	vector<vtkDICOMWriter*> writerBiplane;
	vtkDICOMWriter* writer;
	vtkDICOMWriter* writer0;
	vtkDICOMWriter* writer1;
	//vtkDICOMGenerator* generator;
	vtkDICOMCTGenerator* generator;
	vector<vtkDICOMCTGenerator*> generatorBiplane;
	vtkDICOMCTGenerator* generator0;
	vtkDICOMCTGenerator* generator1;
	//vtkDICOMSCGenerator* generator;
	//vtkDICOMReader* DICOMreader;
	//vtkDICOMMetaData* metaRead;
	//vtkDICOMImageReader* DICOMreader;
	XRAYReader* DICOMreader;
	//_________________
	//char* movieDirectory;
	//char* patientDir;
	char* curRunDir;
	char* curRunDirBiplane[2];

	//========================================================
	vtkChartXY* chart[3];
	int frameNumberForPeak[3];

	//Epipolarlines
	vtkLineSource* epiline1_1;
	vtkPolyDataMapper* epilineMapper1_1;
	vtkActor* epilineActor1_1;
	vtkLineSource* epiline1_2;
	vtkPolyDataMapper* epilineMapper1_2;
	vtkActor* epilineActor1_2;
	vtkLineSource* epiline2_1;
	vtkPolyDataMapper* epilineMapper2_1;
	vtkActor* epilineActor2_1;
	vtkLineSource* epiline2_2;
	vtkPolyDataMapper* epilineMapper2_2;
	vtkActor* epilineActor2_2;

	//std::std::vector<vtkWin32VideoSource*> theFramegrabbers;
	vtkWin32VideoSource* theFramegrabbers[2];
	vtkWin32VideoSource* theFramegrabbersTemp;
	//vtkWin32VideoSource* FramegrabbersCR; // framegrabber object for Character Recognition
	/*vtkImageLuminance* luminanceFilter[2];*/
	vtkImageLuminance* luminanceFilter[2];
	vtkImageLuminance* luminanceFilterTemp;
	Panel *panelFront;
	Panel *panelLat;

	// default values
	int inputChannels;
	int outputWindows;
	
	double meshOpacity;
	double meshColor[3];
	bool activate;
	int orientation;

	double primAngleMain;
	double secAngleMain;

	// vergleich old new references
	double primAngleRef[2], secAngleRef[2];
	int SIDRef[2], FDRef[2], SODRef[2];
	double mmPerPxlRef[2];
	int longRef[2], latRef[2], HoeheRef[2];
	// new
	double primAngleRefNew[2], secAngleRefNew[2];
	int SIDRefNew[2], FDRefNew[2], SODRefNew[2];
	double mmPerPxlRefNew[2];
	int longRefNew[2], latRefNew[2], HoeheRefNew[2];


	// vergleich old new play
	double primAnglePlay, secAnglePlay;
	int SIDPlay, FDPlay, SODPlay;
	double mmPerPxlPlay;
	int longPlay, latPlay, HoehePlay;
	// new
	double primAnglePlayNew, secAnglePlayNew;
	int SIDPlayNew, FDPlayNew, SODPlayNew;
	double mmPerPxlPlayNew;
	int longPlayNew, latPlayNew, HoehePlayNew;


	bool activeFilterIsBiplane; /// whether active filter is biplane or monoplan
	//bool getImagesFromFile; /// whether to read from file or from framegrabber
	int shiftVertical;
	int shiftHorizontal;
	
	//int FD[2];
	ofstream g;	
	//===================================================
	// overwrite first image after the geometry has changed
	int countOverwrittenframes;

	vector<std::string> meshFileNames;
	};

#endif
