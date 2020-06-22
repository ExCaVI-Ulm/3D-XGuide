#ifndef POINT_LIST_BASED_FILTER_H
#define POINT_LIST_BASED_FILTER_H

#include <vector>
//using namespace std;

/** This class represents a list of lists of points.
	As default, there is only one list of points, but one
	can create and access an arbitrary number of them.

	Usage:
	- derive your own class public from PointListBasedFilter
	- overwrite pointWasAdded and pointWasDeleted as necessary
	- if you need more than one list call the PointListBasedFilter
	  constructor explicitly in the constructor of your derived class
	  (this is normally the case for biplane filters)
  */
class PointListBasedFilter {
public:
	struct Annotated2dPoint {
		double x;
		double y;
		int type;
	};

	typedef std::vector<Annotated2dPoint> PointVector;
	typedef std::vector<PointVector> PointVectorVector;

	PointListBasedFilter(int numberOfLists = 1);

	virtual ~PointListBasedFilter();

	PointVector getPointList(unsigned int listNumber = 0) const;
	void addPoint(const double point[2], int pointType, unsigned int listNumber = 0);
	void overwritePoint(const double point[2], int pointType, unsigned int listNumber = 0);
	void removePoint(int pointNumber, unsigned int listNumber = 0);
	void removeAllPoints(unsigned int listNumber = 0);
	int indexOfNearestPoint(double p[2], unsigned int listNumber = 0);

protected:
	/// Overwrite these methods in your subclass to keep your state up to date.
	virtual void pointWasAdded(unsigned int listNumber, unsigned int pointNumber);
	virtual void pointWasDeleted(unsigned int listNumber, unsigned int pointNumber);
	
private:
	PointVectorVector pointLists;
};

#endif //POINT_LIST_BASED_FILTER_H
