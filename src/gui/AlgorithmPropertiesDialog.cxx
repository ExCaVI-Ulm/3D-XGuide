#include "AlgorithmPropertiesDialog.h"

#include "AbstractFilterWidget.h"

#include <qcheckbox.h>

#include <iostream>
using namespace std;
using std::vector;

AlgorithmPropertiesDialog::AlgorithmPropertiesDialog(QWidget* parent, FilterWidgetVector widgets, QString settingsFile, QString settingsPrefix)
: QDialog(parent)
{
	setupUi(this);

	setModal(true);
	setWindowTitle("Properties");

	//fileNameLineEdit->setText(settingsFile);

	filterWidgets = widgets;
	this->settingsPrefix = settingsPrefix;

	// update and show all widgets
	for(FilterWidgetVector::iterator w = filterWidgets.begin(); w != filterWidgets.end(); ++w) {
		(*w)->updateGuiFromFilter();
		contentLayout->addWidget(*w);
	}
}

AlgorithmPropertiesDialog::~AlgorithmPropertiesDialog()
{
	// clean up
	for(FilterWidgetVector::iterator w = filterWidgets.begin(); w != filterWidgets.end(); ++w) {
		delete (*w);
	}
}

void AlgorithmPropertiesDialog::accept()
{
	// let the widgets update it's filter
	for(FilterWidgetVector::iterator w = filterWidgets.begin(); w != filterWidgets.end(); ++w) {
		(*w)->updateFilterProperties();
	}

	// closes the dialog etc.
	this->QDialog::accept();
}

void AlgorithmPropertiesDialog::on_saveButton_clicked()
{
	QString file = QFileDialog::getSaveFileName(this, "Save settings", "", "TXT file (*.txt)");
	if(file.isNull()) return;
	fileNameLineEdit->setText(file);
	for(FilterWidgetVector::iterator w = filterWidgets.begin(); w != filterWidgets.end(); ++w) {
		(*w)->setSettingsFileAndPrefix(file, settingsPrefix);
		(*w)->writeToSettings();
	}
}

void AlgorithmPropertiesDialog::on_loadButton_clicked()
{
	QString file = QFileDialog::getOpenFileName(this, "Load settings", ".", "Settings (*.txt)");
	if(file.isNull()) return;
	fileNameLineEdit->setText(file);
	for(FilterWidgetVector::iterator w = filterWidgets.begin(); w != filterWidgets.end(); ++w) {
		(*w)->setSettingsFileAndPrefix(file, settingsPrefix);
		(*w)->loadFromSettings();
	}
}