#ifndef MESHESDIALOG_H
#define MESHESDIALOG_H

#include <QWidget>
#include "ui_MeshesDialog.h"

class OverlayScene;

class MeshesDialog : public QWidget, protected Ui::MeshesDialog
{
	Q_OBJECT

public:
	MeshesDialog(QWidget* parent, OverlayScene* scene);
	virtual ~MeshesDialog();

	void update();

	void activateAddMeshButton(bool activate);

private:
	OverlayScene* scene;
	QString lastDirectory;

	// Suppress slider writing values of old selection to the scene
	bool doNotWriteOnSliderUpdate;

	void updateList();


signals:

	private slots :
	void on_addMeshButton_clicked();
	void on_removeMeshButton_clicked();
	void on_hideMeshesCheckBox_stateChanged(int state);
	void on_changeColorButton_clicked();
	void on_opacitySlider_valueChanged(int value);
	void on_meshesList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

	
};

#endif // MESHESDIALOG_H
