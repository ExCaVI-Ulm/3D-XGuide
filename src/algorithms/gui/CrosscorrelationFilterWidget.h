#ifndef CROSSCORRELTAION_FILTER_WIDGET_H
#define CROSSCORRELTAION_FILTER_WIDGET_H

#include "AbstractFilterWidget.h"
#include "ui_CrosscorrelationFilterWidget.h"

class GenericMotionCorrectionFilter;

// Property widget for CrosscorrelationFilter.

class CrosscorrelationFilterWidget : public AbstractFilterWidget, protected Ui::CrosscorrelationFilterWidget
{
public:
	CrosscorrelationFilterWidget(GenericMotionCorrectionFilter* algorithm);
	CrosscorrelationFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile);
	virtual ~CrosscorrelationFilterWidget();

	virtual void updateGuiFromFilter();
	virtual void updateFilterProperties();
	virtual void loadFromSettings();
	virtual void writeToSettings();

protected:
private:

private slots :
	void on_useROI_stateChanged(int state);
};

#endif //CROSSCORRELTAION_FILTER_WIDGET_H