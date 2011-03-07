#ifndef CLIPLISTEDITORBASE_H
#define CLIPLISTEDITORBASE_H

#include <qvariant.h>


#include <Qt3Support/Q3GroupBox>
#include <Qt3Support/Q3Header>
#include <Qt3Support/Q3ListView>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "awl/posedit.h"
#include <awl::posedit.h>
#include <Qt3Support/Q3MimeSourceFactory>

class Ui_ClipListEditorBase
{
public:
    QVBoxLayout *vboxLayout;
    Q3ListView *view;
    Q3GroupBox *GroupBox1;
    QHBoxLayout *hboxLayout;
    QLabel *TextLabel1;
    Awl::PosEdit *start;
    QLabel *TextLabel2;
    Awl::PosEdit *len;
    QSpacerItem *spacerItem;

    void setupUi(QWidget *ClipListEditorBase)
    {
    ClipListEditorBase->setObjectName(QString::fromUtf8("ClipListEditorBase"));
    ClipListEditorBase->resize(QSize(600, 480).expandedTo(ClipListEditorBase->minimumSizeHint()));
    vboxLayout = new QVBoxLayout(ClipListEditorBase);
    vboxLayout->setSpacing(6);
    vboxLayout->setMargin(11);
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    view = new Q3ListView(ClipListEditorBase);
    view->addColumn(QApplication::translate("ClipListEditorBase", "Name", 0, QApplication::UnicodeUTF8));
    view->header()->setClickEnabled(true, view->header()->count() - 1);
    view->addColumn(QApplication::translate("ClipListEditorBase", "Refs", 0, QApplication::UnicodeUTF8));
    view->header()->setClickEnabled(true, view->header()->count() - 1);
    view->addColumn(QApplication::translate("ClipListEditorBase", "Start", 0, QApplication::UnicodeUTF8));
    view->header()->setClickEnabled(true, view->header()->count() - 1);
    view->addColumn(QApplication::translate("ClipListEditorBase", "Len", 0, QApplication::UnicodeUTF8));
    view->header()->setClickEnabled(true, view->header()->count() - 1);
    view->addColumn(QApplication::translate("ClipListEditorBase", "Data", 0, QApplication::UnicodeUTF8));
    view->header()->setClickEnabled(true, view->header()->count() - 1);
    view->setObjectName(QString::fromUtf8("view"));
    view->setAllColumnsShowFocus(true);
    view->setRootIsDecorated(true);

    vboxLayout->addWidget(view);

    GroupBox1 = new Q3GroupBox(ClipListEditorBase);
    GroupBox1->setObjectName(QString::fromUtf8("GroupBox1"));
    GroupBox1->setColumnLayout(0, Qt::Vertical);
    GroupBox1->layout()->setSpacing(6);
    GroupBox1->layout()->setMargin(11);
    hboxLayout = new QHBoxLayout(GroupBox1->layout());
    hboxLayout->setAlignment(Qt::AlignTop);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    hboxLayout->setMargin(11);
    hboxLayout->setSpacing(6);
    TextLabel1 = new QLabel(GroupBox1);
    TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));

    hboxLayout->addWidget(TextLabel1);

    start = new Awl::PosEdit(GroupBox1);
    start->setObjectName(QString::fromUtf8("start"));
    start->setSmpte(true);

    hboxLayout->addWidget(start);

    TextLabel2 = new QLabel(GroupBox1);
    TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));

    hboxLayout->addWidget(TextLabel2);

    len = new Awl::PosEdit(GroupBox1);
    len->setObjectName(QString::fromUtf8("len"));
    len->setSmpte(true);

    hboxLayout->addWidget(len);

    spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout->addItem(spacerItem);


    vboxLayout->addWidget(GroupBox1);

    retranslateUi(ClipListEditorBase);

    QMetaObject::connectSlotsByName(ClipListEditorBase);
    } // setupUi

    void retranslateUi(QWidget *ClipListEditorBase)
    {
    ClipListEditorBase->setWindowTitle(QApplication::translate("ClipListEditorBase", "MusE: ClipList", 0, QApplication::UnicodeUTF8));
    view->header()->setLabel(0, QApplication::translate("ClipListEditorBase", "Name", 0, QApplication::UnicodeUTF8));
    view->header()->setLabel(1, QApplication::translate("ClipListEditorBase", "Refs", 0, QApplication::UnicodeUTF8));
    view->header()->setLabel(2, QApplication::translate("ClipListEditorBase", "Start", 0, QApplication::UnicodeUTF8));
    view->header()->setLabel(3, QApplication::translate("ClipListEditorBase", "Len", 0, QApplication::UnicodeUTF8));
    view->header()->setLabel(4, QApplication::translate("ClipListEditorBase", "Data", 0, QApplication::UnicodeUTF8));
    GroupBox1->setTitle(QApplication::translate("ClipListEditorBase", "Clip Properties", 0, QApplication::UnicodeUTF8));
    TextLabel1->setText(QApplication::translate("ClipListEditorBase", "Pos:", 0, QApplication::UnicodeUTF8));
    TextLabel2->setText(QApplication::translate("ClipListEditorBase", "Len:", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(ClipListEditorBase);
    } // retranslateUi

};

namespace Ui {
    class ClipListEditorBase: public Ui_ClipListEditorBase {};
} // namespace Ui

class ClipListEditorBase : public QWidget, public Ui::ClipListEditorBase
{
    Q_OBJECT

public:
    ClipListEditorBase(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0);
    ~ClipListEditorBase();

protected slots:
    virtual void languageChange();

};

#endif // CLIPLISTEDITORBASE_H
