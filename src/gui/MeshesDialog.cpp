#include "MeshesDialog.h"

#include "OverlayScene.h"

#include <QStringList>
#include <QFileDialog>
#include <QColorDialog>
//
//#include <vtkRenderWindow.h>
//#include <vtkRenderer.h>

#include <vector>
using namespace std;

MeshesDialog::MeshesDialog(QWidget *parent, OverlayScene* scene)
: QWidget(parent)
{
	lastDirectory = "B:/NAVIGATION";
	setupUi(this);

	this->scene = scene;

	doNotWriteOnSliderUpdate = false;
	updateList();

	addMeshButton->setEnabled(false);


}

void MeshesDialog::activateAddMeshButton(bool activate)
{
	addMeshButton->setEnabled(activate);

}

MeshesDialog::~MeshesDialog()
{
   
}

void MeshesDialog::update()
{
	updateList();
}


void MeshesDialog::on_addMeshButton_clicked()
{
	QString fileFormats("Mesh File (*.vtk)");
	QStringList fileNames = QFileDialog::getOpenFileNames(this, "Select overlay file(s)", lastDirectory, fileFormats);
	if (fileNames.empty()) return; // user cancelled

	QStringList files = fileNames;	// one should iterate over a copy of the returned list, according to Qt's docu!
	for (QStringList::Iterator file = files.begin(); file != files.end(); ++file)
		{
			scene->addOverlayMesh(file->toLocal8Bit().constData());
		}

	int pos = files.front().lastIndexOf('/');
	lastDirectory = files.front().left(pos);
	
	updateList();

	//// ensure that visibility for added meshes is according to checkbox
	scene->setMeshVisibility(!hideMeshesCheckBox->isChecked());
	
}

void MeshesDialog::on_removeMeshButton_clicked()
{
	if (meshesList->currentRow() < 0) return; // nothing selected

	scene->removeOverlayMesh(meshesList->currentRow());

	updateList();
}

void MeshesDialog::updateList()
{
	vector<const char*> fileNamesMeshes = scene->getMeshFileNames();

	// copy fileNames to QStringList
	QStringList files;
	for (unsigned int i = 0; i < fileNamesMeshes.size(); ++i)
	{ 
		files << fileNamesMeshes[i];
		//files << QString::fromStdString(fileNamesMeshes[i]);
	}

	meshesList->clear();
	meshesList->addItems(files);
	hideMeshesCheckBox->setChecked(false);
}

void MeshesDialog::on_hideMeshesCheckBox_stateChanged(int state)
{
	scene->setMeshVisibility(!hideMeshesCheckBox->isChecked());	
}


void MeshesDialog::on_changeColorButton_clicked()
{
	QColor col1 = changeColorButton->palette().color(QPalette::Button);
	QColor col2 = QColorDialog::getColor(col1, this, "Select mesh color");
	if (!col2.isValid()) return; // user cancelled

	int r, g, b;
	col2.getRgb(&r, &g, &b);
	double color[3];
	color[0] = r / 255.0;
	color[1] = g / 255.0;
	color[2] = b / 255.0;

	for (int i = 0; i < meshesList->count(); ++i) {
		if (meshesList->item(i)->isSelected()) {
			scene->setMeshColor(color, i);
		}
		else scene->giveMeshColorToScene(color);
	}

	if (meshesList->count() == 0)
	{
		scene->giveMeshColorToScene(color);
	}
		

	changeColorButton->setPalette(col2);
}

void MeshesDialog::on_opacitySlider_valueChanged(int value)
{
	if (doNotWriteOnSliderUpdate) return;

	const double opacity = opacitySlider->value() / 100.0;

	for (int i = 0; i < meshesList->count(); ++i) {
		if (meshesList->item(i)->isSelected()) {
			scene->setMeshOpacity(opacity, i);
		}
		else scene->giveMeshOpacityToScene(opacity);

	}
	if (meshesList->count() == 0)
	{
		scene->giveMeshOpacityToScene(opacity);
	}
}

void MeshesDialog::on_meshesList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (meshesList->currentRow() < 0) return; // nothing selected

	double color[3];
	scene->getMeshColor(color, meshesList->currentRow());
	QColor col((int)(color[0] * 255.0), (int)(color[1] * 255.0), (int)(color[2] * 255.0));
	changeColorButton->setPalette(col);

	// Supress slider writing here!
	doNotWriteOnSliderUpdate = true;
	opacitySlider->setValue((int)(scene->getMeshOpacity(meshesList->currentRow())*100.0));
	doNotWriteOnSliderUpdate = false;
}


