#ifndef VTK_INTERACTOR_STYLE_ONLY_ZOOM_WITH_VIEW_ANGLE_H
#define VTK_INTERACTOR_STYLE_ONLY_ZOOM_WITH_VIEW_ANGLE_H

#include <vtkInteractorStyle2DRotateZoomWithViewAngle.h>

/** With vtkInteractorStyleOnlyZoomWithViewAngle, the user can only zoom.
  * No camera rotation or movement.
  * Zoom is performed by changing the camera's view angle, not by moving
  * the camera in the scene.
  */
class vtkInteractorStyleOnlyZoomWithViewAngle : public vtkInteractorStyle2DRotateZoomWithViewAngle {
public:
	static vtkInteractorStyleOnlyZoomWithViewAngle *New() { return new vtkInteractorStyleOnlyZoomWithViewAngle(); }

	// do not react to this events - exclude all kind of rotations 2D and 3D and level window
	virtual void OnMouseMove() {}
	virtual void OnLeftButtonDown() {}
	virtual void OnLeftButtonUp() {}
	
};

#endif // VTK_INTERACTOR_STYLE_ONLY_ZOOM_WITH_VIEW_ANGLE_H