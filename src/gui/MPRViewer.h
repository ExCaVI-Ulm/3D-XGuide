#ifndef MPRViewer_H
#define MPRViewer_H

#include "vtkSmartPointer.h"
#include "vtkResliceImageViewer.h"
#include "vtkImagePlaneWidget.h"
#include "vtkDistanceWidget.h"
#include "vtkContourWidget.h"
#include "vtkResliceImageViewerMeasurements.h"
#include "vtkPolyData.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include <QMainWindow>

using namespace std;
// Forward Qt class declarations
class Ui_MPRViewer;
class OverlayScene;

class MPRViewer : public QMainWindow
{
  Q_OBJECT
public:

  // Constructor/Destructor
  //MPRViewer(int argc, char *argv[]);
	static MPRViewer* New(QWidget *parent, OverlayScene* scene);
  ~MPRViewer() {}

public slots:

  virtual void slotExit();
  virtual void resliceMode(int);
  virtual void thickMode(int);
  virtual void SetBlendModeToMaxIP();
  virtual void SetBlendModeToMinIP();
  virtual void SetBlendModeToMeanIP();
  virtual void SetBlendMode(int);
  virtual void ResetViews();
  virtual void Render();
  virtual void AddDistanceMeasurementToView3();
  virtual void AddDistanceMeasurementToView( int );
  virtual void AddCircumferenceMeasurementToView3();
  virtual void calculateCircumference();
  virtual void LoadFile();
  bool hasEnding(string const file, string const ending);

protected:
  vtkSmartPointer< vtkResliceImageViewer > riw[3];
  vtkSmartPointer< vtkImagePlaneWidget > planeWidget[3];
  vtkSmartPointer< vtkDistanceWidget > DistanceWidget[3];
  vtkSmartPointer< vtkContourWidget > contourWidget;
  vtkSmartPointer<vtkPolyData> polydata;
  vtkSmartPointer <vtkOrientedGlyphContourRepresentation> contourRepresentation;
  vtkSmartPointer< vtkResliceImageViewerMeasurements > ResliceMeasurements;

protected slots:

private:
	MPRViewer(QWidget *parent, OverlayScene* scene);
	static MPRViewer* theInstance;
	OverlayScene* scene;
	vtkSmartPointer< vtkRenderer > renMesh;
	vtkSmartPointer< vtkRenderer > ren;
	//vtkSmartPointer< vtkRenderer > renObj;
	int imageDims[3];
	double spacing[3];
	double bounds[6];
	vtkActor* actor;
	vtkActor* actorCirc;
  // Designer form
  Ui_MPRViewer *ui;
};

#endif // MPRViewer_H
