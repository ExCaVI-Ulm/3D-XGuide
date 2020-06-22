#ifndef PREPROCESSING_FILTER_WIDGET_H
#define PREPROCESSING_FILTER_WIDGET_H

#include "AbstractFilterWidget.h"
#include "ui_PreprocessingFilterWidget.h"

class GenericMotionCorrectionFilter;

/** Property widget for GenericMotionCorrectionFilter.
 */
class PreprocessingFilterWidget : public AbstractFilterWidget, protected Ui::PreprocessingFilterWidget
{
	Q_OBJECT

public:
	PreprocessingFilterWidget(GenericMotionCorrectionFilter* algorithm);
	PreprocessingFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile);
	virtual ~PreprocessingFilterWidget();

	virtual void updateGuiFromFilter();
	virtual void updateFilterProperties();
	virtual void loadFromSettings();
	virtual void writeToSettings();

protected:
private:

private slots :
	//void on_useUnsharpMasking_stateChanged(int state);
	//void on_useSobelFilter_stateChanged(int state);
	//void on_useScharrFilter_stateChanged(int state);
	void on_defaultButtonUnsharp_clicked();
	void on_defaultButtonSobel_clicked();
	void on_defaultButtonScharr_clicked();
};

#endif // PREPROCESSING_FILTER_WIDGET_H