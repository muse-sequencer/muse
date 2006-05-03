#ifndef MITTRANSPOSEBASE_H
#define MITTRANSPOSEBASE_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QWidget>
#include "awl/pitchedit.h"
#include <awl::pitchedit.h>
#include <Qt3Support/Q3MimeSourceFactory>

class Ui_MITTransposeBase
{
public:
    QHBoxLayout *hboxLayout;
    QCheckBox *onCheckBox;
    QLabel *TextLabel1;
    Awl::PitchEdit *triggerKeySpinBox;
    QLabel *TextLabel2;
    QLabel *transposeLabel;

    void setupUi(QWidget *MITTransposeBase)
    {
    MITTransposeBase->setObjectName(QString::fromUtf8("MITTransposeBase"));
    MITTransposeBase->resize(QSize(423, 50).expandedTo(MITTransposeBase->minimumSizeHint()));
    hboxLayout = new QHBoxLayout(MITTransposeBase);
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(11);
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    onCheckBox = new QCheckBox(MITTransposeBase);
    onCheckBox->setObjectName(QString::fromUtf8("onCheckBox"));

    hboxLayout->addWidget(onCheckBox);

    TextLabel1 = new QLabel(MITTransposeBase);
    TextLabel1->setObjectName(QString::fromUtf8("TextLabel1"));
    TextLabel1->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    TextLabel1->setIndent(5);

    hboxLayout->addWidget(TextLabel1);

    triggerKeySpinBox = new Awl::PitchEdit(MITTransposeBase);
    triggerKeySpinBox->setObjectName(QString::fromUtf8("triggerKeySpinBox"));

    hboxLayout->addWidget(triggerKeySpinBox);

    TextLabel2 = new QLabel(MITTransposeBase);
    TextLabel2->setObjectName(QString::fromUtf8("TextLabel2"));
    TextLabel2->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    TextLabel2->setIndent(5);

    hboxLayout->addWidget(TextLabel2);

    transposeLabel = new QLabel(MITTransposeBase);
    transposeLabel->setObjectName(QString::fromUtf8("transposeLabel"));
    QSizePolicy sizePolicy((QSizePolicy::Policy)5, (QSizePolicy::Policy)0);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(transposeLabel->sizePolicy().hasHeightForWidth());
    transposeLabel->setSizePolicy(sizePolicy);
    transposeLabel->setFrameShape(QFrame::Panel);
    transposeLabel->setLineWidth(2);
    transposeLabel->setMargin(2);
    transposeLabel->setMidLineWidth(2);
    transposeLabel->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    transposeLabel->setIndent(5);

    hboxLayout->addWidget(transposeLabel);

    retranslateUi(MITTransposeBase);

    QMetaObject::connectSlotsByName(MITTransposeBase);
    } // setupUi

    void retranslateUi(QWidget *MITTransposeBase)
    {
    MITTransposeBase->setWindowTitle(QApplication::translate("MITTransposeBase", "MusE: Midi Input Plugin: Transpose", 0, QApplication::UnicodeUTF8));
    onCheckBox->setText(QApplication::translate("MITTransposeBase", "On", 0, QApplication::UnicodeUTF8));
    TextLabel1->setText(QApplication::translate("MITTransposeBase", "TriggerKey", 0, QApplication::UnicodeUTF8));
    TextLabel2->setText(QApplication::translate("MITTransposeBase", "Transpose:", 0, QApplication::UnicodeUTF8));
    transposeLabel->setText(QApplication::translate("MITTransposeBase", "+0", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(MITTransposeBase);
    } // retranslateUi

};

namespace Ui {
    class MITTransposeBase: public Ui_MITTransposeBase {};
} // namespace Ui

class MITTransposeBase : public QWidget, public Ui::MITTransposeBase
{
    Q_OBJECT

public:
    MITTransposeBase(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0);
    ~MITTransposeBase();

protected slots:
    virtual void languageChange();

};

#endif // MITTRANSPOSEBASE_H
