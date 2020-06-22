#ifndef VTK_INTERACTOR_STYLE_TRACKBALL_JOYSTICK_HYBRID_ACTOR_H
#define VTK_INTERACTOR_STYLE_TRACKBALL_JOYSTICK_HYBRID_ACTOR_H

#include <vtkInteractorStyleTrackballActor.h>
/** The class vktInteractorStyleTrackballJoystickHybridActor behaves like vtkInteractorStyleTrackballActor,
  * but Pan() is implemented like in vtkInteractorStyleJoystickActor.
  * Thus, actors are moved to the cursor position instead of a movement proportional to the mouse movement.
  * Moreover zooming with right mousebutton should not be implemented, since it zoom the individual objects, not the scene
  */
class vktInteractorStyleTrackballJoystickHybridActor : public vtkInteractorStyleTrackballActor
{
public:
	static vktInteractorStyleTrackballJoystickHybridActor *New() { return new vktInteractorStyleTrackballJoystickHybridActor(); }

	virtual void Pan();
	// do not react on these events (this is zooming of individual object)
	virtual void OnRightButtonDown() {}
	virtual void OnRightButtonUp() {}
};

#endif