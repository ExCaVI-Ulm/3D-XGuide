#include "SceneLabeling.h"

#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataWriter.h>
#include <vtkPolyDataReader.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMatrix4x4.h>
#include <vtkMath.h>

#include <qstring.h>

#include <limits>
using namespace std;

SceneLabeling::SceneLabeling()
: theRenderWindow(0), nextPointNumber(1), pointVisible(true), labelVisible(true)
{
	startPosition[0] = startPosition[1] = startPosition[2] = 0.0;
	/*markerColor[0] = 1.0;
	markerColor[1] = 1.0;
	markerColor[2] = 0.0;*/
	markerColor[0] = 0.0;
	markerColor[1] = 0.0;
	markerColor[2] = 1.0;

	markerSize = 2;
}

void SceneLabeling::setRenderWindow(vtkRenderWindow* win)
{
	theRenderWindow = win;
}

void SceneLabeling::setAssembly(vtkAssembly* assembly)
{
	theAssembly = assembly;
}

void SceneLabeling::setStartPosition(double pos[3])
{
	startPosition[0] = pos[0];
	startPosition[1] = pos[1];
	startPosition[2] = pos[2];
}

void SceneLabeling::setMarkerColor(double color[3])
{
	markerColor[0] = color[0];
	markerColor[1] = color[1];
	markerColor[2] = color[2];

	for (MarkerPointVector::iterator it = pointList.begin(); it != pointList.end(); ++it) {
		it->actor->GetProperty()->SetColor(markerColor);
		it->label->GetCaptionTextProperty()->SetColor(markerColor);
	}
	theRenderWindow->Render();

}

void SceneLabeling::getMarkerColor(double color[3])
{
	color[0] = markerColor[0];
	color[1] = markerColor[1];
	color[2] = markerColor[2];
}

void SceneLabeling::setMarkerSize(double size)
{
	markerSize = size;
}

void SceneLabeling::addPoint(const char* extraLabel)
{
	MarkerPoint point;

	if(extraLabel == 0) {
		point.setLabel(qPrintable(QString("%1").arg(nextPointNumber)));
		++nextPointNumber;
	}
	else {
		point.setLabel(extraLabel);
	}

	point.sphere->SetRadius(markerSize);
	point.actor->SetPosition(startPosition);
	point.actor->GetProperty()->SetColor(markerColor);
	point.actor->SetVisibility(pointVisible);
	point.label->SetAttachmentPoint(startPosition);
	point.label->GetCaptionTextProperty()->SetColor(markerColor);
	point.label->SetVisibility(labelVisible);

	point.setParent(theRenderWindow->GetRenderers()->GetFirstRenderer(), theAssembly);

	pointList.push_back(point);

	updateLabelPositions();

	theRenderWindow->Render();
}

void SceneLabeling::deleteLastPoint()
{
	if(pointList.empty()) return;

	pointList.rbegin()->remove();
	pointList.pop_back();
	theRenderWindow->Render();

	--nextPointNumber;
}

void SceneLabeling::deletePoint(int pointNumber)
{
	if(pointNumber < 0 || pointNumber >= pointList.size()) return;

	MarkerPointVector::iterator toDelete = pointList.begin();
	toDelete += pointNumber;

	toDelete->remove(); // remove from VTK
	pointList.erase(toDelete); // remove from list

	// all following markers need their number to be updated
	for(int i = pointNumber; i < pointList.size(); ++i) {
		pointList[i].setLabel(qPrintable(QString("%1").arg(pointNumber+1)));
		++pointNumber;
	}

	theRenderWindow->Render();

	--nextPointNumber;
}

void SceneLabeling::deletePoints()
{
	for(MarkerPointVector::iterator it = pointList.begin(); it != pointList.end(); ++it) {
		it->remove();
	}
	pointList.clear();

	nextPointNumber = 1;

	theRenderWindow->Render();
}

