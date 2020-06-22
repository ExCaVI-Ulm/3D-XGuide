#ifndef ABSTRACT_FILTER_WIDGET_H
#define ABSTRACT_FILTER_WIDGET_H

#include <qwidget.h>
#include <qstring.h>

class GenericMotionCorrectionFilter;
class GenericKalmanFilter;
/**
 Interface which all widgets corresponding to a filter must implement.
 */
class AbstractFilterWidget : public QWidget {

public:
	AbstractFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile = QString(), QString settingsPrefix = QString());
	virtual ~AbstractFilterWidget();

	/// GUI parameters -> filter
	virtual void updateFilterProperties() = 0;

	/// filter -> GUI properties
	virtual void updateGuiFromFilter() = 0;

	/// settings  -> GUI
	virtual void loadFromSettings() = 0;

	/// GUI -> settings
	virtual void writeToSettings() = 0;

	void setSettingsFileAndPrefix(QString file, QString prefix);
	QString getSettingsFile() const;

protected:
	GenericMotionCorrectionFilter* myFilter;
	QString mySettingsFile;
	QString mySettingsPrefix;

private:

};

#endif // ABSTRACT_FILTER_WIDGET_H