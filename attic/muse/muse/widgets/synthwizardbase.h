/****************************************************************************
** Form interface generated from reading ui file 'synthwizardbase.ui'
**
** Created: Wed Nov 12 10:38:24 2003
**      by: The User Interface Compiler ($Id: synthwizardbase.h,v 1.5 2003/11/12 18:55:06 wschweer Exp $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef SYNTHCONFIGBASE_H
#define SYNTHCONFIGBASE_H

#include <qvariant.h>
#include <qwizard.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QWidget;
class QGroupBox;
class QListView;
class QListViewItem;
class QPushButton;

class SynthConfigBase : public QWizard
{
    Q_OBJECT

public:
    SynthConfigBase( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~SynthConfigBase();

    QWidget* WizardPage;
    QGroupBox* GroupBox2;
    QListView* synthList;
    QPushButton* addInstance;
    QGroupBox* GroupBox3;
    QListView* instanceList;
    QPushButton* removeInstance;
    QWidget* WizardPage_2;

protected:
    QVBoxLayout* WizardPageLayout;
    QVBoxLayout* GroupBox2Layout;
    QHBoxLayout* Layout1;
    QVBoxLayout* GroupBox3Layout;
    QHBoxLayout* Layout3;

protected slots:
    virtual void languageChange();

};

#endif // SYNTHCONFIGBASE_H
