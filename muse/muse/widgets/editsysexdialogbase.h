#ifndef EDITSYSEXDIALOGBASE_H
#define EDITSYSEXDIALOGBASE_H

#include <qvariant.h>


#include <Qt3Support/Q3ListBox>
#include <Qt3Support/Q3TextEdit>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "awl/posedit.h"
#include <awl::posedit.h>
#include <Qt3Support/Q3MimeSourceFactory>

class Ui_EditSysexDialogBase
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout1;
    QLabel *textLabel1;
    Q3ListBox *sysexList;
    QVBoxLayout *vboxLayout2;
    QHBoxLayout *hboxLayout1;
    QLabel *TextLabel1;
    Awl::PosEdit *pos;
    QSpacerItem *spacerItem;
    Q3TextEdit *edit;
    QLabel *TextLabel2;
    Q3TextEdit *comment;
    QHBoxLayout *hboxLayout2;
    QPushButton *buttonAdd;
    QSpacerItem *spacerItem1;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *EditSysexDialogBase)
    {
    EditSysexDialogBase->setObjectName(QString::fromUtf8("EditSysexDialogBase"));
    EditSysexDialogBase->resize(QSize(667, 556).expandedTo(EditSysexDialogBase->minimumSizeHint()));
    EditSysexDialogBase->setSizeGripEnabled(true);
    vboxLayout = new QVBoxLayout(EditSysexDialogBase);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(11);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    vboxLayout1 = new QVBoxLayout();
    vboxLayout1->setSpacing(6);
    vboxLayout1->setMargin(0);
    vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
    textLabel1 = new QLabel(EditSysexDialogBase);
    textLabel1->setObjectName(QString::fromUtf8("textLabel1"));
    QSizePolicy sizePolicy((QSizePolicy::Policy)4, (QSizePolicy::Policy)5);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(textLabel1->sizePolicy().hasHeightForWidth());
    textLabel1->setSizePolicy(sizePolicy);

    vboxLayout1->addWidget(textLabel1);

    sysexList = new Q3ListBox(EditSysexDialogBase);
    sysexList->setObjectName(QString::fromUtf8("sysexList"));
    QSizePolicy sizePolicy1((QSizePolicy::Policy)4, (QSizePolicy::Policy)7);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(sysexList->sizePolicy().hasHeightForWidth());
    sysexList->setSizePolicy(sizePolicy1);

    vboxLayout1->addWidget(sysexList);


    hboxLayout->addLayout(vboxLayout1);

    vboxLayout2 = new QVBoxLayout();
    vboxLayout2->setSpacing(6);
    vboxLayout2->setMargin(0);
    vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
    hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing(6);
    hboxLayout1->setMargin(0);
    hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
    TextLabel1 = new QLabel(EditSysexDialogBase);
    TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));

    hboxLayout1->addWidget(TextLabel1);

    pos = new Awl::PosEdit(EditSysexDialogBase);
    pos->setObjectName(QString::fromUtf8("pos"));

    hboxLayout1->addWidget(pos);

    spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout1->addItem(spacerItem);


    vboxLayout2->addLayout(hboxLayout1);

    edit = new Q3TextEdit(EditSysexDialogBase);
    edit->setObjectName(QString::fromUtf8("edit"));

    vboxLayout2->addWidget(edit);

    TextLabel2 = new QLabel(EditSysexDialogBase);
    TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));

    vboxLayout2->addWidget(TextLabel2);

    comment = new Q3TextEdit(EditSysexDialogBase);
    comment->setObjectName(QString::fromUtf8("comment"));

    vboxLayout2->addWidget(comment);


    hboxLayout->addLayout(vboxLayout2);


    vboxLayout->addLayout(hboxLayout);

    hboxLayout2 = new QHBoxLayout();
    hboxLayout2->setSpacing(6);
    hboxLayout2->setMargin(0);
    hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
    buttonAdd = new QPushButton(EditSysexDialogBase);
    buttonAdd->setObjectName(QString::fromUtf8("buttonAdd"));
    QSizePolicy sizePolicy2((QSizePolicy::Policy)1, (QSizePolicy::Policy)0);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(buttonAdd->sizePolicy().hasHeightForWidth());
    buttonAdd->setSizePolicy(sizePolicy2);

    hboxLayout2->addWidget(buttonAdd);

    spacerItem1 = new QSpacerItem(350, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout2->addItem(spacerItem1);

    buttonOk = new QPushButton(EditSysexDialogBase);
    buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
    buttonOk->setAutoDefault(true);
    buttonOk->setDefault(true);

    hboxLayout2->addWidget(buttonOk);

    buttonCancel = new QPushButton(EditSysexDialogBase);
    buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
    buttonCancel->setAutoDefault(true);

    hboxLayout2->addWidget(buttonCancel);


    vboxLayout->addLayout(hboxLayout2);

    retranslateUi(EditSysexDialogBase);

    QMetaObject::connectSlotsByName(EditSysexDialogBase);
    } // setupUi

    void retranslateUi(QDialog *EditSysexDialogBase)
    {
    EditSysexDialogBase->setWindowTitle(QApplication::translate("EditSysexDialogBase", "MusE: Enter SysEx", 0, QApplication::UnicodeUTF8));
    textLabel1->setText(QApplication::translate("EditSysexDialogBase", "Known SysEx:", 0, QApplication::UnicodeUTF8));
    TextLabel1->setText(QApplication::translate("EditSysexDialogBase", "TimePosition:", 0, QApplication::UnicodeUTF8));
    TextLabel2->setText(QApplication::translate("EditSysexDialogBase", "Comment:", 0, QApplication::UnicodeUTF8));
    buttonAdd->setText(QApplication::translate("EditSysexDialogBase", "Add", 0, QApplication::UnicodeUTF8));
    buttonOk->setText(QApplication::translate("EditSysexDialogBase", "OK", 0, QApplication::UnicodeUTF8));
    buttonOk->setShortcut(QApplication::translate("EditSysexDialogBase", "", 0, QApplication::UnicodeUTF8));
    buttonCancel->setText(QApplication::translate("EditSysexDialogBase", "Cancel", 0, QApplication::UnicodeUTF8));
    buttonCancel->setShortcut(QApplication::translate("EditSysexDialogBase", "", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(EditSysexDialogBase);
    } // retranslateUi

};

namespace Ui {
    class EditSysexDialogBase: public Ui_EditSysexDialogBase {};
} // namespace Ui

class EditSysexDialogBase : public QDialog, public Ui::EditSysexDialogBase
{
    Q_OBJECT

public:
    EditSysexDialogBase(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
    ~EditSysexDialogBase();

protected slots:
    virtual void languageChange();

};

#endif // EDITSYSEXDIALOGBASE_H
