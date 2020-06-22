#include "CrosscorrelationFilterWidget.h"

#include "GenericMotionCorrectionFilter.h"
#include "CrosscorrelationFilter.h"
#include "SimpleBiplaneFilter.h"

#include <qsettings.h>
#include <qvariant.h>

#include <qmessagebox>

CrosscorrelationFilterWidget::CrosscorrelationFilterWidget(GenericMotionCorrectionFilter* algorithm)
: AbstractFilterWidget(algorithm)
{
	setupUi(this);
}

CrosscorrelationFilterWidget::CrosscorrelationFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile)
: AbstractFilterWidget(algorithm, settingsFile)
{
	setupUi(this);
}

CrosscorrelationFilterWidget::~CrosscorrelationFilterWidget()
{
}

void CrosscorrelationFilterWidget::on_useROI_stateChanged(int state)
{
	bool checked = (state == Qt::Checked);

	restrictY->setEnabled(checked);

}

void CrosscorrelationFilterWidget::updateGuiFromFilter() {
	CrosscorrelationFilter* algorithm = dynamic_cast<CrosscorrelationFilter*>(myFilter);
	if(algorithm == NULL) return;

	tplSizeX->setValue(algorithm->getTplSize(0));
	tplSizeY->setValue(algorithm->getTplSize(1));
	useROI->setChecked(algorithm->getUseROI());
	restrictY->setChecked(algorithm->getRestrictY());
	roiSize->setValue(algorithm->getFactorROISize());
	useBasicTpl->setChecked(algorithm->getUseBasicTpl());
	useModelBasedTpl->setChecked(algorithm->getUseModelBasedTpl());
	numberOfRotations->setValue(algorithm->getnumberOfRotations());
	markersVisible->setChecked(algorithm->getMarkersVisible());
	//useUnsharpMasking->setChecked(algorithm->getUseUnsharpMasking());
	//maskingSigma->setValue(algorithm->getUnsharpMaskingSigma());
	//maskingAmount->setValue(algorithm->getUnsharpMaskingWeight());
}

void CrosscorrelationFilterWidget::updateFilterProperties()
{
	CrosscorrelationFilter* algorithm = dynamic_cast<CrosscorrelationFilter*>(myFilter);
	if(algorithm == NULL) return;

	algorithm->setTplSize(tplSizeX->value(), tplSizeY->value());
	algorithm->setUseROI(useROI->isChecked());
	algorithm->setFactorROISize(roiSize->value());
	algorithm->setUseBasicTpl(useBasicTpl->isChecked());
	algorithm->setUseModelBasedTpl(useModelBasedTpl->isChecked());
	algorithm->setnumberOfRotations(numberOfRotations->value());
	algorithm->setMarkersVisible(markersVisible->isChecked());
	algorithm->setRestrictY(restrictY->isChecked());
	//algorithm->setUseUnsharpMasking(useUnsharpMasking->isChecked());
	//algorithm->setUnsharpMaskingSigma(maskingSigma->value());
	//algorithm->setUnsharpMaskingWeight(maskingAmount->value());
}

void CrosscorrelationFilterWidget::loadFromSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	if(settings.status() != QSettings::NoError) {
		QMessageBox::information(this, "Load filter settings", "The filter settings could not be loaded from file.");
	}

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("CrosscorrelationFilterWidget");

	tplSizeX->setValue(settings.value("TplSiteX").toInt());
	tplSizeY->setValue(settings.value("TplSiteY").toInt());
	useROI->setChecked(settings.value("UseRoi").toBool());
	roiSize->setValue(settings.value("RoiSize").toDouble());
	useBasicTpl->setChecked(settings.value("UseBasicTpl").toBool());
	useModelBasedTpl->setChecked(settings.value("UseModelBasedTpl").toBool());
	numberOfRotations->setValue(settings.value("NumberOfRotations").toInt());
	markersVisible->setChecked(settings.value("MarkersVisible").toBool());
	//useUnsharpMasking->setChecked(settings.value("UseUnsharpMasking").toBool());
	//maskingSigma->setValue(settings.value("MaskingSigma").toDouble());
	//maskingAmount->setValue(settings.value("MaskingWeight").toDouble());

	settings.endGroup();
	settings.endGroup();
}

void CrosscorrelationFilterWidget::writeToSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("CrosscorrelationFilterWidget");

	settings.setValue("TplSiteX", tplSizeX->value());
	settings.setValue("TplSiteY", tplSizeY->value());
	settings.setValue("UseRoi", useROI->isChecked());
	settings.setValue("RoiSize", roiSize->value());
	settings.setValue("UseBasicTpl", useBasicTpl->isChecked());
	settings.setValue("UseModelBasedTpl", useModelBasedTpl->isChecked());
	settings.setValue("NumberOfRotations", numberOfRotations->value());
	settings.setValue("MarkersVisible", markersVisible->isChecked());
	//settings.setValue("UseUnsharpMasking", useUnsharpMasking->isChecked());
	//settings.setValue("MaskingSigma", maskingSigma->value());
	//settings.setValue("MaskingWeight", maskingAmount->value());

	settings.endGroup();	
	settings.endGroup();	
}