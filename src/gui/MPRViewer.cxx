#include "ui_MPRViewer.h"
#include "MPRViewer.h"

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include "vtkResliceImageViewer.h"
#include "vtkResliceCursorLineRepresentation.h"
#include "vtkResliceCursorThickLineRepresentation.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkResliceCursor.h"
#include "vtkDICOMImageReader.h"
#include "vtkMetaImageReader.h"
#include "vtkCellPicker.h"
#include "vtkProperty.h"
#include "vtkPlane.h"
#include "vtkImageData.h"
#include "vtkCommand.h"
#include "vtkPlaneSource.h"
#include "vtkLookupTable.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkInteractorStyleImage.h"
#include "vtkImageSlabReslice.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkDistanceWidget.h"
#include "vtkDistanceRepresentation.h"
#include "vtkHandleRepresentation.h"
#include "vtkResliceImageViewerMeasurements.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkPointHandleRepresentation2D.h"
#include <QFileDialog>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

#include <vtkCellArray.h>
#include <vtkContourWidget.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOrientedGlyphContourRepresentation.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTriangleFilter.h>
#include <vtkMassProperties.h>
#include <vtkPolygon.h>
#include <vtkAppendPolyData.h>
#include <vtkDelaunay2D.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataWriter.h>
#include <vtkContourLoopExtraction.h>


//----------------------------------------------------------------------------
class vtkResliceCursorCallback : public vtkCommand
{
public:
  static vtkResliceCursorCallback *New()
  { return new vtkResliceCursorCallback; }

  void Execute( vtkObject *caller, unsigned long ev,
                void *callData )
    {

    if (ev == vtkResliceCursorWidget::WindowLevelEvent ||
        ev == vtkCommand::WindowLevelEvent ||
        ev == vtkResliceCursorWidget::ResliceThicknessChangedEvent)
      {
      // Render everything
      for (int i = 0; i < 3; i++)
        {
        this->RCW[i]->Render();
        }
      this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
      return;
      }

    vtkImagePlaneWidget* ipw =
      dynamic_cast< vtkImagePlaneWidget* >( caller );
    if (ipw)
      {
      double* wl = static_cast<double*>( callData );

      if ( ipw == this->IPW[0] )
        {
        this->IPW[1]->SetWindowLevel(wl[0],wl[1],1);
        this->IPW[2]->SetWindowLevel(wl[0],wl[1],1);
        }
      else if( ipw == this->IPW[1] )
        {
        this->IPW[0]->SetWindowLevel(wl[0],wl[1],1);
        this->IPW[2]->SetWindowLevel(wl[0],wl[1],1);
        }
      else if (ipw == this->IPW[2])
        {
        this->IPW[0]->SetWindowLevel(wl[0],wl[1],1);
        this->IPW[1]->SetWindowLevel(wl[0],wl[1],1);
        }
      }

    vtkResliceCursorWidget *rcw = dynamic_cast<
      vtkResliceCursorWidget * >(caller);
    if (rcw)
      {
      vtkResliceCursorLineRepresentation *rep = dynamic_cast<
        vtkResliceCursorLineRepresentation * >(rcw->GetRepresentation());
      // Although the return value is not used, we keep the get calls
      // in case they had side-effects
      rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
      for (int i = 0; i < 3; i++)
        {
        vtkPlaneSource *ps = static_cast< vtkPlaneSource * >(
            this->IPW[i]->GetPolyDataAlgorithm());
        ps->SetOrigin(this->RCW[i]->GetResliceCursorRepresentation()->
                                          GetPlaneSource()->GetOrigin());
        ps->SetPoint1(this->RCW[i]->GetResliceCursorRepresentation()->
                                          GetPlaneSource()->GetPoint1());
        ps->SetPoint2(this->RCW[i]->GetResliceCursorRepresentation()->
                                          GetPlaneSource()->GetPoint2());

        // If the reslice plane has modified, update it on the 3D widget
        this->IPW[i]->UpdatePlacement();
        }
      }

    // Render everything
    for (int i = 0; i < 3; i++)
      {
      this->RCW[i]->Render();
      }
    this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
    }

  vtkResliceCursorCallback() {}
  vtkImagePlaneWidget* IPW[3];
  vtkResliceCursorWidget *RCW[3];
};

MPRViewer* MPRViewer::theInstance = NULL;

