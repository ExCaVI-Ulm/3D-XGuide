#include "vtkInteractorStyleMy2D.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWorldPointPicker.h"

//--------------------------------------------------------------------------
vtkInteractorStyleMy2D::vtkInteractorStyleMy2D()
{
  this->PixelArray = vtkUnsignedCharArray::New();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
vtkInteractorStyleMy2D::~vtkInteractorStyleMy2D()
{
  this->PixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnLeftButtonDown()
{
  if(this->Interaction == NONE)
    {
    this->Interaction = SELECTING;
    vtkRenderWindow *renWin = this->Interactor->GetRenderWindow();
    
    this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
    this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
    this->EndPosition[0] = this->StartPosition[0];
    this->EndPosition[1] = this->StartPosition[1];
    
    this->PixelArray->Initialize();
    this->PixelArray->SetNumberOfComponents(4);
    int *size = renWin->GetSize();
    this->PixelArray->SetNumberOfTuples(size[0]*size[1]);
    
    renWin->GetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, 1, this->PixelArray);
    this->FindPokedRenderer(this->StartPosition[0], this->StartPosition[1]);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnLeftButtonUp()
{
	/*vtkRenderer* ren = this->GetDefaultRenderer();

	int* clickPos = this->GetInteractor()->GetEventPosition();
	vtkWorldPointPicker * picker = vtkWorldPointPicker::New();
	picker->Pick(clickPos[0], clickPos[1], 0, ren);
	
	double pos[3];
	pos[0] = clickPos[0];
	pos[1] = clickPos[1];
	pos[2] = 0;

	ren->SetDisplayPoint(pos);
	ren->DisplayToWorld();*/

	int* clickPos = this->GetInteractor()->GetEventPosition();
	vtkWorldPointPicker * picker = vtkWorldPointPicker::New();
	picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

	double* pos = picker->GetPickPosition();
	this->InvokeEvent(vtkCommand::SelectionChangedEvent, reinterpret_cast<void*>(pos));
	this->InvokeEvent(vtkCommand::EndInteractionEvent);
	this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnMiddleButtonDown()
{
  if(this->Interaction == NONE)
    {
    this->Interaction = PANNING;
    this->FindPokedRenderer(
      this->Interactor->GetEventPosition()[0], 
      this->Interactor->GetEventPosition()[1]);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnMiddleButtonUp()
{
  if(this->Interaction == PANNING)
    {
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->Interaction = NONE;
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnRightButtonDown()
{
  if(this->Interaction == NONE)
    {
		this->Interaction = ZOOMING;

		this->FindPokedRenderer(
			this->Interactor->GetEventPosition()[0], 
			this->Interactor->GetEventPosition()[1]);
		this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnRightButtonUp()
{
  if(this->Interaction == ZOOMING || this->Interaction == ROTATING)
    {
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->Interaction = NONE;
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnMouseMove()
{
  if (this->Interaction == PANNING)
    {
    this->Superclass::Pan();
    }
  else if (this->Interaction == ZOOMING)
    {
    this->Superclass::Dolly();
    }
  else if (this->Interaction == ROTATING)
    {
    this->Superclass::Rotate();
    }
  else if (this->Interaction == SELECTING)
    {
    this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
    this->EndPosition[1] = this->Interactor->GetEventPosition()[1];  
    int *size = this->Interactor->GetRenderWindow()->GetSize();  
    if (this->EndPosition[0] > (size[0]-1))
      {
      this->EndPosition[0] = size[0]-1;
      }
    if (this->EndPosition[0] < 0)
      {
      this->EndPosition[0] = 0;
      }
    if (this->EndPosition[1] > (size[1]-1))
      {
      this->EndPosition[1] = size[1]-1;
      }
    if (this->EndPosition[1] < 0)
      {
      this->EndPosition[1] = 0;
      }
    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->RedrawRubberBand();
    }
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnMouseWheelForward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], 
    this->Interactor->GetEventPosition()[1]);
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (!camera)
    {
    return;
    }
  this->Interaction = ZOOMING;
  this->Superclass::OnMouseWheelForward();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::OnMouseWheelBackward()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], 
    this->Interactor->GetEventPosition()[1]);
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (!camera)
    {
    return;
    }
  this->Interaction = ZOOMING;
  this->Superclass::OnMouseWheelBackward();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::RedrawRubberBand()
{
  // Update the rubber band on the screen
  int *size = this->Interactor->GetRenderWindow()->GetSize();  

  vtkUnsignedCharArray *tmpPixelArray = vtkUnsignedCharArray::New();
  tmpPixelArray->DeepCopy(this->PixelArray);  
  unsigned char *pixels = tmpPixelArray->GetPointer(0);

  int min[2], max[2];

  min[0] = this->StartPosition[0] <= this->EndPosition[0] ?
    this->StartPosition[0] : this->EndPosition[0];
  if (min[0] < 0) { min[0] = 0; }
  if (min[0] >= size[0]) { min[0] = size[0] - 1; }

  min[1] = this->StartPosition[1] <= this->EndPosition[1] ?
    this->StartPosition[1] : this->EndPosition[1];
  if (min[1] < 0) { min[1] = 0; }
  if (min[1] >= size[1]) { min[1] = size[1] - 1; }

  max[0] = this->EndPosition[0] > this->StartPosition[0] ?
    this->EndPosition[0] : this->StartPosition[0];
  if (max[0] < 0) { max[0] = 0; }
  if (max[0] >= size[0]) { max[0] = size[0] - 1; }

  max[1] = this->EndPosition[1] > this->StartPosition[1] ?
    this->EndPosition[1] : this->StartPosition[1];
  if (max[1] < 0) { max[1] = 0; }
  if (max[1] >= size[1]) { max[1] = size[1] - 1; }

  int i;
  for (i = min[0]; i <= max[0]; i++)
    {
    pixels[4*(min[1]*size[0]+i)] = 255 ^ pixels[4*(min[1]*size[0]+i)];
    pixels[4*(min[1]*size[0]+i)+1] = 255 ^ pixels[4*(min[1]*size[0]+i)+1];
    pixels[4*(min[1]*size[0]+i)+2] = 255 ^ pixels[4*(min[1]*size[0]+i)+2];
    pixels[4*(max[1]*size[0]+i)] = 255 ^ pixels[4*(max[1]*size[0]+i)];
    pixels[4*(max[1]*size[0]+i)+1] = 255 ^ pixels[4*(max[1]*size[0]+i)+1];
    pixels[4*(max[1]*size[0]+i)+2] = 255 ^ pixels[4*(max[1]*size[0]+i)+2];
    }
  for (i = min[1]+1; i < max[1]; i++)
    {
    pixels[4*(i*size[0]+min[0])] = 255 ^ pixels[4*(i*size[0]+min[0])];
    pixels[4*(i*size[0]+min[0])+1] = 255 ^ pixels[4*(i*size[0]+min[0])+1];
    pixels[4*(i*size[0]+min[0])+2] = 255 ^ pixels[4*(i*size[0]+min[0])+2];
    pixels[4*(i*size[0]+max[0])] = 255 ^ pixels[4*(i*size[0]+max[0])];
    pixels[4*(i*size[0]+max[0])+1] = 255 ^ pixels[4*(i*size[0]+max[0])+1];
    pixels[4*(i*size[0]+max[0])+2] = 255 ^ pixels[4*(i*size[0]+max[0])+2];
    }
  
  this->Interactor->GetRenderWindow()->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, pixels, 0);
  this->Interactor->GetRenderWindow()->Frame();
  
  tmpPixelArray->Delete();
}

//--------------------------------------------------------------------------
void vtkInteractorStyleMy2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interaction: " << this->Interaction << endl;
}
