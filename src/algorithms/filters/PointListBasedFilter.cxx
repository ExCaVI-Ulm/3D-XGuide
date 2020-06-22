#include "PointListBasedFilter.h"

#include <limits>

PointListBasedFilter::PointListBasedFilter(int numberOfLists)
{
	pointLists.resize(numberOfLists);
}

PointListBasedFilter::~PointListBasedFilter()
{
}

PointListBasedFilter::PointVector PointListBasedFilter::getPointList(unsigned int listNumber) const {
	return pointLists[listNumber];
}

void PointListBasedFilter::addPoint(const double point[2], int pointType, unsigned int listNumber) {
	Annotated2dPoint toAdd;
	toAdd.x = point[0];
	toAdd.y = point[1];
	toAdd.type = pointType;
	pointLists[listNumber].push_back(toAdd);

	// notify subclasses
	pointWasAdded(listNumber, pointLists[listNumber].size() - 1);
}

void PointListBasedFilter::overwritePoint(const double point[2], int pointType, unsigned int listNumber) {
	
	int index = pointLists[listNumber].size() - 1;
	PointVector::iterator toOverwrite = pointLists[listNumber].begin();

	toOverwrite += index;

	// ... and then delete it
	pointLists[listNumber].erase(toOverwrite);

	Annotated2dPoint toAdd;
	toAdd.x = point[0];
	toAdd.y = point[1];
	toAdd.type = pointType;
	pointLists[listNumber].push_back(toAdd);

	// notify subclasses
	pointWasAdded(listNumber, pointLists[listNumber].size() - 1);
}

void PointListBasedFilter::pointWasAdded(unsigned int listNumber, unsigned int pointNumber) {
	// Do nothing in this default implementation.
}

void PointListBasedFilter::removePoint(int pointNumber, unsigned int listNumber) {
	// std::vector has no delete(index) function. Thus:
	// Set an iterator to the point to be deleted...
	PointVector::iterator toDelete = pointLists[listNumber].begin();
	toDelete += pointNumber;

	// ... and then delete it
	pointLists[listNumber].erase(toDelete);

	// notify subclasses
	pointWasDeleted(listNumber, pointNumber);
}

void PointListBasedFilter::removeAllPoints(unsigned int listNumber) {

	/*
	for(PointVector::iterator toDelete; toDelete < pointLists[0].end(); ++toDelete)
	{
		pointLists[0].erase(toDelete);
		pointWasDeleted(0, toDelete);
	}
	for(PointVector::iterator toDelete; toDelete < pointLists[1].end(); ++toDelete)
	{
		pointLists[1].erase(toDelete);
		pointWasDeleted(1, toDelete);
	}*/

	if(pointLists.size() > 0 && pointLists[listNumber].size() > 0)
	{
		for (unsigned int i = 0; i < pointLists[listNumber].size(); i++)
		{		
			pointWasDeleted(listNumber, pointLists[listNumber].size() - 1 - i);
		}
		pointLists[listNumber].clear();
	}
	/*if(pointLists.size() > 1 && pointLists[1].size() > 0)
	{
		for(unsigned int i = 0; i < pointLists[1].size(); i++)
		{		
			pointWasDeleted(1, 0);
		}
		pointLists[1].clear();
	}*/
}

void PointListBasedFilter::pointWasDeleted(unsigned int listNumber, unsigned int pointNumber) {
	// Do nothing in this default implementation.
}

int PointListBasedFilter::indexOfNearestPoint(double p[2], unsigned int listNumber) {
	double minDist = std::numeric_limits<double>::max();
	int minIndex = -1;

	// compute argmin ||p-p'||^2 with p' in the selected list
	for(unsigned int i = 0; i < pointLists[listNumber].size(); ++i) {
		double dist = (p[0] - pointLists[listNumber][i].x)*(p[0] - pointLists[listNumber][i].x)
			+ (p[1] - pointLists[listNumber][i].y)*(p[1] - pointLists[listNumber][i].y);
		if(dist < minDist) {
			minDist = dist;
			minIndex = i;
		}
	}

	return minIndex;
}