MPRViewer* MPRViewer::New(QWidget *parent, OverlayScene* scene)
{
	if (!theInstance)
	{
		theInstance = new MPRViewer(parent, scene);
		return theInstance;
	}
	else
	{
		cout << "ERROR: MPR Viewer::New should only be called once!" << endl;
		return NULL;
	}
}

MPRViewer::MPRViewer(QWidget *parent, OverlayScene* scene)
{
	setWindowTitle("MPR Viewer");

	this->scene = scene;

  this->ui = new Ui_MPRViewer;
  this->ui->setupUi(this);
 
  ren = vtkSmartPointer< vtkRenderer >::New();
  this->ui->view4->GetRenderWindow()->AddRenderer(ren);

  renMesh = vtkSmartPointer< vtkRenderer >::New();
  this->ui->view4->GetRenderWindow()->AddRenderer(renMesh);

 /* renObj = vtkSmartPointer< vtkRenderer >::New();
  this->ui->view4->GetRenderWindow()->AddRenderer(renObj);
  */
  for (int i = 0; i < 3; i++)
  {
	  riw[i] = vtkSmartPointer< vtkResliceImageViewer >::New();
  }

  this->ui->view1->SetRenderWindow(riw[0]->GetRenderWindow());
  riw[0]->SetupInteractor(
	  this->ui->view1->GetRenderWindow()->GetInteractor());

  this->ui->view2->SetRenderWindow(riw[1]->GetRenderWindow());
  riw[1]->SetupInteractor(
	  this->ui->view2->GetRenderWindow()->GetInteractor());

  this->ui->view3->SetRenderWindow(riw[2]->GetRenderWindow());
  riw[2]->SetupInteractor(
	  this->ui->view3->GetRenderWindow()->GetInteractor());

  vtkSmartPointer<vtkCellPicker> picker =
	  vtkSmartPointer<vtkCellPicker>::New();
  picker->SetTolerance(0.005);

  vtkSmartPointer<vtkProperty> ipwProp =
	  vtkSmartPointer<vtkProperty>::New();

  /*vtkSmartPointer< vtkRenderer > ren =
  vtkSmartPointer< vtkRenderer >::New();

  this->ui->view4->GetRenderWindow()->AddRenderer(ren);*/
  vtkRenderWindowInteractor *iren = this->ui->view4->GetInteractor();

  for (int i = 0; i < 3; i++)
  {
	  planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
	  planeWidget[i]->SetInteractor(iren);
	  planeWidget[i]->SetPicker(picker);
	  planeWidget[i]->RestrictPlaneToVolumeOn();
	  double color[3] = { 0, 0, 0 };
	  color[i] = 1;
	  planeWidget[i]->GetPlaneProperty()->SetColor(color);

	  color[0] /= 4.0;
	  color[1] /= 4.0;
	  color[2] /= 4.0;
	  riw[i]->GetRenderer()->SetBackground(color);

	  planeWidget[i]->SetTexturePlaneProperty(ipwProp);
	  planeWidget[i]->TextureInterpolateOff();
	  planeWidget[i]->SetResliceInterpolateToLinear();
	  //planeWidget[i]->SetInputConnection(reader->GetOutputPort());
	  //planeWidget[i]->SetPlaneOrientation(i);
	  //planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
	  planeWidget[i]->DisplayTextOn();
	  //planeWidget[i]->SetDefaultRenderer(ren);
	  planeWidget[i]->SetWindowLevel(1358, -27);
	  //planeWidget[i]->On();
	  //planeWidget[i]->InteractionOn();
  }

  vtkSmartPointer<vtkResliceCursorCallback> cbk =
	  vtkSmartPointer<vtkResliceCursorCallback>::New();

  for (int i = 0; i < 3; i++)
  {
	  cbk->IPW[i] = planeWidget[i];
	  cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
	  riw[i]->GetResliceCursorWidget()->AddObserver(
		  vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
	  riw[i]->GetResliceCursorWidget()->AddObserver(
		  vtkResliceCursorWidget::WindowLevelEvent, cbk);
	  riw[i]->GetResliceCursorWidget()->AddObserver(
		  vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
	  riw[i]->GetResliceCursorWidget()->AddObserver(
		  vtkResliceCursorWidget::ResetCursorEvent, cbk);
	  riw[i]->GetInteractorStyle()->AddObserver(
		  vtkCommand::WindowLevelEvent, cbk);

	  // Make them all share the same color map.
	  riw[i]->SetLookupTable(riw[0]->GetLookupTable());
	  planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
	  //planeWidget[i]->GetColorMap()->SetInput(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
	  planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());

  }

  this->ui->view1->show();
  this->ui->view2->show();
  this->ui->view3->show();



  // Set up action signals and slots
  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
  connect(this->ui->resliceModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(resliceMode(int)));
  this->ui->resliceModeCheckBox->setEnabled(0);
  connect(this->ui->thickModeCheckBox, SIGNAL(stateChanged(int)), this, SLOT(thickMode(int)));
  this->ui->thickModeCheckBox->setEnabled(0);

  connect(this->ui->radioButton_Max, SIGNAL(pressed()), this, SLOT(SetBlendModeToMaxIP()));
  connect(this->ui->radioButton_Min, SIGNAL(pressed()), this, SLOT(SetBlendModeToMinIP()));
  connect(this->ui->radioButton_Mean, SIGNAL(pressed()), this, SLOT(SetBlendModeToMeanIP()));
  this->ui->blendModeGroupBox->setEnabled(0);

  connect(this->ui->resetButton, SIGNAL(pressed()), this, SLOT(ResetViews()));
  connect(this->ui->AddDistance3Button, SIGNAL(pressed()), this, SLOT(AddDistanceMeasurementToView3()));

  connect(this->ui->addCircumferenceButton, SIGNAL(pressed()), this, SLOT(AddCircumferenceMeasurementToView3()));
  connect(this->ui->calculateCircumferenceButton, SIGNAL(pressed()), this, SLOT(calculateCircumference()));
  this->ui->resetButton->setEnabled(0);
  
  

  this->ui->loadVTKButton->setEnabled(0);
  connect(this->ui->loadFileButton, SIGNAL(pressed()), this, SLOT(LoadFile()));
  connect(this->ui->loadVTKButton, SIGNAL(pressed()), this, SLOT(LoadFile()));

  this->ui->listWidget->clear();

};

void MPRViewer::slotExit()
{
  qApp->exit();
}

void MPRViewer::resliceMode(int mode)
{
  this->ui->thickModeCheckBox->setEnabled(mode ? 1 : 0);
  this->ui->blendModeGroupBox->setEnabled(mode ? 1 : 0);

  for (int i = 0; i < 3; i++)
    {
    riw[i]->SetResliceMode(mode ? 1 : 0);
    riw[i]->GetRenderer()->ResetCamera();
    riw[i]->Render();
    }
}

void MPRViewer::thickMode(int mode)
{
  for (int i = 0; i < 3; i++)
    {
    riw[i]->SetThickMode(mode ? 1 : 0);
    riw[i]->Render();
    }
}

void MPRViewer::SetBlendMode(int m)
{
  for (int i = 0; i < 3; i++)
    {
    vtkImageSlabReslice *thickSlabReslice = vtkImageSlabReslice::SafeDownCast(
        vtkResliceCursorThickLineRepresentation::SafeDownCast(
          riw[i]->GetResliceCursorWidget()->GetRepresentation())->GetReslice());
    thickSlabReslice->SetBlendMode(m);
    riw[i]->Render();
    }
}

void MPRViewer::SetBlendModeToMaxIP()
{
  this->SetBlendMode(VTK_IMAGE_SLAB_MAX);
}

void MPRViewer::SetBlendModeToMinIP()
{
  this->SetBlendMode(VTK_IMAGE_SLAB_MIN);
}

void MPRViewer::SetBlendModeToMeanIP()
{
  this->SetBlendMode(VTK_IMAGE_SLAB_MEAN);
}

void MPRViewer::ResetViews()
{
  // Reset the reslice image views
  for (int i = 0; i < 3; i++)
    {
    riw[i]->Reset();
    }

  // Also sync the Image plane widget on the 3D top right view with any
  // changes to the reslice cursor.
  for (int i = 0; i < 3; i++)
    {
    vtkPlaneSource *ps = static_cast< vtkPlaneSource * >(
        planeWidget[i]->GetPolyDataAlgorithm());
    ps->SetNormal(riw[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
    ps->SetCenter(riw[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());

    // If the reslice plane has modified, update it on the 3D widget
    this->planeWidget[i]->UpdatePlacement();
    }

  // remove existing widgets.
  if (this->DistanceWidget[2] != NULL)
  {
	  this->DistanceWidget[2]->SetEnabled(0);
	  this->DistanceWidget[2] = NULL;
  }
  //reset priority of contour widget
  if (this->contourWidget != NULL)
  {
	  this->contourWidget->SetPriority(
		  this->riw[2]->GetResliceCursorWidget()->GetPriority() - 0.01);
  }
  // remove existing widgets.
  if (this->contourWidget != NULL)
  {
	  this->contourWidget->SetEnabled(0);
	  this->contourWidget = NULL;
  }

  // Render in response to changes.
  this->Render();
}

void MPRViewer::Render()
{
  for (int i = 0; i < 3; i++)
    {
    riw[i]->Render();
    }
  this->ui->view3->GetRenderWindow()->Render();
}

bool MPRViewer::hasEnding(string const file, string const ending)
{
	std::string end;
	std::string fn;
	if (file.length() >= ending.length()) {
		return (0 == file.compare(file.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
};

void MPRViewer::LoadFile()
{
	QString fileFormats({ "MetaData File (*.mhd);; MetaData File (*.mha);; Mesh File(*.vtk)" });
	QString fileName = QFileDialog::getOpenFileName(this, "Select metadata file for viewing", "./", fileFormats);
	if (fileName.isNull()) return; // user cancelled	

	if (hasEnding(fileName.toStdString(), "mhd") || hasEnding(fileName.toStdString(), "mha")) {
		
		//vtkSmartPointer< vtkDICOMImageReader > reader =
		//  vtkSmartPointer< vtkDICOMImageReader >::New();
		////reader->SetDirectoryName("b://NAVIGATION//DATA//LUO_LAA//DATA//20211021//DICOM");
		//reader->SetDirectoryName("b://NAVIGATION//SOURCE_CODE//EP_Navigator_ACTUAL//mha");
		//reader->Update();
		//int imageDims[3];
		//reader->GetOutput()->GetDimensions(imageDims);

		vtkSmartPointer< vtkMetaImageReader > reader =
			vtkSmartPointer< vtkMetaImageReader >::New();
		reader->SetFileName(qPrintable(fileName));
		reader->Update();
		//int imageDims[3];
		reader->GetOutput()->GetDimensions(imageDims);
		reader->GetOutput()->GetSpacing(spacing);
		reader->GetOutput()->GetBounds(bounds);

		for (int i = 0; i < 3; i++)
		{
			// make them all share the same reslice cursor object.
			vtkResliceCursorLineRepresentation *rep =
				vtkResliceCursorLineRepresentation::SafeDownCast(
					riw[i]->GetResliceCursorWidget()->GetRepresentation());
			riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());

			rep->GetResliceCursorActor()->
				GetCursorAlgorithm()->SetReslicePlaneNormal(i);

			riw[i]->SetInputData(reader->GetOutput());
			riw[i]->SetSliceOrientation(i);
			riw[i]->SetResliceModeToAxisAligned();

			//planeWidget[i]->RestrictPlaneToVolumeOn();
			planeWidget[i]->SetInputConnection(reader->GetOutputPort());
			planeWidget[i]->SetPlaneOrientation(i);
			planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
			planeWidget[i]->On();
			planeWidget[i]->InteractionOn();
		}

		this->ui->resliceModeCheckBox->setEnabled(1);
		this->ui->listWidget->addItem(fileName);
		this->ui->loadVTKButton->setEnabled(1);
		this->ui->resetButton->setEnabled(1);
	}

	else if (hasEnding(fileName.toStdString(), "vtk")) {

		renMesh->RemoveActor(actor);

		vtkSmartPointer<vtkPolyDataReader> meshReader = vtkSmartPointer<vtkPolyDataReader>::New();
		meshReader->SetFileName(qPrintable(fileName));
		meshReader->Update();		

		double meshColor[3];
		double meshOpacity = 0.5;
		//VTK uses normalized color values so you must do these divisions:    r=double(re)/255.0;  g=double(ge)/255.0; b=double(be)/255.0;
		meshColor[0] = 0.8;
		meshColor[1] = 0.05;
		meshColor[2] = 0.05;
		double position[3];
		
		position[0] = bounds[0] + spacing[0]*imageDims[0] / 2;
		position[1] = bounds[2] + spacing[1] * imageDims[1]/2;
		position[2] = bounds[4] + spacing[2] * imageDims[2] / 2;

		vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
		mapper->SetInputConnection(meshReader->GetOutputPort());

		actor = vtkActor::New();
		actor->SetMapper(mapper);
		actor->GetProperty()->SetColor(meshColor);
		actor->GetProperty()->SetOpacity(meshOpacity);		
		actor->SetPosition(position);
		
		//vtkSmartPointer< vtkRenderer > renMesh =
		//	vtkSmartPointer< vtkRenderer >::New();

		/*vtkRenderWindow *window = this->ui->view4->GetRenderWindow();
		window->AddRenderer(renMesh);*/
		//this->ui->view4->GetRenderWindow()->AddRenderer(renMesh);
		renMesh->AddActor(actor);
		this->ui->view4->GetRenderWindow()->Render();
		this->ui->listWidget_2->addItem(fileName);
		
	}
	
		
}

void MPRViewer::AddDistanceMeasurementToView3()
{
  this->AddDistanceMeasurementToView(2);
}


void MPRViewer::AddDistanceMeasurementToView(int i)
{
  // remove existing widgets.
  if (this->DistanceWidget[i] != NULL)
    {
    this->DistanceWidget[i]->SetEnabled(0);
    this->DistanceWidget[i] = NULL;
    }

  // add new widget
  this->DistanceWidget[i] = vtkSmartPointer< vtkDistanceWidget >::New();
  this->DistanceWidget[i]->SetInteractor(
    this->riw[i]->GetResliceCursorWidget()->GetInteractor());

  // Set a priority higher than our reslice cursor widget
  this->DistanceWidget[i]->SetPriority(
    this->riw[i]->GetResliceCursorWidget()->GetPriority() + 0.01);

  vtkSmartPointer< vtkPointHandleRepresentation2D > handleRep =
    vtkSmartPointer< vtkPointHandleRepresentation2D >::New();
  vtkSmartPointer< vtkDistanceRepresentation2D > distanceRep =
    vtkSmartPointer< vtkDistanceRepresentation2D >::New();
  distanceRep->SetHandleRepresentation(handleRep);
  this->DistanceWidget[i]->SetRepresentation(distanceRep);
  distanceRep->InstantiateHandleRepresentation();
  distanceRep->GetPoint1Representation()->SetPointPlacer(riw[i]->GetPointPlacer());
  distanceRep->GetPoint2Representation()->SetPointPlacer(riw[i]->GetPointPlacer());

  // Add the distance to the list of widgets whose visibility is managed based
  // on the reslice plane by the ResliceImageViewerMeasurements class
  this->riw[i]->GetMeasurements()->AddItem(this->DistanceWidget[i]);

  this->DistanceWidget[i]->CreateDefaultRepresentation();
  this->DistanceWidget[i]->EnabledOn();
}

void MPRViewer::calculateCircumference()
{	
	if (actorCirc != NULL)
	{
		renMesh->RemoveActor(actorCirc);
	}
	vtkSmartPointer< vtkPolyDataWriter > writer = vtkSmartPointer< vtkPolyDataWriter >::New();
	writer->SetInputData(contourRepresentation->GetContourRepresentationAsPolyData());
	//writer->SetInputData(polydata);
	writer->SetFileName("Contour.vtk");
	writer->Update();

	string strReplace = "LINES";
	string strNew = "POLYGONS";
	ifstream filein("Contour.vtk");
	ofstream fileout("Contour_new.vtk");
	string strTemp;
	for (std::string line; std::getline(filein, line); )
	{
		int index = 0;
		while (true) {
			/* Locate the substring to replace. */
			index = line.find(strReplace, index);
			if (index == std::string::npos) break;
			line.replace(index, 5, strNew);
			/* Advance index forward so the next iteration doesn't pick it up as well. */
			index += 5;
		}

		line += "\n";
		fileout << line;
	}

	filein.close();
	fileout.close();

	vtkSmartPointer<vtkPolyDataReader> Reader = vtkSmartPointer<vtkPolyDataReader>::New();
	Reader->SetFileName("Contour_new.vtk");
	Reader->Update();
	vtkSmartPointer< vtkTriangleFilter > triangleTrans =
		vtkSmartPointer< vtkTriangleFilter >::New();
	triangleTrans->SetInputData(Reader->GetOutput());
	triangleTrans->Update();

	vtkSmartPointer< vtkMassProperties > massProp =
		vtkSmartPointer< vtkMassProperties >::New();

	massProp->SetInputConnection(triangleTrans->GetOutputPort());	

	massProp->Update();
	double area = massProp->GetSurfaceArea();

	this->ui->surfaceAreaLabel->setNum(area);
	this->ui->diameterLabel->setNum(2*std::sqrt(area/vtkMath::Pi()));


	/*vtkSmartPointer<vtkAppendPolyData> data = 
		vtkSmartPointer< vtkAppendPolyData >::New();
	data->AddInputData(contourRepresentation->GetContourRepresentationAsPolyData());	
	data->Update();*/
	
	vtkSmartPointer<vtkDelaunay2D> Delaunay2Dobj =
		vtkSmartPointer< vtkDelaunay2D >::New();

	// Clean the polydata. This will remove duplicate points that may be
	// present in the input data.
	//vtkNew<vtkCleanPolyData> cleaner;
	//cleaner->SetInputConnection(Reader->GetOutputPort());

	Delaunay2Dobj->SetInputData(Reader->GetOutput());
	//Delaunay2Dobj->SetInputData(data->GetOutput());
	//Delaunay2Dobj->SetAlpha(0.5);
	Delaunay2Dobj->Update();
	vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
	mapper->SetInputConnection(Delaunay2Dobj->GetOutputPort());

	vtkNew<vtkNamedColors> color;

	actorCirc = vtkActor::New();
	actorCirc->SetMapper(mapper);
	actorCirc->GetProperty()->SetColor(color->GetColor3d("banana").GetData());
	actorCirc->GetProperty()->SetInterpolationToFlat();
	actorCirc->GetProperty()->SetLineWidth(100.0);
	actorCirc->GetProperty()->SetPointSize(100.0);

	//renObj->AddActor(actor);
	/*vtkSmartPointer< vtkRenderer > renDelaunay = vtkSmartPointer< vtkRenderer >::New();
	this->ui->view4->GetRenderWindow()->AddRenderer(renDelaunay);*/
	renMesh->AddActor(actorCirc);
	this->ui->view4->GetRenderWindow()->Render();
	
}

void MPRViewer::AddCircumferenceMeasurementToView3()
{
	vtkNew<vtkNamedColors> colors;

	// remove existing widgets.
	if (this->contourWidget)
	{
		this->contourWidget->SetEnabled(0);
		this->contourWidget = NULL;
	}

	// add new widget
	// Create the contour widget
	//vtkNew<vtkContourWidget> contourWidget;
	this->contourWidget = vtkSmartPointer< vtkContourWidget >::New();

	// Use the "trackball camera" interactor style, rather than the default
	// "joystick camera"
	//vtkNew<vtkInteractorStyleTrackballCamera> style;
	//interactor->SetInteractorStyle(style);
	//this->riw[2]->GetResliceCursorWidget()->GetInteractor()->SetInteractorStyle(style);

	// Set up the contour widget within the visualization pipeline just assembled
	this->contourWidget->SetInteractor(
		this->riw[2]->GetResliceCursorWidget()->GetInteractor());
	// Set a priority higher than our reslice cursor widget
	this->contourWidget->SetPriority(
		this->riw[2]->GetResliceCursorWidget()->GetPriority() + 0.01);

	// Override the default representation for the contour widget to customize its
	// look
	contourRepresentation = vtkSmartPointer <vtkOrientedGlyphContourRepresentation>::New();
	contourRepresentation->GetLinesProperty()->SetColor(
		colors->GetColor3d("Red").GetData());
	this->contourWidget->SetRepresentation(contourRepresentation);

	polydata = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cellArray =
		vtkSmartPointer<vtkCellArray>::New();

	// Generate a set of points arranged in a circle
	const unsigned int numberOfPoints = 10;
	// Create the polygon
	vtkSmartPointer<vtkPolygon> polygon =
		vtkSmartPointer<vtkPolygon>::New();
	polygon->GetPointIds()->SetNumberOfIds(numberOfPoints);

	vtkIdType* lineIndices = new vtkIdType[numberOfPoints];
	for (int i = 0; i < numberOfPoints; i++)
	{
		polygon->GetPointIds()->SetId(i, i);
		// Create numPts points evenly spread around a circumference of radius 10
		const double angle =
			2.0*vtkMath::Pi()*i / static_cast<double>(numberOfPoints);
		points->InsertPoint(static_cast<vtkIdType>(i), 10*cos(angle),
			10*sin(angle), 0.0);
		lineIndices[i] = static_cast<vtkIdType>(i);
	}

	cellArray->InsertNextCell(polygon);
	polydata->SetPoints(points);
	polydata->SetPolys(cellArray);
		
	this->riw[2]->GetMeasurements()->AddItem(this->contourWidget);

	this->contourWidget->On(); // Turn on the interactor observer
	//this->contourWidget->Initialize(polydata);
	//this->contourWidget->ProcessEventsOn();

	//this->riw[2]->GetResliceCursorWidget()->GetInteractor()->Start();


}