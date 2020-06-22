#ifndef VTKINTERACTORSTYLEMY2D_H
#define VTKINTERACTORSTYLEMY2D_H

#include "vtkInteractorStyleTrackballCamera.h"

class vtkUnsignedCharArray;

/** Triggers a SelectionChangeEvent when the user clicks.
  * reinterpret_cast<double*>(callData) holds the click
  * position (x,y,z) in VTK world coordinates.
  */
class vtkInteractorStyleMy2D : public vtkInteractorStyleTrackballCamera
{
public:
	static vtkInteractorStyleMy2D *New() { return new vtkInteractorStyleMy2D(); }
	void PrintSelf(ostream& os, vtkIndent indent);

	virtual void OnLeftButtonDown();
	virtual void OnLeftButtonUp();
	virtual void OnMiddleButtonDown();
	virtual void OnMiddleButtonUp();
	virtual void OnRightButtonDown();
	virtual void OnRightButtonUp();
	virtual void OnMouseMove();
	virtual void OnMouseWheelForward();
	virtual void OnMouseWheelBackward();

	//BTX
	// Description:
	// Selection types
	enum
	{
		SELECT_NORMAL = 0,
		SELECT_UNION = 1
	};
	//ETX

	// Description:
	// Current interaction state
	vtkGetMacro(Interaction, int);

	//BTX
	enum
	{
		NONE,
		PANNING,
		ZOOMING,
		ROTATING,
		SELECTING
	};
	//ETX

protected:
	vtkInteractorStyleMy2D();
	~vtkInteractorStyleMy2D();

	// The interaction mode
	int Interaction;

	/// Draws the selection rubber band
	void RedrawRubberBand();

	// The end position of the selection
	int StartPosition[2];

	// The start position of the selection
	int EndPosition[2];

	// The pixel array for the rubber band
	vtkUnsignedCharArray* PixelArray;

private:
	vtkInteractorStyleMy2D(const vtkInteractorStyleMy2D&); //Not implemented
	void operator=(const vtkInteractorStyleMy2D&); // Not implemented
};

#endif
