#ifndef ALGORITHM_PROPERTIES_DIALOG_H
#define ALGORITHM_PROPERTIES_DIALOG_H

#include "ui_AlgorithmPropertiesDialog.h"
#include <qwidget.h>
#include <qfiledialog.h>

#include <vector>
//using std::vector;

class AbstractFilterWidget;

/** Dialog window showing the given property widgets.
 */
class AlgorithmPropertiesDialog : public QDialog, protected Ui::AlgorithmPropertiesDialog {
	Q_OBJECT

public:
	typedef std::vector<AbstractFilterWidget*> FilterWidgetVector;

	AlgorithmPropertiesDialog(QWidget* parent, FilterWidgetVector widgets, QString settingsFile = QString(), QString settingsPrefix = QString());
	virtual ~AlgorithmPropertiesDialog();

protected:
private:	
	FilterWidgetVector filterWidgets;
	QString settingsPrefix;

private slots:
	void accept();
	void on_loadButton_clicked();
	void on_saveButton_clicked();

};

#endif // ALGORITHM_PROPERTIES_DIALOG_H