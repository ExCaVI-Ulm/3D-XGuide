#include "PreprocessingFilterWidget.h"

#include "GenericMotionCorrectionFilter.h"

#include <qsettings.h>
#include <qvariant.h>

#include <qmessagebox>

PreprocessingFilterWidget::PreprocessingFilterWidget(GenericMotionCorrectionFilter* algorithm)
: AbstractFilterWidget(algorithm)
{
	setupUi(this);
}

PreprocessingFilterWidget::PreprocessingFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile)
: AbstractFilterWidget(algorithm, settingsFile)
{
	setupUi(this);
}

PreprocessingFilterWidget::~PreprocessingFilterWidget()
{
}

void PreprocessingFilterWidget::updateGuiFromFilter() {
	GenericMotionCorrectionFilter* algorithm = dynamic_cast<GenericMotionCorrectionFilter*>(myFilter);
	if(algorithm == NULL) return;

	useUnsharpMasking->setChecked(algorithm->GetUseUnsharpMasking());
	maskingSigma->setValue(algorithm->GetMaskingSigma());
	maskingWeight->setValue(algorithm->GetMaskingWeight());
	useSobelFilter->setChecked(algorithm->GetUseSobelFilter());
	ksizeSobel->setValue(algorithm->GetKSizeSobel());
	scaleSobel->setValue(algorithm->GetScaleSobel());
	deltaSobel->setValue(algorithm->GetDeltaSobel());
	useScharrFilter->setChecked(algorithm->GetUseScharrFilter());
	scaleScharr->setValue(algorithm->GetScaleScharr());
	deltaScharr->setValue(algorithm->GetDeltaScharr());
	useCensus->setChecked(algorithm->GetUseCensus());


}

void PreprocessingFilterWidget::updateFilterProperties()
{
	GenericMotionCorrectionFilter* algorithm = dynamic_cast<GenericMotionCorrectionFilter*>(myFilter);
	if (algorithm == NULL) return;

	algorithm->SetUseUnsharpMasking(useUnsharpMasking->isChecked());
	algorithm->SetMaskingSigma(maskingSigma->value());
	algorithm->SetMaskingWeight(maskingWeight->value());
	algorithm->SetUseSobelFilter(useSobelFilter->isChecked());
	algorithm->SetKSizeSobel(ksizeSobel->value());
	algorithm->SetScaleSobel(scaleSobel->value());
	algorithm->SetDeltaSobel(deltaSobel->value());
	algorithm->SetUseScharrFilter(useScharrFilter->isChecked());
	algorithm->SetScaleScharr(scaleScharr->value());
	algorithm->SetDeltaScharr(deltaScharr->value());
	algorithm->SetUseCensus(useCensus->isChecked());
}

void PreprocessingFilterWidget::loadFromSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	if(settings.status() != QSettings::NoError) {
		QMessageBox::information(this, "Load preprocessing filter settings", "The filter settings could not be loaded from file.");
	}

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("PreprocessingFilter");

	useUnsharpMasking->setChecked(settings.value("UseUnsharpMasking").toBool());
	maskingSigma->setValue(settings.value("MaskingSigma").toDouble());
	maskingWeight->setValue(settings.value("MaskingWeight").toDouble());

	useSobelFilter->setChecked(settings.value("UseSobelFilter").toBool());
	ksizeSobel->setValue(settings.value("KSizeSobel").toDouble());
	scaleSobel->setValue(settings.value("ScaleSobel").toDouble());
	deltaSobel->setValue(settings.value("DeltaSobel").toBool());

	useScharrFilter->setChecked(settings.value("UseScharrFilter").toBool());
	scaleScharr->setValue(settings.value("ScaleScharr").toDouble());
	deltaScharr->setValue(settings.value("DeltaScharr").toBool());

	useCensus->setChecked(settings.value("UseCensusTransformation").toBool());

	settings.endGroup();
	settings.endGroup();
}

void PreprocessingFilterWidget::writeToSettings()
{
	QSettings settings(mySettingsFile, QSettings::IniFormat);

	settings.beginGroup(mySettingsPrefix);
	settings.beginGroup("PreprocessingFilter");

	settings.setValue("UseUnsharpMasking", useUnsharpMasking->isChecked());
	settings.setValue("MaskingSigma", maskingSigma->value());
	settings.setValue("MaskingWeight", maskingWeight->value());

	settings.setValue("UseSobelFilter", useSobelFilter->isChecked());
	settings.setValue("KSizeSobel", ksizeSobel->value());
	settings.setValue("ScaleSobel", scaleSobel->value());
	settings.setValue("DeltaSobel", deltaSobel->value());

	settings.setValue("UseScharrFilter", useScharrFilter->isChecked());
	settings.setValue("ScaleScharr", scaleScharr->value());
	settings.setValue("DeltaScharr", deltaScharr->value());

	settings.setValue("UseCensusTransformation", useCensus->isChecked());

	settings.endGroup();	
	settings.endGroup();	
}

//void PreprocessingFilterWidget::on_useUnsharpMasking_stateChanged(int state)
//{
//	bool checked = (state == Qt::Checked);
//
//	sobelGroupBox->setDisabled(checked);
//	scharrGroupBox->setDisabled(checked);
//
//}
//
//void PreprocessingFilterWidget::on_useSobelFilter_stateChanged(int state)
//{
//	bool checked = (state == Qt::Checked);
//
//	unsharpMaskingGroupBox->setDisabled(checked);
//	scharrGroupBox->setDisabled(checked);
//
//}
//
//void PreprocessingFilterWidget::on_useScharrFilter_stateChanged(int state)
//{
//	bool checked = (state == Qt::Checked);
//
//	unsharpMaskingGroupBox->setDisabled(checked);
//	sobelGroupBox->setDisabled(checked);
//
//}

void PreprocessingFilterWidget::on_defaultButtonUnsharp_clicked()
{
	//useUnsharpMasking->setChecked(1);
	maskingSigma->setValue(3.0);
	maskingWeight->setValue(5.0);

}

void PreprocessingFilterWidget::on_defaultButtonSobel_clicked()
{
	//useSobelFilter->setChecked(1);
	scaleSobel->setValue(2);
	deltaSobel->setValue(0);
	ksizeSobel->setValue(3);

}

void PreprocessingFilterWidget::on_defaultButtonScharr_clicked()
{
	//useScharrFilter->setChecked(1);
	scaleScharr->setValue(1);
	deltaScharr->setValue(0);

}
