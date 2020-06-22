#ifndef SCENELABELING_H
#define SCENELABELING_H

#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCaptionActor2D.h>
#include <vtkAssembly.h>
#include <vtkRenderWindow.h>

#include <vtkProperty.h>
#include <vtkTextProperty.h>
#include <vtkRenderer.h>

#include <vector>
//using namespace std;

class MarkerPoint;
typedef std::vector<MarkerPoint> MarkerPointVector;

/** This class supports labeling of a VTK scene.
 *  The user can create and delete points represented by spheres.
 */
class SceneLabeling
{
public:
	SceneLabeling();

	void setRenderWindow(vtkRenderWindow* win);
	void setAssembly(vtkAssembly* assembly);

	void setStartPosition(double pos[3]); /// set where a newly added point is positioned
	void setMarkerColor(double color[3]); /// set the color for new(!) points
	void getMarkerColor(double color[3]);

	void setMarkerSize(double size);

	void addPoint(const char* extraLabel = 0);
	void deleteLastPoint();
	void deletePoint(int pointNumber); // Delete the point with the given number (starting with 0)
	void deletePoints(); /// Delete all points.
	void deletePointNextTo(double pos[3]);
	void setPointVisibility(bool visible); /// set whether the points and labels are visible
	void setLabelVisibility(bool visible); /// set whether the labes are visible
	void updateLabelPositions(); /// position the labels at their corresponding point

	void writePoints(const char* filepath); /// wirte current points to file
	void loadPoints(const char* filepath); /// load points from file and display them

	/// get the coordinates of the two points added latest: (x1,y1),(x2,y2)
	bool getBoundingBox(int data[4]);

	int getNumberOfPoints() { return pointList.size(); }
	
	/// "returns" the position of the point with the given index.
	/// if the index is invalid, pos is not modified.
	void getPointPosition(int index, double pos[3]);

protected:
	MarkerPointVector pointList;
	vtkRenderWindow* theRenderWindow;
	vtkAssembly* theAssembly;

	double startPosition[3];
	double markerColor[3];
	double markerSize;

	int nextPointNumber;

	bool pointVisible, labelVisible;
};

/// For internal use with SceneLabeling only.
class MarkerPoint
{
public:
	/// Create a MarkerPoint. Ensure to call remove() explicitly when deleting a point.
	MarkerPoint() {
		sphere = vtkSphereSource::New();
		sphere->SetRadius(2);

		mapper = vtkPolyDataMapper::New();
		mapper->SetInputConnection(sphere->GetOutputPort());

		actor = vtkActor::New();
		actor->SetMapper(mapper);
		actor->GetProperty()->SetColor(0.9, 0.9, 0.9);
		//actor->GetProperty()->SetOpacity(0.5);
		actor->GetProperty()->SetOpacity(1.0);
		actor->PickableOff();

		label = vtkCaptionActor2D::New();
		
		double* pos = actor->GetPosition();
		label->SetAttachmentPoint(pos[0], pos[1], pos[2]);
		label->SetHeight(0.07);
		
		label->ThreeDimensionalLeaderOff();
		label->BorderOff();
		//label->LeaderOff();
		label->GetCaptionTextProperty()->SetFontSize(12);
		label->GetCaptionTextProperty()->BoldOff();
		label->GetCaptionTextProperty()->SetFontFamilyToArial();
		//label->SetPosition(30, 30);
		label->SetCaption("");
	}

	void setLabel(const char* text) { label->SetCaption(text); }

	void setParent(vtkRenderer* ren, vtkAssembly* asbly) {
		if(!ren) return;

		renderer = ren;
		assembly = asbly;

		renderer->AddActor(label);
		assembly->AddPart(actor);
	}

	/// remove point from the scene and delete all resources
	void remove() {
		if(renderer) {
			renderer->RemoveActor2D(label);
		}
		if(assembly) {
			assembly->RemovePart(actor);
		}
		if(sphere) {
			sphere->Delete();
			sphere = 0;
		}
		if(mapper) {
			mapper->Delete();
			mapper = 0;
		}
		if(actor) {
			actor->Delete();
			actor = 0;
		}
		if(label) {
			label->Delete();
			label = 0;
		}
	}


protected:
	friend class SceneLabeling; // may access my protected stuff

	vtkRenderer       *renderer;
	vtkAssembly		  *assembly;
	vtkSphereSource   *sphere;
	vtkPolyDataMapper *mapper;
	vtkActor          *actor;
	vtkCaptionActor2D *label;
};


#endif
