#ifndef MOVE_MESH_INTERACTION_COMMAND
#define MOVE_MESH_INTERACTION_COMMAND

#include <QUndoCommand>

#include <QString>
class OverlayScene;

/** Provides undo/redo functionality for moving the meshes in the scene.
  * Stores the assembly's matrix and the window in which the interaction
  * was performed.
  */
class MoveMeshInteractionCommand : public QUndoCommand {
public:
	MoveMeshInteractionCommand(OverlayScene* scene, int prevWindow, double prevMatrix[4][4], int window, double matrix[4][4]);
	virtual ~MoveMeshInteractionCommand();

	virtual void undo();
	virtual void redo();

private:
	OverlayScene* scene;
	
	double previousMatrix[4][4];
	int previousWindow;

	double matrix[4][4];
	int window;

	static QString description[3];
};


#endif //MOVE_MESH_INTERACTION_COMMAND