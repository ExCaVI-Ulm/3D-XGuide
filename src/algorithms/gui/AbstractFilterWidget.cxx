#include "AbstractFilterWidget.h"

AbstractFilterWidget::AbstractFilterWidget(GenericMotionCorrectionFilter* algorithm, QString settingsFile, QString settingsPrefix)
: myFilter(algorithm), mySettingsFile(settingsFile), mySettingsPrefix(settingsPrefix)
{
}


AbstractFilterWidget::~AbstractFilterWidget()
{
	// do nothing here
}

void AbstractFilterWidget::setSettingsFileAndPrefix(QString file, QString prefix)
{
	mySettingsFile = file;
	mySettingsPrefix = prefix;
}

QString AbstractFilterWidget::getSettingsFile() const
{
	return mySettingsFile;
}
