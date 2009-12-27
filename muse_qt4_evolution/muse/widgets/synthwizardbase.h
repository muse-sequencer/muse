/****************************************************************************
** Form interface generated from reading ui file 'synthwizardbase.ui'
**
** Created: Wed Nov 12 10:38:24 2003
**      by: The User Interface Compiler ($Id: synthwizardbase.h,v 1.6 2005/09/22 20:13:39 wschweer Exp $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef SYNTHCONFIGBASE_H
#define SYNTHCONFIGBASE_H

#include <qvariant.h>
#include <q3wizard.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QWidget;
class Q3GroupBox;
class Q3ListView;
class Q3ListViewItem;
class QPushButton;

class SynthConfigBase : public Q3Wizard
{
    Q_OBJECT

public:
    SynthConfigBase( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    ~SynthConfigBase();

    QWidget* WizardPage;
    Q3GroupBox* GroupBox2;
    Q3ListView* synthList;
    QPushButton* addInstance;
    Q3GroupBox* GroupBox3;
    Q3ListView* instanceList;
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
