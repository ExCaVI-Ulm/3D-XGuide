#include "vktInteractorStyleTrackballJoystickHybridActor.h"

#include <vtkProp3d.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTransform.h>

// This implementation is a unmodified copy of vtkInteractorStyleJoystickActor::Pan()
void vktInteractorStyleTrackballJoystickHybridActor::Pan()
{
	if(this->CurrentRenderer == NULL || this->InteractionProp == NULL)
	{
		return;
	}

	vtkRenderWindowInteractor *rwi = this->Interactor;

	// Use initial center as the origin from which to pan
	double *obj_center = this->InteractionProp->GetCenter();

	double disp_obj_center[3], new_pick_point[4], motion_vector[3];

	this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
		disp_obj_center);

	this->ComputeDisplayToWorld(rwi->GetEventPosition()[0], 
		rwi->GetEventPosition()[1],
		disp_obj_center[2],
		new_pick_point);

	// Compute a translation vector, moving everything 1/10
	// the distance to the cursor. (Arbitrary scale factor)
	motion_vector[0] = (new_pick_point[0] - obj_center[0]) / this->MotionFactor;
	motion_vector[1] = (new_pick_point[1] - obj_center[1]) / this->MotionFactor;
	motion_vector[2] = (new_pick_point[2] - obj_center[2]) / this->MotionFactor;

	if (this->InteractionProp->GetUserMatrix() != NULL)
	{
		vtkTransform *t = vtkTransform::New();
		t->PostMultiply();
		t->SetMatrix(this->InteractionProp->GetUserMatrix());
		t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
		this->InteractionProp->GetUserMatrix()->DeepCopy(t->GetMatrix());
		t->Delete();
	}
	else
	{
		this->InteractionProp->AddPosition(motion_vector[0],
			motion_vector[1],
			motion_vector[2]);
	}

	rwi->Render();
}

