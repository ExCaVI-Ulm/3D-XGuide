#ifndef VTK_INTERACTOR_STYLE_2DROTATE_ZOOM_WITH_VIEW_ANGLE_H
#define VTK_INTERACTOR_STYLE_2DROTATE_ZOOM_WITH_VIEW_ANGLE_H

#include <vtkInteractorStyleImage.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>

/** With vtkInteractorStyleOnlyZoomWithViewAngle, the user can only zoom.
// However 2D rotation is left, in order to have levelWindow function - can be changed: TODO!!!
  * No camera rotation or movement.
  * Zoom is performed by changing the camera's view angle, not by moving
  * the camera in the scene.
  */
class vtkInteractorStyle2DRotateZoomWithViewAngle : public vtkInteractorStyleImage {
public:
	static vtkInteractorStyle2DRotateZoomWithViewAngle *New() { return new vtkInteractorStyle2DRotateZoomWithViewAngle(); }

	// the following methods shall behave like in the superclass
	// this is levelWindow function with left mousebutton
	// and 2D rotation with ctrl+left mousebutton
	//virtual void OnMouseMove() {}
	virtual void OnLeftButtonDown() {}
	virtual void OnLeftButtonUp() {}

	// do not react to this events
	virtual void OnMiddleButtonDown() {}
	virtual void OnMiddleButtonUp() {}

	// the following methods shall behave like in the superclass
	// for zooming with right mousebutton and wheel
	//virtual void OnRightButtonDown() {}
	//virtual void OnRightButtonUp() {}
	//virtual void OnMouseWheelBackward() {}
	//virtual void OnMouseWheelForward() {}

protected:
	virtual void Dolly(double factor) {
		vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
		// instead of moving the camera, the view angle is modified
		camera->Zoom(factor);
	}
};

#endif // VTK_INTERACTOR_STYLE_2DROTATE_ZOOM_WITH_VIEW_ANGLE_H