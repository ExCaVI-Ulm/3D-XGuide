#ifndef DL_FILTER_WIDGET_H
#define DL_FILTER_WIDGET_H

#include "AbstractFilterWidget.h"
#include "ui_DLFilterWidget.h"

class GenericMotionCorrectionFilter;

/** Property widget for GenericMotionCorrectionFilter.
 */
class DLFilterWidget : public AbstractFilterWidget, protected Ui::DLFilterWidget
{
	Q_OBJECT

public:
	DLFilterWidget(GenericMotionCorrectionFilter* algorithm);
	DLFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile);
	virtual ~DLFilterWidget();

	virtual void updateGuiFromFilter();
	virtual void updateFilterProperties();
	virtual void loadFromSettings();
	virtual void writeToSettings();

protected:
private:

private slots:
	void on_useDLManualScaling_stateChanged(int state);
};

#endif // DL_FILTER_WIDGET_H