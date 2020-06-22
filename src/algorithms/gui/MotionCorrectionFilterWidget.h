#ifndef MOTION_CORRECTION_FILTER_WIDGET_H
#define MOTION_CORRECTION_FILTER_WIDGET_H

#include "AbstractFilterWidget.h"
#include "ui_MotionCorrectionFilterWidget.h"

class GenericMotionCorrectionFilter;

/** Property widget for GenericMotionCorrectionFilter.
 */
class MotionCorrectionFilterWidget : public AbstractFilterWidget, protected Ui::MotionCorrectionFilterWidget
{
	Q_OBJECT

public:
	MotionCorrectionFilterWidget(GenericMotionCorrectionFilter* algorithm);
	MotionCorrectionFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile);
	virtual ~MotionCorrectionFilterWidget();

	virtual void updateGuiFromFilter();
	virtual void updateFilterProperties();
	virtual void loadFromSettings();
	virtual void writeToSettings();

protected:
private:

private slots:
	void on_useManualScaling_stateChanged(int state);
};

#endif // MOTION_CORRECTION_FILTER_WIDGET_H