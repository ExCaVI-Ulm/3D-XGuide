#include "DLFilterWidget.h"

#include "GenericMotionCorrectionFilter.h"

#include <qsettings.h>
#include <qvariant.h>

#include <qmessagebox>

DLFilterWidget::DLFilterWidget(GenericMotionCorrectionFilter* algorithm)
: AbstractFilterWidget(algorithm)
{
	setupUi(this);

}

DLFilterWidget::DLFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile)
: AbstractFilterWidget(algorithm, settingsFile)
{
	setupUi(this);
}

DLFilterWidget::~DLFilterWidget()
{
}

void DLFilterWidget::updateGuiFromFilter() {
	GenericMotionCorrectionFilter* algorithm = dynamic_cast<GenericMotionCorrectionFilter*>(myFilter);
	if(algorithm == NULL) return;

	useDLManualScaling->setChecked(algorithm->GetUseDLManualScaling());

	moveX->setChecked(algorithm->GetMoveX());
	moveY->setChecked(algorithm->GetMoveY());

	scaleX->setValue(algorithm->GetDLScaleX());
	scaleY->setValue(algorithm->GetDLScaleY());

	scaleX->setDisabled(!useDLManualScaling->isChecked());
	scaleY->setDisabled(!useDLManualScaling->isChecked());

}

void DLFilterWidget::updateFilterProperties()
{
	GenericMotionCorrectionFilter* algorithm = dynamic_cast<GenericMotionCorrectionFilter*>(myFilter);
	if(algorithm == NULL) return;

	algorithm->SetUseDLManualScaling(useDLManualScaling->isChecked());

	algorithm->SetMoveX(moveX->isChecked());
	algorithm->SetMoveY(moveY->isChecked());

	algorithm->SetDLScaleX(scaleX->value());
	algorithm->SetDLScaleY(scaleY->value());

}

void DLFilterWidget::loadFromSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	if(settings.status() != QSettings::NoError) {
		QMessageBox::information(this, "Load DL filter settings", "The filter settings could not be loaded from file.");
	}

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("DLFilter");

	useDLManualScaling->setChecked(settings.value("UseManualScaling").toBool());

	moveX->setChecked(settings.value("MoveX").toBool());
	moveY->setChecked(settings.value("MoveY").toBool());

	scaleX->setValue(settings.value("ScaleX").toDouble());
	scaleY->setValue(settings.value("ScaleY").toDouble());
	
	settings.endGroup();
	settings.endGroup();
}

void DLFilterWidget::writeToSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("DLFilter");

	settings.setValue("UseManualScaling", useDLManualScaling->isChecked());

	settings.setValue("MoveX", moveX->isChecked());
	settings.setValue("MoveY", moveY->isChecked());

	settings.setValue("ScaleX", scaleX->value());
	settings.setValue("ScaleY", scaleY->value());
	
	settings.endGroup();	
	settings.endGroup();	
}

void DLFilterWidget::on_useDLManualScaling_stateChanged(int state)
{
	bool checked = (state == Qt::Checked);

	scaleX->setDisabled(!checked);
	scaleY->setDisabled(!checked);

}