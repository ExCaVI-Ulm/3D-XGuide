#include "MotionCorrectionFilterWidget.h"

#include "GenericMotionCorrectionFilter.h"

#include <qsettings.h>
#include <qvariant.h>

#include <qmessagebox>

MotionCorrectionFilterWidget::MotionCorrectionFilterWidget(GenericMotionCorrectionFilter* algorithm)
: AbstractFilterWidget(algorithm)
{
	setupUi(this);
}

MotionCorrectionFilterWidget::MotionCorrectionFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile)
: AbstractFilterWidget(algorithm, settingsFile)
{
	setupUi(this);
}

MotionCorrectionFilterWidget::~MotionCorrectionFilterWidget()
{
}

void MotionCorrectionFilterWidget::updateGuiFromFilter() {
	GenericMotionCorrectionFilter* algorithm = dynamic_cast<GenericMotionCorrectionFilter*>(myFilter);
	if(algorithm == NULL) return;

	enableFilter->setChecked(algorithm->GetProcessImages());
	useManualScaling->setChecked(algorithm->GetUseManualScaling());
	
	moveHorizontally->setChecked(algorithm->GetMoveHorizontally());
	moveVertically->setChecked(algorithm->GetMoveVertically());
	moveAlongZAxis->setChecked(algorithm->GetMoveAlongZAxis());
	
	shiftHorizontally->setValue(algorithm->GetShiftX());
	shiftVertically->setValue(algorithm->GetShiftY());
	shiftAlongZAxis->setValue(algorithm->GetShiftZ());
	
	scaleHorizontally->setValue(algorithm->GetScaleX());
	scaleVertically->setValue(algorithm->GetScaleY());
	scaleAlongZAxis->setValue(algorithm->GetScaleZ());

	scaleHorizontally->setDisabled(!useManualScaling->isChecked());
	scaleVertically->setDisabled(!useManualScaling->isChecked());
	scaleAlongZAxis->setDisabled(!useManualScaling->isChecked());
}

void MotionCorrectionFilterWidget::updateFilterProperties()
{
	GenericMotionCorrectionFilter* algorithm = dynamic_cast<GenericMotionCorrectionFilter*>(myFilter);
	if(algorithm == NULL) return;

	algorithm->SetProcessImages(enableFilter->isChecked());
	algorithm->SetUseManualScaling(useManualScaling->isChecked());

	algorithm->SetMoveHorizontally(moveHorizontally->isChecked());
	algorithm->SetMoveVertically(moveVertically->isChecked());
	algorithm->SetMoveAlongZAxis(moveAlongZAxis->isChecked());

	algorithm->SetShiftX(shiftHorizontally->value());
	algorithm->SetShiftY(shiftVertically->value());
	algorithm->SetShiftZ(shiftAlongZAxis->value());

	algorithm->SetScaleX(scaleHorizontally->value());
	algorithm->SetScaleY(scaleVertically->value());
	algorithm->SetScaleZ(scaleAlongZAxis->value());
}

void MotionCorrectionFilterWidget::loadFromSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	if(settings.status() != QSettings::NoError) {
		QMessageBox::information(this, "Load filter settings", "The filter settings could not be loaded from file.");
	}

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("GenericMotionCorrectionFilter");

	enableFilter->setChecked(settings.value("EnableFilter").toBool());
	
	moveHorizontally->setChecked(settings.value("MoveHorizontally").toBool());
	moveVertically->setChecked(settings.value("MoveVertically").toBool());
	moveAlongZAxis->setChecked(settings.value("MoveAlongZAxis").toBool());
	
	shiftHorizontally->setValue(settings.value("ShiftHorizontally").toDouble());
	shiftVertically->setValue(settings.value("ShiftVertically").toDouble());
	shiftAlongZAxis->setValue(settings.value("ShiftAlongZAxis").toDouble());
	
	scaleHorizontally->setValue(settings.value("ScaleHorizontally").toDouble());
	scaleVertically->setValue(settings.value("ScaleVertically").toDouble());
	scaleAlongZAxis->setValue(settings.value("ScaleAlongZAxis").toDouble());

	settings.endGroup();
	settings.endGroup();
}

void MotionCorrectionFilterWidget::writeToSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("GenericMotionCorrectionFilter");

	settings.setValue("EnableFilter", enableFilter->isChecked());
	
	settings.setValue("MoveHorizontally", moveHorizontally->isChecked());
	settings.setValue("MoveVertically", moveVertically->isChecked());
	settings.setValue("MoveAlongZAxis", moveAlongZAxis->isChecked());
	
	settings.setValue("ShiftHorizontally", shiftHorizontally->value());
	settings.setValue("ShiftVertically", shiftVertically->value());
	settings.setValue("ShiftAlongZAxis", shiftAlongZAxis->value());
	
	settings.setValue("ScaleHorizontally", scaleHorizontally->value());
	settings.setValue("ScaleVertically", scaleVertically->value());
	settings.setValue("ScaleAlongZAxis", scaleAlongZAxis->value());

	settings.endGroup();	
	settings.endGroup();	
}

void MotionCorrectionFilterWidget::on_useManualScaling_stateChanged(int state)
{
	bool checked = (state == Qt::Checked);

	scaleHorizontally->setDisabled(!checked);
	scaleVertically->setDisabled(!checked);
	scaleAlongZAxis->setDisabled(!checked);
}