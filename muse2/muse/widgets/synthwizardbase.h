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
#include <q3wizard.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <Q3HBoxLayout>

class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
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
    Q3VBoxLayout* WizardPageLayout;
    Q3VBoxLayout* GroupBox2Layout;
    Q3HBoxLayout* Layout1;
    Q3VBoxLayout* GroupBox3Layout;
    Q3HBoxLayout* Layout3;

protected slots:
    virtual void languageChange();

};

#endif // SYNTHCONFIGBASE_H
