#include "MoveMeshInteractionCommand.h"

#include "OverlayScene.h"

QString MoveMeshInteractionCommand::description[3] = { "Reference 1", "Reference 2", "3D View"};

MoveMeshInteractionCommand::
MoveMeshInteractionCommand(OverlayScene* scene,
						   int prevWindow,
						   double prevMatrix[4][4],
						   int window,
						   double matrix[4][4])
{
	this->scene = scene;

	this->previousWindow = prevWindow;
	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			this->previousMatrix[i][j] = prevMatrix[i][j];
		}
	}

	this->window = window;
	for(int i = 0; i < 4; ++i) {
		for(int j = 0; j < 4; ++j) {
			this->matrix[i][j] = matrix[i][j];
		}
	}

	setText(QString("Mesh Movement in %1").arg(description[window]));
}

MoveMeshInteractionCommand::~MoveMeshInteractionCommand()
{
}

void MoveMeshInteractionCommand::undo()
{
	if(scene != 0 && previousWindow >= 0) {
		scene->setUserMatrix(previousWindow, previousMatrix);
	}
}

void MoveMeshInteractionCommand::redo()
{
	scene->setUserMatrix(window, matrix);
}