void SceneLabeling::deletePointNextTo(double pos[3])
{
	double minDist = std::numeric_limits<double>::max();
	int minIndex = 0;

	vtkMatrix4x4* matrix = theAssembly->GetUserMatrix();
	for(unsigned int i = 0; i < pointList.size(); ++i) {
		double p[3];
		pointList[i].actor->GetPosition(p);
		
		// if existent, add translation of assembly
		if(matrix != NULL) {
			p[0] += matrix->GetElement(0, 3);
			p[1] += matrix->GetElement(1, 3);
			p[2] += matrix->GetElement(2, 3);
		}

		double dist = vtkMath::Distance2BetweenPoints(p, pos);
		if(dist < minDist) {
			minDist = dist;
			minIndex = i;
		}
	}

	deletePoint(minIndex);
}

void SceneLabeling::setPointVisibility(bool visible)
{
	pointVisible = visible;
	for(MarkerPointVector::iterator it = pointList.begin(); it != pointList.end(); ++it) {
		it->actor->SetVisibility(visible);
	}
	theRenderWindow->Render();
}

void SceneLabeling::setLabelVisibility(bool visible)
{
	labelVisible = visible;
	for(MarkerPointVector::iterator it = pointList.begin(); it != pointList.end(); ++it) {		
		it->label->SetVisibility(visible);
	}
	theRenderWindow->Render();
}

void SceneLabeling::updateLabelPositions()
{
	double pos[4];
	for(MarkerPointVector::iterator it = pointList.begin(); it != pointList.end(); ++it) {
		it->actor->GetPosition(pos);
		pos[3] = 1;
		
		// labels are placed outside of the assembly, so
		// do not forget the assembly's positioning
		theAssembly->GetUserMatrix()->MultiplyPoint(pos, pos);

		it->label->SetAttachmentPoint(pos[0], pos[1], pos[2]);
	}
}

void SceneLabeling::writePoints(const char* filepath)
{
	vtkPoints* points = vtkPoints::New();
	points->SetNumberOfPoints(pointList.size());
	
	int i = 0;
	for(MarkerPointVector::iterator it = pointList.begin(); it != pointList.end(); ++it) {
		points->SetPoint(i, it->actor->GetPosition());
		++i;
	}

	vtkPolyData *data = vtkPolyData::New();
	data->SetPoints(points);
	vtkPolyDataWriter *writer = vtkPolyDataWriter::New();
	writer->SetInputData(data);
	writer->SetFileName(filepath);
	writer->Write();
	
	writer->Delete();
	data->Delete();
	points->Delete();
}

void SceneLabeling::loadPoints(const char* filepath)
{
	deletePoints(); // removes old ones

	vtkPolyDataReader* reader = vtkPolyDataReader::New();
	reader->SetFileName(filepath);
	reader->Update();

	nextPointNumber = 1;

	vtkPoints* points = reader->GetOutput()->GetPoints();
	for(int i = 0; i < points->GetNumberOfPoints(); ++i) {
		addPoint();
		pointList.rbegin()->actor->SetPosition(points->GetPoint(i));
		pointList.rbegin()->label->SetAttachmentPoint(points->GetPoint(i));
	}

	theRenderWindow->Render();

	reader->Delete();
}

bool SceneLabeling::getBoundingBox(int data[4])
{
	if(pointList.size() < 2) return false; // not enough points

	//double p1[3], p2[3];
	double* p1 = pointList[pointList.size() - 1].actor->GetPosition();
	double* p2 = pointList[pointList.size() - 2].actor->GetPosition();
	data[0] = (int) p1[0];
	data[1] = (int) p1[1];
	data[2] = (int) p2[0];
	data[3] = (int) p2[1];

	return true;
}

void SceneLabeling::getPointPosition(int index, double pos[3])
{
	if(index < 0 || index >= pointList.size()) return;
	else pointList[index].actor->GetPosition(pos); // sets pos
}