#ifndef DEICSONZEGUIBASE_H
#define DEICSONZEGUIBASE_H

#include <qvariant.h>


#include <Qt3Support/Q3GroupBox>
#include <Qt3Support/Q3Header>
#include <Qt3Support/Q3ListView>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>
#include <QtGui/QTabWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DeicsOnzeGuiBase
{
public:
    QTabWidget *deicsOnzeTabWidget;
    QWidget *TabPage;
    QPushButton *loadPushButton;
    QPushButton *savePushButton;
    Q3GroupBox *nameGroupBox;
    QLineEdit *nameLineEdit;
    Q3GroupBox *subcategoryGroupBox;
    QLineEdit *subcategoryLineEdit;
    Q3GroupBox *categoryGroupBox;
    QLineEdit *categoryLineEdit;
    QPushButton *deletePushButton;
    Q3ListView *categoryListView;
    Q3ListView *subcategoryListView;
    Q3ListView *presetsListView;
    QPushButton *newPushButton;
    Q3GroupBox *bankGroupBox;
    QSpinBox *bankSpinBox;
    Q3GroupBox *progGroupBox;
    QSpinBox *progSpinBox;
    QLabel *presentTextLAbel;
    QWidget *TabPage1;
    Q3GroupBox *masterVolGroupBox;
    QSlider *masterVolSlider;
    QSpinBox *MasterVolumeSpinBox;
    Q3GroupBox *FeedbackGroupBox;
    QSpinBox *feedbackSpinBox;
    QSlider *feedbackSlider;
    Q3GroupBox *functionGroupBox;
    QComboBox *polyMonoComboBox;
    QLabel *PitchBendRangeLabel;
    QSlider *PitchBendRangeSlider;
    QSpinBox *pitchBendRangeSpinBox;
    Q3GroupBox *LFOGroupBox;
    QLabel *PModSensLabel;
    QLabel *PModDepthLabel;
    QLabel *AModDepthLabel;
    QLabel *LFOSpeedLabel;
    QLabel *LFODelayLabel;
    QLabel *AModSensLabel;
    QComboBox *LFOWaveComboBox;
    QCheckBox *LFOSyncCheckBox;
    QSlider *AModSensSlider;
    QSlider *PModSensSlider;
    QSpinBox *PMSSpinBox;
    QSpinBox *AMSSpinBox;
    QSlider *PModDepthSlider;
    QSlider *AModDepthSlider;
    QSlider *LFOSpeedSlider;
    QSpinBox *LFOSpeedSpinBox;
    QSlider *LFODelaySlider;
    QSpinBox *LFODelaySpinBox;
    QSpinBox *PModDepthSpinBox;
    QSpinBox *AModDepthSpinBox;
    Q3GroupBox *transposeGroupBox;
    QSlider *transposeSlider;
    QSpinBox *transposeSpinBox;
    QSlider *globalDetuneSlider;
    QSpinBox *globalDetuneSpinBox;
    QComboBox *algorithmComboBox;
    QWidget *tab;
    Q3GroupBox *Frequency1groupBox;
    QCheckBox *Fix1CheckBox;
    QLabel *CoarseRatio1Label;
    QLabel *FineRatio1Label;
    QLabel *Freq1Label;
    QSpinBox *CoarseRatio1SpinBox;
    QSpinBox *FineRatio1SpinBox;
    QSpinBox *Freq1SpinBox;
    Q3GroupBox *Env1GroupBox;
    QLabel *RR1Label;
    QLabel *D1R1Label;
    QLabel *D1L1Label;
    QLabel *D2R1Label;
    QSlider *D1L1Slider;
    QSlider *D2R1Slider;
    QSlider *D1R1Slider;
    QSlider *RR1Slider;
    QLabel *AR1Label;
    QSpinBox *D1R1SpinBox;
    QSpinBox *D1L1SpinBox;
    QSpinBox *D2R1SpinBox;
    QSpinBox *RR1SpinBox;
    QSlider *AR1Slider;
    QSpinBox *AR1SpinBox;
    Q3GroupBox *Scaling1GroupBox;
    QLabel *LS1Label;
    QLabel *RS1Label;
    QSlider *LS1Slider;
    QSlider *RS1Slider;
    QSpinBox *LS1SpinBox;
    QSpinBox *RS1SpinBox;
    Q3GroupBox *Vol1groupBox;
    QSlider *Vol1Slider;
    QSpinBox *Vol1SpinBox;
    Q3GroupBox *sensitivity1groupBox;
    QLabel *EGS1Label;
    QLabel *KVS1Label;
    QCheckBox *AME1CheckBox;
    QSlider *KVS1Slider;
    QSlider *EBS1Slider;
    QSpinBox *KVS1SpinBox;
    QSpinBox *EBS1SpinBox;
    Q3GroupBox *DetWaveEGS1GroupBox;
    QComboBox *WaveForm1ComboBox;
    QLabel *DET1Label;
    QComboBox *EGQ1ComboBox;
    QSlider *DET1Slider;
    QSpinBox *DET1SpinBox;
    QWidget *tab1;
    Q3GroupBox *Frequency2groupBox;
    QCheckBox *Fix2CheckBox;
    QLabel *Freq2Label;
    QLabel *FineRatio2Label;
    QLabel *CoarseRatio2Label;
    QSpinBox *CoarseRatio2SpinBox;
    QSpinBox *FineRatio2SpinBox;
    QSpinBox *Freq2SpinBox;
    Q3GroupBox *Env2GroupBox;
    QLabel *AR2Label;
    QLabel *RR2Label;
    QLabel *D2R2Label;
    QLabel *D1L2Label;
    QLabel *D1R2Label;
    QSlider *AR2Slider;
    QSlider *D1R2Slider;
    QSlider *D1L2Slider;
    QSlider *D2R2Slider;
    QSlider *RR2Slider;
    QSpinBox *D1R2SpinBox;
    QSpinBox *D1L2SpinBox;
    QSpinBox *D2R2SpinBox;
    QSpinBox *RR2SpinBox;
    QSpinBox *AR2SpinBox;
    Q3GroupBox *Scaling2GroupBox;
    QLabel *LS2Label;
    QLabel *RS2Label;
    QSlider *LS2Slider;
    QSlider *RS2Slider;
    QSpinBox *LS2SpinBox;
    QSpinBox *RS2SpinBox;
    Q3GroupBox *Vol2groupBox;
    QSlider *Vol2Slider;
    QSpinBox *Vol2SpinBox;
    Q3GroupBox *sensitivity2groupBox;
    QLabel *EGS2Label;
    QLabel *KVS2Label;
    QCheckBox *AME2CheckBox;
    QSlider *KVS2Slider;
    QSlider *EBS2Slider;
    QSpinBox *EBS2SpinBox;
    QSpinBox *KVS2SpinBox;
    Q3GroupBox *DetWaveEGS2GroupBox;
    QSlider *DET2Slider;
    QLabel *DET2Label;
    QComboBox *WaveForm2ComboBox;
    QComboBox *EGS2comboBox;
    QSpinBox *DET2SpinBox;
    QWidget *TabPage2;
    Q3GroupBox *Frequency3groupBox;
    QCheckBox *Fix3CheckBox;
    QLabel *CoarseRatio3Label;
    QLabel *FineRatio3Label;
    QLabel *Freq3Label;
    QSpinBox *CoarseRatio3SpinBox;
    QSpinBox *FineRatio3SpinBox;
    QSpinBox *Freq3SpinBox;
    Q3GroupBox *Env3GroupBox;
    QLabel *RR3Label;
    QLabel *D2R3Label;
    QLabel *D1L3Label;
    QLabel *D1R3Label;
    QLabel *AR3Label;
    QSlider *AR3Slider;
    QSlider *D1R3Slider;
    QSlider *D1L3Slider;
    QSlider *D2R3Slider;
    QSlider *RR3Slider;
    QSpinBox *D1R3SpinBox;
    QSpinBox *D1L3SpinBox;
    QSpinBox *D2R3SpinBox;
    QSpinBox *RR3SpinBox;
    QSpinBox *AR3SpinBox;
    Q3GroupBox *Scaling3GroupBox;
    QLabel *LS3Label;
    QLabel *RS3Label;
    QSlider *LS3Slider;
    QSlider *RS3Slider;
    QSpinBox *LS3SpinBox;
    QSpinBox *RS3SpinBox;
    Q3GroupBox *Vol3groupBox;
    QSlider *Vol3Slider;
    QSpinBox *Vol3SpinBox;
    Q3GroupBox *sensitivity3groupBox;
    QLabel *EGS3Label;
    QLabel *KVS3Label;
    QCheckBox *AME3CheckBox;
    QSlider *KVS3Slider;
    QSlider *EBS3Slider;
    QSpinBox *EBS3SpinBox;
    QSpinBox *KVS3SpinBox;
    Q3GroupBox *DetWaveEGS3GroupBox;
    QComboBox *WaveForm3ComboBox;
    QComboBox *EGS3comboBox;
    QLabel *DET3Label;
    QSlider *DET3Slider;
    QSpinBox *DET3SpinBox;
    QWidget *TabPage3;
    Q3GroupBox *Frequency4groupBox;
    QLabel *CoarseRatio4Label;
    QLabel *FineRatio4Label;
    QLabel *Freq4Label;
    QCheckBox *Fix4CheckBox;
    QSpinBox *FineRatio4SpinBox;
    QSpinBox *Freq4SpinBox;
    QSpinBox *CoarseRatio4SpinBox;
    Q3GroupBox *Scaling4GroupBox;
    QLabel *LS4Label;
    QLabel *RS4Label;
    QSlider *LS4Slider;
    QSlider *RS4Slider;
    QSpinBox *RS4SpinBox;
    QSpinBox *LS4SpinBox;
    Q3GroupBox *Env4GroupBox;
    QSlider *AR4Slider;
    QLabel *AR4Label;
    QLabel *RR4Label;
    QLabel *D2R4Label;
    QSlider *D2R4Slider;
    QSlider *D1L4Slider;
    QLabel *D1L4Label;
    QLabel *D1R4Label;
    QSlider *D1R4Slider;
    QSlider *RR4Slider;
    QSpinBox *D1R4SpinBox;
    QSpinBox *D1L4SpinBox;
    QSpinBox *D2R4SpinBox;
    QSpinBox *RR4SpinBox;
    QSpinBox *AR4SpinBox;
    Q3GroupBox *Vol4groupBox;
    QSlider *Vol4Slider;
    QSpinBox *Vol4SpinBox;
    Q3GroupBox *sensitivity4groupBox;
    QLabel *EGS4Label;
    QLabel *KVS4Label;
    QCheckBox *AME4CheckBox;
    QSlider *KVS4Slider;
    QSlider *EBS4Slider;
    QSpinBox *KVS4SpinBox;
    QSpinBox *EBS4SpinBox;
    Q3GroupBox *DetWaveEGS4GroupBox;
    QSlider *DET4Slider;
    QLabel *DET4Label;
    QComboBox *WaveForm4ComboBox;
    QComboBox *EGS4comboBox;
    QSpinBox *DET4SpinBox;

    void setupUi(QDialog *DeicsOnzeGuiBase)
    {
        if (DeicsOnzeGuiBase->objectName().isEmpty())
            DeicsOnzeGuiBase->setObjectName(QString::fromUtf8("DeicsOnzeGuiBase"));
        DeicsOnzeGuiBase->resize(468, 357);
        DeicsOnzeGuiBase->setSizeGripEnabled(false);
        DeicsOnzeGuiBase->setModal(false);
        deicsOnzeTabWidget = new QTabWidget(DeicsOnzeGuiBase);
        deicsOnzeTabWidget->setObjectName(QString::fromUtf8("deicsOnzeTabWidget"));
        deicsOnzeTabWidget->setGeometry(QRect(0, 0, 470, 360));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(7), static_cast<QSizePolicy::Policy>(7));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(deicsOnzeTabWidget->sizePolicy().hasHeightForWidth());
        deicsOnzeTabWidget->setSizePolicy(sizePolicy);
        QFont font;
        font.setFamily(QString::fromUtf8("Teen"));
        deicsOnzeTabWidget->setFont(font);
        deicsOnzeTabWidget->setCursor(QCursor(static_cast<Qt::CursorShape>(0)));
        deicsOnzeTabWidget->setTabShape(QTabWidget::Rounded);
        TabPage = new QWidget();
        TabPage->setObjectName(QString::fromUtf8("TabPage"));
        loadPushButton = new QPushButton(TabPage);
        loadPushButton->setObjectName(QString::fromUtf8("loadPushButton"));
        loadPushButton->setEnabled(true);
        loadPushButton->setGeometry(QRect(340, 260, 60, 30));
        loadPushButton->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        savePushButton = new QPushButton(TabPage);
        savePushButton->setObjectName(QString::fromUtf8("savePushButton"));
        savePushButton->setEnabled(true);
        savePushButton->setGeometry(QRect(400, 260, 60, 30));
        savePushButton->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        nameGroupBox = new Q3GroupBox(TabPage);
        nameGroupBox->setObjectName(QString::fromUtf8("nameGroupBox"));
        nameGroupBox->setGeometry(QRect(340, 10, 120, 50));
        nameLineEdit = new QLineEdit(nameGroupBox);
        nameLineEdit->setObjectName(QString::fromUtf8("nameLineEdit"));
        nameLineEdit->setGeometry(QRect(10, 20, 100, 21));
        QFont font1;
        font1.setFamily(QString::fromUtf8("Zero Threes"));
        font1.setPointSize(8);
        nameLineEdit->setFont(font1);
        nameLineEdit->setMaxLength(12);
        subcategoryGroupBox = new Q3GroupBox(TabPage);
        subcategoryGroupBox->setObjectName(QString::fromUtf8("subcategoryGroupBox"));
        subcategoryGroupBox->setGeometry(QRect(340, 60, 120, 50));
        subcategoryLineEdit = new QLineEdit(subcategoryGroupBox);
        subcategoryLineEdit->setObjectName(QString::fromUtf8("subcategoryLineEdit"));
        subcategoryLineEdit->setGeometry(QRect(10, 20, 100, 21));
        subcategoryLineEdit->setFont(font1);
        subcategoryLineEdit->setMaxLength(12);
        categoryGroupBox = new Q3GroupBox(TabPage);
        categoryGroupBox->setObjectName(QString::fromUtf8("categoryGroupBox"));
        categoryGroupBox->setGeometry(QRect(340, 110, 120, 50));
        categoryLineEdit = new QLineEdit(categoryGroupBox);
        categoryLineEdit->setObjectName(QString::fromUtf8("categoryLineEdit"));
        categoryLineEdit->setGeometry(QRect(10, 20, 100, 21));
        categoryLineEdit->setFont(font1);
        categoryLineEdit->setMaxLength(12);
        deletePushButton = new QPushButton(TabPage);
        deletePushButton->setObjectName(QString::fromUtf8("deletePushButton"));
        deletePushButton->setEnabled(false);
        deletePushButton->setGeometry(QRect(400, 220, 60, 29));
        deletePushButton->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        categoryListView = new Q3ListView(TabPage);
        categoryListView->addColumn(QApplication::translate("DeicsOnzeGuiBase", "Category", 0, QApplication::UnicodeUTF8));
        categoryListView->header()->setClickEnabled(false, categoryListView->header()->count() - 1);
        categoryListView->header()->setResizeEnabled(false, categoryListView->header()->count() - 1);
        categoryListView->setObjectName(QString::fromUtf8("categoryListView"));
        categoryListView->setGeometry(QRect(10, 10, 100, 280));
        categoryListView->setFont(font1);
        categoryListView->setFrameShadow(QFrame::Sunken);
        categoryListView->setAllColumnsShowFocus(false);
        categoryListView->setShowSortIndicator(false);
        categoryListView->setResizeMode(Q3ListView::AllColumns);
        subcategoryListView = new Q3ListView(TabPage);
        subcategoryListView->addColumn(QApplication::translate("DeicsOnzeGuiBase", "Subcategory", 0, QApplication::UnicodeUTF8));
        subcategoryListView->header()->setClickEnabled(false, subcategoryListView->header()->count() - 1);
        subcategoryListView->header()->setResizeEnabled(false, subcategoryListView->header()->count() - 1);
        subcategoryListView->setObjectName(QString::fromUtf8("subcategoryListView"));
        subcategoryListView->setGeometry(QRect(120, 10, 100, 280));
        subcategoryListView->setFont(font1);
        subcategoryListView->setFrameShadow(QFrame::Sunken);
        subcategoryListView->setAllColumnsShowFocus(false);
        subcategoryListView->setShowSortIndicator(false);
        subcategoryListView->setResizeMode(Q3ListView::AllColumns);
        presetsListView = new Q3ListView(TabPage);
        presetsListView->addColumn(QApplication::translate("DeicsOnzeGuiBase", "Preset", 0, QApplication::UnicodeUTF8));
        presetsListView->header()->setClickEnabled(false, presetsListView->header()->count() - 1);
        presetsListView->header()->setResizeEnabled(false, presetsListView->header()->count() - 1);
        presetsListView->setObjectName(QString::fromUtf8("presetsListView"));
        presetsListView->setGeometry(QRect(230, 10, 100, 280));
        presetsListView->setFont(font1);
        presetsListView->setFrameShadow(QFrame::Sunken);
        presetsListView->setAllColumnsShowFocus(false);
        presetsListView->setShowSortIndicator(false);
        presetsListView->setResizeMode(Q3ListView::AllColumns);
        newPushButton = new QPushButton(TabPage);
        newPushButton->setObjectName(QString::fromUtf8("newPushButton"));
        newPushButton->setEnabled(true);
        newPushButton->setGeometry(QRect(340, 220, 60, 29));
        newPushButton->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        bankGroupBox = new Q3GroupBox(TabPage);
        bankGroupBox->setObjectName(QString::fromUtf8("bankGroupBox"));
        bankGroupBox->setGeometry(QRect(340, 160, 60, 50));
        bankSpinBox = new QSpinBox(bankGroupBox);
        bankSpinBox->setObjectName(QString::fromUtf8("bankSpinBox"));
        bankSpinBox->setGeometry(QRect(10, 20, 40, 21));
        QFont font2;
        font2.setFamily(QString::fromUtf8("Zero Threes"));
        font2.setItalic(true);
        bankSpinBox->setFont(font2);
        bankSpinBox->setMaximum(128);
        bankSpinBox->setMinimum(1);
        progGroupBox = new Q3GroupBox(TabPage);
        progGroupBox->setObjectName(QString::fromUtf8("progGroupBox"));
        progGroupBox->setGeometry(QRect(400, 160, 60, 50));
        progSpinBox = new QSpinBox(progGroupBox);
        progSpinBox->setObjectName(QString::fromUtf8("progSpinBox"));
        progSpinBox->setGeometry(QRect(10, 20, 40, 21));
        progSpinBox->setFont(font2);
        progSpinBox->setMaximum(128);
        progSpinBox->setMinimum(1);
        presentTextLAbel = new QLabel(TabPage);
        presentTextLAbel->setObjectName(QString::fromUtf8("presentTextLAbel"));
        presentTextLAbel->setGeometry(QRect(20, 300, 420, 20));
        presentTextLAbel->setFrameShape(QFrame::NoFrame);
        presentTextLAbel->setFrameShadow(QFrame::Plain);
        presentTextLAbel->setWordWrap(false);
        deicsOnzeTabWidget->addTab(TabPage, QString());
        TabPage1 = new QWidget();
        TabPage1->setObjectName(QString::fromUtf8("TabPage1"));
        masterVolGroupBox = new Q3GroupBox(TabPage1);
        masterVolGroupBox->setObjectName(QString::fromUtf8("masterVolGroupBox"));
        masterVolGroupBox->setGeometry(QRect(10, 0, 330, 50));
        masterVolSlider = new QSlider(masterVolGroupBox);
        masterVolSlider->setObjectName(QString::fromUtf8("masterVolSlider"));
        masterVolSlider->setGeometry(QRect(10, 20, 250, 20));
        masterVolSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        masterVolSlider->setMaximum(255);
        masterVolSlider->setOrientation(Qt::Horizontal);
        MasterVolumeSpinBox = new QSpinBox(masterVolGroupBox);
        MasterVolumeSpinBox->setObjectName(QString::fromUtf8("MasterVolumeSpinBox"));
        MasterVolumeSpinBox->setGeometry(QRect(270, 20, 50, 21));
        MasterVolumeSpinBox->setFont(font2);
        MasterVolumeSpinBox->setMaximum(255);
        FeedbackGroupBox = new Q3GroupBox(TabPage1);
        FeedbackGroupBox->setObjectName(QString::fromUtf8("FeedbackGroupBox"));
        FeedbackGroupBox->setGeometry(QRect(350, 0, 110, 50));
        feedbackSpinBox = new QSpinBox(FeedbackGroupBox);
        feedbackSpinBox->setObjectName(QString::fromUtf8("feedbackSpinBox"));
        feedbackSpinBox->setGeometry(QRect(70, 20, 30, 21));
        feedbackSpinBox->setFont(font2);
        feedbackSpinBox->setMaximum(7);
        feedbackSlider = new QSlider(FeedbackGroupBox);
        feedbackSlider->setObjectName(QString::fromUtf8("feedbackSlider"));
        feedbackSlider->setGeometry(QRect(10, 20, 50, 20));
        feedbackSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        feedbackSlider->setMaximum(7);
        feedbackSlider->setSingleStep(1);
        feedbackSlider->setPageStep(1);
        feedbackSlider->setOrientation(Qt::Horizontal);
        feedbackSlider->setTickPosition(QSlider::NoTicks);
        functionGroupBox = new Q3GroupBox(TabPage1);
        functionGroupBox->setObjectName(QString::fromUtf8("functionGroupBox"));
        functionGroupBox->setGeometry(QRect(10, 50, 200, 200));
        polyMonoComboBox = new QComboBox(functionGroupBox);
        polyMonoComboBox->setObjectName(QString::fromUtf8("polyMonoComboBox"));
        polyMonoComboBox->setEnabled(false);
        polyMonoComboBox->setGeometry(QRect(10, 20, 180, 20));
        QPalette palette;
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(1), QColor(195, 195, 195));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(2), QColor(255, 255, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(3), QColor(225, 225, 225));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(4), QColor(97, 97, 97));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(5), QColor(130, 130, 130));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(7), QColor(249, 249, 249));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(10), QColor(177, 177, 177));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(12), QColor(57, 15, 195));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(14), QColor(195, 18, 71));
        palette.setColor(QPalette::Active, static_cast<QPalette::ColorRole>(15), QColor(128, 0, 128));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(0), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(1), QColor(195, 195, 195));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(2), QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(3), QColor(224, 224, 224));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(4), QColor(97, 97, 97));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(5), QColor(130, 130, 130));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(7), QColor(249, 249, 249));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(8), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(10), QColor(177, 177, 177));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(12), QColor(57, 15, 195));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(14), QColor(195, 18, 71));
        palette.setColor(QPalette::Inactive, static_cast<QPalette::ColorRole>(15), QColor(128, 0, 128));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(0), QColor(128, 128, 128));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(1), QColor(195, 195, 195));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(2), QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(3), QColor(224, 224, 224));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(4), QColor(97, 97, 97));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(5), QColor(130, 130, 130));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(6), QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(7), QColor(249, 249, 249));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(8), QColor(128, 128, 128));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(9), QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(10), QColor(177, 177, 177));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(11), QColor(0, 0, 0));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(12), QColor(57, 15, 195));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(13), QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(14), QColor(195, 18, 71));
        palette.setColor(QPalette::Disabled, static_cast<QPalette::ColorRole>(15), QColor(128, 0, 128));
        polyMonoComboBox->setPalette(palette);
        polyMonoComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        PitchBendRangeLabel = new QLabel(functionGroupBox);
        PitchBendRangeLabel->setObjectName(QString::fromUtf8("PitchBendRangeLabel"));
        PitchBendRangeLabel->setGeometry(QRect(10, 50, 40, 22));
        PitchBendRangeLabel->setFrameShape(QFrame::Box);
        PitchBendRangeLabel->setFrameShadow(QFrame::Sunken);
        PitchBendRangeLabel->setWordWrap(false);
        PitchBendRangeSlider = new QSlider(functionGroupBox);
        PitchBendRangeSlider->setObjectName(QString::fromUtf8("PitchBendRangeSlider"));
        PitchBendRangeSlider->setGeometry(QRect(60, 50, 90, 20));
        PitchBendRangeSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        PitchBendRangeSlider->setMinimum(0);
        PitchBendRangeSlider->setMaximum(12);
        PitchBendRangeSlider->setPageStep(1);
        PitchBendRangeSlider->setValue(0);
        PitchBendRangeSlider->setOrientation(Qt::Horizontal);
        PitchBendRangeSlider->setTickPosition(QSlider::NoTicks);
        pitchBendRangeSpinBox = new QSpinBox(functionGroupBox);
        pitchBendRangeSpinBox->setObjectName(QString::fromUtf8("pitchBendRangeSpinBox"));
        pitchBendRangeSpinBox->setGeometry(QRect(160, 50, 30, 21));
        pitchBendRangeSpinBox->setFont(font2);
        pitchBendRangeSpinBox->setMaximum(12);
        LFOGroupBox = new Q3GroupBox(TabPage1);
        LFOGroupBox->setObjectName(QString::fromUtf8("LFOGroupBox"));
        LFOGroupBox->setGeometry(QRect(220, 50, 240, 200));
        PModSensLabel = new QLabel(LFOGroupBox);
        PModSensLabel->setObjectName(QString::fromUtf8("PModSensLabel"));
        PModSensLabel->setGeometry(QRect(100, 20, 40, 22));
        PModSensLabel->setFrameShape(QFrame::Box);
        PModSensLabel->setFrameShadow(QFrame::Sunken);
        PModSensLabel->setWordWrap(false);
        PModDepthLabel = new QLabel(LFOGroupBox);
        PModDepthLabel->setObjectName(QString::fromUtf8("PModDepthLabel"));
        PModDepthLabel->setGeometry(QRect(10, 80, 40, 24));
        PModDepthLabel->setFrameShape(QFrame::Box);
        PModDepthLabel->setFrameShadow(QFrame::Sunken);
        PModDepthLabel->setWordWrap(false);
        AModDepthLabel = new QLabel(LFOGroupBox);
        AModDepthLabel->setObjectName(QString::fromUtf8("AModDepthLabel"));
        AModDepthLabel->setGeometry(QRect(10, 110, 40, 22));
        AModDepthLabel->setFrameShape(QFrame::Box);
        AModDepthLabel->setFrameShadow(QFrame::Sunken);
        AModDepthLabel->setWordWrap(false);
        LFOSpeedLabel = new QLabel(LFOGroupBox);
        LFOSpeedLabel->setObjectName(QString::fromUtf8("LFOSpeedLabel"));
        LFOSpeedLabel->setGeometry(QRect(10, 140, 50, 24));
        LFOSpeedLabel->setFrameShape(QFrame::Box);
        LFOSpeedLabel->setFrameShadow(QFrame::Sunken);
        LFOSpeedLabel->setWordWrap(false);
        LFODelayLabel = new QLabel(LFOGroupBox);
        LFODelayLabel->setObjectName(QString::fromUtf8("LFODelayLabel"));
        LFODelayLabel->setGeometry(QRect(10, 170, 50, 24));
        LFODelayLabel->setFrameShape(QFrame::Box);
        LFODelayLabel->setFrameShadow(QFrame::Sunken);
        LFODelayLabel->setWordWrap(false);
        AModSensLabel = new QLabel(LFOGroupBox);
        AModSensLabel->setObjectName(QString::fromUtf8("AModSensLabel"));
        AModSensLabel->setGeometry(QRect(100, 50, 40, 22));
        AModSensLabel->setFrameShape(QFrame::Box);
        AModSensLabel->setFrameShadow(QFrame::Sunken);
        AModSensLabel->setWordWrap(false);
        LFOWaveComboBox = new QComboBox(LFOGroupBox);
        LFOWaveComboBox->setObjectName(QString::fromUtf8("LFOWaveComboBox"));
        LFOWaveComboBox->setGeometry(QRect(10, 50, 80, 20));
        LFOWaveComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LFOSyncCheckBox = new QCheckBox(LFOGroupBox);
        LFOSyncCheckBox->setObjectName(QString::fromUtf8("LFOSyncCheckBox"));
        LFOSyncCheckBox->setGeometry(QRect(10, 20, 80, 25));
        LFOSyncCheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AModSensSlider = new QSlider(LFOGroupBox);
        AModSensSlider->setObjectName(QString::fromUtf8("AModSensSlider"));
        AModSensSlider->setGeometry(QRect(150, 50, 40, 20));
        AModSensSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AModSensSlider->setMaximum(3);
        AModSensSlider->setSingleStep(1);
        AModSensSlider->setPageStep(1);
        AModSensSlider->setOrientation(Qt::Horizontal);
        AModSensSlider->setTickPosition(QSlider::NoTicks);
        PModSensSlider = new QSlider(LFOGroupBox);
        PModSensSlider->setObjectName(QString::fromUtf8("PModSensSlider"));
        PModSensSlider->setGeometry(QRect(150, 20, 40, 20));
        PModSensSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        PModSensSlider->setMaximum(7);
        PModSensSlider->setSingleStep(1);
        PModSensSlider->setPageStep(1);
        PModSensSlider->setOrientation(Qt::Horizontal);
        PModSensSlider->setTickPosition(QSlider::NoTicks);
        PMSSpinBox = new QSpinBox(LFOGroupBox);
        PMSSpinBox->setObjectName(QString::fromUtf8("PMSSpinBox"));
        PMSSpinBox->setGeometry(QRect(200, 20, 30, 21));
        PMSSpinBox->setFont(font2);
        PMSSpinBox->setMaximum(7);
        AMSSpinBox = new QSpinBox(LFOGroupBox);
        AMSSpinBox->setObjectName(QString::fromUtf8("AMSSpinBox"));
        AMSSpinBox->setGeometry(QRect(200, 50, 30, 21));
        AMSSpinBox->setFont(font2);
        AMSSpinBox->setMaximum(7);
        PModDepthSlider = new QSlider(LFOGroupBox);
        PModDepthSlider->setObjectName(QString::fromUtf8("PModDepthSlider"));
        PModDepthSlider->setGeometry(QRect(70, 80, 110, 20));
        PModDepthSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        PModDepthSlider->setMaximum(99);
        PModDepthSlider->setSingleStep(1);
        PModDepthSlider->setPageStep(1);
        PModDepthSlider->setOrientation(Qt::Horizontal);
        PModDepthSlider->setTickPosition(QSlider::NoTicks);
        AModDepthSlider = new QSlider(LFOGroupBox);
        AModDepthSlider->setObjectName(QString::fromUtf8("AModDepthSlider"));
        AModDepthSlider->setGeometry(QRect(70, 110, 110, 20));
        AModDepthSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AModDepthSlider->setMaximum(99);
        AModDepthSlider->setSingleStep(1);
        AModDepthSlider->setPageStep(1);
        AModDepthSlider->setOrientation(Qt::Horizontal);
        AModDepthSlider->setTickPosition(QSlider::NoTicks);
        LFOSpeedSlider = new QSlider(LFOGroupBox);
        LFOSpeedSlider->setObjectName(QString::fromUtf8("LFOSpeedSlider"));
        LFOSpeedSlider->setGeometry(QRect(70, 140, 110, 20));
        LFOSpeedSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LFOSpeedSlider->setMaximum(99);
        LFOSpeedSlider->setSingleStep(1);
        LFOSpeedSlider->setPageStep(1);
        LFOSpeedSlider->setOrientation(Qt::Horizontal);
        LFOSpeedSlider->setTickPosition(QSlider::NoTicks);
        LFOSpeedSpinBox = new QSpinBox(LFOGroupBox);
        LFOSpeedSpinBox->setObjectName(QString::fromUtf8("LFOSpeedSpinBox"));
        LFOSpeedSpinBox->setGeometry(QRect(190, 140, 40, 21));
        LFOSpeedSpinBox->setFont(font2);
        LFOSpeedSpinBox->setMaximum(99);
        LFODelaySlider = new QSlider(LFOGroupBox);
        LFODelaySlider->setObjectName(QString::fromUtf8("LFODelaySlider"));
        LFODelaySlider->setGeometry(QRect(70, 170, 110, 20));
        LFODelaySlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LFODelaySlider->setMaximum(99);
        LFODelaySlider->setSingleStep(1);
        LFODelaySlider->setPageStep(1);
        LFODelaySlider->setOrientation(Qt::Horizontal);
        LFODelaySlider->setTickPosition(QSlider::NoTicks);
        LFODelaySpinBox = new QSpinBox(LFOGroupBox);
        LFODelaySpinBox->setObjectName(QString::fromUtf8("LFODelaySpinBox"));
        LFODelaySpinBox->setGeometry(QRect(190, 170, 40, 21));
        LFODelaySpinBox->setFont(font2);
        LFODelaySpinBox->setMaximum(99);
        PModDepthSpinBox = new QSpinBox(LFOGroupBox);
        PModDepthSpinBox->setObjectName(QString::fromUtf8("PModDepthSpinBox"));
        PModDepthSpinBox->setGeometry(QRect(190, 80, 40, 21));
        PModDepthSpinBox->setFont(font2);
        PModDepthSpinBox->setMaximum(99);
        AModDepthSpinBox = new QSpinBox(LFOGroupBox);
        AModDepthSpinBox->setObjectName(QString::fromUtf8("AModDepthSpinBox"));
        AModDepthSpinBox->setGeometry(QRect(190, 110, 40, 21));
        AModDepthSpinBox->setFont(font2);
        AModDepthSpinBox->setMaximum(99);
        transposeGroupBox = new Q3GroupBox(TabPage1);
        transposeGroupBox->setObjectName(QString::fromUtf8("transposeGroupBox"));
        transposeGroupBox->setGeometry(QRect(10, 250, 310, 70));
        transposeSlider = new QSlider(transposeGroupBox);
        transposeSlider->setObjectName(QString::fromUtf8("transposeSlider"));
        transposeSlider->setGeometry(QRect(10, 20, 110, 19));
        transposeSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        transposeSlider->setMinimum(-24);
        transposeSlider->setMaximum(24);
        transposeSlider->setPageStep(1);
        transposeSlider->setOrientation(Qt::Horizontal);
        transposeSlider->setTickPosition(QSlider::NoTicks);
        transposeSpinBox = new QSpinBox(transposeGroupBox);
        transposeSpinBox->setObjectName(QString::fromUtf8("transposeSpinBox"));
        transposeSpinBox->setGeometry(QRect(130, 20, 40, 21));
        transposeSpinBox->setFont(font2);
        transposeSpinBox->setMaximum(99);
        transposeSpinBox->setMinimum(-24);
        globalDetuneSlider = new QSlider(transposeGroupBox);
        globalDetuneSlider->setObjectName(QString::fromUtf8("globalDetuneSlider"));
        globalDetuneSlider->setGeometry(QRect(180, 20, 70, 20));
        globalDetuneSlider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        globalDetuneSlider->setMinimum(-15);
        globalDetuneSlider->setMaximum(15);
        globalDetuneSlider->setPageStep(1);
        globalDetuneSlider->setOrientation(Qt::Horizontal);
        globalDetuneSlider->setTickPosition(QSlider::NoTicks);
        globalDetuneSpinBox = new QSpinBox(transposeGroupBox);
        globalDetuneSpinBox->setObjectName(QString::fromUtf8("globalDetuneSpinBox"));
        globalDetuneSpinBox->setGeometry(QRect(260, 20, 40, 21));
        globalDetuneSpinBox->setFont(font2);
        globalDetuneSpinBox->setMaximum(15);
        globalDetuneSpinBox->setMinimum(-15);
        algorithmComboBox = new QComboBox(TabPage1);
        algorithmComboBox->setObjectName(QString::fromUtf8("algorithmComboBox"));
        algorithmComboBox->setGeometry(QRect(340, 280, 110, 30));
        algorithmComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        deicsOnzeTabWidget->addTab(TabPage1, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        Frequency1groupBox = new Q3GroupBox(tab);
        Frequency1groupBox->setObjectName(QString::fromUtf8("Frequency1groupBox"));
        Frequency1groupBox->setGeometry(QRect(10, 190, 450, 50));
        Fix1CheckBox = new QCheckBox(Frequency1groupBox);
        Fix1CheckBox->setObjectName(QString::fromUtf8("Fix1CheckBox"));
        Fix1CheckBox->setEnabled(true);
        Fix1CheckBox->setGeometry(QRect(400, 20, 40, 25));
        Fix1CheckBox->setFont(font);
        Fix1CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        CoarseRatio1Label = new QLabel(Frequency1groupBox);
        CoarseRatio1Label->setObjectName(QString::fromUtf8("CoarseRatio1Label"));
        CoarseRatio1Label->setGeometry(QRect(10, 20, 70, 22));
        CoarseRatio1Label->setFrameShape(QFrame::Box);
        CoarseRatio1Label->setFrameShadow(QFrame::Sunken);
        CoarseRatio1Label->setWordWrap(false);
        FineRatio1Label = new QLabel(Frequency1groupBox);
        FineRatio1Label->setObjectName(QString::fromUtf8("FineRatio1Label"));
        FineRatio1Label->setGeometry(QRect(140, 20, 50, 22));
        FineRatio1Label->setFrameShape(QFrame::Box);
        FineRatio1Label->setFrameShadow(QFrame::Sunken);
        FineRatio1Label->setWordWrap(false);
        Freq1Label = new QLabel(Frequency1groupBox);
        Freq1Label->setObjectName(QString::fromUtf8("Freq1Label"));
        Freq1Label->setGeometry(QRect(250, 20, 50, 22));
        Freq1Label->setFrameShape(QFrame::Box);
        Freq1Label->setFrameShadow(QFrame::Sunken);
        Freq1Label->setWordWrap(false);
        CoarseRatio1SpinBox = new QSpinBox(Frequency1groupBox);
        CoarseRatio1SpinBox->setObjectName(QString::fromUtf8("CoarseRatio1SpinBox"));
        CoarseRatio1SpinBox->setGeometry(QRect(90, 20, 40, 21));
        CoarseRatio1SpinBox->setFont(font2);
        FineRatio1SpinBox = new QSpinBox(Frequency1groupBox);
        FineRatio1SpinBox->setObjectName(QString::fromUtf8("FineRatio1SpinBox"));
        FineRatio1SpinBox->setGeometry(QRect(200, 20, 40, 21));
        FineRatio1SpinBox->setFont(font2);
        Freq1SpinBox = new QSpinBox(Frequency1groupBox);
        Freq1SpinBox->setObjectName(QString::fromUtf8("Freq1SpinBox"));
        Freq1SpinBox->setEnabled(false);
        Freq1SpinBox->setGeometry(QRect(310, 20, 80, 21));
        Freq1SpinBox->setFont(font2);
        Freq1SpinBox->setMaximum(32000);
        Env1GroupBox = new Q3GroupBox(tab);
        Env1GroupBox->setObjectName(QString::fromUtf8("Env1GroupBox"));
        Env1GroupBox->setGeometry(QRect(10, 0, 260, 190));
        RR1Label = new QLabel(Env1GroupBox);
        RR1Label->setObjectName(QString::fromUtf8("RR1Label"));
        RR1Label->setGeometry(QRect(210, 160, 43, 22));
        RR1Label->setFrameShape(QFrame::Box);
        RR1Label->setFrameShadow(QFrame::Sunken);
        RR1Label->setWordWrap(false);
        D1R1Label = new QLabel(Env1GroupBox);
        D1R1Label->setObjectName(QString::fromUtf8("D1R1Label"));
        D1R1Label->setGeometry(QRect(60, 160, 43, 22));
        D1R1Label->setFrameShape(QFrame::Box);
        D1R1Label->setFrameShadow(QFrame::Sunken);
        D1R1Label->setWordWrap(false);
        D1L1Label = new QLabel(Env1GroupBox);
        D1L1Label->setObjectName(QString::fromUtf8("D1L1Label"));
        D1L1Label->setGeometry(QRect(110, 160, 43, 22));
        D1L1Label->setFrameShape(QFrame::Box);
        D1L1Label->setFrameShadow(QFrame::Sunken);
        D1L1Label->setWordWrap(false);
        D2R1Label = new QLabel(Env1GroupBox);
        D2R1Label->setObjectName(QString::fromUtf8("D2R1Label"));
        D2R1Label->setGeometry(QRect(160, 160, 43, 22));
        D2R1Label->setFrameShape(QFrame::Box);
        D2R1Label->setFrameShadow(QFrame::Sunken);
        D2R1Label->setWordWrap(false);
        D1L1Slider = new QSlider(Env1GroupBox);
        D1L1Slider->setObjectName(QString::fromUtf8("D1L1Slider"));
        D1L1Slider->setGeometry(QRect(120, 50, 20, 100));
        D1L1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1L1Slider->setMaximum(15);
        D1L1Slider->setSingleStep(1);
        D1L1Slider->setPageStep(1);
        D1L1Slider->setValue(0);
        D1L1Slider->setOrientation(Qt::Vertical);
        D1L1Slider->setTickPosition(QSlider::NoTicks);
        D2R1Slider = new QSlider(Env1GroupBox);
        D2R1Slider->setObjectName(QString::fromUtf8("D2R1Slider"));
        D2R1Slider->setGeometry(QRect(170, 50, 20, 100));
        D2R1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D2R1Slider->setMaximum(31);
        D2R1Slider->setSingleStep(1);
        D2R1Slider->setPageStep(1);
        D2R1Slider->setValue(0);
        D2R1Slider->setOrientation(Qt::Vertical);
        D2R1Slider->setTickPosition(QSlider::NoTicks);
        D1R1Slider = new QSlider(Env1GroupBox);
        D1R1Slider->setObjectName(QString::fromUtf8("D1R1Slider"));
        D1R1Slider->setGeometry(QRect(70, 50, 20, 100));
        D1R1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1R1Slider->setMouseTracking(false);
        D1R1Slider->setFocusPolicy(Qt::TabFocus);
        D1R1Slider->setAcceptDrops(false);
        D1R1Slider->setMaximum(31);
        D1R1Slider->setSingleStep(1);
        D1R1Slider->setPageStep(1);
        D1R1Slider->setValue(0);
        D1R1Slider->setOrientation(Qt::Vertical);
        D1R1Slider->setTickPosition(QSlider::NoTicks);
        RR1Slider = new QSlider(Env1GroupBox);
        RR1Slider->setObjectName(QString::fromUtf8("RR1Slider"));
        RR1Slider->setGeometry(QRect(220, 50, 20, 100));
        RR1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RR1Slider->setMinimum(0);
        RR1Slider->setMaximum(15);
        RR1Slider->setSingleStep(1);
        RR1Slider->setPageStep(1);
        RR1Slider->setValue(0);
        RR1Slider->setOrientation(Qt::Vertical);
        RR1Slider->setTickPosition(QSlider::NoTicks);
        RR1Slider->setTickInterval(0);
        AR1Label = new QLabel(Env1GroupBox);
        AR1Label->setObjectName(QString::fromUtf8("AR1Label"));
        AR1Label->setGeometry(QRect(10, 160, 40, 22));
        AR1Label->setFrameShape(QFrame::Box);
        AR1Label->setFrameShadow(QFrame::Sunken);
        AR1Label->setWordWrap(false);
        D1R1SpinBox = new QSpinBox(Env1GroupBox);
        D1R1SpinBox->setObjectName(QString::fromUtf8("D1R1SpinBox"));
        D1R1SpinBox->setGeometry(QRect(60, 20, 40, 21));
        D1R1SpinBox->setFont(font2);
        D1R1SpinBox->setMaximum(31);
        D1L1SpinBox = new QSpinBox(Env1GroupBox);
        D1L1SpinBox->setObjectName(QString::fromUtf8("D1L1SpinBox"));
        D1L1SpinBox->setGeometry(QRect(110, 20, 40, 21));
        D1L1SpinBox->setFont(font2);
        D1L1SpinBox->setMaximum(15);
        D2R1SpinBox = new QSpinBox(Env1GroupBox);
        D2R1SpinBox->setObjectName(QString::fromUtf8("D2R1SpinBox"));
        D2R1SpinBox->setGeometry(QRect(160, 20, 40, 21));
        D2R1SpinBox->setFont(font2);
        D2R1SpinBox->setMaximum(31);
        RR1SpinBox = new QSpinBox(Env1GroupBox);
        RR1SpinBox->setObjectName(QString::fromUtf8("RR1SpinBox"));
        RR1SpinBox->setGeometry(QRect(210, 20, 40, 21));
        RR1SpinBox->setFont(font2);
        RR1SpinBox->setMaximum(15);
        AR1Slider = new QSlider(Env1GroupBox);
        AR1Slider->setObjectName(QString::fromUtf8("AR1Slider"));
        AR1Slider->setGeometry(QRect(20, 50, 20, 100));
        AR1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AR1Slider->setMaximum(31);
        AR1Slider->setSingleStep(1);
        AR1Slider->setPageStep(1);
        AR1Slider->setValue(0);
        AR1Slider->setOrientation(Qt::Vertical);
        AR1Slider->setTickPosition(QSlider::NoTicks);
        AR1SpinBox = new QSpinBox(Env1GroupBox);
        AR1SpinBox->setObjectName(QString::fromUtf8("AR1SpinBox"));
        AR1SpinBox->setGeometry(QRect(10, 20, 40, 21));
        AR1SpinBox->setFont(font2);
        AR1SpinBox->setMaximum(31);
        Scaling1GroupBox = new Q3GroupBox(tab);
        Scaling1GroupBox->setObjectName(QString::fromUtf8("Scaling1GroupBox"));
        Scaling1GroupBox->setGeometry(QRect(280, 0, 110, 190));
        LS1Label = new QLabel(Scaling1GroupBox);
        LS1Label->setObjectName(QString::fromUtf8("LS1Label"));
        LS1Label->setGeometry(QRect(10, 160, 40, 22));
        LS1Label->setFrameShape(QFrame::Box);
        LS1Label->setFrameShadow(QFrame::Sunken);
        LS1Label->setWordWrap(false);
        RS1Label = new QLabel(Scaling1GroupBox);
        RS1Label->setObjectName(QString::fromUtf8("RS1Label"));
        RS1Label->setGeometry(QRect(60, 160, 40, 22));
        RS1Label->setFrameShape(QFrame::Box);
        RS1Label->setFrameShadow(QFrame::Sunken);
        RS1Label->setWordWrap(false);
        LS1Slider = new QSlider(Scaling1GroupBox);
        LS1Slider->setObjectName(QString::fromUtf8("LS1Slider"));
        LS1Slider->setGeometry(QRect(20, 50, 19, 100));
        LS1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LS1Slider->setMaximum(99);
        LS1Slider->setSingleStep(1);
        LS1Slider->setPageStep(1);
        LS1Slider->setValue(0);
        LS1Slider->setOrientation(Qt::Vertical);
        LS1Slider->setTickPosition(QSlider::NoTicks);
        RS1Slider = new QSlider(Scaling1GroupBox);
        RS1Slider->setObjectName(QString::fromUtf8("RS1Slider"));
        RS1Slider->setGeometry(QRect(70, 50, 19, 100));
        RS1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RS1Slider->setMaximum(3);
        RS1Slider->setSingleStep(1);
        RS1Slider->setPageStep(1);
        RS1Slider->setValue(0);
        RS1Slider->setOrientation(Qt::Vertical);
        RS1Slider->setTickPosition(QSlider::NoTicks);
        LS1SpinBox = new QSpinBox(Scaling1GroupBox);
        LS1SpinBox->setObjectName(QString::fromUtf8("LS1SpinBox"));
        LS1SpinBox->setGeometry(QRect(10, 20, 40, 21));
        LS1SpinBox->setFont(font2);
        LS1SpinBox->setMaximum(99);
        RS1SpinBox = new QSpinBox(Scaling1GroupBox);
        RS1SpinBox->setObjectName(QString::fromUtf8("RS1SpinBox"));
        RS1SpinBox->setGeometry(QRect(60, 20, 40, 21));
        RS1SpinBox->setFont(font2);
        RS1SpinBox->setMaximum(3);
        Vol1groupBox = new Q3GroupBox(tab);
        Vol1groupBox->setObjectName(QString::fromUtf8("Vol1groupBox"));
        Vol1groupBox->setGeometry(QRect(400, 0, 60, 190));
        Vol1Slider = new QSlider(Vol1groupBox);
        Vol1Slider->setObjectName(QString::fromUtf8("Vol1Slider"));
        Vol1Slider->setGeometry(QRect(20, 50, 19, 130));
        Vol1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        Vol1Slider->setMaximum(99);
        Vol1Slider->setSingleStep(1);
        Vol1Slider->setPageStep(1);
        Vol1Slider->setValue(0);
        Vol1Slider->setOrientation(Qt::Vertical);
        Vol1Slider->setTickPosition(QSlider::NoTicks);
        Vol1SpinBox = new QSpinBox(Vol1groupBox);
        Vol1SpinBox->setObjectName(QString::fromUtf8("Vol1SpinBox"));
        Vol1SpinBox->setGeometry(QRect(10, 20, 40, 21));
        Vol1SpinBox->setFont(font2);
        Vol1SpinBox->setMaximum(99);
        sensitivity1groupBox = new Q3GroupBox(tab);
        sensitivity1groupBox->setObjectName(QString::fromUtf8("sensitivity1groupBox"));
        sensitivity1groupBox->setGeometry(QRect(10, 240, 260, 80));
        EGS1Label = new QLabel(sensitivity1groupBox);
        EGS1Label->setObjectName(QString::fromUtf8("EGS1Label"));
        EGS1Label->setGeometry(QRect(80, 20, 40, 22));
        EGS1Label->setFrameShape(QFrame::Box);
        EGS1Label->setFrameShadow(QFrame::Sunken);
        EGS1Label->setWordWrap(false);
        KVS1Label = new QLabel(sensitivity1groupBox);
        KVS1Label->setObjectName(QString::fromUtf8("KVS1Label"));
        KVS1Label->setGeometry(QRect(10, 50, 40, 22));
        KVS1Label->setFrameShape(QFrame::Box);
        KVS1Label->setFrameShadow(QFrame::Sunken);
        KVS1Label->setWordWrap(false);
        AME1CheckBox = new QCheckBox(sensitivity1groupBox);
        AME1CheckBox->setObjectName(QString::fromUtf8("AME1CheckBox"));
        AME1CheckBox->setGeometry(QRect(10, 20, 60, 25));
        AME1CheckBox->setFont(font);
        AME1CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS1Slider = new QSlider(sensitivity1groupBox);
        KVS1Slider->setObjectName(QString::fromUtf8("KVS1Slider"));
        KVS1Slider->setGeometry(QRect(60, 50, 84, 19));
        KVS1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS1Slider->setAcceptDrops(false);
        KVS1Slider->setMaximum(7);
        KVS1Slider->setSingleStep(1);
        KVS1Slider->setPageStep(1);
        KVS1Slider->setOrientation(Qt::Horizontal);
        KVS1Slider->setTickPosition(QSlider::NoTicks);
        EBS1Slider = new QSlider(sensitivity1groupBox);
        EBS1Slider->setObjectName(QString::fromUtf8("EBS1Slider"));
        EBS1Slider->setGeometry(QRect(130, 20, 70, 20));
        EBS1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EBS1Slider->setMaximum(7);
        EBS1Slider->setSingleStep(1);
        EBS1Slider->setPageStep(1);
        EBS1Slider->setOrientation(Qt::Horizontal);
        EBS1Slider->setTickPosition(QSlider::NoTicks);
        KVS1SpinBox = new QSpinBox(sensitivity1groupBox);
        KVS1SpinBox->setObjectName(QString::fromUtf8("KVS1SpinBox"));
        KVS1SpinBox->setGeometry(QRect(160, 50, 40, 21));
        KVS1SpinBox->setFont(font2);
        KVS1SpinBox->setMaximum(7);
        EBS1SpinBox = new QSpinBox(sensitivity1groupBox);
        EBS1SpinBox->setObjectName(QString::fromUtf8("EBS1SpinBox"));
        EBS1SpinBox->setGeometry(QRect(210, 20, 40, 21));
        EBS1SpinBox->setFont(font2);
        EBS1SpinBox->setMaximum(7);
        DetWaveEGS1GroupBox = new Q3GroupBox(tab);
        DetWaveEGS1GroupBox->setObjectName(QString::fromUtf8("DetWaveEGS1GroupBox"));
        DetWaveEGS1GroupBox->setGeometry(QRect(280, 240, 180, 80));
        WaveForm1ComboBox = new QComboBox(DetWaveEGS1GroupBox);
        WaveForm1ComboBox->setObjectName(QString::fromUtf8("WaveForm1ComboBox"));
        WaveForm1ComboBox->setGeometry(QRect(10, 50, 80, 20));
        WaveForm1ComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET1Label = new QLabel(DetWaveEGS1GroupBox);
        DET1Label->setObjectName(QString::fromUtf8("DET1Label"));
        DET1Label->setGeometry(QRect(10, 20, 44, 24));
        DET1Label->setFrameShape(QFrame::Box);
        DET1Label->setFrameShadow(QFrame::Sunken);
        DET1Label->setWordWrap(false);
        EGQ1ComboBox = new QComboBox(DetWaveEGS1GroupBox);
        EGQ1ComboBox->setObjectName(QString::fromUtf8("EGQ1ComboBox"));
        EGQ1ComboBox->setEnabled(false);
        EGQ1ComboBox->setGeometry(QRect(100, 50, 70, 20));
        EGQ1ComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET1Slider = new QSlider(DetWaveEGS1GroupBox);
        DET1Slider->setObjectName(QString::fromUtf8("DET1Slider"));
        DET1Slider->setGeometry(QRect(60, 20, 70, 19));
        DET1Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET1Slider->setMinimum(-3);
        DET1Slider->setMaximum(3);
        DET1Slider->setSingleStep(1);
        DET1Slider->setPageStep(1);
        DET1Slider->setValue(0);
        DET1Slider->setOrientation(Qt::Horizontal);
        DET1Slider->setTickPosition(QSlider::NoTicks);
        DET1SpinBox = new QSpinBox(DetWaveEGS1GroupBox);
        DET1SpinBox->setObjectName(QString::fromUtf8("DET1SpinBox"));
        DET1SpinBox->setGeometry(QRect(140, 20, 30, 21));
        DET1SpinBox->setFont(font2);
        DET1SpinBox->setMaximum(3);
        DET1SpinBox->setMinimum(-3);
        deicsOnzeTabWidget->addTab(tab, QString());
        tab1 = new QWidget();
        tab1->setObjectName(QString::fromUtf8("tab1"));
        Frequency2groupBox = new Q3GroupBox(tab1);
        Frequency2groupBox->setObjectName(QString::fromUtf8("Frequency2groupBox"));
        Frequency2groupBox->setGeometry(QRect(10, 190, 450, 50));
        Fix2CheckBox = new QCheckBox(Frequency2groupBox);
        Fix2CheckBox->setObjectName(QString::fromUtf8("Fix2CheckBox"));
        Fix2CheckBox->setEnabled(true);
        Fix2CheckBox->setGeometry(QRect(400, 20, 40, 25));
        Fix2CheckBox->setFont(font);
        Fix2CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        Freq2Label = new QLabel(Frequency2groupBox);
        Freq2Label->setObjectName(QString::fromUtf8("Freq2Label"));
        Freq2Label->setGeometry(QRect(250, 20, 50, 22));
        Freq2Label->setFrameShape(QFrame::Box);
        Freq2Label->setFrameShadow(QFrame::Sunken);
        Freq2Label->setWordWrap(false);
        FineRatio2Label = new QLabel(Frequency2groupBox);
        FineRatio2Label->setObjectName(QString::fromUtf8("FineRatio2Label"));
        FineRatio2Label->setGeometry(QRect(140, 20, 50, 22));
        FineRatio2Label->setFrameShape(QFrame::Box);
        FineRatio2Label->setFrameShadow(QFrame::Sunken);
        FineRatio2Label->setWordWrap(false);
        CoarseRatio2Label = new QLabel(Frequency2groupBox);
        CoarseRatio2Label->setObjectName(QString::fromUtf8("CoarseRatio2Label"));
        CoarseRatio2Label->setGeometry(QRect(10, 20, 70, 22));
        CoarseRatio2Label->setFrameShape(QFrame::Box);
        CoarseRatio2Label->setFrameShadow(QFrame::Sunken);
        CoarseRatio2Label->setWordWrap(false);
        CoarseRatio2SpinBox = new QSpinBox(Frequency2groupBox);
        CoarseRatio2SpinBox->setObjectName(QString::fromUtf8("CoarseRatio2SpinBox"));
        CoarseRatio2SpinBox->setGeometry(QRect(90, 20, 40, 21));
        CoarseRatio2SpinBox->setFont(font2);
        FineRatio2SpinBox = new QSpinBox(Frequency2groupBox);
        FineRatio2SpinBox->setObjectName(QString::fromUtf8("FineRatio2SpinBox"));
        FineRatio2SpinBox->setGeometry(QRect(200, 20, 40, 21));
        FineRatio2SpinBox->setFont(font2);
        Freq2SpinBox = new QSpinBox(Frequency2groupBox);
        Freq2SpinBox->setObjectName(QString::fromUtf8("Freq2SpinBox"));
        Freq2SpinBox->setEnabled(false);
        Freq2SpinBox->setGeometry(QRect(310, 20, 80, 21));
        Freq2SpinBox->setFont(font2);
        Freq2SpinBox->setMaximum(32000);
        Env2GroupBox = new Q3GroupBox(tab1);
        Env2GroupBox->setObjectName(QString::fromUtf8("Env2GroupBox"));
        Env2GroupBox->setGeometry(QRect(10, 0, 260, 190));
        AR2Label = new QLabel(Env2GroupBox);
        AR2Label->setObjectName(QString::fromUtf8("AR2Label"));
        AR2Label->setGeometry(QRect(10, 160, 40, 22));
        AR2Label->setFrameShape(QFrame::Box);
        AR2Label->setFrameShadow(QFrame::Sunken);
        AR2Label->setWordWrap(false);
        RR2Label = new QLabel(Env2GroupBox);
        RR2Label->setObjectName(QString::fromUtf8("RR2Label"));
        RR2Label->setGeometry(QRect(210, 160, 43, 22));
        RR2Label->setFrameShape(QFrame::Box);
        RR2Label->setFrameShadow(QFrame::Sunken);
        RR2Label->setWordWrap(false);
        D2R2Label = new QLabel(Env2GroupBox);
        D2R2Label->setObjectName(QString::fromUtf8("D2R2Label"));
        D2R2Label->setGeometry(QRect(160, 160, 43, 22));
        D2R2Label->setFrameShape(QFrame::Box);
        D2R2Label->setFrameShadow(QFrame::Sunken);
        D2R2Label->setWordWrap(false);
        D1L2Label = new QLabel(Env2GroupBox);
        D1L2Label->setObjectName(QString::fromUtf8("D1L2Label"));
        D1L2Label->setGeometry(QRect(110, 160, 43, 22));
        D1L2Label->setFrameShape(QFrame::Box);
        D1L2Label->setFrameShadow(QFrame::Sunken);
        D1L2Label->setWordWrap(false);
        D1R2Label = new QLabel(Env2GroupBox);
        D1R2Label->setObjectName(QString::fromUtf8("D1R2Label"));
        D1R2Label->setGeometry(QRect(60, 160, 43, 22));
        D1R2Label->setFrameShape(QFrame::Box);
        D1R2Label->setFrameShadow(QFrame::Sunken);
        D1R2Label->setWordWrap(false);
        AR2Slider = new QSlider(Env2GroupBox);
        AR2Slider->setObjectName(QString::fromUtf8("AR2Slider"));
        AR2Slider->setEnabled(true);
        AR2Slider->setGeometry(QRect(20, 50, 20, 100));
        AR2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AR2Slider->setMaximum(31);
        AR2Slider->setPageStep(1);
        AR2Slider->setValue(0);
        AR2Slider->setTracking(true);
        AR2Slider->setOrientation(Qt::Vertical);
        AR2Slider->setTickPosition(QSlider::NoTicks);
        D1R2Slider = new QSlider(Env2GroupBox);
        D1R2Slider->setObjectName(QString::fromUtf8("D1R2Slider"));
        D1R2Slider->setGeometry(QRect(70, 50, 20, 100));
        D1R2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1R2Slider->setMaximum(31);
        D1R2Slider->setPageStep(1);
        D1R2Slider->setValue(0);
        D1R2Slider->setOrientation(Qt::Vertical);
        D1R2Slider->setTickPosition(QSlider::NoTicks);
        D1L2Slider = new QSlider(Env2GroupBox);
        D1L2Slider->setObjectName(QString::fromUtf8("D1L2Slider"));
        D1L2Slider->setGeometry(QRect(120, 50, 20, 100));
        D1L2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1L2Slider->setMaximum(15);
        D1L2Slider->setPageStep(1);
        D1L2Slider->setValue(0);
        D1L2Slider->setOrientation(Qt::Vertical);
        D1L2Slider->setTickPosition(QSlider::NoTicks);
        D2R2Slider = new QSlider(Env2GroupBox);
        D2R2Slider->setObjectName(QString::fromUtf8("D2R2Slider"));
        D2R2Slider->setGeometry(QRect(170, 50, 20, 100));
        D2R2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D2R2Slider->setMaximum(31);
        D2R2Slider->setPageStep(1);
        D2R2Slider->setValue(0);
        D2R2Slider->setOrientation(Qt::Vertical);
        D2R2Slider->setTickPosition(QSlider::NoTicks);
        RR2Slider = new QSlider(Env2GroupBox);
        RR2Slider->setObjectName(QString::fromUtf8("RR2Slider"));
        RR2Slider->setGeometry(QRect(220, 50, 20, 100));
        RR2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RR2Slider->setMinimum(0);
        RR2Slider->setMaximum(15);
        RR2Slider->setPageStep(1);
        RR2Slider->setValue(0);
        RR2Slider->setOrientation(Qt::Vertical);
        RR2Slider->setTickPosition(QSlider::NoTicks);
        RR2Slider->setTickInterval(0);
        D1R2SpinBox = new QSpinBox(Env2GroupBox);
        D1R2SpinBox->setObjectName(QString::fromUtf8("D1R2SpinBox"));
        D1R2SpinBox->setGeometry(QRect(60, 20, 40, 21));
        D1R2SpinBox->setFont(font2);
        D1R2SpinBox->setMaximum(31);
        D1L2SpinBox = new QSpinBox(Env2GroupBox);
        D1L2SpinBox->setObjectName(QString::fromUtf8("D1L2SpinBox"));
        D1L2SpinBox->setGeometry(QRect(110, 20, 40, 21));
        D1L2SpinBox->setFont(font2);
        D1L2SpinBox->setMaximum(15);
        D2R2SpinBox = new QSpinBox(Env2GroupBox);
        D2R2SpinBox->setObjectName(QString::fromUtf8("D2R2SpinBox"));
        D2R2SpinBox->setGeometry(QRect(160, 20, 40, 21));
        D2R2SpinBox->setFont(font2);
        D2R2SpinBox->setMaximum(31);
        RR2SpinBox = new QSpinBox(Env2GroupBox);
        RR2SpinBox->setObjectName(QString::fromUtf8("RR2SpinBox"));
        RR2SpinBox->setGeometry(QRect(210, 20, 40, 21));
        RR2SpinBox->setFont(font2);
        RR2SpinBox->setMaximum(15);
        AR2SpinBox = new QSpinBox(Env2GroupBox);
        AR2SpinBox->setObjectName(QString::fromUtf8("AR2SpinBox"));
        AR2SpinBox->setGeometry(QRect(10, 20, 40, 21));
        AR2SpinBox->setFont(font2);
        AR2SpinBox->setMaximum(31);
        Scaling2GroupBox = new Q3GroupBox(tab1);
        Scaling2GroupBox->setObjectName(QString::fromUtf8("Scaling2GroupBox"));
        Scaling2GroupBox->setGeometry(QRect(280, 0, 110, 190));
        LS2Label = new QLabel(Scaling2GroupBox);
        LS2Label->setObjectName(QString::fromUtf8("LS2Label"));
        LS2Label->setGeometry(QRect(10, 160, 40, 22));
        LS2Label->setFrameShape(QFrame::Box);
        LS2Label->setFrameShadow(QFrame::Sunken);
        LS2Label->setWordWrap(false);
        RS2Label = new QLabel(Scaling2GroupBox);
        RS2Label->setObjectName(QString::fromUtf8("RS2Label"));
        RS2Label->setGeometry(QRect(60, 160, 40, 22));
        RS2Label->setFrameShape(QFrame::Box);
        RS2Label->setFrameShadow(QFrame::Sunken);
        RS2Label->setWordWrap(false);
        LS2Slider = new QSlider(Scaling2GroupBox);
        LS2Slider->setObjectName(QString::fromUtf8("LS2Slider"));
        LS2Slider->setGeometry(QRect(20, 50, 20, 100));
        LS2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LS2Slider->setMaximum(99);
        LS2Slider->setPageStep(1);
        LS2Slider->setValue(0);
        LS2Slider->setOrientation(Qt::Vertical);
        LS2Slider->setTickPosition(QSlider::NoTicks);
        LS2Slider->setTickInterval(1);
        RS2Slider = new QSlider(Scaling2GroupBox);
        RS2Slider->setObjectName(QString::fromUtf8("RS2Slider"));
        RS2Slider->setGeometry(QRect(70, 50, 20, 100));
        RS2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RS2Slider->setMaximum(3);
        RS2Slider->setPageStep(1);
        RS2Slider->setValue(0);
        RS2Slider->setOrientation(Qt::Vertical);
        RS2Slider->setTickPosition(QSlider::NoTicks);
        LS2SpinBox = new QSpinBox(Scaling2GroupBox);
        LS2SpinBox->setObjectName(QString::fromUtf8("LS2SpinBox"));
        LS2SpinBox->setGeometry(QRect(10, 20, 40, 21));
        LS2SpinBox->setFont(font2);
        LS2SpinBox->setMaximum(99);
        RS2SpinBox = new QSpinBox(Scaling2GroupBox);
        RS2SpinBox->setObjectName(QString::fromUtf8("RS2SpinBox"));
        RS2SpinBox->setGeometry(QRect(60, 20, 40, 21));
        RS2SpinBox->setFont(font2);
        RS2SpinBox->setMaximum(3);
        Vol2groupBox = new Q3GroupBox(tab1);
        Vol2groupBox->setObjectName(QString::fromUtf8("Vol2groupBox"));
        Vol2groupBox->setGeometry(QRect(400, 0, 60, 190));
        Vol2Slider = new QSlider(Vol2groupBox);
        Vol2Slider->setObjectName(QString::fromUtf8("Vol2Slider"));
        Vol2Slider->setGeometry(QRect(20, 50, 19, 130));
        Vol2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        Vol2Slider->setMaximum(99);
        Vol2Slider->setPageStep(1);
        Vol2Slider->setValue(0);
        Vol2Slider->setOrientation(Qt::Vertical);
        Vol2Slider->setTickPosition(QSlider::NoTicks);
        Vol2SpinBox = new QSpinBox(Vol2groupBox);
        Vol2SpinBox->setObjectName(QString::fromUtf8("Vol2SpinBox"));
        Vol2SpinBox->setGeometry(QRect(10, 20, 40, 21));
        Vol2SpinBox->setFont(font2);
        Vol2SpinBox->setMaximum(99);
        sensitivity2groupBox = new Q3GroupBox(tab1);
        sensitivity2groupBox->setObjectName(QString::fromUtf8("sensitivity2groupBox"));
        sensitivity2groupBox->setGeometry(QRect(10, 240, 260, 80));
        EGS2Label = new QLabel(sensitivity2groupBox);
        EGS2Label->setObjectName(QString::fromUtf8("EGS2Label"));
        EGS2Label->setGeometry(QRect(80, 20, 40, 22));
        EGS2Label->setFrameShape(QFrame::Box);
        EGS2Label->setFrameShadow(QFrame::Sunken);
        EGS2Label->setWordWrap(false);
        KVS2Label = new QLabel(sensitivity2groupBox);
        KVS2Label->setObjectName(QString::fromUtf8("KVS2Label"));
        KVS2Label->setGeometry(QRect(10, 50, 40, 22));
        KVS2Label->setFrameShape(QFrame::Box);
        KVS2Label->setFrameShadow(QFrame::Sunken);
        KVS2Label->setWordWrap(false);
        AME2CheckBox = new QCheckBox(sensitivity2groupBox);
        AME2CheckBox->setObjectName(QString::fromUtf8("AME2CheckBox"));
        AME2CheckBox->setGeometry(QRect(10, 20, 60, 25));
        AME2CheckBox->setFont(font);
        AME2CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS2Slider = new QSlider(sensitivity2groupBox);
        KVS2Slider->setObjectName(QString::fromUtf8("KVS2Slider"));
        KVS2Slider->setGeometry(QRect(60, 50, 84, 19));
        KVS2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS2Slider->setAcceptDrops(false);
        KVS2Slider->setMaximum(7);
        KVS2Slider->setPageStep(1);
        KVS2Slider->setOrientation(Qt::Horizontal);
        KVS2Slider->setTickPosition(QSlider::NoTicks);
        EBS2Slider = new QSlider(sensitivity2groupBox);
        EBS2Slider->setObjectName(QString::fromUtf8("EBS2Slider"));
        EBS2Slider->setGeometry(QRect(130, 20, 70, 20));
        EBS2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EBS2Slider->setMaximum(7);
        EBS2Slider->setPageStep(1);
        EBS2Slider->setOrientation(Qt::Horizontal);
        EBS2Slider->setTickPosition(QSlider::NoTicks);
        EBS2SpinBox = new QSpinBox(sensitivity2groupBox);
        EBS2SpinBox->setObjectName(QString::fromUtf8("EBS2SpinBox"));
        EBS2SpinBox->setGeometry(QRect(210, 20, 40, 21));
        EBS2SpinBox->setFont(font2);
        EBS2SpinBox->setMaximum(7);
        KVS2SpinBox = new QSpinBox(sensitivity2groupBox);
        KVS2SpinBox->setObjectName(QString::fromUtf8("KVS2SpinBox"));
        KVS2SpinBox->setGeometry(QRect(160, 50, 40, 21));
        KVS2SpinBox->setFont(font2);
        KVS2SpinBox->setMaximum(7);
        DetWaveEGS2GroupBox = new Q3GroupBox(tab1);
        DetWaveEGS2GroupBox->setObjectName(QString::fromUtf8("DetWaveEGS2GroupBox"));
        DetWaveEGS2GroupBox->setGeometry(QRect(280, 240, 180, 80));
        DET2Slider = new QSlider(DetWaveEGS2GroupBox);
        DET2Slider->setObjectName(QString::fromUtf8("DET2Slider"));
        DET2Slider->setGeometry(QRect(60, 20, 70, 19));
        DET2Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET2Slider->setMinimum(-3);
        DET2Slider->setMaximum(3);
        DET2Slider->setPageStep(1);
        DET2Slider->setValue(0);
        DET2Slider->setOrientation(Qt::Horizontal);
        DET2Slider->setTickPosition(QSlider::NoTicks);
        DET2Label = new QLabel(DetWaveEGS2GroupBox);
        DET2Label->setObjectName(QString::fromUtf8("DET2Label"));
        DET2Label->setGeometry(QRect(10, 20, 44, 24));
        DET2Label->setFrameShape(QFrame::Box);
        DET2Label->setFrameShadow(QFrame::Sunken);
        DET2Label->setWordWrap(false);
        WaveForm2ComboBox = new QComboBox(DetWaveEGS2GroupBox);
        WaveForm2ComboBox->setObjectName(QString::fromUtf8("WaveForm2ComboBox"));
        WaveForm2ComboBox->setGeometry(QRect(10, 50, 80, 20));
        WaveForm2ComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EGS2comboBox = new QComboBox(DetWaveEGS2GroupBox);
        EGS2comboBox->setObjectName(QString::fromUtf8("EGS2comboBox"));
        EGS2comboBox->setEnabled(false);
        EGS2comboBox->setGeometry(QRect(100, 50, 70, 20));
        EGS2comboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET2SpinBox = new QSpinBox(DetWaveEGS2GroupBox);
        DET2SpinBox->setObjectName(QString::fromUtf8("DET2SpinBox"));
        DET2SpinBox->setGeometry(QRect(140, 20, 30, 21));
        DET2SpinBox->setFont(font2);
        DET2SpinBox->setMaximum(3);
        DET2SpinBox->setMinimum(-3);
        deicsOnzeTabWidget->addTab(tab1, QString());
        TabPage2 = new QWidget();
        TabPage2->setObjectName(QString::fromUtf8("TabPage2"));
        Frequency3groupBox = new Q3GroupBox(TabPage2);
        Frequency3groupBox->setObjectName(QString::fromUtf8("Frequency3groupBox"));
        Frequency3groupBox->setGeometry(QRect(10, 190, 450, 50));
        Fix3CheckBox = new QCheckBox(Frequency3groupBox);
        Fix3CheckBox->setObjectName(QString::fromUtf8("Fix3CheckBox"));
        Fix3CheckBox->setEnabled(true);
        Fix3CheckBox->setGeometry(QRect(400, 20, 40, 25));
        Fix3CheckBox->setFont(font);
        Fix3CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        CoarseRatio3Label = new QLabel(Frequency3groupBox);
        CoarseRatio3Label->setObjectName(QString::fromUtf8("CoarseRatio3Label"));
        CoarseRatio3Label->setGeometry(QRect(10, 20, 70, 22));
        CoarseRatio3Label->setFrameShape(QFrame::Box);
        CoarseRatio3Label->setFrameShadow(QFrame::Sunken);
        CoarseRatio3Label->setWordWrap(false);
        FineRatio3Label = new QLabel(Frequency3groupBox);
        FineRatio3Label->setObjectName(QString::fromUtf8("FineRatio3Label"));
        FineRatio3Label->setGeometry(QRect(140, 20, 50, 22));
        FineRatio3Label->setFrameShape(QFrame::Box);
        FineRatio3Label->setFrameShadow(QFrame::Sunken);
        FineRatio3Label->setWordWrap(false);
        Freq3Label = new QLabel(Frequency3groupBox);
        Freq3Label->setObjectName(QString::fromUtf8("Freq3Label"));
        Freq3Label->setGeometry(QRect(250, 20, 50, 22));
        Freq3Label->setFrameShape(QFrame::Box);
        Freq3Label->setFrameShadow(QFrame::Sunken);
        Freq3Label->setWordWrap(false);
        CoarseRatio3SpinBox = new QSpinBox(Frequency3groupBox);
        CoarseRatio3SpinBox->setObjectName(QString::fromUtf8("CoarseRatio3SpinBox"));
        CoarseRatio3SpinBox->setGeometry(QRect(90, 20, 40, 21));
        CoarseRatio3SpinBox->setFont(font2);
        CoarseRatio3SpinBox->setCursor(QCursor(static_cast<Qt::CursorShape>(0)));
        FineRatio3SpinBox = new QSpinBox(Frequency3groupBox);
        FineRatio3SpinBox->setObjectName(QString::fromUtf8("FineRatio3SpinBox"));
        FineRatio3SpinBox->setGeometry(QRect(200, 20, 40, 21));
        FineRatio3SpinBox->setFont(font2);
        FineRatio3SpinBox->setCursor(QCursor(static_cast<Qt::CursorShape>(0)));
        Freq3SpinBox = new QSpinBox(Frequency3groupBox);
        Freq3SpinBox->setObjectName(QString::fromUtf8("Freq3SpinBox"));
        Freq3SpinBox->setEnabled(false);
        Freq3SpinBox->setGeometry(QRect(310, 20, 80, 21));
        Freq3SpinBox->setFont(font2);
        Freq3SpinBox->setMaximum(32000);
        Env3GroupBox = new Q3GroupBox(TabPage2);
        Env3GroupBox->setObjectName(QString::fromUtf8("Env3GroupBox"));
        Env3GroupBox->setGeometry(QRect(10, 0, 260, 190));
        RR3Label = new QLabel(Env3GroupBox);
        RR3Label->setObjectName(QString::fromUtf8("RR3Label"));
        RR3Label->setGeometry(QRect(210, 160, 43, 22));
        RR3Label->setFrameShape(QFrame::Box);
        RR3Label->setFrameShadow(QFrame::Sunken);
        RR3Label->setWordWrap(false);
        D2R3Label = new QLabel(Env3GroupBox);
        D2R3Label->setObjectName(QString::fromUtf8("D2R3Label"));
        D2R3Label->setGeometry(QRect(160, 160, 43, 22));
        D2R3Label->setFrameShape(QFrame::Box);
        D2R3Label->setFrameShadow(QFrame::Sunken);
        D2R3Label->setWordWrap(false);
        D1L3Label = new QLabel(Env3GroupBox);
        D1L3Label->setObjectName(QString::fromUtf8("D1L3Label"));
        D1L3Label->setGeometry(QRect(110, 160, 43, 22));
        D1L3Label->setFrameShape(QFrame::Box);
        D1L3Label->setFrameShadow(QFrame::Sunken);
        D1L3Label->setWordWrap(false);
        D1R3Label = new QLabel(Env3GroupBox);
        D1R3Label->setObjectName(QString::fromUtf8("D1R3Label"));
        D1R3Label->setGeometry(QRect(60, 160, 43, 22));
        D1R3Label->setFrameShape(QFrame::Box);
        D1R3Label->setFrameShadow(QFrame::Sunken);
        D1R3Label->setWordWrap(false);
        AR3Label = new QLabel(Env3GroupBox);
        AR3Label->setObjectName(QString::fromUtf8("AR3Label"));
        AR3Label->setGeometry(QRect(10, 160, 40, 22));
        AR3Label->setFrameShape(QFrame::Box);
        AR3Label->setFrameShadow(QFrame::Sunken);
        AR3Label->setWordWrap(false);
        AR3Slider = new QSlider(Env3GroupBox);
        AR3Slider->setObjectName(QString::fromUtf8("AR3Slider"));
        AR3Slider->setGeometry(QRect(20, 50, 20, 100));
        AR3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AR3Slider->setMaximum(31);
        AR3Slider->setPageStep(1);
        AR3Slider->setValue(0);
        AR3Slider->setOrientation(Qt::Vertical);
        AR3Slider->setTickPosition(QSlider::NoTicks);
        D1R3Slider = new QSlider(Env3GroupBox);
        D1R3Slider->setObjectName(QString::fromUtf8("D1R3Slider"));
        D1R3Slider->setGeometry(QRect(70, 50, 20, 100));
        D1R3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1R3Slider->setMaximum(31);
        D1R3Slider->setPageStep(1);
        D1R3Slider->setValue(0);
        D1R3Slider->setOrientation(Qt::Vertical);
        D1R3Slider->setTickPosition(QSlider::NoTicks);
        D1L3Slider = new QSlider(Env3GroupBox);
        D1L3Slider->setObjectName(QString::fromUtf8("D1L3Slider"));
        D1L3Slider->setGeometry(QRect(120, 50, 20, 100));
        D1L3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1L3Slider->setMaximum(15);
        D1L3Slider->setPageStep(1);
        D1L3Slider->setValue(0);
        D1L3Slider->setOrientation(Qt::Vertical);
        D1L3Slider->setTickPosition(QSlider::NoTicks);
        D2R3Slider = new QSlider(Env3GroupBox);
        D2R3Slider->setObjectName(QString::fromUtf8("D2R3Slider"));
        D2R3Slider->setGeometry(QRect(170, 50, 20, 100));
        D2R3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D2R3Slider->setMaximum(31);
        D2R3Slider->setPageStep(1);
        D2R3Slider->setValue(0);
        D2R3Slider->setOrientation(Qt::Vertical);
        D2R3Slider->setTickPosition(QSlider::NoTicks);
        RR3Slider = new QSlider(Env3GroupBox);
        RR3Slider->setObjectName(QString::fromUtf8("RR3Slider"));
        RR3Slider->setGeometry(QRect(220, 50, 20, 100));
        RR3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RR3Slider->setMinimum(0);
        RR3Slider->setMaximum(15);
        RR3Slider->setPageStep(1);
        RR3Slider->setValue(0);
        RR3Slider->setOrientation(Qt::Vertical);
        RR3Slider->setTickPosition(QSlider::NoTicks);
        RR3Slider->setTickInterval(0);
        D1R3SpinBox = new QSpinBox(Env3GroupBox);
        D1R3SpinBox->setObjectName(QString::fromUtf8("D1R3SpinBox"));
        D1R3SpinBox->setGeometry(QRect(60, 20, 40, 21));
        D1R3SpinBox->setFont(font2);
        D1R3SpinBox->setMaximum(31);
        D1L3SpinBox = new QSpinBox(Env3GroupBox);
        D1L3SpinBox->setObjectName(QString::fromUtf8("D1L3SpinBox"));
        D1L3SpinBox->setGeometry(QRect(110, 20, 40, 21));
        D1L3SpinBox->setFont(font2);
        D1L3SpinBox->setMaximum(15);
        D2R3SpinBox = new QSpinBox(Env3GroupBox);
        D2R3SpinBox->setObjectName(QString::fromUtf8("D2R3SpinBox"));
        D2R3SpinBox->setGeometry(QRect(160, 20, 40, 21));
        D2R3SpinBox->setFont(font2);
        D2R3SpinBox->setMaximum(31);
        RR3SpinBox = new QSpinBox(Env3GroupBox);
        RR3SpinBox->setObjectName(QString::fromUtf8("RR3SpinBox"));
        RR3SpinBox->setGeometry(QRect(210, 20, 40, 21));
        RR3SpinBox->setFont(font2);
        RR3SpinBox->setMaximum(15);
        AR3SpinBox = new QSpinBox(Env3GroupBox);
        AR3SpinBox->setObjectName(QString::fromUtf8("AR3SpinBox"));
        AR3SpinBox->setGeometry(QRect(10, 20, 40, 21));
        AR3SpinBox->setFont(font2);
        AR3SpinBox->setMaximum(31);
        Scaling3GroupBox = new Q3GroupBox(TabPage2);
        Scaling3GroupBox->setObjectName(QString::fromUtf8("Scaling3GroupBox"));
        Scaling3GroupBox->setGeometry(QRect(280, 0, 110, 190));
        LS3Label = new QLabel(Scaling3GroupBox);
        LS3Label->setObjectName(QString::fromUtf8("LS3Label"));
        LS3Label->setGeometry(QRect(10, 160, 40, 22));
        LS3Label->setFrameShape(QFrame::Box);
        LS3Label->setFrameShadow(QFrame::Sunken);
        LS3Label->setWordWrap(false);
        RS3Label = new QLabel(Scaling3GroupBox);
        RS3Label->setObjectName(QString::fromUtf8("RS3Label"));
        RS3Label->setGeometry(QRect(60, 160, 40, 22));
        RS3Label->setFrameShape(QFrame::Box);
        RS3Label->setFrameShadow(QFrame::Sunken);
        RS3Label->setWordWrap(false);
        LS3Slider = new QSlider(Scaling3GroupBox);
        LS3Slider->setObjectName(QString::fromUtf8("LS3Slider"));
        LS3Slider->setGeometry(QRect(20, 50, 19, 100));
        LS3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LS3Slider->setMaximum(99);
        LS3Slider->setPageStep(1);
        LS3Slider->setValue(0);
        LS3Slider->setOrientation(Qt::Vertical);
        LS3Slider->setTickPosition(QSlider::NoTicks);
        RS3Slider = new QSlider(Scaling3GroupBox);
        RS3Slider->setObjectName(QString::fromUtf8("RS3Slider"));
        RS3Slider->setGeometry(QRect(70, 50, 19, 100));
        RS3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RS3Slider->setMaximum(3);
        RS3Slider->setPageStep(1);
        RS3Slider->setValue(0);
        RS3Slider->setOrientation(Qt::Vertical);
        RS3Slider->setTickPosition(QSlider::NoTicks);
        LS3SpinBox = new QSpinBox(Scaling3GroupBox);
        LS3SpinBox->setObjectName(QString::fromUtf8("LS3SpinBox"));
        LS3SpinBox->setGeometry(QRect(10, 20, 40, 21));
        LS3SpinBox->setFont(font2);
        LS3SpinBox->setMaximum(99);
        RS3SpinBox = new QSpinBox(Scaling3GroupBox);
        RS3SpinBox->setObjectName(QString::fromUtf8("RS3SpinBox"));
        RS3SpinBox->setGeometry(QRect(60, 20, 40, 21));
        RS3SpinBox->setFont(font2);
        RS3SpinBox->setMaximum(3);
        Vol3groupBox = new Q3GroupBox(TabPage2);
        Vol3groupBox->setObjectName(QString::fromUtf8("Vol3groupBox"));
        Vol3groupBox->setGeometry(QRect(400, 0, 60, 190));
        Vol3Slider = new QSlider(Vol3groupBox);
        Vol3Slider->setObjectName(QString::fromUtf8("Vol3Slider"));
        Vol3Slider->setGeometry(QRect(20, 50, 19, 130));
        Vol3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        Vol3Slider->setMaximum(99);
        Vol3Slider->setPageStep(1);
        Vol3Slider->setValue(0);
        Vol3Slider->setOrientation(Qt::Vertical);
        Vol3Slider->setTickPosition(QSlider::NoTicks);
        Vol3SpinBox = new QSpinBox(Vol3groupBox);
        Vol3SpinBox->setObjectName(QString::fromUtf8("Vol3SpinBox"));
        Vol3SpinBox->setGeometry(QRect(10, 20, 40, 21));
        Vol3SpinBox->setFont(font2);
        Vol3SpinBox->setMaximum(99);
        sensitivity3groupBox = new Q3GroupBox(TabPage2);
        sensitivity3groupBox->setObjectName(QString::fromUtf8("sensitivity3groupBox"));
        sensitivity3groupBox->setGeometry(QRect(10, 240, 260, 80));
        EGS3Label = new QLabel(sensitivity3groupBox);
        EGS3Label->setObjectName(QString::fromUtf8("EGS3Label"));
        EGS3Label->setGeometry(QRect(80, 20, 40, 22));
        EGS3Label->setFrameShape(QFrame::Box);
        EGS3Label->setFrameShadow(QFrame::Sunken);
        EGS3Label->setWordWrap(false);
        KVS3Label = new QLabel(sensitivity3groupBox);
        KVS3Label->setObjectName(QString::fromUtf8("KVS3Label"));
        KVS3Label->setGeometry(QRect(10, 50, 40, 22));
        KVS3Label->setFrameShape(QFrame::Box);
        KVS3Label->setFrameShadow(QFrame::Sunken);
        KVS3Label->setWordWrap(false);
        AME3CheckBox = new QCheckBox(sensitivity3groupBox);
        AME3CheckBox->setObjectName(QString::fromUtf8("AME3CheckBox"));
        AME3CheckBox->setGeometry(QRect(10, 20, 60, 25));
        AME3CheckBox->setFont(font);
        AME3CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS3Slider = new QSlider(sensitivity3groupBox);
        KVS3Slider->setObjectName(QString::fromUtf8("KVS3Slider"));
        KVS3Slider->setGeometry(QRect(60, 50, 84, 19));
        KVS3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS3Slider->setAcceptDrops(false);
        KVS3Slider->setMaximum(7);
        KVS3Slider->setPageStep(1);
        KVS3Slider->setOrientation(Qt::Horizontal);
        KVS3Slider->setTickPosition(QSlider::NoTicks);
        EBS3Slider = new QSlider(sensitivity3groupBox);
        EBS3Slider->setObjectName(QString::fromUtf8("EBS3Slider"));
        EBS3Slider->setGeometry(QRect(130, 20, 70, 20));
        EBS3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EBS3Slider->setMaximum(7);
        EBS3Slider->setPageStep(1);
        EBS3Slider->setOrientation(Qt::Horizontal);
        EBS3Slider->setTickPosition(QSlider::NoTicks);
        EBS3SpinBox = new QSpinBox(sensitivity3groupBox);
        EBS3SpinBox->setObjectName(QString::fromUtf8("EBS3SpinBox"));
        EBS3SpinBox->setGeometry(QRect(210, 20, 40, 21));
        EBS3SpinBox->setFont(font2);
        EBS3SpinBox->setMaximum(7);
        KVS3SpinBox = new QSpinBox(sensitivity3groupBox);
        KVS3SpinBox->setObjectName(QString::fromUtf8("KVS3SpinBox"));
        KVS3SpinBox->setGeometry(QRect(160, 50, 40, 21));
        KVS3SpinBox->setFont(font2);
        KVS3SpinBox->setMaximum(7);
        DetWaveEGS3GroupBox = new Q3GroupBox(TabPage2);
        DetWaveEGS3GroupBox->setObjectName(QString::fromUtf8("DetWaveEGS3GroupBox"));
        DetWaveEGS3GroupBox->setGeometry(QRect(280, 240, 180, 80));
        WaveForm3ComboBox = new QComboBox(DetWaveEGS3GroupBox);
        WaveForm3ComboBox->setObjectName(QString::fromUtf8("WaveForm3ComboBox"));
        WaveForm3ComboBox->setGeometry(QRect(10, 50, 80, 20));
        WaveForm3ComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EGS3comboBox = new QComboBox(DetWaveEGS3GroupBox);
        EGS3comboBox->setObjectName(QString::fromUtf8("EGS3comboBox"));
        EGS3comboBox->setEnabled(false);
        EGS3comboBox->setGeometry(QRect(100, 50, 70, 20));
        EGS3comboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET3Label = new QLabel(DetWaveEGS3GroupBox);
        DET3Label->setObjectName(QString::fromUtf8("DET3Label"));
        DET3Label->setGeometry(QRect(10, 20, 44, 24));
        DET3Label->setFrameShape(QFrame::Box);
        DET3Label->setFrameShadow(QFrame::Sunken);
        DET3Label->setWordWrap(false);
        DET3Slider = new QSlider(DetWaveEGS3GroupBox);
        DET3Slider->setObjectName(QString::fromUtf8("DET3Slider"));
        DET3Slider->setGeometry(QRect(60, 20, 70, 19));
        DET3Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET3Slider->setMinimum(-3);
        DET3Slider->setMaximum(3);
        DET3Slider->setPageStep(1);
        DET3Slider->setValue(0);
        DET3Slider->setOrientation(Qt::Horizontal);
        DET3Slider->setTickPosition(QSlider::NoTicks);
        DET3SpinBox = new QSpinBox(DetWaveEGS3GroupBox);
        DET3SpinBox->setObjectName(QString::fromUtf8("DET3SpinBox"));
        DET3SpinBox->setGeometry(QRect(140, 20, 30, 21));
        DET3SpinBox->setFont(font2);
        DET3SpinBox->setMaximum(3);
        DET3SpinBox->setMinimum(-3);
        deicsOnzeTabWidget->addTab(TabPage2, QString());
        TabPage3 = new QWidget();
        TabPage3->setObjectName(QString::fromUtf8("TabPage3"));
        Frequency4groupBox = new Q3GroupBox(TabPage3);
        Frequency4groupBox->setObjectName(QString::fromUtf8("Frequency4groupBox"));
        Frequency4groupBox->setGeometry(QRect(10, 190, 450, 50));
        CoarseRatio4Label = new QLabel(Frequency4groupBox);
        CoarseRatio4Label->setObjectName(QString::fromUtf8("CoarseRatio4Label"));
        CoarseRatio4Label->setGeometry(QRect(10, 20, 70, 22));
        CoarseRatio4Label->setFrameShape(QFrame::Box);
        CoarseRatio4Label->setFrameShadow(QFrame::Sunken);
        CoarseRatio4Label->setWordWrap(false);
        FineRatio4Label = new QLabel(Frequency4groupBox);
        FineRatio4Label->setObjectName(QString::fromUtf8("FineRatio4Label"));
        FineRatio4Label->setGeometry(QRect(140, 20, 50, 22));
        FineRatio4Label->setFrameShape(QFrame::Box);
        FineRatio4Label->setFrameShadow(QFrame::Sunken);
        FineRatio4Label->setWordWrap(false);
        Freq4Label = new QLabel(Frequency4groupBox);
        Freq4Label->setObjectName(QString::fromUtf8("Freq4Label"));
        Freq4Label->setGeometry(QRect(250, 20, 50, 22));
        Freq4Label->setFrameShape(QFrame::Box);
        Freq4Label->setFrameShadow(QFrame::Sunken);
        Freq4Label->setWordWrap(false);
        Fix4CheckBox = new QCheckBox(Frequency4groupBox);
        Fix4CheckBox->setObjectName(QString::fromUtf8("Fix4CheckBox"));
        Fix4CheckBox->setEnabled(true);
        Fix4CheckBox->setGeometry(QRect(400, 20, 40, 25));
        Fix4CheckBox->setFont(font);
        Fix4CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        FineRatio4SpinBox = new QSpinBox(Frequency4groupBox);
        FineRatio4SpinBox->setObjectName(QString::fromUtf8("FineRatio4SpinBox"));
        FineRatio4SpinBox->setGeometry(QRect(200, 20, 40, 21));
        FineRatio4SpinBox->setFont(font2);
        Freq4SpinBox = new QSpinBox(Frequency4groupBox);
        Freq4SpinBox->setObjectName(QString::fromUtf8("Freq4SpinBox"));
        Freq4SpinBox->setEnabled(false);
        Freq4SpinBox->setGeometry(QRect(310, 20, 80, 21));
        Freq4SpinBox->setFont(font2);
        Freq4SpinBox->setMaximum(32000);
        CoarseRatio4SpinBox = new QSpinBox(Frequency4groupBox);
        CoarseRatio4SpinBox->setObjectName(QString::fromUtf8("CoarseRatio4SpinBox"));
        CoarseRatio4SpinBox->setGeometry(QRect(90, 20, 40, 21));
        CoarseRatio4SpinBox->setFont(font2);
        Scaling4GroupBox = new Q3GroupBox(TabPage3);
        Scaling4GroupBox->setObjectName(QString::fromUtf8("Scaling4GroupBox"));
        Scaling4GroupBox->setGeometry(QRect(280, 0, 110, 190));
        LS4Label = new QLabel(Scaling4GroupBox);
        LS4Label->setObjectName(QString::fromUtf8("LS4Label"));
        LS4Label->setGeometry(QRect(10, 160, 40, 22));
        LS4Label->setFrameShape(QFrame::Box);
        LS4Label->setFrameShadow(QFrame::Sunken);
        LS4Label->setWordWrap(false);
        RS4Label = new QLabel(Scaling4GroupBox);
        RS4Label->setObjectName(QString::fromUtf8("RS4Label"));
        RS4Label->setGeometry(QRect(60, 160, 40, 22));
        RS4Label->setFrameShape(QFrame::Box);
        RS4Label->setFrameShadow(QFrame::Sunken);
        RS4Label->setWordWrap(false);
        LS4Slider = new QSlider(Scaling4GroupBox);
        LS4Slider->setObjectName(QString::fromUtf8("LS4Slider"));
        LS4Slider->setGeometry(QRect(20, 50, 19, 100));
        LS4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        LS4Slider->setMaximum(99);
        LS4Slider->setPageStep(1);
        LS4Slider->setValue(0);
        LS4Slider->setOrientation(Qt::Vertical);
        LS4Slider->setTickPosition(QSlider::NoTicks);
        RS4Slider = new QSlider(Scaling4GroupBox);
        RS4Slider->setObjectName(QString::fromUtf8("RS4Slider"));
        RS4Slider->setGeometry(QRect(70, 50, 19, 100));
        RS4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RS4Slider->setMaximum(3);
        RS4Slider->setPageStep(1);
        RS4Slider->setValue(0);
        RS4Slider->setOrientation(Qt::Vertical);
        RS4Slider->setTickPosition(QSlider::NoTicks);
        RS4SpinBox = new QSpinBox(Scaling4GroupBox);
        RS4SpinBox->setObjectName(QString::fromUtf8("RS4SpinBox"));
        RS4SpinBox->setGeometry(QRect(60, 20, 40, 21));
        RS4SpinBox->setFont(font2);
        RS4SpinBox->setMaximum(3);
        LS4SpinBox = new QSpinBox(Scaling4GroupBox);
        LS4SpinBox->setObjectName(QString::fromUtf8("LS4SpinBox"));
        LS4SpinBox->setGeometry(QRect(10, 20, 40, 21));
        LS4SpinBox->setFont(font2);
        LS4SpinBox->setMaximum(99);
        Env4GroupBox = new Q3GroupBox(TabPage3);
        Env4GroupBox->setObjectName(QString::fromUtf8("Env4GroupBox"));
        Env4GroupBox->setGeometry(QRect(10, 0, 260, 190));
        AR4Slider = new QSlider(Env4GroupBox);
        AR4Slider->setObjectName(QString::fromUtf8("AR4Slider"));
        AR4Slider->setGeometry(QRect(20, 50, 20, 100));
        AR4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        AR4Slider->setMaximum(31);
        AR4Slider->setPageStep(1);
        AR4Slider->setValue(0);
        AR4Slider->setOrientation(Qt::Vertical);
        AR4Slider->setTickPosition(QSlider::NoTicks);
        AR4Label = new QLabel(Env4GroupBox);
        AR4Label->setObjectName(QString::fromUtf8("AR4Label"));
        AR4Label->setGeometry(QRect(10, 160, 40, 22));
        AR4Label->setFrameShape(QFrame::Box);
        AR4Label->setFrameShadow(QFrame::Sunken);
        AR4Label->setWordWrap(false);
        RR4Label = new QLabel(Env4GroupBox);
        RR4Label->setObjectName(QString::fromUtf8("RR4Label"));
        RR4Label->setGeometry(QRect(210, 160, 43, 22));
        RR4Label->setFrameShape(QFrame::Box);
        RR4Label->setFrameShadow(QFrame::Sunken);
        RR4Label->setWordWrap(false);
        D2R4Label = new QLabel(Env4GroupBox);
        D2R4Label->setObjectName(QString::fromUtf8("D2R4Label"));
        D2R4Label->setGeometry(QRect(160, 160, 43, 22));
        D2R4Label->setFrameShape(QFrame::Box);
        D2R4Label->setFrameShadow(QFrame::Sunken);
        D2R4Label->setWordWrap(false);
        D2R4Slider = new QSlider(Env4GroupBox);
        D2R4Slider->setObjectName(QString::fromUtf8("D2R4Slider"));
        D2R4Slider->setGeometry(QRect(170, 50, 20, 100));
        D2R4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D2R4Slider->setMaximum(31);
        D2R4Slider->setPageStep(1);
        D2R4Slider->setValue(0);
        D2R4Slider->setOrientation(Qt::Vertical);
        D2R4Slider->setTickPosition(QSlider::NoTicks);
        D1L4Slider = new QSlider(Env4GroupBox);
        D1L4Slider->setObjectName(QString::fromUtf8("D1L4Slider"));
        D1L4Slider->setGeometry(QRect(120, 50, 20, 100));
        D1L4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1L4Slider->setMaximum(15);
        D1L4Slider->setPageStep(1);
        D1L4Slider->setValue(0);
        D1L4Slider->setOrientation(Qt::Vertical);
        D1L4Slider->setTickPosition(QSlider::NoTicks);
        D1L4Label = new QLabel(Env4GroupBox);
        D1L4Label->setObjectName(QString::fromUtf8("D1L4Label"));
        D1L4Label->setGeometry(QRect(110, 160, 43, 22));
        D1L4Label->setFrameShape(QFrame::Box);
        D1L4Label->setFrameShadow(QFrame::Sunken);
        D1L4Label->setWordWrap(false);
        D1R4Label = new QLabel(Env4GroupBox);
        D1R4Label->setObjectName(QString::fromUtf8("D1R4Label"));
        D1R4Label->setGeometry(QRect(60, 160, 43, 22));
        D1R4Label->setFrameShape(QFrame::Box);
        D1R4Label->setFrameShadow(QFrame::Sunken);
        D1R4Label->setWordWrap(false);
        D1R4Slider = new QSlider(Env4GroupBox);
        D1R4Slider->setObjectName(QString::fromUtf8("D1R4Slider"));
        D1R4Slider->setGeometry(QRect(70, 50, 20, 100));
        D1R4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        D1R4Slider->setMaximum(31);
        D1R4Slider->setPageStep(1);
        D1R4Slider->setValue(0);
        D1R4Slider->setOrientation(Qt::Vertical);
        D1R4Slider->setTickPosition(QSlider::NoTicks);
        RR4Slider = new QSlider(Env4GroupBox);
        RR4Slider->setObjectName(QString::fromUtf8("RR4Slider"));
        RR4Slider->setGeometry(QRect(220, 50, 20, 100));
        RR4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        RR4Slider->setMinimum(0);
        RR4Slider->setMaximum(15);
        RR4Slider->setPageStep(1);
        RR4Slider->setValue(0);
        RR4Slider->setOrientation(Qt::Vertical);
        RR4Slider->setTickPosition(QSlider::NoTicks);
        RR4Slider->setTickInterval(0);
        D1R4SpinBox = new QSpinBox(Env4GroupBox);
        D1R4SpinBox->setObjectName(QString::fromUtf8("D1R4SpinBox"));
        D1R4SpinBox->setGeometry(QRect(60, 20, 40, 21));
        D1R4SpinBox->setFont(font2);
        D1R4SpinBox->setMaximum(31);
        D1L4SpinBox = new QSpinBox(Env4GroupBox);
        D1L4SpinBox->setObjectName(QString::fromUtf8("D1L4SpinBox"));
        D1L4SpinBox->setGeometry(QRect(110, 20, 40, 21));
        D1L4SpinBox->setFont(font2);
        D1L4SpinBox->setMaximum(15);
        D2R4SpinBox = new QSpinBox(Env4GroupBox);
        D2R4SpinBox->setObjectName(QString::fromUtf8("D2R4SpinBox"));
        D2R4SpinBox->setGeometry(QRect(160, 20, 40, 21));
        D2R4SpinBox->setFont(font2);
        D2R4SpinBox->setMaximum(31);
        RR4SpinBox = new QSpinBox(Env4GroupBox);
        RR4SpinBox->setObjectName(QString::fromUtf8("RR4SpinBox"));
        RR4SpinBox->setGeometry(QRect(210, 20, 40, 21));
        RR4SpinBox->setFont(font2);
        RR4SpinBox->setMaximum(15);
        AR4SpinBox = new QSpinBox(Env4GroupBox);
        AR4SpinBox->setObjectName(QString::fromUtf8("AR4SpinBox"));
        AR4SpinBox->setGeometry(QRect(10, 20, 40, 21));
        AR4SpinBox->setFont(font2);
        AR4SpinBox->setMaximum(31);
        Vol4groupBox = new Q3GroupBox(TabPage3);
        Vol4groupBox->setObjectName(QString::fromUtf8("Vol4groupBox"));
        Vol4groupBox->setGeometry(QRect(400, 0, 60, 190));
        Vol4Slider = new QSlider(Vol4groupBox);
        Vol4Slider->setObjectName(QString::fromUtf8("Vol4Slider"));
        Vol4Slider->setGeometry(QRect(20, 50, 19, 130));
        Vol4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        Vol4Slider->setMaximum(99);
        Vol4Slider->setPageStep(1);
        Vol4Slider->setValue(0);
        Vol4Slider->setOrientation(Qt::Vertical);
        Vol4Slider->setTickPosition(QSlider::NoTicks);
        Vol4SpinBox = new QSpinBox(Vol4groupBox);
        Vol4SpinBox->setObjectName(QString::fromUtf8("Vol4SpinBox"));
        Vol4SpinBox->setGeometry(QRect(10, 20, 40, 21));
        Vol4SpinBox->setFont(font2);
        Vol4SpinBox->setMaximum(99);
        sensitivity4groupBox = new Q3GroupBox(TabPage3);
        sensitivity4groupBox->setObjectName(QString::fromUtf8("sensitivity4groupBox"));
        sensitivity4groupBox->setGeometry(QRect(10, 240, 260, 80));
        EGS4Label = new QLabel(sensitivity4groupBox);
        EGS4Label->setObjectName(QString::fromUtf8("EGS4Label"));
        EGS4Label->setGeometry(QRect(80, 20, 40, 22));
        EGS4Label->setFrameShape(QFrame::Box);
        EGS4Label->setFrameShadow(QFrame::Sunken);
        EGS4Label->setWordWrap(false);
        KVS4Label = new QLabel(sensitivity4groupBox);
        KVS4Label->setObjectName(QString::fromUtf8("KVS4Label"));
        KVS4Label->setGeometry(QRect(10, 50, 40, 22));
        KVS4Label->setFrameShape(QFrame::Box);
        KVS4Label->setFrameShadow(QFrame::Sunken);
        KVS4Label->setWordWrap(false);
        AME4CheckBox = new QCheckBox(sensitivity4groupBox);
        AME4CheckBox->setObjectName(QString::fromUtf8("AME4CheckBox"));
        AME4CheckBox->setGeometry(QRect(10, 20, 60, 25));
        AME4CheckBox->setFont(font);
        AME4CheckBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS4Slider = new QSlider(sensitivity4groupBox);
        KVS4Slider->setObjectName(QString::fromUtf8("KVS4Slider"));
        KVS4Slider->setGeometry(QRect(60, 50, 84, 19));
        KVS4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        KVS4Slider->setAcceptDrops(false);
        KVS4Slider->setMaximum(7);
        KVS4Slider->setPageStep(1);
        KVS4Slider->setOrientation(Qt::Horizontal);
        KVS4Slider->setTickPosition(QSlider::NoTicks);
        EBS4Slider = new QSlider(sensitivity4groupBox);
        EBS4Slider->setObjectName(QString::fromUtf8("EBS4Slider"));
        EBS4Slider->setGeometry(QRect(130, 20, 70, 20));
        EBS4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EBS4Slider->setMaximum(7);
        EBS4Slider->setPageStep(1);
        EBS4Slider->setOrientation(Qt::Horizontal);
        EBS4Slider->setTickPosition(QSlider::NoTicks);
        KVS4SpinBox = new QSpinBox(sensitivity4groupBox);
        KVS4SpinBox->setObjectName(QString::fromUtf8("KVS4SpinBox"));
        KVS4SpinBox->setGeometry(QRect(160, 50, 40, 21));
        KVS4SpinBox->setFont(font2);
        KVS4SpinBox->setMaximum(7);
        EBS4SpinBox = new QSpinBox(sensitivity4groupBox);
        EBS4SpinBox->setObjectName(QString::fromUtf8("EBS4SpinBox"));
        EBS4SpinBox->setGeometry(QRect(210, 20, 40, 21));
        EBS4SpinBox->setFont(font2);
        EBS4SpinBox->setMaximum(7);
        DetWaveEGS4GroupBox = new Q3GroupBox(TabPage3);
        DetWaveEGS4GroupBox->setObjectName(QString::fromUtf8("DetWaveEGS4GroupBox"));
        DetWaveEGS4GroupBox->setGeometry(QRect(280, 240, 180, 80));
        DET4Slider = new QSlider(DetWaveEGS4GroupBox);
        DET4Slider->setObjectName(QString::fromUtf8("DET4Slider"));
        DET4Slider->setGeometry(QRect(60, 20, 70, 19));
        DET4Slider->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET4Slider->setMinimum(-3);
        DET4Slider->setMaximum(3);
        DET4Slider->setPageStep(1);
        DET4Slider->setValue(0);
        DET4Slider->setOrientation(Qt::Horizontal);
        DET4Slider->setTickPosition(QSlider::NoTicks);
        DET4Label = new QLabel(DetWaveEGS4GroupBox);
        DET4Label->setObjectName(QString::fromUtf8("DET4Label"));
        DET4Label->setGeometry(QRect(10, 20, 44, 24));
        DET4Label->setFrameShape(QFrame::Box);
        DET4Label->setFrameShadow(QFrame::Sunken);
        DET4Label->setWordWrap(false);
        WaveForm4ComboBox = new QComboBox(DetWaveEGS4GroupBox);
        WaveForm4ComboBox->setObjectName(QString::fromUtf8("WaveForm4ComboBox"));
        WaveForm4ComboBox->setGeometry(QRect(10, 50, 80, 20));
        WaveForm4ComboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        EGS4comboBox = new QComboBox(DetWaveEGS4GroupBox);
        EGS4comboBox->setObjectName(QString::fromUtf8("EGS4comboBox"));
        EGS4comboBox->setEnabled(false);
        EGS4comboBox->setGeometry(QRect(100, 50, 70, 20));
        EGS4comboBox->setCursor(QCursor(static_cast<Qt::CursorShape>(13)));
        DET4SpinBox = new QSpinBox(DetWaveEGS4GroupBox);
        DET4SpinBox->setObjectName(QString::fromUtf8("DET4SpinBox"));
        DET4SpinBox->setGeometry(QRect(140, 20, 30, 21));
        DET4SpinBox->setFont(font2);
        DET4SpinBox->setMaximum(3);
        DET4SpinBox->setMinimum(-3);
        deicsOnzeTabWidget->addTab(TabPage3, QString());
        QWidget::setTabOrder(deicsOnzeTabWidget, categoryListView);
        QWidget::setTabOrder(categoryListView, nameLineEdit);
        QWidget::setTabOrder(nameLineEdit, masterVolSlider);
        QWidget::setTabOrder(masterVolSlider, feedbackSlider);
        QWidget::setTabOrder(feedbackSlider, polyMonoComboBox);
        QWidget::setTabOrder(polyMonoComboBox, LFOSyncCheckBox);
        QWidget::setTabOrder(LFOSyncCheckBox, PModSensSlider);
        QWidget::setTabOrder(PModSensSlider, LFOWaveComboBox);
        QWidget::setTabOrder(LFOWaveComboBox, AModSensSlider);
        QWidget::setTabOrder(AModSensSlider, PModDepthSlider);
        QWidget::setTabOrder(PModDepthSlider, AModDepthSlider);
        QWidget::setTabOrder(AModDepthSlider, LFOSpeedSlider);
        QWidget::setTabOrder(LFOSpeedSlider, LFODelaySlider);
        QWidget::setTabOrder(LFODelaySlider, transposeSlider);
        QWidget::setTabOrder(transposeSlider, globalDetuneSlider);
        QWidget::setTabOrder(globalDetuneSlider, algorithmComboBox);
        QWidget::setTabOrder(algorithmComboBox, AR1Slider);
        QWidget::setTabOrder(AR1Slider, D1R1Slider);
        QWidget::setTabOrder(D1R1Slider, D1L1Slider);
        QWidget::setTabOrder(D1L1Slider, D2R1Slider);
        QWidget::setTabOrder(D2R1Slider, RR1Slider);
        QWidget::setTabOrder(RR1Slider, LS1Slider);
        QWidget::setTabOrder(LS1Slider, RS1Slider);
        QWidget::setTabOrder(RS1Slider, Vol1Slider);
        QWidget::setTabOrder(Vol1Slider, Fix1CheckBox);
        QWidget::setTabOrder(Fix1CheckBox, AME1CheckBox);
        QWidget::setTabOrder(AME1CheckBox, EBS1Slider);
        QWidget::setTabOrder(EBS1Slider, DET1Slider);
        QWidget::setTabOrder(DET1Slider, KVS1Slider);
        QWidget::setTabOrder(KVS1Slider, WaveForm1ComboBox);
        QWidget::setTabOrder(WaveForm1ComboBox, EGQ1ComboBox);
        QWidget::setTabOrder(EGQ1ComboBox, AR2Slider);
        QWidget::setTabOrder(AR2Slider, D1R2Slider);
        QWidget::setTabOrder(D1R2Slider, D1L2Slider);
        QWidget::setTabOrder(D1L2Slider, D2R2Slider);
        QWidget::setTabOrder(D2R2Slider, RR2Slider);
        QWidget::setTabOrder(RR2Slider, LS2Slider);
        QWidget::setTabOrder(LS2Slider, RS2Slider);
        QWidget::setTabOrder(RS2Slider, Vol2Slider);
        QWidget::setTabOrder(Vol2Slider, AME2CheckBox);
        QWidget::setTabOrder(AME2CheckBox, EBS2Slider);
        QWidget::setTabOrder(EBS2Slider, DET2Slider);
        QWidget::setTabOrder(DET2Slider, KVS2Slider);
        QWidget::setTabOrder(KVS2Slider, WaveForm2ComboBox);
        QWidget::setTabOrder(WaveForm2ComboBox, EGS2comboBox);
        QWidget::setTabOrder(EGS2comboBox, AR3Slider);
        QWidget::setTabOrder(AR3Slider, D1R3Slider);
        QWidget::setTabOrder(D1R3Slider, D1L3Slider);
        QWidget::setTabOrder(D1L3Slider, D2R3Slider);
        QWidget::setTabOrder(D2R3Slider, RR3Slider);
        QWidget::setTabOrder(RR3Slider, LS3Slider);
        QWidget::setTabOrder(LS3Slider, RS3Slider);
        QWidget::setTabOrder(RS3Slider, Vol3Slider);
        QWidget::setTabOrder(Vol3Slider, AME3CheckBox);
        QWidget::setTabOrder(AME3CheckBox, EBS3Slider);
        QWidget::setTabOrder(EBS3Slider, DET3Slider);
        QWidget::setTabOrder(DET3Slider, KVS3Slider);
        QWidget::setTabOrder(KVS3Slider, WaveForm3ComboBox);
        QWidget::setTabOrder(WaveForm3ComboBox, EGS3comboBox);
        QWidget::setTabOrder(EGS3comboBox, AR4Slider);
        QWidget::setTabOrder(AR4Slider, D1R4Slider);
        QWidget::setTabOrder(D1R4Slider, D1L4Slider);
        QWidget::setTabOrder(D1L4Slider, D2R4Slider);
        QWidget::setTabOrder(D2R4Slider, RR4Slider);
        QWidget::setTabOrder(RR4Slider, LS4Slider);
        QWidget::setTabOrder(LS4Slider, RS4Slider);
        QWidget::setTabOrder(RS4Slider, Vol4Slider);
        QWidget::setTabOrder(Vol4Slider, AME4CheckBox);
        QWidget::setTabOrder(AME4CheckBox, EBS4Slider);
        QWidget::setTabOrder(EBS4Slider, DET4Slider);
        QWidget::setTabOrder(DET4Slider, KVS4Slider);
        QWidget::setTabOrder(KVS4Slider, WaveForm4ComboBox);
        QWidget::setTabOrder(WaveForm4ComboBox, EGS4comboBox);

        retranslateUi(DeicsOnzeGuiBase);
        QObject::connect(Fix1CheckBox, SIGNAL(toggled(bool)), Freq1SpinBox, SLOT(setEnabled(bool)));
        QObject::connect(Fix1CheckBox, SIGNAL(toggled(bool)), FineRatio1SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix1CheckBox, SIGNAL(toggled(bool)), CoarseRatio1SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix2CheckBox, SIGNAL(toggled(bool)), FineRatio2SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix2CheckBox, SIGNAL(toggled(bool)), CoarseRatio2SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix3CheckBox, SIGNAL(toggled(bool)), FineRatio3SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix3CheckBox, SIGNAL(toggled(bool)), CoarseRatio3SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix4CheckBox, SIGNAL(toggled(bool)), FineRatio4SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix4CheckBox, SIGNAL(toggled(bool)), CoarseRatio4SpinBox, SLOT(setDisabled(bool)));
        QObject::connect(Fix2CheckBox, SIGNAL(toggled(bool)), Freq2SpinBox, SLOT(setEnabled(bool)));
        QObject::connect(Fix3CheckBox, SIGNAL(toggled(bool)), Freq3SpinBox, SLOT(setEnabled(bool)));
        QObject::connect(Fix4CheckBox, SIGNAL(toggled(bool)), Freq4SpinBox, SLOT(setEnabled(bool)));
        QObject::connect(AR4Slider, SIGNAL(valueChanged(int)), AR4SpinBox, SLOT(setValue(int)));
        QObject::connect(AR4SpinBox, SIGNAL(valueChanged(int)), AR4Slider, SLOT(setValue(int)));
        QObject::connect(D1R4Slider, SIGNAL(valueChanged(int)), D1R4SpinBox, SLOT(setValue(int)));
        QObject::connect(D1R4SpinBox, SIGNAL(valueChanged(int)), D1R4Slider, SLOT(setValue(int)));
        QObject::connect(D1L4Slider, SIGNAL(valueChanged(int)), D1L4SpinBox, SLOT(setValue(int)));
        QObject::connect(D1L4SpinBox, SIGNAL(valueChanged(int)), D1L4Slider, SLOT(setValue(int)));
        QObject::connect(D2R4Slider, SIGNAL(valueChanged(int)), D2R4SpinBox, SLOT(setValue(int)));
        QObject::connect(RR4Slider, SIGNAL(valueChanged(int)), RR4SpinBox, SLOT(setValue(int)));
        QObject::connect(RR4SpinBox, SIGNAL(valueChanged(int)), RR4Slider, SLOT(setValue(int)));
        QObject::connect(D2R4SpinBox, SIGNAL(valueChanged(int)), D2R4Slider, SLOT(setValue(int)));
        QObject::connect(AR3SpinBox, SIGNAL(valueChanged(int)), AR3Slider, SLOT(setValue(int)));
        QObject::connect(AR3Slider, SIGNAL(valueChanged(int)), AR3SpinBox, SLOT(setValue(int)));
        QObject::connect(D1R3Slider, SIGNAL(valueChanged(int)), D1R3SpinBox, SLOT(setValue(int)));
        QObject::connect(D1R3SpinBox, SIGNAL(valueChanged(int)), D1R3Slider, SLOT(setValue(int)));
        QObject::connect(D1L3Slider, SIGNAL(valueChanged(int)), D1L3SpinBox, SLOT(setValue(int)));
        QObject::connect(D1L3SpinBox, SIGNAL(valueChanged(int)), D1L3Slider, SLOT(setValue(int)));
        QObject::connect(D2R3Slider, SIGNAL(valueChanged(int)), D2R3SpinBox, SLOT(setValue(int)));
        QObject::connect(D2R3SpinBox, SIGNAL(valueChanged(int)), D2R3Slider, SLOT(setValue(int)));
        QObject::connect(RR3Slider, SIGNAL(valueChanged(int)), RR3SpinBox, SLOT(setValue(int)));
        QObject::connect(RR3SpinBox, SIGNAL(valueChanged(int)), RR3Slider, SLOT(setValue(int)));
        QObject::connect(AR2Slider, SIGNAL(valueChanged(int)), AR2SpinBox, SLOT(setValue(int)));
        QObject::connect(AR2SpinBox, SIGNAL(valueChanged(int)), AR2Slider, SLOT(setValue(int)));
        QObject::connect(D1R2Slider, SIGNAL(valueChanged(int)), D1R2SpinBox, SLOT(setValue(int)));
        QObject::connect(D1R2SpinBox, SIGNAL(valueChanged(int)), D1R2Slider, SLOT(setValue(int)));
        QObject::connect(D1L2Slider, SIGNAL(valueChanged(int)), D1L2SpinBox, SLOT(setValue(int)));
        QObject::connect(D1L2SpinBox, SIGNAL(valueChanged(int)), D1L2Slider, SLOT(setValue(int)));
        QObject::connect(D2R2Slider, SIGNAL(valueChanged(int)), D2R2SpinBox, SLOT(setValue(int)));
        QObject::connect(D2R2SpinBox, SIGNAL(valueChanged(int)), D2R2Slider, SLOT(setValue(int)));
        QObject::connect(RR2Slider, SIGNAL(valueChanged(int)), RR2SpinBox, SLOT(setValue(int)));
        QObject::connect(RR2SpinBox, SIGNAL(valueChanged(int)), RR2Slider, SLOT(setValue(int)));
        QObject::connect(AR1Slider, SIGNAL(valueChanged(int)), AR1SpinBox, SLOT(setValue(int)));
        QObject::connect(AR1SpinBox, SIGNAL(valueChanged(int)), AR1Slider, SLOT(setValue(int)));
        QObject::connect(D1R1Slider, SIGNAL(valueChanged(int)), D1R1SpinBox, SLOT(setValue(int)));
        QObject::connect(D1R1SpinBox, SIGNAL(valueChanged(int)), D1R1Slider, SLOT(setValue(int)));
        QObject::connect(D1L1Slider, SIGNAL(valueChanged(int)), D1L1SpinBox, SLOT(setValue(int)));
        QObject::connect(D1L1SpinBox, SIGNAL(valueChanged(int)), D1L1Slider, SLOT(setValue(int)));
        QObject::connect(D2R1Slider, SIGNAL(valueChanged(int)), D2R1SpinBox, SLOT(setValue(int)));
        QObject::connect(D2R1SpinBox, SIGNAL(valueChanged(int)), D2R1Slider, SLOT(setValue(int)));
        QObject::connect(RR1Slider, SIGNAL(valueChanged(int)), RR1SpinBox, SLOT(setValue(int)));
        QObject::connect(RR1SpinBox, SIGNAL(valueChanged(int)), RR1Slider, SLOT(setValue(int)));
        QObject::connect(LS1Slider, SIGNAL(valueChanged(int)), LS1SpinBox, SLOT(setValue(int)));
        QObject::connect(LS1SpinBox, SIGNAL(valueChanged(int)), LS1Slider, SLOT(setValue(int)));
        QObject::connect(RS1Slider, SIGNAL(valueChanged(int)), RS1SpinBox, SLOT(setValue(int)));
        QObject::connect(RS1SpinBox, SIGNAL(valueChanged(int)), RS1Slider, SLOT(setValue(int)));
        QObject::connect(LS2Slider, SIGNAL(valueChanged(int)), LS2SpinBox, SLOT(setValue(int)));
        QObject::connect(LS2SpinBox, SIGNAL(valueChanged(int)), LS2Slider, SLOT(setValue(int)));
        QObject::connect(RS2Slider, SIGNAL(valueChanged(int)), RS2SpinBox, SLOT(setValue(int)));
        QObject::connect(RS2SpinBox, SIGNAL(valueChanged(int)), RS2Slider, SLOT(setValue(int)));
        QObject::connect(LS3Slider, SIGNAL(valueChanged(int)), LS3SpinBox, SLOT(setValue(int)));
        QObject::connect(LS3SpinBox, SIGNAL(valueChanged(int)), LS3Slider, SLOT(setValue(int)));
        QObject::connect(RS3Slider, SIGNAL(valueChanged(int)), RS3SpinBox, SLOT(setValue(int)));
        QObject::connect(RS3SpinBox, SIGNAL(valueChanged(int)), RS3Slider, SLOT(setValue(int)));
        QObject::connect(LS4Slider, SIGNAL(valueChanged(int)), LS4SpinBox, SLOT(setValue(int)));
        QObject::connect(LS4SpinBox, SIGNAL(valueChanged(int)), LS4Slider, SLOT(setValue(int)));
        QObject::connect(RS4Slider, SIGNAL(valueChanged(int)), RS4SpinBox, SLOT(setValue(int)));
        QObject::connect(RS4SpinBox, SIGNAL(valueChanged(int)), RS4Slider, SLOT(setValue(int)));
        QObject::connect(Vol4Slider, SIGNAL(valueChanged(int)), Vol4SpinBox, SLOT(setValue(int)));
        QObject::connect(Vol4SpinBox, SIGNAL(valueChanged(int)), Vol4Slider, SLOT(setValue(int)));
        QObject::connect(Vol3Slider, SIGNAL(valueChanged(int)), Vol3SpinBox, SLOT(setValue(int)));
        QObject::connect(Vol3SpinBox, SIGNAL(valueChanged(int)), Vol3Slider, SLOT(setValue(int)));
        QObject::connect(Vol2Slider, SIGNAL(valueChanged(int)), Vol2SpinBox, SLOT(setValue(int)));
        QObject::connect(Vol2SpinBox, SIGNAL(valueChanged(int)), Vol2Slider, SLOT(setValue(int)));
        QObject::connect(Vol1Slider, SIGNAL(valueChanged(int)), Vol1SpinBox, SLOT(setValue(int)));
        QObject::connect(Vol1SpinBox, SIGNAL(valueChanged(int)), Vol1Slider, SLOT(setValue(int)));
        QObject::connect(EBS1Slider, SIGNAL(valueChanged(int)), EBS1SpinBox, SLOT(setValue(int)));
        QObject::connect(EBS1SpinBox, SIGNAL(valueChanged(int)), EBS1Slider, SLOT(setValue(int)));
        QObject::connect(KVS1Slider, SIGNAL(valueChanged(int)), KVS1SpinBox, SLOT(setValue(int)));
        QObject::connect(KVS1SpinBox, SIGNAL(valueChanged(int)), KVS1Slider, SLOT(setValue(int)));
        QObject::connect(EBS2Slider, SIGNAL(valueChanged(int)), EBS2SpinBox, SLOT(setValue(int)));
        QObject::connect(EBS2SpinBox, SIGNAL(valueChanged(int)), EBS2Slider, SLOT(setValue(int)));
        QObject::connect(KVS2Slider, SIGNAL(valueChanged(int)), KVS2SpinBox, SLOT(setValue(int)));
        QObject::connect(KVS2SpinBox, SIGNAL(valueChanged(int)), KVS2Slider, SLOT(setValue(int)));
        QObject::connect(EBS3Slider, SIGNAL(valueChanged(int)), EBS3SpinBox, SLOT(setValue(int)));
        QObject::connect(EBS3SpinBox, SIGNAL(valueChanged(int)), EBS3Slider, SLOT(setValue(int)));
        QObject::connect(KVS3Slider, SIGNAL(valueChanged(int)), KVS3SpinBox, SLOT(setValue(int)));
        QObject::connect(KVS3SpinBox, SIGNAL(valueChanged(int)), KVS3Slider, SLOT(setValue(int)));
        QObject::connect(EBS4Slider, SIGNAL(valueChanged(int)), EBS4SpinBox, SLOT(setValue(int)));
        QObject::connect(EBS4SpinBox, SIGNAL(valueChanged(int)), EBS4Slider, SLOT(setValue(int)));
        QObject::connect(KVS4Slider, SIGNAL(valueChanged(int)), KVS4SpinBox, SLOT(setValue(int)));
        QObject::connect(KVS4SpinBox, SIGNAL(valueChanged(int)), KVS4Slider, SLOT(setValue(int)));
        QObject::connect(DET4Slider, SIGNAL(valueChanged(int)), DET4SpinBox, SLOT(setValue(int)));
        QObject::connect(DET4SpinBox, SIGNAL(valueChanged(int)), DET4Slider, SLOT(setValue(int)));
        QObject::connect(DET3Slider, SIGNAL(valueChanged(int)), DET3SpinBox, SLOT(setValue(int)));
        QObject::connect(DET3SpinBox, SIGNAL(valueChanged(int)), DET3Slider, SLOT(setValue(int)));
        QObject::connect(DET2Slider, SIGNAL(valueChanged(int)), DET2SpinBox, SLOT(setValue(int)));
        QObject::connect(DET2SpinBox, SIGNAL(valueChanged(int)), DET2Slider, SLOT(setValue(int)));
        QObject::connect(DET1Slider, SIGNAL(valueChanged(int)), DET1SpinBox, SLOT(setValue(int)));
        QObject::connect(DET1SpinBox, SIGNAL(valueChanged(int)), DET1Slider, SLOT(setValue(int)));
        QObject::connect(masterVolSlider, SIGNAL(valueChanged(int)), MasterVolumeSpinBox, SLOT(setValue(int)));
        QObject::connect(MasterVolumeSpinBox, SIGNAL(valueChanged(int)), masterVolSlider, SLOT(setValue(int)));
        QObject::connect(feedbackSlider, SIGNAL(valueChanged(int)), feedbackSpinBox, SLOT(setValue(int)));
        QObject::connect(feedbackSpinBox, SIGNAL(valueChanged(int)), feedbackSlider, SLOT(setValue(int)));
        QObject::connect(PitchBendRangeSlider, SIGNAL(valueChanged(int)), pitchBendRangeSpinBox, SLOT(setValue(int)));
        QObject::connect(pitchBendRangeSpinBox, SIGNAL(valueChanged(int)), PitchBendRangeSlider, SLOT(setValue(int)));
        QObject::connect(PModSensSlider, SIGNAL(valueChanged(int)), PMSSpinBox, SLOT(setValue(int)));
        QObject::connect(PMSSpinBox, SIGNAL(valueChanged(int)), PModSensSlider, SLOT(setValue(int)));
        QObject::connect(AModSensSlider, SIGNAL(valueChanged(int)), AMSSpinBox, SLOT(setValue(int)));
        QObject::connect(AMSSpinBox, SIGNAL(valueChanged(int)), AModSensSlider, SLOT(setValue(int)));
        QObject::connect(PModDepthSlider, SIGNAL(valueChanged(int)), PModDepthSpinBox, SLOT(setValue(int)));
        QObject::connect(PModDepthSpinBox, SIGNAL(valueChanged(int)), PModDepthSlider, SLOT(setValue(int)));
        QObject::connect(AModDepthSlider, SIGNAL(valueChanged(int)), AModDepthSpinBox, SLOT(setValue(int)));
        QObject::connect(AModDepthSpinBox, SIGNAL(valueChanged(int)), AModDepthSlider, SLOT(setValue(int)));
        QObject::connect(LFOSpeedSlider, SIGNAL(valueChanged(int)), LFOSpeedSpinBox, SLOT(setValue(int)));
        QObject::connect(LFOSpeedSpinBox, SIGNAL(valueChanged(int)), LFOSpeedSlider, SLOT(setValue(int)));
        QObject::connect(LFODelaySlider, SIGNAL(valueChanged(int)), LFODelaySpinBox, SLOT(setValue(int)));
        QObject::connect(LFODelaySpinBox, SIGNAL(valueChanged(int)), LFODelaySlider, SLOT(setValue(int)));
        QObject::connect(transposeSlider, SIGNAL(valueChanged(int)), transposeSpinBox, SLOT(setValue(int)));
        QObject::connect(transposeSpinBox, SIGNAL(valueChanged(int)), transposeSlider, SLOT(setValue(int)));
        QObject::connect(globalDetuneSlider, SIGNAL(valueChanged(int)), globalDetuneSpinBox, SLOT(setValue(int)));
        QObject::connect(globalDetuneSpinBox, SIGNAL(valueChanged(int)), globalDetuneSlider, SLOT(setValue(int)));

        QMetaObject::connectSlotsByName(DeicsOnzeGuiBase);
    } // setupUi

    void retranslateUi(QDialog *DeicsOnzeGuiBase)
    {
        DeicsOnzeGuiBase->setWindowTitle(QApplication::translate("DeicsOnzeGuiBase", "DeicsOnze", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deicsOnzeTabWidget->setProperty("toolTip", QVariant(QString()));
#endif // QT_NO_TOOLTIP
        loadPushButton->setText(QApplication::translate("DeicsOnzeGuiBase", "Load", 0, QApplication::UnicodeUTF8));
        savePushButton->setText(QApplication::translate("DeicsOnzeGuiBase", "Save", 0, QApplication::UnicodeUTF8));
        nameGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Preset Name", 0, QApplication::UnicodeUTF8));
        nameLineEdit->setText(QApplication::translate("DeicsOnzeGuiBase", "INIT VOICE", 0, QApplication::UnicodeUTF8));
        subcategoryGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Subcategory", 0, QApplication::UnicodeUTF8));
        subcategoryLineEdit->setText(QApplication::translate("DeicsOnzeGuiBase", "NONE", 0, QApplication::UnicodeUTF8));
        categoryGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Category", 0, QApplication::UnicodeUTF8));
        categoryLineEdit->setText(QApplication::translate("DeicsOnzeGuiBase", "NONE", 0, QApplication::UnicodeUTF8));
        deletePushButton->setText(QApplication::translate("DeicsOnzeGuiBase", "Delete", 0, QApplication::UnicodeUTF8));
        categoryListView->header()->setLabel(0, QApplication::translate("DeicsOnzeGuiBase", "Category", 0, QApplication::UnicodeUTF8));
        subcategoryListView->header()->setLabel(0, QApplication::translate("DeicsOnzeGuiBase", "Subcategory", 0, QApplication::UnicodeUTF8));
        presetsListView->header()->setLabel(0, QApplication::translate("DeicsOnzeGuiBase", "Preset", 0, QApplication::UnicodeUTF8));
        newPushButton->setText(QApplication::translate("DeicsOnzeGuiBase", "New", 0, QApplication::UnicodeUTF8));
        bankGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Bank", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        bankSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Bank numerous", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        progGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Prog", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        progSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Program numerous", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        presentTextLAbel->setText(QApplication::translate("DeicsOnzeGuiBase", "DeicsOnze v0.2.2 Copyright (c) 2004 Nil Geisweiller under GPL licence", 0, QApplication::UnicodeUTF8));
        deicsOnzeTabWidget->setTabText(deicsOnzeTabWidget->indexOf(TabPage), QApplication::translate("DeicsOnzeGuiBase", "&Presets", 0, QApplication::UnicodeUTF8));
        masterVolGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Master Volume", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        masterVolSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Master Volume", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        MasterVolumeSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Master volume", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        FeedbackGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "feedback", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        feedbackSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        feedbackSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Feedback of Op 4", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        functionGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Function", 0, QApplication::UnicodeUTF8));
        polyMonoComboBox->clear();
        polyMonoComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "POLY", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "MONO", 0, QApplication::UnicodeUTF8)
        );
        PitchBendRangeLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "PBR", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        PitchBendRangeSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Pitch Bend Range", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        pitchBendRangeSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        LFOGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "LFO", 0, QApplication::UnicodeUTF8));
        PModSensLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "PMS", 0, QApplication::UnicodeUTF8));
        PModDepthLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "PMD", 0, QApplication::UnicodeUTF8));
        AModDepthLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "AMD", 0, QApplication::UnicodeUTF8));
        LFOSpeedLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "Speed", 0, QApplication::UnicodeUTF8));
        LFODelayLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "Delay", 0, QApplication::UnicodeUTF8));
        AModSensLabel->setText(QApplication::translate("DeicsOnzeGuiBase", "AMS", 0, QApplication::UnicodeUTF8));
        LFOWaveComboBox->clear();
        LFOWaveComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "Saw Up", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Square", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Triangl", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "S/Hold", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        LFOWaveComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "LFO Waveform", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        LFOSyncCheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "LFO Sync", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AModSensSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Amplitude Modulation Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        PModSensSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Pitch Modulation Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        PMSSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AMSSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        PModDepthSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Pitch Modulation Depth", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AModDepthSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Pitch Modulation Depth", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        LFOSpeedSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "LFO Speed", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        LFOSpeedSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        LFODelaySlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "LFO Delay", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        LFODelaySpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        PModDepthSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AModDepthSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        transposeGroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Transpose and Global Detune", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        transposeSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Transpose", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        transposeSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        globalDetuneSlider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Global Detune", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        globalDetuneSpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        algorithmComboBox->clear();
        algorithmComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Algorithm 8", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        algorithmComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Modulation Matrix", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        algorithmComboBox->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "<b>Algorithm 1</b> : <i>Op 1</i> modulated by <i>Op 2</i> modulated by <i>Op 3</i> modulated by <i>Op 4</i><br>\n"
"<b>Algorithm 2</b> : <i>Op 1</i> modulated by <i>Op 2</i> modulated by both <i>Op 3</i> and <i>Op 4</i><br>\n"
"<b>Algorithm 3</b> : <i>Op 1</i> modulated by both <i>Op 4</i> and <i>Op 2</i> modulated by <i>Op 3</i><br>\n"
"<b>Algorithm 4</b> : <i>Op 1</i> modulated by both <i>Op 2</i> and <i>Op 3</i> modulated by <i>Op 4</i><br>\n"
"<b>Algorithm 5</b> : (<i>Op 1</i> modulated by <i>Op 2</i>) add to (<i>Op 3</i> modulated by <i>Op 4</i>) <br>\n"
"<b>Algorithm 6</b> : addition of the three <i>Op 1, 2, 3</i> all modulated by <i>Op 4</i><br>\n"
"<b>Algorithm 7</b> : addition of the three <i>Op 1, 2, 3</i> with <i>Op 3</i> modulated by <i>Op 4</i><br>\n"
"<b>Algorithm 8</b> : addition of the four <i>Op 1, 2, 3, 4</i>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        deicsOnzeTabWidget->setTabText(deicsOnzeTabWidget->indexOf(TabPage1), QApplication::translate("DeicsOnzeGuiBase", "&Global", 0, QApplication::UnicodeUTF8));
        Frequency1groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Frequency 1", 0, QApplication::UnicodeUTF8));
        Fix1CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "FIX", 0, QApplication::UnicodeUTF8));
        CoarseRatio1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Coarse 1", 0, QApplication::UnicodeUTF8));
        FineRatio1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Fine 1", 0, QApplication::UnicodeUTF8));
        Freq1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Freq 1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        CoarseRatio1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        FineRatio1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fine Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        Freq1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fixed Frequency", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Env1GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Envelope 1", 0, QApplication::UnicodeUTF8));
        RR1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RR1", 0, QApplication::UnicodeUTF8));
        D1R1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1R1", 0, QApplication::UnicodeUTF8));
        D1L1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1L1", 0, QApplication::UnicodeUTF8));
        D2R1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D2R1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        D1L1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Level", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "2\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1R1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Release Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        AR1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "AR1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        D1R1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AR1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        AR1Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        AR1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Scaling1GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Scaling 1", 0, QApplication::UnicodeUTF8));
        LS1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "LS1", 0, QApplication::UnicodeUTF8));
        RS1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RS1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        LS1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Level Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        LS1Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        RS1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Rate Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        RS1Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        LS1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RS1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Vol1groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Vol 1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        Vol1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Volume", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        Vol1Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        Vol1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        sensitivity1groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Sensitivity 1", 0, QApplication::UnicodeUTF8));
        EGS1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "EBS1", 0, QApplication::UnicodeUTF8));
        KVS1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "KVS1", 0, QApplication::UnicodeUTF8));
        AME1CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "AME1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AME1CheckBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Amplitude Modulation Enable", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Key Velocity Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Bias Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        DetWaveEGS1GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Detune Wave EGShift 1", 0, QApplication::UnicodeUTF8));
        WaveForm1ComboBox->clear();
        WaveForm1ComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "Wave1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave8", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        WaveForm1ComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave Form", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        WaveForm1ComboBox->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave form 1 = <i>sin(<b>t</b>)</i><br>\n"
"Wave form 2 = <i>sin(<b>t</b>)*abs(sin(<b>t</b>))</i><br>\n"
"Wave form 3 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>) else 0</i><br>\n"
"Wave form 4 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>)*abs(sin(<b>t</b>)) else 0</i><br>\n"
"Wave form 5 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>) else 0</i><br>\n"
"Wave form 6 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 7 = <i>if <b>t</b>&#060 pi then abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 8 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*sin(2*<b>t</b>) else 0</i>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        DET1Label->setText(QApplication::translate("DeicsOnzeGuiBase", "DET1", 0, QApplication::UnicodeUTF8));
        EGQ1ComboBox->clear();
        EGQ1ComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "96dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "48dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "24dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "12dB", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        EGQ1ComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Shift", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        DET1Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Detune", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        DET1Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        DET1SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        deicsOnzeTabWidget->setTabText(deicsOnzeTabWidget->indexOf(tab), QApplication::translate("DeicsOnzeGuiBase", "Op &1", 0, QApplication::UnicodeUTF8));
        Frequency2groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Frequency 2", 0, QApplication::UnicodeUTF8));
        Fix2CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "FIX", 0, QApplication::UnicodeUTF8));
        Freq2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Freq 2", 0, QApplication::UnicodeUTF8));
        FineRatio2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Fine 2", 0, QApplication::UnicodeUTF8));
        CoarseRatio2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Coarse 2", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        CoarseRatio2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        FineRatio2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fine Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        Freq2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fixed Frequency", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Env2GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Envelope 2", 0, QApplication::UnicodeUTF8));
        AR2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "AR2", 0, QApplication::UnicodeUTF8));
        RR2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RR2", 0, QApplication::UnicodeUTF8));
        D2R2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D2R2", 0, QApplication::UnicodeUTF8));
        D1L2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1L2", 0, QApplication::UnicodeUTF8));
        D1R2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1R2", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AR2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        AR2Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        D1R2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Level", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "2\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Release Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1R2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AR2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Scaling2GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Scaling 2", 0, QApplication::UnicodeUTF8));
        LS2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "LS2", 0, QApplication::UnicodeUTF8));
        RS2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RS2", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        LS2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Level Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        LS2Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        RS2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Rate Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        RS2Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        LS2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RS2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Vol2groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Vol 2", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        Vol2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Volume", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        Vol2Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        Vol2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        sensitivity2groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Sensitivity 2", 0, QApplication::UnicodeUTF8));
        EGS2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "EBS2", 0, QApplication::UnicodeUTF8));
        KVS2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "KVS2", 0, QApplication::UnicodeUTF8));
        AME2CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "AME2", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AME2CheckBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Amplitude Modulation Enable", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Key Velocity Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Bias Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        DetWaveEGS2GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Detune Wave EGShift 2", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        DET2Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Detune", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        DET2Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        DET2Label->setText(QApplication::translate("DeicsOnzeGuiBase", "DET2", 0, QApplication::UnicodeUTF8));
        WaveForm2ComboBox->clear();
        WaveForm2ComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "Wave1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave8", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        WaveForm2ComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave Form", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        WaveForm2ComboBox->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave form 1 = <i>sin(<b>t</b>)</i><br>\n"
"Wave form 2 = <i>sin(<b>t</b>)*abs(sin(<b>t</b>))</i><br>\n"
"Wave form 3 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>) else 0</i><br>\n"
"Wave form 4 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>)*abs(sin(<b>t</b>)) else 0</i><br>\n"
"Wave form 5 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>) else 0</i><br>\n"
"Wave form 6 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 7 = <i>if <b>t</b>&#060 pi then abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 8 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*sin(2*<b>t</b>) else 0</i>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        EGS2comboBox->clear();
        EGS2comboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "96dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "48dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "24dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "12dB", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        EGS2comboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Shift", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        DET2SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        deicsOnzeTabWidget->setTabText(deicsOnzeTabWidget->indexOf(tab1), QApplication::translate("DeicsOnzeGuiBase", "Op &2", 0, QApplication::UnicodeUTF8));
        Frequency3groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Frequency 3", 0, QApplication::UnicodeUTF8));
        Fix3CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "FIX", 0, QApplication::UnicodeUTF8));
        CoarseRatio3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Coarse 3", 0, QApplication::UnicodeUTF8));
        FineRatio3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Fine 3", 0, QApplication::UnicodeUTF8));
        Freq3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Freq 3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        CoarseRatio3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        FineRatio3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fine Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        Freq3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fixed Frequency", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Env3GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Envelope 3", 0, QApplication::UnicodeUTF8));
        RR3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RR3", 0, QApplication::UnicodeUTF8));
        D2R3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D2R3", 0, QApplication::UnicodeUTF8));
        D1L3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1L3", 0, QApplication::UnicodeUTF8));
        D1R3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1R3", 0, QApplication::UnicodeUTF8));
        AR3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "AR3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AR3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        AR3Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        D1R3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Level", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "2\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Release Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1R3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AR3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Scaling3GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Scaling 3", 0, QApplication::UnicodeUTF8));
        LS3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "LS3", 0, QApplication::UnicodeUTF8));
        RS3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RS3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        LS3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Level Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        LS3Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        RS3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Rate Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        RS3Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        LS3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RS3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Vol3groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Vol 3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        Vol3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Volume", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        Vol3Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        Vol3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        sensitivity3groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Sensitivity 3", 0, QApplication::UnicodeUTF8));
        EGS3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "EBS3", 0, QApplication::UnicodeUTF8));
        KVS3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "KVS3", 0, QApplication::UnicodeUTF8));
        AME3CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "AME3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AME3CheckBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Amplitude Modulation Enable", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Key Velocity Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Bias Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        DetWaveEGS3GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Detune Wave EGShift 3", 0, QApplication::UnicodeUTF8));
        WaveForm3ComboBox->clear();
        WaveForm3ComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "Wave1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave8", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        WaveForm3ComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave Form", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        WaveForm3ComboBox->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave form 1 = <i>sin(<b>t</b>)</i><br>\n"
"Wave form 2 = <i>sin(<b>t</b>)*abs(sin(<b>t</b>))</i><br>\n"
"Wave form 3 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>) else 0</i><br>\n"
"Wave form 4 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>)*abs(sin(<b>t</b>)) else 0</i><br>\n"
"Wave form 5 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>) else 0</i><br>\n"
"Wave form 6 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 7 = <i>if <b>t</b>&#060 pi then abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 8 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*sin(2*<b>t</b>) else 0</i>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        EGS3comboBox->clear();
        EGS3comboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "96dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "48dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "24dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "12dB", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        EGS3comboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Shift", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        DET3Label->setText(QApplication::translate("DeicsOnzeGuiBase", "DET3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        DET3Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Detune", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        DET3Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        DET3SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        deicsOnzeTabWidget->setTabText(deicsOnzeTabWidget->indexOf(TabPage2), QApplication::translate("DeicsOnzeGuiBase", "Op &3", 0, QApplication::UnicodeUTF8));
        Frequency4groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Frequency 4", 0, QApplication::UnicodeUTF8));
        CoarseRatio4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Coarse 4", 0, QApplication::UnicodeUTF8));
        FineRatio4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Fine 4", 0, QApplication::UnicodeUTF8));
        Freq4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "Freq 4", 0, QApplication::UnicodeUTF8));
        Fix4CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "FIX", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        FineRatio4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fine Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        Freq4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Fixed Frequency", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        CoarseRatio4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Scaling4GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Scaling 4", 0, QApplication::UnicodeUTF8));
        LS4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "LS4", 0, QApplication::UnicodeUTF8));
        RS4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RS4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        LS4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Level Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        LS4Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        RS4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Rate Scaling", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        RS4Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        RS4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        LS4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Env4GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Envelope 4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AR4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        AR4Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
        AR4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "AR4", 0, QApplication::UnicodeUTF8));
        RR4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "RR4", 0, QApplication::UnicodeUTF8));
        D2R4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D2R4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        D2R4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "2\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Level", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        D1L4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1L4", 0, QApplication::UnicodeUTF8));
        D1R4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "D1R4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        D1R4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "1\302\260 Decay Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Release Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1R4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D1L4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        D2R4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        RR4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        AR4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        Vol4groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Vol 4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        Vol4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Volume", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        Vol4Slider->setProperty("whatsThis", QVariant(QString()));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        Vol4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        sensitivity4groupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Sensitivity 4", 0, QApplication::UnicodeUTF8));
        EGS4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "EBS4", 0, QApplication::UnicodeUTF8));
        KVS4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "KVS4", 0, QApplication::UnicodeUTF8));
        AME4CheckBox->setText(QApplication::translate("DeicsOnzeGuiBase", "AME4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        AME4CheckBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Amplitude Modulation Enable", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Key Velocity Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Bias Sensitivity", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        KVS4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        EBS4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        DetWaveEGS4GroupBox->setTitle(QApplication::translate("DeicsOnzeGuiBase", "Detune Wave EGShift 4", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        DET4Slider->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Detune", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        DET4Slider->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Attack Rate of the operator 1", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        DET4Label->setText(QApplication::translate("DeicsOnzeGuiBase", "DET4", 0, QApplication::UnicodeUTF8));
        WaveForm4ComboBox->clear();
        WaveForm4ComboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "Wave1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "Wave8", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        WaveForm4ComboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave Form", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        WaveForm4ComboBox->setProperty("whatsThis", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Wave form 1 = <i>sin(<b>t</b>)</i><br>\n"
"Wave form 2 = <i>sin(<b>t</b>)*abs(sin(<b>t</b>))</i><br>\n"
"Wave form 3 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>) else 0</i><br>\n"
"Wave form 4 = <i>if <b>t</b>&#060 pi then sin(<b>t</b>)*abs(sin(<b>t</b>)) else 0</i><br>\n"
"Wave form 5 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>) else 0</i><br>\n"
"Wave form 6 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 7 = <i>if <b>t</b>&#060 pi then abs(sin(2*<b>t</b>)) else 0</i><br>\n"
"Wave form 8 = <i>if <b>t</b>&#060 pi then sin(2*<b>t</b>)*sin(2*<b>t</b>) else 0</i>", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_WHATSTHIS
        EGS4comboBox->clear();
        EGS4comboBox->insertItems(0, QStringList()
         << QApplication::translate("DeicsOnzeGuiBase", "96dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "48dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "24dB", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("DeicsOnzeGuiBase", "12dB", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        EGS4comboBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "EG Shift", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        DET4SpinBox->setProperty("toolTip", QVariant(QApplication::translate("DeicsOnzeGuiBase", "Coarse Ratio", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        deicsOnzeTabWidget->setTabText(deicsOnzeTabWidget->indexOf(TabPage3), QApplication::translate("DeicsOnzeGuiBase", "Op &4", 0, QApplication::UnicodeUTF8));
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"479 360 270 2",
"cl c #6a3039",
"ch c #733039",
"cb c #733439",
"bY c #7b3439",
"ca c #7b3441",
"bV c #7b3839",
"b0 c #7b3841",
"bS c #833839",
"bO c #833841",
"bX c #833c41",
"bK c #8b3841",
"bF c #8b3c41",
"bL c #8b3c4a",
"bZ c #8b4041",
"b5 c #8b404a",
"bx c #943c41",
"bG c #943c4a",
"bv c #944041",
"bA c #94404a",
"b4 c #94444a",
"bh c #9c4041",
"be c #9c404a",
"bj c #9c444a",
"b8 c #9c484a",
"cd c #9c4852",
"ci c #9c5d5a",
"ce c #9c5d62",
"cj c #9c615a",
"cf c #9c6162",
"#a c #a43841",
"b. c #a4404a",
"a5 c #a4444a",
"bd c #a44452",
"bq c #a4484a",
"bo c #a44852",
"c# c #a44c4a",
"cc c #a44c52",
"bP c #a46162",
"b1 c #a46562",
".2 c #ac3841",
".1 c #ac3c41",
"aV c #ac444a",
"a9 c #ac4452",
"aR c #ac484a",
"a1 c #ac4852",
"b7 c #ac4c4a",
"bs c #ac4c52",
"c. c #ac5052",
"bN c #ac6162",
"bC c #ac6562",
"bE c #ac656a",
".Y c #b43c41",
".0 c #b43c4a",
"#c c #b44041",
".Z c #b4404a",
"aD c #b4484a",
"aF c #b44852",
"aO c #b44c52",
"bz c #b44c5a",
"br c #b45052",
"bB c #b4505a",
"b2 c #b45552",
"b3 c #b4555a",
"bQ c #b46562",
"bt c #b4656a",
"cg c #b46962",
"bb c #b4696a",
"## c #bd4041",
".X c #bd404a",
".W c #bd444a",
"ay c #bd484a",
"ax c #bd4852",
"as c #bd4c52",
"aP c #bd4c5a",
"aQ c #bd5052",
"aX c #bd505a",
"bu c #bd5552",
"bw c #bd555a",
"bW c #bd595a",
"a8 c #bd696a",
"bk c #bd6973",
"a0 c #bd6d6a",
"a2 c #bd6d73",
".U c #c5444a",
".V c #c54452",
"#s c #c5484a",
"#v c #c54852",
"ad c #c54c52",
"av c #c54c5a",
"ak c #c55052",
"aq c #c5505a",
"aN c #c5555a",
"bn c #c5595a",
"by c #c55962",
"bR c #c55d5a",
"bU c #c55d62",
".3 c #c5656a",
".# c #c5696a",
"a3 c #c56d6a",
"aJ c #c56d73",
"aE c #c57173",
"#. c #cd444a",
"#n c #cd4452",
".T c #cd484a",
".S c #cd4852",
"#A c #cd4c52",
"#3 c #cd5052",
"a. c #cd505a",
"ah c #cd555a",
"aL c #cd595a",
"aZ c #cd5962",
"bm c #cd5d5a",
"bg c #cd5d62",
"bJ c #cd6162",
"b9 c #cd6562",
".a c #cd696a",
"Qt c #cd6973",
"#g c #cd6d6a",
".b c #cd6d73",
"b6 c #cd716a",
"at c #cd7173",
"az c #cd717b",
"an c #cd7573",
"au c #cd757b",
".9 c #d5484a",
".R c #d54852",
".Q c #d54c52",
"#G c #d54c5a",
"#E c #d55052",
"#U c #d5505a",
"#1 c #d5555a",
"ag c #d5595a",
"ap c #d55962",
"aK c #d55d5a",
"aI c #d55d62",
"ba c #d56162",
"bl c #d5616a",
"bD c #d56562",
"bH c #d5656a",
"bT c #d5696a",
".4 c #d56d6a",
".c c #d56d73",
"ck c #d5716a",
"#i c #d57173",
"#r c #d5717b",
"al c #d57573",
"aa c #d5757b",
"#9 c #d5797b",
".P c #de4c52",
"#f c #de4c5a",
".N c #de5052",
".O c #de505a",
"#J c #de555a",
"#Y c #de595a",
"#5 c #de5962",
"ai c #de5d5a",
"aj c #de5d62",
"aH c #de6162",
"aW c #de616a",
"a7 c #de6562",
"a6 c #de656a",
"bp c #de696a",
"bM c #de6973",
"bI c #de6d6a",
".d c #de6d73",
".e c #de7173",
".f c #de717b",
"#o c #de7573",
"#l c #de757b",
"#C c #de797b",
"#6 c #de7983",
"#7 c #de7d7b",
"#8 c #de7d83",
".8 c #e65052",
".M c #e6505a",
".L c #e6555a",
"#M c #e6595a",
"#R c #e65962",
"#0 c #e65d5a",
"#Z c #e65d62",
"ae c #e66162",
"ao c #e6616a",
"aC c #e66562",
"aA c #e6656a",
"aY c #e6696a",
"b# c #e66973",
"bc c #e66d6a",
"bf c #e66d73",
".i c #e67173",
".g c #e6717b",
".j c #e67573",
".h c #e6757b",
"#p c #e6797b",
"#B c #e67983",
"#H c #e67d7b",
"#F c #e67d83",
".J c #ee555a",
".I c #ee595a",
".K c #ee5962",
"#N c #ee5d5a",
"#P c #ee5d62",
"#X c #ee6162",
"#4 c #ee616a",
"ab c #ee6562",
"ac c #ee656a",
"aw c #ee696a",
"aG c #ee6973",
"aM c #ee6d6a",
"aS c #ee6d73",
"a4 c #ee7173",
"bi c #ee717b",
".5 c #ee7573",
".k c #ee757b",
".l c #ee797b",
"#m c #ee7983",
"#u c #ee7d7b",
"#t c #ee7d83",
"#K c #ee8183",
"#2 c #ee818b",
".H c #f6595a",
".G c #f65962",
"#d c #f65d5a",
".F c #f65d62",
"#L c #f66162",
"#T c #f6616a",
"#V c #f66562",
"#W c #f6656a",
"a# c #f6696a",
"af c #f66973",
"am c #f66d6a",
"ar c #f66d73",
"aB c #f67173",
"aU c #f6717b",
"aT c #f67573",
".6 c #f6757b",
".m c #f6797b",
".n c #f67983",
"#h c #f67d7b",
"#e c #f67d83",
"#x c #f68183",
"#I c #f6818b",
"#Q c #f68583",
"#O c #f6858b",
".E c #ff5d62",
".C c #ff6162",
".D c #ff616a",
".7 c #ff6562",
".B c #ff656a",
".A c #ff696a",
".z c #ff6973",
".y c #ff6d6a",
".x c #ff6d73",
".v c #ff7173",
".w c #ff717b",
".u c #ff7573",
".t c #ff757b",
".q c #ff797b",
".o c #ff7983",
".r c #ff7d7b",
".p c #ff7d83",
".s c #ff8183",
"#b c #ff818b",
"#k c #ff8583",
"#j c #ff858b",
"#q c #ff898b",
"#w c #ff8994",
"#z c #ff8d8b",
"#y c #ff8d94",
"#D c #ff9194",
"#S c #ff919c",
"Qt.#Qt.aQt.a.b.a.c.a.c.a.c.c.c.c.c.c.c.c.d.c.e.c.e.d.e.d.f.e.e.e.g.e.g.e.g.e.h.e.h.i.h.i.h.i.h.j.k.h.k.h.k.k.k.k.k.k.l.k.l.k.m.k.m.k.m.k.m.m.m.m.n.m.m.m.n.m.n.m.o.m.p.m.p.m.p.m.p.q.p.q.p.q.p.q.p.r.p.r.p.p.p.r.p.p.p.r.p.p.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p"
".q.o.q.o.q.o.q.q.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.D.E.D.E.C.F.C.F.E.F.E.F.F.F.F.G.F.H.F.H.G.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.L.N.M.N.M.N.O.P.O.P.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.R.T.S.T.S.U.S.U.V.U.U.W.U.X.W.X.X.X.X.Y.Z.Y.0.1.0.2.1",
".3.a.#.a.aQt.aQt.a.b.a.c.a.c.a.c.4.c.4.c.c.d.c.d.c.e.c.e.d.e.d.e.e.e.e.e.e.g.e.i.e.h.e.h.i.h.i.h.i.k.i.k.j.k.j.k.k.k.5.k.k.l.k.l.k.m.k.m.k.m.k.m.6.m.m.m.m.m.m.m.m.o.m.q.m.o.m.r.m.p.m.p.q.p.q.p.q.p.q.p.q.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.p.p.r.p.p.s.r.s.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q"
".q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.F.F.F.H.F.H.F.H.F.H.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.L.L.M.L.8.L.8.M.N.M.N.M.P.N.P.O.Q.N.Q.P.Q.Q.Q.Q.9.Q.S.R.T.S.T.S#..S.U.S.U.U.U.U.X.U.X.U##.X##.X.Y.Z.Y.0.1.Y.2.1#a",
"Qt.#Qt.a.b.a.c.a.c.a.cQt.c.c.c.c.c.c.d.c.e.c.e.c.e.d.f.e.e.e.g.e.g.e.g.e.h.e.h.i.h.i.h.i.h.j.k.h.k.h.k.k.k.k.l.k.l.k.l.k.l.k.m.k.m.l.n.m.m.m.n.m.n.m.n.m.n.m.p.m.p.m.p.m.p.m.p.q.p.q.p.q.p.r.p.p.p.r.p.p.p.r.p.p.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p"
".q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.I.G.J.K.J.K.J.J.L.J.L.J.L.J.M.L.M.L.M.M.N.M.N.O.N.O.P.O.Q.O.Q.P.Q.Q.Q.Q.R.Q.S.R.T.S.T.S.U.S.U.S.U.V.U.U.X.U.X.W##.X#c.X.Y.Z.Y.0.1.1",
".#.a.#Qt.aQt.a.b.a.b.a.c.a.c.4.c.4.c.c.c.c.c.c.e.c.e.d.e.d.e.e.e.e.e.e.g.e.i.e.h.e.h.i.h.i.h.i.k.j.k.j.k.j.k.k.k.5.k.k.l.k.l.k.m.k.m.k.m.k.m.m.m.l.m.m.m.m.m.m.o.m.q.m.p.m.p.m.p.m.p.q.p.q.p.q.p.q.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.p.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q"
".o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.8.L.M.L.8.L.N.M.N.M.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T.S.U.T.U.S.U.U.U.U.X.U.X.X##.X#c.X.Y.Z.Y.Y.1.1.2",
"Qt.aQt.a.b.a.c.a.cQt.c.b.c.c.c.c.d.c.e.c.e.c.e.c.f.e.e.e.f.e.f.e.g.e.h.e.h.i.h.i.h.i.h.j.k.h.k.h.k.h.k.h.l.k.l.k.l.k.l.k.m.k.m.l.n.l.m.l.n.m.n.m.n.m#e.m.p.m.p.m.p.m.p.m.p.q.p.q.p.r.p.r.p.p.p.r.p.p.p.p.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p"
".q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.I.G.J.I.J.K.J.J.L.J.L.J.L.L.M.L.8.L.M.M.N.M.N.O.P.O.P.O.Q#f.Q.P.Q.Q.R.Q.S.Q.S.R.T.S.T.S.U.S.U.U.U.V.U.U.X.U.X.X#c.X.Y.Z.Y.Z.1.Y",
".#Qt.#Qt.a.b.a.b.a.c.a.c#g.c.4.c.c.c.c.c.c.e.c.e.c.e.c.e.e.e.e.e.e.g.e.i.e.h.e.h.i.h.i.h.i.h.j.k.j.k.j.k.h.k.j.k.k.l.k.l.k.m.k.m.k.m.k.m.l.m.l.m.m.m.m.m.m.o.m#h.m.p.m.p.m.p.m.p.q.p.q.p.q.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.p.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q"
".r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.J.J.L.J.L.J.L.J.8.L.M.L.N.M.N.M.N.M.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S#..S.U.T.U#..U.U.U.U.X.W.X.X#c.X.Y.X.Y#c.Y.Y.1",
"Qt.a.b.a.b.a.cQt.c#g.c.c.c.c.c.c#i.c.e.c.e.c.f.e.e.e.f.e.f.e.g.e.h.e.h.i.h.i.h.g.h.j.k.h.k.h.k.h.k.h.l.k.l.k.l.k.l.k.m.k.m.l.n.l.m.l.n.m.n.m.n.m#e.m.p.m.p.m.p.m.p.m.p.q.p.r.p.p.p.r.p.p.p.r.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#b.s#b.s#b.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.q.p.q.p"
".q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.G.H.G.I.G.I.K.J.K.J.I.J.K.J.J.L.J.L.L.M.L.M.L.8.L.N.M.N.M.N.O.P.O.P.N.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S#..S.U.S.U.U.U.U.X.U.X.W.X.X#c.X.Y.Z.Y.0",
".#Qt.aQt.a.b.a.c.a.c.a.c#g.c.c.c.c.c.c.e.c.e.c.e.c.e.e.e.e.e.e.g.e.i.e.h.e.h.i.h.i.h.i.h.j.h.j.k.j.k.h.k.j.l.k.l.k.l.k.m.k.m.k.m.l.m.l.m.l.m.m.n.m.m.m.n.m#h.m.p.m.p.m.p.m.p.q.p.q.p.r.p.r.p.r.p.r.p.r.p.r.p.p.p.r.p.p.s.r.s.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q"
".p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.H.J.G.J.I.J.I.L.J.L.J.L.J.L.L.8.L.8.L.N.M.N.M.N.O.P.O.P.N.P.P.Q.P.Q.Q.R.Q.R.Q.T.R.T.R.T.S#..S.U.T.U.U.U.U.X.U##.W##.X#c.X.Y.Z.Y.Y.1",
".b.a.b.a.cQt.c#g.c.c.c.c.c.c#i.c.e.c.e.c.f.e.e.e.f.e.f.e.g.e.h.e.h.i.h.i.h.g.h.j.k.h.k.h.k.h.k.h.l.k.l.k.l.k.l.k.m.l.m.l.n.l.m.l.n.m.n.m.n.m#e.m.p.m.p.m.p.m.p.m.p.r.p.r.p.p.p.r.p.p.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#b.s#b.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p"
".q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.G.F.I.G.I.G.I.G.I.K.J.K.J.I.J.J.L.J.L.J.L.L.M.L.M.L.N.M.N.M.N.M.P.O.P.O.P.P.Q.P.Q.Q.R.Q.R.Q.T.R.S.S.T.S#..S.U.S.U.U.U.U.X.U.X.X.X.X#c.X.Y.Z",
".aQt.a.b.a.c.a.c.a.c#g.c.c.c.4.c.c.e.c.e.c.e.c.e.e.e.e.e.e.g.e.i.e.h.e.h.i.h.i.h.i.h.j.h.j.k.j.k.h.k.j.l.k.l.k.l.k.l.k.l.k.m.l.m.l.m.l.m.m.n.m.m.m#e.m#e.m.p.m.p.m.p.m.p.q.p.q.p.r.p.r.p.r.p.r.p.r.p.r.p.p.p.r.s.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q"
".p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.I.I.J.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.8.L.N.M.N.M.N.O.P.O.P.N.Q.P.Q.Q.Q.Q.R.Q.R.Q.T.R.T.S.T.S#..S.U.U.U.U.U.U.X.U##.X##.X#c.X.Y.Z.1",
".b.a.cQt.c#g.c.c.c.c.c.c#i.c.e.c.e.c.f.e.e.e.f.e.f.e.g.e.h.e.h.e.h.e.h.g.h.j.h.h.k.h.k.h.k.h.l.k.l.k.l.k.l.k.m.l.m.l.n.l.m.l.n.m.n.m#e.m#e.m.p.m.p.m.p.m.p.m.p.r.p.r.p.p.p.r.p.p.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p"
".r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.G.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.L.N.M.N.M.N.O.P.O.P.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.R.T.S.T.S.U.S.U.V.U.U.W.U.X.W.X.X.Z.X.Y.Z",
".a.b.a.c.a.c.a.c#g.c.c.c.4.c.c.e.c.e.c.e.c.e.e.e.e.e.e.f.e.e.e.h.e.h.e.h.i.h.i.h.j.h.j.k.j.k.h.k.j.l.k.l.k.l.k.l.k.l.k.m.l.m.l.m.l.m.m.n.m.m.m#e.m#e.m.p.m.p.m.p.m.p.q.p.r.p.r.p.r.p.r.p.r.p.p.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q"
".p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.I.I.J.I.J.I.J.J.L.J.L.J.L.L.M.L.8.L.8.M.N.M.N.M.P.N.P.O.Q.N.Q.P.Q.Q.Q.Q.9.Q.S.R.T.S.T.S#..S.U.S.U.U.U.U.X.U.X.U##.X##.X.Y.Z.Y",
".cQt.c#g.c.b.c.c.c.c#i.c.e.c.e.c.f#i.e.e.f.e.f.e.g.e.h.e.h.e.h.e.h.g.h.j.h.h.h.h.k.h.k.h.l.k.l.k.l.k.l.k.m.l.m.l.n.l.m.l.n.m.n.m#e.m#e.m.p.m.p.m.p.m.p#h.p.p.p.r.p.p.p.r.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p"
".r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.F.E.F.E.F.E.F.E.G.F.H.F.H.G.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.M.N.M.N.O.N.O.P.O.Q.O.Q.P.Q.Q.Q.Q.S.Q.S.R.T.S.T.S.U.S.U.S.U.V.U.U.X.U.X.X##.X#c.X",
".a.b.a.b.a.c#g.c.c.c.4.c.c#i.c#i.c.e.c.e.e.e.e.e.e.f.e.e.e.h.e.h.e.h.e.h.i.h.j.h.j.k.j.k.h.k.j.l.k.l.k.l.k.l.k.l.l.m.l.m.l.m.l.m.m.n.m.m.m#e.m#e.m.p.m.p.m.p.m.p.r.p#h.p.r.p.r.p.r.p.r.p.p.p.r.s.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.q"
".p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.8.L.M.L.8.L.8.M.N.M.N.M.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T.S.U.T.U#..U.U.U.U.X.U.X.X##.X#c.X.Y",
".c#g.c.b.c.b.c.c#i.c.e.c.e.c.f.c.e#i.f.e.f.e.f.e.h.e.h.e.h.e.h.g.h.j.h.h.h.h.k.h.k.h.l.k.l.k.l.k.l.k.l.l.l.l.n.l.n.l.n.m.n.m#e.m#e.m#e.m#e.m.p.m.p#h.p.p.p.r.p.p.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p"
".p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.I.G.J.I.J.K.J.J.L.J.L.J.L.L.M.L.8.L.M.M.N.M.N.O.P.O.P.N.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S.T.S.U.S.U.U.U.U.U.U.X.U.X.X#c.X",
".a.b.a.c#g.c.b.c.4.c.c#i.c#i.c.e.c.e.c.e#i.e.e.f.e.e.e.h.e.h.e.h.e.h.i.h.j.h.j.k.j.k.h.k.h.l.k.l.k.l.k.l.k.l.l.m.l.m.l.m.l.m.m.n.m#h.m#e.m#e.m.p.m.p.m.p.m.p#h.p#h.p.r.p.r.p.r.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#b.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r"
".p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.8.L.M.L.N.M.N.M.N.O.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S#..S.U.T.U.U.U.U.U.U.X.W.X.X#c.X.Y",
".c.b.c.b.c.c.c.c#i.c#i.c.f.c.e#i.f.e.f.e.f.e#l.e.h.e.h.e.h.g.h.j.h.h.h.h.k.h.k.h.l.h.l.h.l.k.l.k#m.l.l.l.n.l.n.l.n.m#e.m#e.m#e.m#e.m#e.m.p.m.p#h.p#e.p#h.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p"
".r.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.J.K.J.I.J.K.J.J.L.J.L.L.M.L.M.L.N.L.N.M.N.M.N.O.P.O.P.N.Q.P.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S#..S.U.S.U.U.U.U.X.U.X.W.X.X",
".a.c#g.c.b.c#g.c.c#i.c#i.c.e.c.e.c.e#i.e.e.f.e.e.e.h.e.h.e.h.e.h.i.h.j.h.j.h.j.h.h.k.h.l.h.l.h.l.k.l.k.l.l.m.l.m.l.m.l.m.m.n.m#h.m#e.m#e.m.p.m.p.m.p#h.p#h.p#h.p.r.p.r.p.p.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r"
".p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.L.J.L.J.L.J.L.L.8.L.8.L.N.M.N.M.N.O.P.O.P.N.P.P.Q.P.Q.Q.R.Q.R.Q.T.R.T.S.T.S#..S.U.T.U.U.U.U.X.U##.X##.X#c",
".c.b.c.c.c.c#i.c#i.c.e.c.e#i.f.e.f.e.f.e#l.e.h.e.h.e.h.g.h.j.h.h.h.h.k.h.k.h.l.h.l.h.l.k.l.k#m.l.l.l.n.l.n.l.n.l#e.l#e.m#e.m#e.m#e.m.p#h.p#h.p#e.p#h.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p"
".p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.G.F.H.F.G.F.H.G.I.G.I.G.I.K.J.K.J.I.J.J.L.J.L.J.L.L.M.L.M.L.N.M.N.M.N.M.P.O.P.O.Q.P.Q.P.Q.Q.R.Q.R.Q.T.R.T.S.T.S.U.S.U#n.U.U.W.U.X.W.X.X",
".a.c.b.c#g.c.c.c.c#i.c.e.c.e.c.e#i.e.e.f.e.e.e.h.e.h.e.h.e.h.i.h.j.h.j.h.j.h.h.k.h.l.h.l.h.l.k.l.k.l.l.m.l.m.l.m.l.m.l.n.l#h.m#e.m#e.m.p.m.p.m.p#h.p#h.p#h.p.r.p.r.p.p.p.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#j.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r"
".p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E#d.F.H.F.H.F.H.G.I.G.I.G.I.I.J.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.8.M.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.R.Q.S.Q.T.S.T.S#..S.U.S.U.U.U.U.W.U.X.U##.X##",
".c.c.c.c#i.c#i.c.e.c.e#i.f.e.f.e.f.e#l.e.h.e.h.e.h.e.h#o.h.h.h.h.h.h.k.h.l.h.l.h.l.k.l.k#m.l.l.l.n.l.n.l.n.l#e.l#e.m#e.m#e.m#e.m.p#h.p#h.p#e.p#h.p.p.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s"
".p.p.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.G.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.L.N.M.N.M.N.O.P.O.Q.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.R.T.S.U.S.U.S.U.V.U.U.X.U.X.W",
"#g.c#g.c.c.c.c#i.c#i.c.e.c.e#i.e.e.f.e.e.e#l.e#l.e.h.e.h.e.h.j.h.j.h.j.h.h.k.h.l.h.l.h.l.k.l.k.l.l.l.l.l.l.m.l.m.l.n.l#e.m#e.m#e.m#e.m#e.m.p#h.p#h.p#h.p.r.p.r.p.p.p.r.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#j.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r"
".p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.I.I.J.I.J.I.J.J.L.J.L.J.L.L.M.L.8.L.8.M.N.M.N.M.P.N.P.O.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T.S.U.T.U.S.U.U.U.U.X.U.X.X##",
".c.b#i.c#i.c.e.c.e#i.f#i.f#i.f.e#l.e.h.e.h.e.h.e.h#o.h.h.h.h.h.h.h.h.l.h.l.h.l.k.l.k#m.l.l.l.n.l.n.l.n.l#e.l#e.m#e.m#e.m#e.m.p#h.p#h.p#e.p#h.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s#b.s.s.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s"
".p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.H.G.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.8.L.M.M.N.M.N.O.P.O.P.O.Q#f.Q.P.Q.Q.R.Q.S.Q.S.R.T.S.T.S.U.S.U.U.U.V.U.U.X.U",
"#g.c.b.c.c#i.c#i.c#i.c.e#i.e#i.f.e.e.e#l.e#l.e.h.e.h.e.h#o.h.j.h.j.h.h.k.h.l.h.l.h.l.k.l.k.l.l.l.l.l.l.n.l.m.l.n.l#e.m#e.m#e.m#e.m#e.m.p#h.p#h.p#h.p.r.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p"
".p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.8.L.M.L.8.L.N.M.N.M.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S#..S.U.T.U.U.U.U.U.U.X.W.X",
"#i.c#i.c#i.c#i#i.f#i.f#i.f.e.f.e#l.e#l.e.h.e.h#o.h.h.h.h.h.h.h.h.l.h.l.h.l.k.l.k#m.l.l.l#m.l.n.l.n.l#e.l#e.m#e.m#e.m#e.m.p#h.p#h.p#e.p#h.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s"
".p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.B.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.I.G.J.I.J.K.J.J.L.J.L.J.M.L.M.L.8.L.N.M.N.M.N.O.P.O.P.N.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S#..S.U.S.U.U.U.U.X.U",
".b.c.b#i.c#i.c#i.c.e#i.e#i.f#i.e.e.f.e#l.e.h.e.h.e.h#o.h.j.h.j.h.h.k.j.l.h.l.h.l.k.l.k.l.l.l.l.l.l.n.l.m.l#e.l#e.m#e.m#e.m#e.m#e#h.p#h.p#h.p#h.p.r.p.r.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r"
".s.p.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.J.J.L.J.L.J.L.J.8.L.8.L.N.M.N.M.N.O.P.O.P.N.P.P.Q.P.Q.Q.R.Q.R.Q.T.R.T.S.T.S#..S.U.T.U.U.U.U.X.U##",
"#i.c#i.c#i.c.f#i.f#i.f.e.f.e#l.e#l.e.h.e.h#o.h.h.h.h.h.h.h.h.l.h.l.h.l.h.l.h#m.l.l.l#m.l#m.l.n.l#e.l#e.m#e.m#e.m#e#h#e#e#e#h.p#e.p#e.p.p.p.p.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s#b.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s"
".p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.G.H.G.I.G.I.K.J.K.J.I.J.I.L.J.L.J.L.L.M.L.M.L.N.M.N.M.N.M.P.O.P.O.Q.P.Q.P.Q.Q.R.Q.R.Q.T.R.S.S.T.S.U.S.U#n.U.U.W.U",
".b.c.c#i.c#i.c.e.c.e#i.f#i.e.e.f.e#l.e.h.e.h.e.h#o.h.j.h.j.h.h.h.j#p.h.l.h.l.h.l.h.l.l.l.l.l.l.n.l.m.l#e.l#e.m#e.m#e.m#e.m#e#h.p#h.p#h.p#h.p#e.p#h.p.p.s.r.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p"
".s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.H.J.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.8.L.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.R.Q.S.Q.T.S.T.S#..S.U.S.U.U.U.U.W.U.X",
"#i.c#i.c.f#i.e#i.f.e.f.e#l.e#l.e.h.e.h#o.h#l.h.h.h.h.h.h.l.h.l.h.l.h.l.h#m.l.l.l#m.l#m.l.n.l#e.l#e.l#e.l#e.m#e#h#e#e#e#h.p#e.p#e.p#e.s#e.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s#b.s.s.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s"
".p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.B.C.D.C.D.C.D.C.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.G.F.I.G.I.G.I.G.I.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.L.N.M.N.M.N.O.P.O.Q.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.S.T.S.U.S.U.S.U.V.U.U",
".c#i.c#i.c#i.c.e#i.e#i.e.e.f.e#l.e#l.e#l.e.h#o.h.j.h.j.h.h.h.j#p.h.l.h.l.h.l.h.l.l.l.l.l.l#m.l.m.l#e.l#e.l#e.l#e.m#e.m#e#h.p#h.p#h.p#h.p#e.p#h.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p"
".s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.I.I.J.K.J.I.J.J.L.J.L.J.L.L.M.L.8.L.8.M.N.M.N.M.P.N.P.O.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T.S.U.T.U.S.U.U.U.U.X",
"#i.c.f#i.e#i.f#i.f#i#l.e#l.e.h.e.h.e.h#l.h#l.h.h.h.h#p.h#p.h.l.h.l.h.l.l.l.l#m.l#m.l.n.l#e.l#e.l#e.l#e.m#e#h#e#e#e#h.p#e.p#e.p#e.s#e.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s"
".p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.H.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.M.N.M.N.O.P.O.P.O.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S.T.S.U.S.U.U.U.U",
".c#i.c#i.c#i#i.e#i.e#i.f.e#l.e#l.e#l.e.h.e.h#o.h#o.h.h.h.j#p.h.l.h.l.h.l.h.l.l.l.l.l.l#m.l.l.l#e.l#e.l#e.l#e.m#e.m#e#h#e#h#e#h.p#h.p#e.p#h.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p"
".s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.8.L.M.L.8.L.N.M.N.M.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S#..S.U.T.U.U.U.U.W",
"#r#i#i#i.f#i.f#i.f.e#l.e#l.e#l.e.h#l.h#o.h.h.h.h#p.h#p.h.l.h.l.h.l.l.l.l#m.l#m.l.n.l#e.l#e.l#e.l#e.m#e#h#e#e#e#h.p#e.p#e.p#e.s#e.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s"
".s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.I.G.J.I.J.K.J.J.L.J.L.L.M.L.M.L.8.L.O.M.N.M.N.O.P.O.P.N.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S#..S.U.S.U.U",
".c#i.c#i#i.e#i.e#i.f#i.f.e#l.e#l.e.h.e.h#o.h#o.h.h.h.j#p.h.l.h.l.h.l.h.l.l.l.l.l.l.l.l.l.l#e.l#e.l#e.l#e.m#e.m#e#h#e#h#e#h.p#h.p#e.p#h.s.p.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p"
".s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.J.J.L.J.L.J.L.J.8.L.8.L.N.M.N.M.N.O.P.O.P.N.Q.P.Q.P.Q.Q.R.Q.R.Q.T.R.T.S.T.S#..S.U#s.U.U.U",
"#i#i.f#i.f#i.f.e#l.e#l.e#l.e.h#l.h#o.h.h.h.h#p.h#p.h.l.h.l.h.l#p.l#p#m.l#m.l#m.l#t.l#e.l#e.l#e.m#e#h#e#e#e#h.p#e.p#e.p#e.s#e.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s#b"
".s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.I.G.I.G.I.K.J.K.J.I.J.I.L.J.L.J.L.L.M.L.M.L.N.M.N.M.N.O.P.O.P.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.R.T.S.T.S.U.S.U.V",
".c#i.c.e#i.e#i.f#i.f.e#l.e#l.e.h.e.h#o.h#o.h.h.h.j.h.h#p.h#p.h.l.h.l#p.l#p.l.l.l.l.l.l.n.l#e.l#e.l#e.m#e.m#e#h#e#h#e#h.p#h.p#e.p#h.s.p.s.p.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p"
".s.p.s.p.s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.I.J.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.8.L.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.9.Q.S.Q.T.S.T.S.U.S.U.S.U.U.U",
".f#i.f#i.f.e#l.e#l.e#l.e.h#l.h#o.h#l.h.h.h.h#p.h.l.h.l.h.l#p.l#p#m.l#m.l#m.l#t.l#e.l#e.l#e.l#e#u#e#e#e#h#e#e#e#e.p#e.s#e.s.p.s#e.s.p.s.p.s.s.s.s#b.s.s.s#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s"
".s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.G.F.H.F.G.F.I.G.I.G.I.G.I.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.L.N.M.N.O.N.O.P.O.Q.O.Q.P.Q.Q.Q.Q.S.Q.S.R.T.S.T.S.U.S.U#v",
".c#i#i.e#i.f#i.e.e#l.e#l.e#l.e#l#o.h#o.h#l.h.j.h.h#p.h#p.h.l.h.l.h.l#p.l.l.l.l.l.l.n.l#e.l#e.l#e.l#e.l#e#h#e#h#e#h.p#h.p#e.p#h.s#e.s#e.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s"
".s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.I.I.J.K.J.I.J.J.L.J.L.J.L.L.M.L.8.L.N.M.N.M.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T.S.U.T.U#..U",
".f#i.f#i#l#i#l.e#l.e#l.f.h#o.h#l.h#l.h.h#p.h#p.h#p.h.l.h.l#p#m.l.l.l#m.l#t.l#e.l#e.l#e.l#e.l#e#e#e#h#e#e#e#e.p#e.s#e.s#e.s#e.s.p.s.p.s.s.s.s#b.s.s.s#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b"
".s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.E.G.F.H.F.H.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.L.M.L.8.L.O.M.N.M.N.O.P.O.P.O.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S.T.S.U.S",
"#i#i#i.f#i.e#i#l#i#l.e#l.e#l.e.h#o.h#o.h#o.h.h#p.h#p.h.l.h.l.h.l#p.l.l.l.l.l.l#m.l#t.l#e.l#e.l#e.l#e#h#e#h#e#h#e#h#e#e.p#h.s#e.s#e.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s"
".s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.J.I.J.I.J.I.J.J.L.J.L.J.8.L.M.L.N.L.N.M.N.O.N.O.P.N.P.N.Q.P.Q.Q.Q.Q.R.Q.T.Q.T.S.T.S#..S.U.T.U",
".f#i.f#i#l.e#l.e#l.e#l#o.h#l.h#l.h.h#p.h#p.h#p.h.l.h.l#p#m.l.l.l#m.l#m.l#e.l#e.l#e.l#e.l#e#e#e#h#e#e#e#e.p#e.s#e.s#e.s#e.s.p.s.p.s.s.s.s#b.s.s.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b"
".s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.G.F.G.F.H.F.H.G.I.G.I.G.I.K.J.I.J.K.J.J.L.J.L.L.M.L.M.L.N.L.N.M.N.M.N.O.P.O.P.N.Q.P.Q.Q.Q.Q.R.Q.S.R.S.S.T.S.U.S",
"#i.e#i.e#i.f#i#l.e#l.e#l.e.h#o.h#o.h#o.h.h#p.h#p.h.l.h.l.h.l#p.l.l.l.l.l.l#m.l#u.l#e.l#e.l#e.l#e#h#e#h#e#h#e#h#e#e.p#h.s#e.s#e.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s"
".s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.F.G.F.H.F.H.F.I.G.I.G.I.G.I.G.J.I.J.I.L.J.L.J.L.J.L.L.8.L.8.L.N.M.N.M.N.O.P.O.P.N.Q.P.Q.Q.Q.Q.R.Q.S.Q.T.S.T.S.T.S.U.S.U",
".f#i#l.e#l.e#l.e#l#o.h#l.h#l.h.h.h.h#p.h#p.h.l.h.l#p#m#p.l#p#m.l#m.l#t.l#t.l#e.l#e.l#e#h#e#h#e#e#e#e.p#e.s#e.s#e.s#e.s.p.s.p.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j"
".s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.I.G.I.G.I.G.I.K.J.K.J.I.L.J.L.J.L.J.L.L.M.L.M.L.N.M.N.M.N.O.P.O.Q.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.S.T.S.U.S",
"#i.e#i.f#i#l.e#l.e#l.e#l#o.h#o.h#o.h.h.h.h#p.h#p.h#p.h.l#p.l#p.l#p.l.l#m.l#u.l#e.l#e.l#e.l#e#h#e#h#e#h#e#h#e#e.p#h.s#e.s#e.s.p.s.p.s.p.s.s.s.s.s.s.s.s.s.s.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s"
".s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.E.C.F.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.I.J.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.8.M.N.M.N.M.N.N.P.O.Q.N.Q.P.Q.Q.Q.Q.T.Q.S.R.T.S.T.S.U.S.U",
"#l#i#l.e#l.e#l.e.h#l.h#l.h#l.h#l#p.h#p.h#p.h#p#p#m#p.l#p#m.l#m.l#t.l#t.l#e.l#e.l#e#u#e#u#e#e#e#e#e#e#x#e.s#e.s#e.s.p.s.p.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j"
".s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.C.E.C.F.E.F.E.F.E.G.F.H.F.G.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.J.L.J.M.L.M.L.M.M.N.M.N.O.N.O.P.O.Q.O.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S.T.S",
"#i.f#i#l#i#l.e#l.e#l.e#l#o.h#o.h#l.h#o#p.h#p.h#p.h.l.h.l#p.l#p.l.l#m.l#u.l#t.l#t.l#e.l#e#u#e#u#e#h#e#h#e#e.p#h.s#e.s#e.s#e.s#e.s.p.s.s.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s#b.s"
".s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E.H.F.H.F.H.F.H.G.I.G.I.G.I.I.J.I.J.I.J.J.L.J.L.J.L.L.M.L.N.L.N.M.N.O.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T.S.U",
"#l#i#l.e#l.e#l#l#l#o.h#l.h#l#p.h#p.h#p.h#p.h#m#p.l#p#m.l#m.l#t.l#t.l#e.l#e.l#e#u#e#u#e#e#e#e#e#e#x#e.s#e.s#e.s#e.s#e.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j"
".s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.E.C.F.E.F.E.F.F.F.F.G.F.H.F.H.G.I.G.I.G.I.G.J.K.J.K.J.J.L.J.L.L.L.L.M.L.N.L.O.M.N.M.N.O.P.O.P.N.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S.T.S",
"#i.f#i#l#i#l.e#l.e#l#o.h#o.h#l.h#o#p.h#p.h#p.h.l.h.l#p.l#p.l.l#m.l.l.l#t.l#t.l#e.l#e.l#e#u#e#h#e#h#e#e#e#h#x#e.s#e.s#e.s#e.s.p.s.s.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s"
"#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.F.F.F.H.F.H.F.I.F.I.G.I.G.I.G.J.I.J.I.L.I.L.J.L.J.L.L.8.L.8.L.N.M.N.M.N.O.P.O.P.N.Q.P.Q.Q.Q.Q.R.Q.R.Q.T.S.T.S.T.S.U",
"#l.e#l.e#l#l#l#o.h#l.h#l#p.h#p.h#p.h#p.h.l#p.l#p#m#p#m#p#m.l#t.l#t.l#t.l#e#u#e#u#e#e#e#h#e#e#e#e.s#e.s#e.s#e.s#e.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#y#q#z#q#y#q#z#q#y#q#z#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j"
".s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.H.F.I.G.I.G.I.K.J.K.J.I.J.K.L.J.L.J.L.L.M.L.M.L.N.M.N.M.N.O.P.O.Q.O.Q.P.Q.P.Q.Q.R.Q.S.Q.T.R.T.S",
"#i#l#i#l.e#l.e#l#o.h#o.h#l.h#o.h.h#p.h#p.h#p.h#p#p.l#p.l#p#m#p.l.l#t.l#t.l#e.l#e.l#e#u#e#h#e#h#e#e#e#h#x#e.s#e.s#e.s#e.s.p.s.s.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s"
".s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.7.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.K.I.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.8.L.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.S.Q.S.Q.T.S.T.S.U",
"#l.e#l.f#l#o.h#l.h#l.h#l#p#l#p.h#p.h.l#p.l#p#m#p#m#p#m.l#t.l#t.l#t.l#e.l#e#u#e#e#e#h#e#e#e#e#x#e#x#e.s#e.s#e.s.s.s.s#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j"
".s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.H.F.G.F.I.G.I.G.I.G.I.K.J.K.J.I.L.J.L.J.L.J.L.L.M.L.M.L.N.M.N.O.N.O.P.O.Q.O.Q.Q.Q.Q.Q.Q.S.Q.S.S.T.S",
"#i#l.e#l.e#l.e#l#o#l#o.h#o.h#l#p.h#p.h#p.h#p#p.l#p.l#p#m#p.l.l#t.l#t.l#e.l#e.l#e#u#e#u#e#u#e#e#e#h#x#e.s#e.s#e.s#e.s.p.s.s.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s"
"#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E#d.F.G.F.H.F.H.G.I.G.I.G.I.I.J.K.J.I.J.J.L.J.L.J.L.L.M.L.8.L.N.M.N.M.N.O.P.N.P.N.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T.S.T",
"#l.e#l#o#l#l#l#l.h#l#p#l#p.h#p.h#p.h#p#p#m#p#m#p#m.l#t.l#t.l#t.l#e.l#e#u#e#t#e#u#e#e#e#e#x#e#x#e.s#e.s#e.s#x.s#x#b.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j"
".s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.E.D.E.C.E.C.E.E.F.E.F.E.F.F.G.F.H.F.G.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.L.L.L.M.L.N.L.O.M.N.M.N.O.P.O.P.O.Q#f.Q.Q.Q.Q.R.Q.S.Q.S.S",
"#i#l#i#l.e#l#o#l#o.h#o.h#l#p#l#p.h#p.h#p.h.l#p.l#p.l#p.l.l#t.l#t.l#t.l#t.l#e#u#e#u#e#u#e#h#e#h#e#e#x#e.s#e.s#e.s#e.s#x.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s"
"#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.B.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.E.F.F.H.F.H.F.I.F.I.G.I.G.I.G.J.I.J.I.L.I.L.J.L.J.L.L.8.L.M.L.N.M.N.M.N.O.N.O.P.N.Q.N.Q.Q.Q.Q.Q.Q.R.Q.T.R.T.S.T",
"#l.e#l#l#l#l.h#l.h#l#p.h#p.h#p.h#p#p#m#p.l#p#m#p#t.l#t.l#t.l#t.l#e#u#e#t#e#u#e#e#e#e#x#e#x#e.s#e.s#e.s#e.s#x.s.s.s.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j"
".s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.G.F.G.F.H.F.I.G.I.G.I.K.I.K.J.I.J.K.L.J.L.J.L.L.M.L.M.L.N.L.N.M.N.O.N.O.P.O.Q.N.Q.P.Q.Q.R.Q.S.Q.T.R",
"#i#l.e#l.e#l#o.h#o.h#l.h#l#p.h#p.h#p.h#p#p#p#p.l#p.l#p#m.l#t.l#t.l#t.l#e.l#e#u#e#u#e#h#e#h#e#e#x#e#x#e.s#e.s#e.s#x.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s"
"#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.F.F.F.H.F.H.F.I.G.I.G.I.K.I.K.J.I.J.I.L.J.L.J.L.L.L.L.8.L.8.L.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.S.Q.S.Q.T.S.T",
"#l#l#l#o.h#l.h#l#p#l#p.h#p.h#p#p#m#p.l#p#m#p#m#p#t.l#t.l#t.l#t.l#e#t#e#u#e#e#e#e#x#e#x#e.s#e.s#e.s#e.s#x.s.s.s.s#b.s#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j"
".s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.I.G.I.G.I.G.I.K.J.K.J.I.L.J.L.J.L.J.L.L.M.L.M.L.N.M.N.O.N.O.P.O.Q.O.Q.Q.Q.Q.Q.Q.S.Q.S#A",
".e#l.e#l#o#l#o#l#l.h#o#p#l#p.h#p.h#p#p#p#p.l#p.l#p#m#p#u.l#t.l#t.l#e.l#e#u#e#u#e#h#e#h#e#e#x#h#x#e.s#e.s#e.s#x.s.s.s.s.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s"
"#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.I.J.K.J.I.J.I.L.J.L.J.L.L.M.L.8.L.N.M.N.M.N.O.P.N.P.O.Q.P.Q.P.Q.Q.R.Q.T.Q.T.R.T",
"#l#o#l#l.h#l#p#l#p#l#p.h#p.h#B#p#p#p#m#p#m#p#t.l#t.l#t.l#t.l#e#u#e#u#e#t#e#t#e#e#x#e#x#e#x#e.s#e.s#x.s#x.s.s#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j"
"#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.E.C.F.C.F.E.F.E.F.F.G.F.H.F.G.F.I.G.I.G.I.G.J.K.J.K.J.I.L.J.L.L.L.L.M.L.O.L.O.M.N.M.N.O.P.O.P.O.Q#f.Q.Q.Q.Q.R.Q.S.Q",
"#i#l#o#l#o#l#l.h#o.h#l#p#l#p.h#p.h#p#p.l#p.l#p#m#p#u.l#t.l#t.l#t.l#t#u#e#u#e#u#e#u#e#e#x#h#x#e.s#e.s#e.s#e.s#x.s#x.s.s.s.s.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s"
"#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.E.F.F.H.F.H.F.I.F.I.G.I.G.I.G.I.I.J.I.L.I.L.J.L.J.L.L.8.L.M.L.N.L.N.M.N.O.N.O.P.N.Q.N.Q.Q.Q.Q.Q.Q.R.Q.T#A.T",
"#l#l#l#l.h#l#p#l#p.h#p.h#p#p#p#p#m#p#m#p#m#p#t.l#t.l#t.l#e#u#e#u#e#t#e#t#e#e#x#e#x#e#x#e.s#e.s#x.s#x.s#x#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j"
"#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.F.F.G.F.H.F.I.G.I.G.I.K.I.K.J.I.L.K.L.J.L.J.L.L.M.L.M.L.N.L.N.M.N.O.N.O.Q.O.Q.N.Q.P.Q.Q.Q.Q.S.Q",
".e#l#o#l#o.h#o.h#l#p#l#p.h#p.h#p#p#p#p#p#p#m#p.l#p#t.l#t.l#t.l#t.l#e#u#e#u#e#u#e#e#e#h#x#e#x#e#x#e.s#e.s#x.s#x.s.s.s.s.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s"
"#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.E.C.F.E.F.E.F.F.F.F.H.F.H.F.I.G.I.G.I.K.I.K.J.I.J.I.L.J.L.J.L.L.L.L.8.L.8.L.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.S.Q.S.Q.T",
"#l#l.h#l#p#l#p#l#p.h#p.h#p#p#m#p#m#p#m#p#t#p#t.l#t.l#t.l#t#u#e#t#e#u#e#e#x#e#x#e#x#e.s#e.s#e.s#x.s#x#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j"
"#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.E.C.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.I.F.I.G.I.G.I.K.J.K.J.I.L.J.L.J.L.J.L.L.M.L.M.L.N.M.N.O.N.O.P.O.Q.O.Q.Q.Q.Q.Q.Q.S.Q",
"#o#l#o#l#o#l#l#p#l#p#l#p.h#p.h#p#p#p#p.l#p.l#p#t#p#t.l#t.l#t.l#e#u#e#u#e#u#e#e#e#h#x#e#x#e#x#e.s#e.s#x.s#x.s.s.s.s.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k"
"#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.7.B.C.B.C.B.C.B.C.D.C.D.C.D.E.C.E.C.F.C.F.E.F.E.F.E#d.F.G.F.H.F.H.F.I.G.I.G.I.I.J.K.J.I.J.I.L.J.L.J.L.L.M.L.N.L.N.M.N.O.N.O.P.N.P.O.Q.P.Q.P.Q.Q.R.Q.T.Q.T",
"#l#l#l#l#p#l#p#l#p.h#p#p#B#p#p#p#m#p#t#p#t.l#t.l#t.l#t#u#e#t#e#u#e#t#e#t#x#e#x#e#x#e#x#e.s#x.s#x#b.s.s.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j"
"#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.F.D.F.C.F.E.F.F.F.F.G.F.H.F.G.F.I.G.I.G.I.K.J.K.J.K.J.I.L.J.L.L.L.L.M.L.O.L.O.M.N.M.N.O.P.O.P.O.Q#f.Q.Q.Q.Q.R.Q",
"#o#l#o#l#l.h#l#p#l#p#l#p.h#p#p#p#p.l#p.l#p#m#p#t.l#t.l#t.l#t#u#t#u#e#u#e#t#e#u#x#e#x#e#x#e.s#e.s#x.s#x.s#x.s#x.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k"
"#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.F.F.F.H.F.H.F.I.F.I.G.I.G.I.G.I.I.J.I.L.I.L.J.L.J.L.L.8.L.M.L.N.L.N.M.N.O.N.O.P.N.Q.N.Q.Q.Q.Q.Q.Q.S.Q.T",
"#l#l#p#l#p#l#p.h#p.h#B#p#p#p#m#p#m#p#t#p#t.l#t.l#t#u#e#t#e#u#e#t#e#t#x#e#x#e#x#e#x#e.s#x.s#x#b#x.s#x#b.s#b.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j"
"#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.F.F.G.F.I.F.I.G.I.G.I.K.I.K.J.K.L.K.L.J.L.J.L.L.M.L.M.L.N.L.N.O.N.O.N.O.Q.O.Q.N.Q.Q.Q.Q.Q.Q",
"#o#l#l.h#o#p#l#p#l#p.h#p.h#p#p#p#p#p#p#m#p#u#p#t.l#t.l#t.l#t#u#e#u#e#u#e#u#e#e#x#e#x#e#x#e#x#e.s#x.s#x.s#x.s.s#b.s#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k"
"#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.E.C.F.C.F.E.F.E.F.F.F.F.H.F.H.F.I.G.I.G.I.K.I.K.J.I.J.I.L.J.L.J.L.L.L.L.8.L.8.L.N.M.N.M.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.S.Q.S",
".h#l#p#l#p#l#p#l#p#p#p#p#m#p#m#p#t#p#t#p#t.l#t.l#t#u#t#u#e#t#e#t#e#e#x#e#x#e#x#e.s#e.s#x#b#x.s#x#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j"
"#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.E.F.E.F.E.F.F.G.F.G.F.I.F.I.G.I.G.I.K.J.K.J.I.L.I.L.J.L.L.L.L.M.L.O.L.N.M.N.O.N.O.P.O.Q.O.Q.Q.Q.Q.Q.Q",
"#o#l#o#l#l#p#l#p#l#p#l#p#p#p#p#p#p#m#p.l#p#t#p#t.l#t.l#t#u#e#u#e#u#e#u#e#e#x#h#x#e#x#e#x#e.s#x.s#x.s#x.s.s.s.s.s.s#j.s#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k"
"#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.E.F.E.F.E#d.F.G.F.H.F.I.F.I.G.I.G.I.I.J.K.J.I.L.I.L.J.L.J.L.L.M.L.N.L.N.M.N.O.N.O.P.N.P.O.Q.N.Q.Q.Q.Q.R.Q.T",
"#C#l#p#l#p#l#p#p#p#p#B#p#B#p#m#p#t#p#t.l#t.l#t#u#t#u#e#t#e#t#e#t#x#t#x#e#x#e#x#e#x#x.s#x.s#x#b.s#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j"
"#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.F.D.F.C.F.E.F.F.F.F.G.F.H.F.G.F.I.G.I.K.I.K.J.K.J.K.L.I.L.J.L.L.L.L.M.L.O.L.O.M.N.M.N.O.P.O.Q.O.Q#f.Q.Q.Q.Q",
"#o#l#l#p#l#p#l#p#l#p.h#p#p#p#p#m#p.l#p#t#p#t.l#t.l#t.l#t#u#t#u#e#u#e#t#x#u#x#e#x#e#x#e.s#e.s#x.s#x.s#x.s#x.s.s#j.s#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k"
"#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.F.F.F.H.F.H.F.I.F.I.G.I.G.I.K.I.I.J.I.L.I.L.J.L.L.L.L.8.L.M.L.N.L.N.M.N.O.N.O.Q.N.Q.N.Q.Q.Q.Q.Q.Q.S",
"#p#l#p#l#p.h#p#p#B#p#p#p#m#p#t#p#t#p#t#p#t.l#t#u#t#t#t#u#e#t#x#t#x#e#x#e#x#e#x#x.s#x.s#x#b#x.s#x#b.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q"
"#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.F.F.G.F.I.F.I.G.I.G.I.K.I.K.J.K.L.K.L.J.L.J.L.L.M.L.M.L.N.L.N.O.N.O.N.O.Q.O.Q.N.Q.Q.Q.Q",
"#l#l#o#p#l#p#l#p.h#p#p#p#p#p#p#p#p#t#p#t#p#t#p#t.l#t#u#t#u#e#u#e#t#e#u#x#e#x#e#x#e#x#e#x#x.s#x.s#x.s#x.s.s#j.s#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k"
"#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.C.F.E.F.E.F.F.F.F.H.F.H.F.I.G.I.G.I.K.I.K.J.I.J.I.L.J.L.J.L.L.L.L.8.L.N.L.N.M.N.O.N.O.P.O.Q.N.Q.P.Q.Q.Q.Q.S",
"#p#l#p#l#p#l#B#p#p#p#B#p#m#p#t#p#t#p#t.l#t#u#t#t#t#u#e#t#e#t#x#e#x#e#x#e#x#e#x#x#x#x#b#x.s#x#b.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q"
"#j#j#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.E.F.E.F.F.F.F.G.F.G.F.I.F.I.G.I.G.I.K.J.K.J.I.L.J.L.L.L.L.L.L.M.L.O.L.N.M.N.O.N.O.P.O.Q.O.Q.Q.Q.Q",
"#o#C#l#p#l#p#l#p#l#p#p#p#p#p#p#m#p#u#p#t#p#t.l#t.l#t#u#e#u#e#u#e#u#x#t#x#t#x#e#x#e#x#x.s#x.s#x.s#x.s.s#b.s#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k"
"#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.E.F.F.F.F#d.F.G.F.I.F.I.F.I.G.I.G.I.I.J.K.L.I.L.I.L.J.L.L.L.L.M.L.N.L.N.M.N.O.N.O.P.N.Q.O.Q.Q.Q.Q.Q.Q.R",
"#p#l#p#l#p#p#p#p#B#p#B#p#t#p#t#p#t.l#t.l#t#t#t#u#e#t#e#t#x#t#x#t#x#e#x#e#x#x#x#x#b#x.s#x#b#x#j#x#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#z#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q"
"#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.F.D.F.C.F.E.F.F.F.F.G.F.H.F.K.F.I.G.I.K.I.K.J.K.L.K.L.I.L.J.L.L.L.L.M.L.O.L.O.M.N.O.N.O.P.O.Q.O.Q#f.Q.Q",
"#l#p#l#p#l#p#l#p#p#p#p#p#p#m#p.l#p#t#p#t.l#t.l#t#u#t#u#t#u#e#u#e#t#x#t#x#e#x#e#x#e#x#x#x#x.s#x.s#x#b#x#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k"
"#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.E.C.E.C.F.C.F.E.F.F.F.F.H.F.H.F.I.F.I.G.I.K.I.K.I.I.J.I.L.I.L.J.L.L.L.L.8.L.M.L.N.L.N.M.N.O.N.O.Q.N.Q.N.Q.Q.Q.Q#A",
"#p#l#p.h#p#p#B#p#B#p#m#p#t#p#t#p#t#p#t#u#t#u#t#t#t#t#e#t#x#t#x#e#x#e#x#e#x#x#b#x.s#x#b#x#b#x#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q"
"#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.E.C.F.C.F.E.F.E.F.F.F.F.G.F.I.F.I.G.I.G.I.K.I.K.J.I.L.K.L.J.L.J.L.L.M.L.M.L.N.L.N.O.N.O.N.O.Q.O.Q#E.Q.Q",
"#l#C#l#p#l#p.h#p#p#p#p#p#p#p#p#t#p#t#p#t#p#t.l#t#u#t#u#e#u#e#t#x#u#x#e#x#e#x#e#x#x#x#x.s#x.s#x.s#x#k.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k"
"#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.C.F.E.F.E.F.F.F.F.H.F.H.F.I.G.I.G.I.K.I.K.J.I.J.I.L.J.L.J.L.L.L.L.N.L.N.L.N.O.N.O.N.O.P.O.Q.N.Q.Q.Q.Q.Q",
"#p#l#p#C#B#p#p#p#B#p#F#p#t#p#t#p#t.l#t#u#t#t#t#u#e#t#x#t#x#t#x#t#x#e#x#x#x#x#x#x#b#x#b#x#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q"
"#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.F.D.F.C.F.E.F.F.F.F.F.F.G.F.G.F.I.F.I.K.I.K.I.K.J.K.J.I.L.J.L.L.L.L.L.L.O.L.O.L.N.M.N.O.N.O.Q.O.Q.O.Q.Q",
"#l#p#l#p#l#p#C#p#p#p#p#p#p#m#p#t#p#t#p#t.l#t#u#t#u#t#u#t#t#e#u#x#t#x#t#x#e#x#e#x#x.s#x.s#x.s#x.s.s#j.s#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k"
"#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.E.F.F.F.F#d.F.G.F.I.F.I.F.I.G.I.K.I.I.J.K.L.I.L.I.L.L.L.L.L.L.M.L.N.L.N.M.N.O.N.O.Q.N.Q.O.Q.Q.Q.Q.Q",
"#p#l#B#p#p#p#B#p#B#p#t#p#t#p#t.l#t#u#t#t#t#u#e#t#e#t#x#t#x#t#x#e#x#e#x#x#x#x#b#x.s#x#b#x#j#x#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q"
"#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.F.D.F.C.F.E.F.F.F.F.G.F.I.F.K.F.I.G.I.K.I.K.J.K.L.K.L.I.L.J.L.L.L.L.M.L.N.L.O.O.N.O.N.O.Q.O.Q.N.Q#G",
"#l#p#l#p#l#p#p#p#p#p#p#B#p#H#p#t#p#t#p#t.l#t#u#t#u#t#u#e#u#x#t#x#t#x#e#x#e#x#x#x#x#x#x.s#x.s#x#j#x#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j"
"#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.F.C.F.C.F.C.F.E.F.F.F.F.H.F.H.F.I.F.I.G.I.K.I.K.J.I.J.I.L.I.L.J.L.L.L.L.8.L.O.L.N.M.N.O.N.O.N.O.Q.N.Q.N.Q.Q.Q",
"#p#C#p#C#B#p#B#p#m#p#t#p#t#p#t#p#t#u#t#u#t#t#t#t#x#t#x#t#x#e#x#e#x#x#x#x#b#x.s#x#b#x#j#x#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q"
"#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.E.D.F.C.F.C.F.F.F.F.F.F.F.F.G.F.I.F.I.G.I.G.I.K.I.K.J.I.L.K.L.L.L.L.L.L.M.L.O.L.N.M.N.O.N.O.P.O.Q.O.Q.Q",
"#l#p#l#p#l#p#C#p#p#B#p#p#p#t#p#t#p#t#p#t#u#t#u#t#u#e#u#e#t#x#t#x#e#x#e#x#e#x#x#x#x.s#x.s#x#b#x#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k"
"#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.C.F.F.F.F.F.F.F.F.I.F.I.F.I.G.I.G.I.K.I.K.L.I.L.I.L.J.L.L.L.L.M.L.N.L.N.L.N.O.N.O.N.O.Q.O.Q.N.Q.Q.Q",
"#p#C#B#p#p#p#B#p#F#p#t#p#t#p#t#u#t#u#t#t#t#t#e#t#x#t#x#t#x#t#x#e#x#x#I#x#x#x#b#x#b#x#j.s#j.s#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q"
"#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.E.C.F.D.F.C.F.E.F.F.F.F.F.F.K.F.K.F.I.G.I.K.I.K.I.K.L.K.L.I.L.J.L.L.L.L.M.L.O.L.O.L.N.O.N.O#E.O.Q.O.Q.O",
"#l#p#l#p#C#p#p#p#p#p#p#t#p#t#p#t#p#t.l#t#u#t#u#t#u#t#t#x#u#x#t#x#t#x#e#x#x#x#x.s#x.s#x.s#x#k#x#j#x#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j"
"#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.7.B.C.B.C.B.C.D.C.D.C.C.C.D.F.C.F.C.F.C.F.E.F.F.F.F#d.F.G.F.I.F.I.G.I.K.I.K.I.I.J.K.L.I.L.J.L.L.L.L.L.L.M.L.N.L.N.M.N.O.N.O.Q.N.Q.N.Q.Q.Q",
"#B#p#p#p#B#p#B#p#t#p#t#p#t#p#t#H#t#t#t#u#t#t#t#t#x#t#x#t#x#e#x#x#x#x#x#x#b#x#b#x#j#x#j#x#j.s#j.s#j.s#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q"
"#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.F.C.F.E.F.E.F.F.F.F.G.F.I.F.K.F.I.G.I.K.I.K.J.K.L.K.L.J.L.L.L.L.L.L.O.L.N.L.O.O.N.O.N.O.Q.O.Q#E",
"#l#p#l#p#p#p#p#p#p#B#p#F#p#t#p#t#p#t#H#t#u#t#u#t#t#e#u#x#t#x#t#x#e#x#e#x#x#x#x#x#x.s#x.s#x#j#x#k.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k"
"#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.F.C.F.C.F.E.F.F.F.F.F.F.I.F.I.F.I.F.I.G.I.K.I.K.J.I.L.I.L.I.L.J.L.L.L.L.N.L.N.L.N.O.N.O.N.O.P.O.Q.N.Q.Q.Q",
"#p#C#B#p#B#p#F#p#F#p#t#p#t#p#t#t#t#u#t#t#t#t#x#t#x#t#x#t#x#t#x#x#x#x#I#x#x#x#b#x#j#x#j.s#j.s#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q"
"#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.F.D.F.C.F.C.F.F.F.F.F.F.G.F.K.F.I.F.I.K.I.K.I.K.J.K.L.I.L.K.L.L.L.L.L.L.O.L.O.L.N.M.N.O.N.O.Q.O.Q.O",
"#l#p#C#p#C#p#p#B#p#p#p#t#p#t#p#t#p#t#u#t#u#t#u#t#u#t#t#x#t#x#t#x#t#x#x#x#x#x#x.s#x.s#x#j#x#k.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j"
"#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.C.F.F.F.F#d.F.G.F.I.F.I.F.I.K.I.K.I.K.J.K.L.I.L.I.L.L.L.L.L.L.M.L.N.L.N.M.N.O.N.O#E.N.Q.O.Q#E.Q",
"#B#p#p#p#B#p#F#p#t#p#t#p#t#H#t#u#t#t#t#t#e#t#x#t#x#t#x#t#x#x#x#x#I#x#x#x#b#x#j#x#j#x#j#x#j.s#j.s#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q"
"#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.C.D.E.C.F.D.F.C.F.E.F.F.F.F.F.F.I.F.K.F.I.G.I.K.I.K.I.K.L.K.L.I.L.J.L.L.L.L.M.L.O.L.O.O.N.O.N.O.Q.O.Q.O",
"#l#p#C#p#p#p#p#p#p#F#p#F#p#t#p#t#p#t#H#t#u#t#u#t#t#x#u#x#t#x#t#x#e#x#x#x#x#x#x#x#x#b#x#k#x#j#x#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j"
"#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.F.C.F.C.F.C.F.E.F.F.F.F#d.F.I.F.I.F.I.G.I.K.I.K.I.I.J.I.L.I.L.J.L.L.L.L.N.L.O.L.N#J.N.O.N.O.N.O.Q.N.Q.N.Q",
"#p#C#B#p#B#p#F#p#t#p#t#p#t#H#t#t#t#u#t#t#K#t#x#t#x#t#x#e#x#x#x#x#x#x#b#x#b#x#j#x#j#x#j.s#j.s#j#k#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q"
"#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.F.D.F.C.F.C.F.F.F.F.F.F.F.F.G.F.I.F.I.K.I.K.I.K.I.K.J.K.L.K.L.L.L.L.L.L.O.L.O.L.N.L.N.O.N.O.N.O.Q.O",
"#l#p#C#p#C#p#p#B#p#F#p#t#p#t#p#t#H#t#u#t#u#t#t#e#u#x#t#x#t#x#t#x#e#x#x#x#x#x#x.s#x.s#x#j#x#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j"
"#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.7.B.C.B.C.B.C.D.C.D.C.D.C.C.E.C.F.C.F.C.F.F.F.F.F.F.F.F.I.F.I.F.I.K.I.K.I.K.I.K.L.I.L.I.L.L.L.L.L.L.L.L.N.L.N.L.N.O.N.O#E.O.Q.O.Q#E.Q",
"#B#p#B#p#F#p#F#p#t#p#t#p#t#t#t#u#t#t#t#t#x#t#x#t#x#t#x#t#x#x#x#x#I#x#x#x#b#x#j#x#j#x#j#x#j.s#j#k#j#k#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q"
"#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.F.D.F.C.F.E.F.F.F.F.F.F.K.F.K.F.I.F.I.K.I.K.I.K.L.K.L.I.L.J.L.L.L.L.L.L.O.L.O#J.N.O.N.O#E.O.Q.O",
"#C#p#C#p#p#B#p#p#p#F#p#t#p#t#p#t#u#t#u#t#u#t#u#K#t#x#t#x#t#x#t#x#x#x#x#x#x#x#x#x#x#j#x#k#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j"
"#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D#L.C.F.C.F.C.F.E.F.F.F.F#d.F.G.F.I.F.I.F.I.K.I.K.I.I.J.K.L.I.L.I.L.L.L.L.L.L.O.L.N.L.N.O.N.O.N.O.Q.N.Q.N.Q",
"#p#p#B#p#F#p#t#p#t#p#t#H#t#H#t#t#t#t#t#t#K#t#x#t#x#t#x#x#x#x#I#x#x#x#b#x#j#x#j#x#j#x#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q"
"#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D#L.D.F.C.F.D.F#L.F.F.F.F.F.F.G.F.I.F.K.F.I.K.I.K.I.K.J.K.L.K.L#M.L.L.L.L#J.L.O.L.N.L.O.O.N.O.N.O.Q.O",
"#C#p#C#p#p#p#p#F#p#F#p#t#p#t#p#t#H#t#u#t#u#t#t#x#u#x#t#x#t#x#e#x#x#x#x#x#x#x#x#b#x#k#x#j#x#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j"
"#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.F.C.F.C.F#L.F.F.F.F.F.F.I.F.I.F.I.F.I.K.I.K.I.K.L.I.L.I.L.I.L.L.L.L.L.L.N.L.O.L.N.O.N.O.N.O.Q.O.Q.N.Q",
"#B#p#B#p#F#p#F#p#t#p#t#H#t#t#t#u#t#t#K#t#x#t#x#t#x#t#x#K#x#x#x#x#I#x#I#x#j#x#j#x#j.s#j.s#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#q#q#q"
"#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D.C.D.C.D.F.D.F.C.F.C.F.F.F.F.F.F.K.F.K.F.I.F.I.K.I.K.I.K.L.K.L.I.L.K.L.L.L.L.L.L.O.L.O.L.N.O.N.O#E.O.Q.O",
"#C#p#C#p#p#B#p#F#p#t#p#t#p#t#H#t#u#t#u#t#t#t#u#K#t#x#t#x#t#x#K#x#x#x#x#x#x.s#x#k#x#j#x#j#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j"
"#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D#L.C.F.C.F.C.F.C.F.F.F.F.F.F.G.F.I.F.I.F.I.K.I.K.I.K.I.K.L.I.L.I.L.L.L.L.L.L.O.L.N.L.N.L.N.O.N.O#E.O.Q.O.Q",
"#B#p#F#p#F#p#t#p#t#p#t#F#t#H#t#t#t#t#K#t#K#t#x#t#x#t#x#x#x#x#I#x#x#x#j#x#j#x#j#x#j#x#j.s#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q"
"#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.D.C.D.C.D#L.D.F.C.F.D.F#L.F.F.F.F.F.F.F.F.K.F.K.F.I.G.I.K.I.K.I.K.L.K.L.I.L.L.L.L.L.L.O.L.O.L.O.O.N.O.N.O#E.O",
"#C#p#p#B#p#p#p#F#p#F#p#t#p#t#H#t#H#t#u#t#u#K#t#x#t#x#t#x#t#x#x#x#x#x#x#x#x#x#x#j#x#k#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j"
"#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D.C.C.C.D.F.C.F.C.F#L.F.F.F.F.F.F#N.F.I.F.I.F.I.G.I.K.I.K#M.I.L.I.L.I.L.L.L.L.L.L#J.L.O.L.N#J.N.O.N.O.N.O.Q.N.Q",
"#B#p#F#p#F#p#F#p#t#H#t#H#t#t#t#t#t#t#K#t#x#t#x#t#x#K#x#K#I#x#x#x#I#x#O#x#j#x#j#x#j.s#j#k#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q"
"#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D.F.D.F.C.F.C.F.F.F.F.F.F#P.F.K.F.I.F.I.K.I.K.I.K#M.K.L.K.L.K.L.L.L.L.L.L#J.L.O.L.N#J.N.O.N.O#E.O",
"#C#p#C#p#p#F#p#F#p#t#p#t#p#t#H#t#u#t#u#t#t#K#u#K#t#x#t#x#t#x#K#x#x#x#x#x#x#b#x#k#x#j#x#j#x#j#x#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j"
"#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D#L.C.F.C.F.C.F.C.F.F.F.F.F.F.F.F.I.F.I.F.I.K.I.K.I.K.I.K.L.I.L.I.L.L.L.L.L.L.L.L.N.L.N.L.N.O.N.O#E.O.Q.O.Q",
"#B#p#F#p#F#p#t#p#t#H#t#F#t#u#t#t#K#t#K#t#x#t#x#t#x#K#x#x#x#x#I#x#I#x#j#x#j#x#j#x#j#x#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#q"
"#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D#L.D#L.D.F.D.F#L.F.F.F.F.F.F.F.F.K.F.K.F.I.F.I.K.I.K.I.K.L.K.L.I.L.L.L.L.L.L#J.L.O.L.O#J.N.O.N.O#E.O",
"#C#p#p#B#p#H#p#F#p#F#p#t#H#t#H#t#H#t#t#t#u#K#t#x#t#x#t#x#K#x#x#x#x#x#x#x#x#Q#x#j#x#j#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j"
"#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D#L.C.F.C.F#L.F.F.F.F.F.F#N.F.K.F.I.F.I.F.I.K.I.K#M.I.L.K.L.I.L#M.L.L.L.L#J.L.O.L.N#J.N.O.N.O.N.O.Q.N.Q",
"#B#p#F#p#F#p#F#p#t#F#t#H#t#t#t#t#K#t#K#t#x#t#x#t#x#K#x#x#I#x#x#x#I#x#O#x#j#x#j#x#j.s#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q"
"#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D.C.D.C.D#L.D.F.C.F.D.F#L.F.F.F.F#P.F.K.F.I.F.K#P.I.K.I.K#M.K.L.K.L.K.L#M.L.L.L.L#J.L.O.L.N.L.O.O.N.O#E.O",
"#C#B#C#p#p#F#p#F#p#t#p#t#H#t#H#t#u#t#u#t#t#K#t#K#t#x#t#x#K#x#K#x#x#x#x#x#x#j#x#k#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D#L.C#L.D.F.C.F.C.F#L.F.F.F.F.F.F.I.F.I.F.I#P.I.K.I.K.I.K.L.I.L.I.L#M.L.L.L.L.L.L.N.L.O.L.N.O.N.O#E.O.Q.O.Q",
"#F#p#F#p#F#p#t#p#t#H#t#t#t#t#t#t#K#t#x#t#x#t#x#K#x#K#I#x#x#x#I#x#O#x#j#x#j#x#j#x#j#Q#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#w#q#q#q#w"
"#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D#L.D#L.D.F.D.F#L.F#L.F.F.F.F.F.F.K.F.K.F.I.F.I.K.I.K.I.K.L.K.L.I.L#R.L.L.L.L.L.L.O.L.O#J.N.O.N.O#E.O",
"#C#p#p#B#p#F#p#F#p#F#p#t#H#t#H#t#u#t#t#K#u#K#t#x#t#x#t#x#K#x#x#x#x#x#x#I#x#Q#x#j#x#j#x#j#x#j#k#j#k#j#k#j#k#j#k#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D#L.C.F.C.F#L.F#L.F.F.F.F#P.F.K.F.I.F.I.F.I.K.I.K#M.K#M.K.L.I.L.I.L.L.L.L#J.L.O.L.N#J.N.O.N.O.N.O#E.N.Q",
"#F#p#F#p#F#p#t#H#t#F#t#H#t#t#t#t#K#t#K#t#x#t#x#K#x#x#x#x#I#x#I#x#O#x#j#x#j#x#j#x#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q"
"#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.D.B.D.B.C.B.C.D.C.D.C.D#L.D.F.C.F.D.F#L.F.F.F.F#P.F#P.F.I.F.K#P.I.K.I.K#M.K#M.K.L.K.L#M.L.L.L.L#J.L.O.L.O.L.O.O.N.O#E.O",
"#C#B#C#H#p#F#p#F#p#t#p#t#H#t#H#t#t#t#u#K#t#K#t#x#t#x#t#x#K#x#K#x#x#x#x#x#x#j#x#j#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#j#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D#L.C#L.D.F.C.F.C.F#L.F.F.F.F.F.F#N.F.I.F.I#P.I.K.I.K.I.K#M.I.L.I.L#M.L.L.L.L.L.L.N.L.O.L.N#J.N.O#E.O#E.O.Q",
"#F#p#F#p#F#p#t#H#t#H#t#t#t#t#K#t#K#t#x#t#x#t#x#K#x#K#I#x#x#x#I#x#O#x#j#x#j#x#j#x#j#Q#j#k#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w"
"#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D#L.D#L.D.F.D.F#L.F#L.F.F.F.F.F.F#P.F.K.F.I.F.I.K.I.K.I.K#M.K.L.I.L#R.L.L.L.L.L.L.O.L.O#J.N#J.N.O#E.O",
"#C#p#p#F#p#F#p#F#p#t#H#t#H#t#H#t#u#t#t#K#t#K#t#x#t#x#K#x#K#x#x#x#x#x#x#O#x#Q#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.C.B.C.B.C.D.C.D.C.D#L.C.F.C.F#L.F#L.F.F.F.F#P.F#P.F.I.F.I.F.I.K.I.K#M.K#M.K.L.I.L.I.L.L.L.L#J.L#J.L.N#J.N#J.N.O.N.O#E.O.Q",
"#F#p#F#p#t#p#t#H#t#F#t#H#t#t#K#t#K#t#K#t#x#t#x#K#I#x#x#x#I#x#O#x#j#x#j#x#j#x#j#Q#j#k#j#k#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q"
"#q#w#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D#L.D.F.C.F.D.F#L.F.F.F.F#P.F#P.F.K.F.K#P.I.K.I.K#M.K#M.K.L.K.L#M.L.L.L.L#J.L#J.L.O.L.O#J.N.O.N.O",
"#C#B#C#F#p#F#p#F#p#t#H#t#H#t#H#t#t#t#u#K#t#x#t#x#t#x#K#x#K#x#K#x#x#I#x#Q#x#j#x#j#x#j#x#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D#L.D#L.D.F.C.F.C.F#L.F.F.F.F.F.F#N.F.K.F.I#P.I.K.I.K.I.K#M.I.L.K.L#M.L.L.L.L.L.L#J.L.O.L.N#J.N.O#E.O#E.O.Q",
"#F#p#F#p#F#p#t#F#t#H#t#t#t#t#K#t#K#t#x#t#x#t#x#K#x#K#I#x#I#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w"
"#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D#L.D#L.D#L.D.F#L.F#L.F.F.F.F.F.F#P.F.K.F.I.F.K#P.I.K.I.K#M.K.L.K.L#R.L.L.L.L.L.L#J.L.O#J.N#J.O.O#E.O",
"#C#p#p#F#p#F#p#F#p#t#H#t#H#t#u#t#u#K#t#K#t#K#t#x#t#x#K#x#K#x#x#x#x#x#x#O#x#O#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D#L.C#L.C.F#L.F#L.F.F.F.F#P.F#P.F.I.F.I.F.I#P.I.K#M.K#M.K.L.I.L.I.L#M.L.L#J.L#J.L.N#J.N#J.N.O.N.O#E.O.Q",
"#F#p#F#p#t#p#t#H#t#F#t#F#t#t#K#t#K#t#K#t#x#K#x#K#I#x#x#x#I#x#O#x#j#x#j#x#j#x#j#Q#j#k#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q"
"#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D#L.D#L.D.F.D.F#L.F#L.F.F#P.F#P.F.K.F.K#P.I#P.I.K#M.K#M.K.L.K.L#M.L#M.L.L#J.L#J.L.O.L.O#J.N.O.N.O",
"#C#B#C#F#p#F#p#F#p#t#H#t#H#t#H#t#t#K#u#K#t#x#t#x#t#x#K#x#K#x#x#x#x#I#x#Q#x#j#x#j#x#j#x#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D#L.D#L.D#L.C.F.C.F#L.F.F.F.F.F.F#N.F.K.F.I#P.I#P.I.K.I.K#M.I.L.K.L#M.L#M.L.L.L.L#J.L.O.L.N#J.N.O#E.O#E.O.Q",
"#F#p#F#p#F#H#t#F#t#H#t#t#t#t#K#t#K#t#x#t#x#K#x#K#x#K#I#x#I#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w"
"#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D#L.D#L.D#L.D.F#L.F#T.F#L.F.F.F.F#P.F.K.F.I.F.K#P.I.K.I.K#M.K.L.K.L#R.L#M.L.L.L.L#J.L.O#J.N#J.O.O#E.O",
"#C#H#p#F#p#F#p#F#p#t#H#t#H#t#t#t#u#K#t#K#t#K#t#x#t#x#K#x#K#x#x#x#x#Q#x#O#x#O#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B.C.D.C.D#L.C#L.D.F#L.F#L.F#L.F.F#P.F#P.F.I.F.I.F.I#P.I.K#M.K#M.K.L.I.L.I.L#M.L.L#J.L#J.L.N#J.O#J.N.O.N.O#E.O.Q",
"#F#p#F#p#t#H#t#H#t#F#t#F#t#t#K#t#K#t#K#t#x#K#x#K#I#x#x#x#I#x#O#x#j#x#j#x#j#x#j#Q#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q"
"#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D#L.D#L.D.F.D.F#L.F#L.F.F#P.F#P.F.K.F.K#P.I#P.I.K#M.K#M.K.L.K.L#M.L#R.L.L#J.L#J.L.O.L.O#J.N.O.N.O",
"#C#F#C#F#p#F#p#F#p#t#H#t#H#t#H#t#t#K#t#K#t#x#t#x#t#x#K#x#K#x#x#x#x#O#x#Q#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D#L.D#L.D#L.C.F.C.F#L.F#L.F.F.F.F#N.F.K.F.I#P.I#P.I.K.I.K#M.K#M.K.L#M.L#M.L.L.L.L#J.L.O.L.N#J.N.O#E.O#E.O#E",
"#F#p#F#p#F#H#t#F#t#H#t#t#K#t#K#t#K#t#x#t#x#K#I#K#x#K#I#x#I#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w"
"#q#q#q#w#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B.C.B.C.D#L.D#L.D#L.D.F#L.F#T.F#L.F.F.F.F#P.F#P.F.I#P.K#P.I.K.I.K#M.K#M.K.L#R.L#M.L.L.L.L#J.L.O#J.O#J.O.O#E.O",
"#C#H#p#F#p#F#p#F#H#t#H#t#H#t#t#t#u#K#t#K#t#K#t#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B#L.D#L.D#L.C#L.D.F#L.F#L.F#L.F.F#P.F#P.F.I.F.I#P.I#P.I.K#M.K#M.K#M.I.L#M.L#M.L.L#J.L#J.L.N#J.O#J.N#J.N.O#E.O#E",
"#F#p#F#p#t#H#t#H#t#F#t#F#K#t#K#t#K#t#K#t#x#K#x#K#I#x#x#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.B.z.B.A.B.A.B.A.B.B.B.B.B.B.D.B.C.B.C.B.C.D.C.D#L.D#L.D.F.D.F#L.F#L.F.F#P.F#P.F#P.F.K#P.I#P.I.K#M.K#M.K#M.K.L#M.L#R.L.L#J.L#J.L.O.L.O#J.N.O.N.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#H#t#H#t#t#K#t#K#t#x#t#x#K#x#K#x#K#x#K#x#x#O#x#Q#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B.C.B.C.D#L.D#L.D#L.C.F#L.F#L.F#L.F.F#P.F#P.F#P.F.I#P.I#P.I.K.I.K#M.K#M.K.L#M.L#M.L.L.L.L#J.L.O.L.N#J.N#J#E.O#E.O#E",
"#F#p#F#p#F#H#t#F#t#F#t#t#K#t#K#t#K#t#x#t#x#K#I#K#x#K#I#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w"
"#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.D#L.D#L.D#L.D.F#L.F#T.F#L.F.F.F.F#P.F#P.F.K#P.K#P.I.K.I.K#M.K#M.K.L#R.L#M.L.L.L.L#J.L.O#J.O#J.O#J#E.O",
"#C#F#p#F#p#F#p#F#H#t#H#t#H#t#t#t#u#K#t#K#t#K#t#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#k#j#k#j#k#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B#L.D#L.D#L.C#L.D.F#L.F#L.F#L.F.F#P.F#P.F#N#P.I#P.I#P.I.K#M.K#M.K#M.I.L#M.L#M.L.L#J.L#J.L#J#J.O#J.N#J.N.O#E.O#E",
"#F#p#F#p#t#F#t#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#x#I#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B#L.B#L.D#L.D#L.D.F.D.F#L.F#L.F.F#P.F#P.F#P.F.K#P.I#P.I.K#M.K#M.K#M.K.L#R.L#R.L.L#J.L#J.L.O.L.O#J.N#J.N.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#H#t#H#K#t#K#t#K#t#K#t#x#K#x#K#x#K#x#K#x#x#O#x#O#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.C.B.C.B.C.D#L.D#L.D#L#L.F#L.F#L.F#L#P.F#P.F#P.F#P.F.I#P.I#P.I.K.I.K#M.K#M.K.L#M.L#M.L.L.L.L#J.L#J.L.N#J.N#J#E.O#E.O#E",
"#F#p#F#p#F#H#t#F#t#F#t#t#K#t#K#t#K#t#x#K#x#K#I#K#x#K#I#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w"
"#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B#L.D#L.D#L.D.F#L.F#T.F#L.F.F#P.F#P.F#P#P.K#P.K#P.I.K.I.K#M.K#M#R.L#R.L#M.L.L#J.L#J.L#J#J.O#J.O#J#E.O",
"#C#F#p#F#p#F#p#F#H#t#H#t#H#t#F#K#u#K#t#K#t#K#t#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B#L.B#L.D#L.C#L.D.F#L.F#L.F#L.F.F#P.F#P.F#N#P.K#P.I#P.I.K#M.K#M.K#M#M.L#R.L#M.L.L#J.L#J.L#J#J.O#J.N#J.N.O#E.O#E",
"#F#p#F#p#t#F#t#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#x#I#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B#L.B#L.D#L.D#L#T#L#T.F#L.F#L.F.F#P.F#P.F#P.F.K#P.I#P.I#P#M.K#M.K#M.K.L#R.L#R.L.L#J.L#J.L#J#J.O#J.N#J#U.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#H#t#H#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#x#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B.C.B.C.B#L.D#L.D#L#L.F#L.F#L.F#L#P.F#P.F#P.F#P.F.I#P.I#P.I#P.I.K#M.K#M.K.L#M.L#M.L.L#J.L#J.L#J.L.N#J.N#J#E.O#E.O#E",
"#F#p#F#H#F#H#t#F#t#F#t#t#K#t#K#t#K#t#x#K#x#K#I#K#x#K#I#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w"
"#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B#L.D#L.D#L.D#L#T.F#T.F#L#P.F#P.F#P.F#P#P.K#P.K#P.I#P.I.K#M.K#M#R.L#R.L#M.L.L#J.L#J.L#J#J.O#J.O#J#E.O",
"#C#F#p#F#p#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B#L.B#L.D#L.D#L.D.F#L.F#L.F#L.F.F#P.F#P.F#N#P.K#P.I#P.I#P#M.K#M.K#M#M.L#R.L#M.L#M#J.L#J.L#J#J.O#J.N#J.N.O#E.O#E",
"#F#p#F#H#F#F#t#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B#L.B#L.D#L.D#L#T#L#T.F#L.F#L.F.F#P.F#P.F#P.F.K#P.I#P.K#P#M.K#M.K#M.K.L#R.L#R.L#M#J.L#J.L#J#J.O#J.N#J#U.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#H#t#H#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#x#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B#L.B#L.B#L.D#L.D#L#L#L#L.F#L.F#L#P.F#P.F#P.F#P.F.I#P.I#P.I#P#M.K#M.K#M.K.L#M.L#M.L#M#J.L#J.L#J#J.N#J.N#J#E.O#E.O#E",
"#F#p#F#H#F#H#t#F#t#F#t#F#K#t#K#t#K#t#K#K#x#K#I#K#x#K#I#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w"
"#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B.C.B.C.B#L.D#L.D#L.D#L#T.F#T.F#L#P.F#P.F#P.F#P#P.K#P.K#P.I#P#M.K#M.K#M#R.L#R.L#M#J#M#J.L#J.L#J#J.O#J.O#J#E.O",
"#C#F#p#F#p#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B.C.B.C.B#L.B#L.D#L#T#L#T#L#L.F#L.F#L#P.F#P.F#P.F#N#P.K#P.I#P.I#P#M.K#M.K#M#M.L#R.L#M.L#M#J.L#J.L#J#J.O#J.N#J#E.O#E.O#E",
"#F#p#F#H#F#F#F#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B#L.B#L.D#L.D#L#T#L#T.F#L.F#L.F#L#P.F#P.F#P#P.K#P.I#P.K#P#M.K#M.K#M.K.L#R.L#R.L#M#J.L#J#J#J#J.O#J.N#J#U.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#F#t#H#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#Q#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B#L.B#L.B#L.D#L.D#L#L#L#T.F#L.F#L#P.F#P.F#P#P#P#P.I#P.I#P#M#P#M.K#M.K#M.K.L#M.L#M#J#M#J.L#J.L#J#J.N#J.O#J#E.O#E.O#E",
"#F#p#F#H#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w"
"#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B#L.B#L.B#L.D#L#T#L#T#L#T.F#T.F#L#P#L#P.F#P.F#P#P.K#P.K#P#M#P#M.K#M.K#M#R.L#R.L#M#J#R#J.L#J.L#J#J.O#J.O#J#E.O",
"#C#F#p#F#p#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#x#K#x#K#x#K#x#x#O#x#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#k#j#k#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B#L.B#L.B#L.B#L.D#L#T#L#T#L#L.F#L#P#L#P.F#P.F#P.F#N#P.K#P.I#P.I#P#M.K#M.K#M#M.L#R.L#M.L#M#J.L#J.L#J#J.O#J.N#J#E.O#E.O#E",
"#F#p#F#H#F#F#F#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B.C.B.D.B#L.B#L.D#L.D#L#T#L#T.F#L#P#T#P#L#P.F#P#P#P#P.K#P.I#P.K#P#M.K#M.K#M#R.L#R.L#R.L#M#J.L#J#J#J#J.O#J#U#J#U.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#F#t#H#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#x#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B#L.B#L.B#L.D#L.D#L#L#L#T.F#L.F#L#P.F#P.F#P#P#P#P.I#P.I#P#M#P#M.K#M.K#M#R.L#M.L#M#J#M#J.L#J#J#J#J.N#J.O#J#E.O#E.O#E",
"#F#p#F#H#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w"
"#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B#L.B#L.B#L.D#L#T#L#T#L#T.F#T.F#L#P#L#P.F#P.F#P#P.K#P.K#P#M#P#M.K#M.K#M#R.L#R.L#M#J#R#J.L#J.L#J#J.O#J.O#J#E.O",
"#C#F#p#F#p#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#x#K#x#K#x#K#x#K#I#K#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B#L.B#L.B#L.B#L.D#L#T#L#T#L#L.F#L#P#L#P.F#P.F#P.F#N#P.K#P.I#P#M#P#M.K#M.K#M#M.L#R.L#M.L#M#J.L#J.L#J#J.O#J#E#J#E.O#E#U#E",
"#F#p#F#H#F#F#F#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q"
"#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B#L.B#T.B#L.B#L.D#L.D#L#T#L#T.F#L#P#T#P#L#P.F#P#P#P#P.K#P.I#P#R#P#M.K#M#R#M#R.L#R.L#R#J#M#J.L#J#J#J#J.O#J#U#J#U.O",
"#C#F#C#F#p#F#p#F#H#t#H#t#H#t#H#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#x#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B.7.B.C.B#L.B#L.B#L#T#L#T#L#L#L#T.F#L#P#L#P.F#P.F#P#P#P#P.I#P.I#P#M#P#M.K#M#R#M#R.L#M.L#M#J#M#J.L#J#J#J#J.N#J.O#J#E.O#E.O#E",
"#F#p#F#H#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w"
"#q#q#q#w#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B.B.B.D.B#L.B#L.B#L.D#L#T#L#T#L#T.F#T.F#L#P#L#P.F#P.F#P#P.K#P.K#P#M#P#M.K#M.K#M#R.L#R.L#M#J#R#J.L#J#J#J#J.O#J.O#J#E.O",
"#C#F#C#F#C#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#Q#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B#L.B#L.B#L.B#L.D#L#T#L#T#L#L.F#L#P#L#P.F#P#P#P#P#N#P.K#P#M#P#M#P#M.K#M.K#M#M.L#R#J#M#J#M#J.L#J.L#J#J.O#J#E#J#E.O#E#U#E",
"#F#p#F#H#F#F#F#H#t#F#t#F#K#t#K#t#K#t#K#K#x#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q"
"#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B#L.B#T.B#L.B#L#T#L#T#L#T#L#T.F#L#P#T#P#L#P.F#P#P#P#P.K#P#M#P#R#P#M.K#M#R#M#R.L#R#J#R#J#M#J.L#J#J#J#J.O#J#U#J#U.O",
"#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#K#F#K#F#K#t#K#t#K#K#x#K#x#K#x#K#x#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#q#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B#V.B#L.B#L.B#L.B#L#T#L#T#L#L#L#T#P#L#P#L#P.F#P.F#P#P#P#P.I#P.I#P#M#P#M.K#M#R#M#R.L#M.L#M#J#M#J.L#J#J#J#J.N#J.O#J#E.O#E.O#E",
"#F#C#F#H#F#H#F#F#t#F#t#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w"
"#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B#W.B#T.B#L.B#L.B#L.D#L#T#L#T#L#T#P#T#P#L#P#L#P#P#P#P#P#P.K#P.K#P#M#P#M.K#M#R#M#R.L#R.L#M#J#R#J#J#J#J#J#J.O#J#U#J#E.O",
"#C#F#C#F#C#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#k#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B.7.B.B.B#L.B#L.B#L#W#L#T#L#T#L#T#L#L.F#L#P#L#P.F#P#P#P#P#N#P.K#P#M#P#M#P#M.K#M#R#M#M.L#R#J#M#J#M#J.L#J#J#J#J.O#J#E#J#E.O#E#U#E",
"#F#p#F#p#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#S#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q"
"#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B.B.B.B.B#L.B#T.B#L.B#L#T#L#T#L#T#L#T.F#L#P#T#P#L#P.F#P#P#P#P.K#P#M#P#R#P#M.K#M#R#M#R.L#R#J#R#J#M#J.L#J#J#J#J.O#J#U#J#U.O",
"#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#K#F#K#F#K#t#K#t#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#j#Q#j#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B.B.B.B.B#V.B#L.B#L.B#L.B#L#T#L#T#L#L#L#T#P#L#P#L#P.F#P#P#P#P#P#P.I#P#M#P#M#P#M.K#M#R#M#R.L#M.L#M#J#M#J.L#J#J#J#J.N#J#U#J#E.O#E#U#E",
"#F#C#F#p#F#H#F#F#F#F#t#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w"
"#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B#W.B#T.B#L.B#L#W#L#T#L#T#L#T#L#T#P#T#P#L#P#L#P#P#P#P#P#P.K#P#R#P#M#P#M#R#M#R#M#R.L#R.L#M#J#R#J#J#J#J#J#J#U#J#U#J#E.O",
"#C#F#C#F#C#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#k#q#j#q#k#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B#V.B#W.B#L.B#L.B#L#W#L#T#L#T#L#T#X#L#P#L#P#L#P.F#P#P#P#P#N#P.K#P#M#P#M#P#M#R#M#R#M#M.L#R#J#M#J#M#J#J#J#J#J#J.O#J#E#J#E.O#E#U#E",
"#F#p#F#p#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q"
"#q#w#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B#W.B.B.B#L.B#T.B#L.B#L#T#L#T#L#T#X#T#P#L#P#T#P#X#P#P#P#P#P#P.K#P#M#P#R#P#M.K#M#R#M#R.L#R#J#R#J#M#J.L#J#J#J#J.O#J#U#J#U#U",
"#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#q#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.A.B.B.B.B.B.B#V.B#L.B#L#W#L#W#L#T#L#T#L#L#L#L#P#L#P#L#P#P#P#P#P#P#P#P#M#P#M#P#M#P#M.K#M#R#M#R#J#M#J#M#J#M#J.L#J#J#J#J#E#J#U#J#E#U#E#U#E",
"#F#C#F#p#F#H#F#F#F#F#t#F#K#F#K#t#K#t#K#t#K#K#I#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w"
"#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B.B.B.B.B#W.B#T.B#L.B#L#W#L#T#L#T#L#T#L#T#P#T#P#L#P.F#P#P#P#P#P#P#R#P#R#P#M#P#M#R#M#R#M#R#J#R#J#M#J#R#J#J#J#J#J#J#U#J#U#J#E.O",
"#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#K#K#K#x#K#x#K#x#K#Q#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.A.B.B.B.B#V.B#W.B#L.B#L.B#L#W#L#T#L#T#L#T#X#L#P#L#P#L#P.F#P#P#P#P#N#P.K#P#M#P#M#P#M#R#M#R#M#M.L#R#J#M#J#M#J#J#J#J#J#J.O#J#E#J#E.O#E#U#E",
"#F#C#F#p#F#H#F#H#F#F#t#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q"
"#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B#W.B#W.B#L.B#T#W#L#W#L#T#L#T#L#T#X#T#P#L#P#L#P#P#P#P#P#P#P#P.K#P#M#P#R#P#M#R#M#R#M#R.L#R#J#R#J#Y#J#J#J#J#J#J#U#J#U#J#U#U",
"#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#t#F#K#F#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#k#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#j#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B#W.B#W.B#V.B#L.B#L#W#L#W#L#T#L#T#X#L#P#L#P#L#P#L#P#P#P#P#P#P#P#P#M#P#M#P#M#Z#M#R#M#R#M#R#J#M#J#M#J#Y#J#J#J#J#J#J#E#J#U#J#E#U#E#U#E",
"#F#C#F#p#F#H#F#F#F#H#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#x#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#w#q#q#q#w"
"#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B#W.B#W.B#W.B#T.B#L.B#L#W#L#T#L#T#X#T#X#T#P#T#P#L#P#P#P#P#P#P#P#P#R#P#R#P#M#P#M#R#M#R#M#R#J#R#J#M#J#R#J#J#J#J#J#J#U#J#U#J#E#U",
"#C#H#C#F#C#F#p#F#p#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#x#K#x#K#x#K#x#K#O#K#O#x#O#x#O#x#j#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q"
"#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B#W.B.B.B#V.B#L.B#L#W#L#W#L#T#L#T#L#L#L#T#P#L#P#L#P#X#P#P#P#P#P#P#0#P#R#P#M#P#M#P#M#R#M#R#M#M#J#R#J#M#J#M#J#J#J#J#J#J#U#J#E#J#E#U#E#U#E",
"#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q"
"#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.B.z.B.A.B.A.B.A.B.B.B.B#W.B#W.B#L.B#T#W#L#W#L#T#L#T#L#T#X#T#P#L#P#L#P#P#P#P#P#P#Z#P#R#P#M#P#R#Z#M#R#M#R#M#R#J#R#J#R#J#Y#J#J#J#J#1#J#U#J#U#J#U#U",
"#C#F#C#F#C#F#C#F#p#F#H#F#H#t#H#t#F#K#H#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j"
"#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A.B.B.B.B#V.B#W.B#L.B#L.B#L#W#L#T#L#T#L#T#X#L#P#L#P#L#P#L#P#P#P#P#P#P#P#P#M#P#M#P#M#Z#M#R#M#R#M#R#J#M#J#M#J#Y#J#J#J#J#J#J#E#J#U#J#E#U#E#U#E",
"#F#C#F#p#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#q"
"#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.A.B.B#W.B#W.B#W.B#T#W#L#W#L#W#L#T#L#T#X#T#P#L#P#T#P#X#P#P#P#P#P#P#P#P#R#P#R#P#M#P#M#R#M#R#M#R#J#R#J#M#J#Y#J#J#J#J#J#J#U#J#U#1#E#U",
"#C#H#C#F#C#F#C#F#p#F#H#F#H#F#H#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j"
"#q#j#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B#W.B#W.B#V.B#L.B#L#W#L#W#L#T#L#T#X#L#X#T#P#L#P#L#P#X#P#P#P#P#P#P#0#P#R#P#M#Z#M#Z#M#R#M#R#Y#M#J#R#J#M#J#Y#J#J#J#J#1#J#U#J#E#1#E#U#E#U#E",
"#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#t#K#t#K#K#2#K#K#K#I#K#O#K#O#x#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q"
"#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B#W.B#W.B#W.B#T.B#L.B#L#W#L#T#L#T#X#T#X#T#P#T#P#L#P#X#P#P#P#P#P#P#Z#P#R#P#M#P#M#Z#M#R#M#R#Y#R#J#R#J#R#J#Y#J#J#J#J#1#J#U#J#E#J#U#U",
"#C#B#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#K#K#K#x#K#x#K#x#K#Q#K#O#K#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j"
"#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A#W.B#W.B#V.B#W.B#L#W#L#W#L#W#L#T#L#T#L#T#X#L#P#L#P#X#P#X#P#P#P#P#Z#P#Z#P#M#P#M#P#M#R#M#R#M#R#M#R#J#M#J#M#J#Y#J#J#J#J#J#J#E#J#E#J#E#U#E#U#3",
"#F#C#F#C#F#C#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q"
"#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A.B.B.B.B#W.B#W.B#L.B#T#W#L#W#L#T#L#T#L#T#X#T#P#L#P#T#P#X#P#P#P#P#Z#P#Z#P#R#P#R#Z#M#R#M#R#M#R#M#R#J#R#J#Y#J#J#J#J#J#J#1#J#U#J#U#1#E#U",
"#C#C#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#t#F#K#F#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j"
"#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A.B.A.B.A.B.A.B.B#W.B#W.B#V.B#L.B#L#W#L#W#L#T#L#T#X#L#X#T#P#L#P#L#P#X#P#P#P#P#P#P#0#P#M#P#M#Z#M#R#M#R#M#R#Y#M#J#R#J#Y#J#J#J#J#J#J#1#J#U#J#E#1#E#U#E#U#E",
"#F#C#F#C#F#p#F#H#F#F#F#H#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#q#q#q"
"#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A.B.A.B.B#W.B#W.B#W.B#T#W#L#W#L#W#L#T#L#T#X#T#X#T#P#T#P#X#P#X#P#P#P#P#P#P#Z#P#R#P#M#P#M#R#M#R#M#R#Y#R#J#R#J#R#J#J#J#J#J#J#1#J#U#J#E#1#U#U",
"#C#B#C#H#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j"
"#q#j#q#j#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.z.A.A.B.A.B.A.B.A#W.B#W.B#V.B#W.B#L#W#L#W#L#W#L#T#X#T#X#T#X#L#P#L#P#X#P#X#P#P#P#P#Z#P#Z#P#M#Z#M#Z#M#R#M#R#Y#R#Y#R#J#M#J#M#J#J#J#J#1#J#1#J#E#J#E#1#E#U#E#U#3",
"#F#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#j#q#O#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q"
"#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.A.B.A#W.B#W.B#W.B#W.B#L.B#T#W#L#W#L#T#X#T#X#T#X#T#P#X#P#4#P#X#P#P#P#P#Z#P#Z#P#R#P#R#Z#M#R#M#R#Y#R#Y#R#J#R#J#Y#J#J#J#J#1#J#1#J#U#J#U#1#E#U",
"#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#j#Q#O#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j"
"#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A#W.A#W.B#W.B#W.B#V#W#L#W#L#W#L#W#X#T#X#T#X#L#X#T#P#X#P#X#P#X#P#P#Z#P#Z#P#M#P#M#P#M#Z#M#R#M#R#M#R#Y#M#J#M#J#Y#J#J#J#J#J#J#1#J#U#J#E#1#E#U#3#U#3",
"#F#C#F#C#F#p#F#H#F#F#F#H#F#F#t#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#O#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#D#D#D#D#D#D#D#D#D#D#D#D#D#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#w#q#q#q#w#q#q#q#q#q#q"
"#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A#W.A#W.B#W.B#W.B#W.B#T#W#L#W#L#W#L#T#L#T#X#T#X#T#P#T#P#X#P#X#P#P#Z#P#Z#P#Z#P#R#Z#M#Z#M#R#M#R#M#R#Y#R#J#Y#J#5#J#J#J#J#1#J#1#J#U#1#E#1#U#U",
"#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j"
"#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A#W.A#W.A#W.B#W.B#V#W#W#W#L#W#L#W#L#W#L#T#X#T#X#T#X#X#P#X#P#X#P#X#P#P#P#P#0#P#R#P#M#Z#M#Z#M#R#M#R#Y#R#Y#R#J#Y#J#Y#J#J#J#J#1#J#1#J#E#1#E#1#E#U#E#U#3",
"#8#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q"
"#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A.B.A.B.A#W.B#W.B#W.B#W#W#L#W#T#W#L#W#L#T#X#T#X#T#X#T#P#X#P#4#P#X#P#P#P#P#Z#P#R#P#M#P#R#Z#M#R#M#R#Y#R#Y#R#J#R#J#Y#J#J#J#J#1#J#U#J#U#1#U#1#E#U",
"#C#C#C#B#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j"
"#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A.B.A.B.A#W.B#W.B#W.B#W.B#V#W#L#W#L#W#L#W#X#T#X#T#X#L#X#L#P#X#P#X#P#P#P#P#Z#P#Z#P#M#Z#M#Z#M#Z#M#R#Y#R#Y#R#Y#M#J#M#J#Y#J#J#1#J#1#J#E#J#U#1#E#1#E#U#3#U#3",
"#F#C#F#C#F#C#F#C#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#O#q#O#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q"
"#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A.B.z.B.A#W.A#W.B#W.B#W#W#W#W#T#W#L#W#L#W#X#T#X#T#X#T#X#4#P#4#P#X#P#X#P#P#Z#P#Z#P#R#P#R#Z#M#Z#M#R#Y#R#Y#R#Y#R#J#Y#J#5#J#J#1#J#1#J#U#J#U#1#E#1#E#U",
"#C#6#C#C#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j"
"#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A.B.A.B.A#W.A#W.B#W.B#W.B#V#W#L#W#L#W#L#W#X#T#X#T#X#T#X#T#P#X#P#X#P#X#P#P#Z#P#Z#P#0#P#R#P#M#Z#M#Z#M#R#M#R#Y#M#J#R#J#Y#J#Y#J#J#J#J#1#J#U#J#E#1#E#1#E#U#3#U#3",
"#6#C#F#C#F#C#F#p#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q"
"#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A#W.A#W.B#W.B#W.B#W.B#W#W#L#W#T#W#X#W#X#T#X#T#X#T#X#T#P#X#P#X#P#P#Z#P#Z#P#Z#P#R#Z#M#Z#R#Z#M#R#M#R#Y#R#J#5#J#5#J#Y#J#J#J#J#1#J#U#1#U#1#U#1#3#U",
"#C#C#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j"
"#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A#W.A#W.A#W.B#W.B#V#W#W#W#L#W#L#W#L#W#L#T#X#T#X#T#X#X#P#X#P#X#P#X#Z#P#Z#P#Z#P#Z#P#M#Z#M#Z#M#Z#M#R#Y#R#Y#R#J#Y#J#Y#J#Y#J#J#1#J#1#J#E#1#U#1#E#1#E#U#3#U#3",
"#6#C#8#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q"
"#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.z.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.B.A#W.A#W.A#W.B#W.B#W.B#W#W#T#W#T#W#L#W#L#T#X#T#X#T#X#T#P#X#P#4#P#X#P#P#P#P#Z#P#Z#P#R#Z#R#Z#M#Z#M#R#Y#R#Y#R#J#R#J#Y#J#5#J#J#1#J#1#J#U#J#U#1#E#U#E#U",
"#C#C#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k"
"#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.A.A#W.A#W.A#W.A#W.B#W#W#W#W#V#W#L#W#L#W#L#W#X#T#X#T#X#L#X#T#P#X#P#X#P#X#P#P#Z#P#Z#P#0#Z#R#Z#M#Z#M#Z#Y#R#Y#R#Y#M#J#R#J#Y#J#Y#1#J#1#J#1#J#U#1#E#1#E#U#3#U#3#U#3",
"#6#C#F#C#F#C#F#C#F#C#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#O#K#O#x#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#q#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q"
"#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.z.A.z.A.A.B.z.B.A#W.A#W.B#W.B#W#W#W#W#T#W#L#W#L#W#X#T#X#T#X#T#X#4#P#4#P#X#P#X#P#P#Z#P#Z#P#Z#P#R#Z#M#Z#R#Z#Y#R#Y#R#Y#R#J#5#J#5#J#Y#1#J#1#J#1#J#U#1#U#1#U#U#3#U",
"#C#C#C#6#C#C#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#K#K#K#K#K#K#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#D#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j"
"#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A#W.A.B.A#W.A#W.A#W.B#W.B#V#W#W#W#L#W#L#W#X#W#X#T#X#T#X#T#X#X#P#X#P#X#P#X#Z#P#Z#P#Z#P#Z#P#M#Z#M#Z#M#R#M#R#Y#R#Y#R#J#Y#J#Y#J#Y#J#J#1#J#1#J#E#1#U#1#E#U#3#U#3#U#3",
"#6#C#8#C#F#C#F#C#F#p#F#H#F#F#F#H#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#O#q#O#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q"
"#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.z.A.z.B.A#W.A#W.A#W.B#W#W#W#W#W#W#L#W#T#W#X#W#X#T#X#T#X#4#X#4#P#X#P#4#P#X#Z#P#Z#P#Z#P#Z#Z#R#Z#R#Z#M#R#Y#R#Y#R#Y#5#J#5#J#Y#J#Y#J#J#1#J#1#1#U#1#U#1#3#U#3#U",
"#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#Q#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k"
"#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.z.A.A.A.z.A.A.B.A#W.A#W.A#W.A#W.B#W#W#W#W#V#W#L#W#X#W#X#W#X#T#X#T#X#X#X#4#P#X#P#X#Z#X#Z#P#Z#P#Z#P#0#Z#M#Z#M#Z#M#R#Y#R#Y#R#Y#Y#J#5#J#Y#J#J#1#J#1#J#1#1#U#1#E#1#E#U#3#U#3#U#A",
"#6#C#6#C#8#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q"
"#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.z.A.z.A.A#W.z#W.A#W.A#W.B#W.B#W#W#W#W#T#W#X#W#X#W#X#T#X#T#X#T#X#4#P#4#P#X#Z#X#Z#P#Z#P#Z#Z#Z#Z#R#Z#M#Z#M#R#Y#R#Y#R#Y#R#J#5#J#5#J#J#1#J#1#J#1#J#U#1#E#1#U#U#3#U",
"#9#C#C#C#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j"
"#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.z.A.A.A.A#W.A#W.A#W.A#W.B#W#W#W#W#V#W#V#W#X#W#L#W#X#W#X#T#X#4#X#4#X#X#P#X#P#X#P#X#Z#P#Z#P#0#Z#R#Z#M#Z#M#Z#Y#R#Y#R#Y#R#Y#5#J#Y#J#Y#1#J#1#J#1#J#1#1#E#1#E#1#3#U#3#U#3a.#A",
"#6#C#6#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#K#K#K#2#K#K#K#I#K#O#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q"
"#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.A#W.z#W.A#W.A#W.A#W.B#W#W#W#W#W#W#L#W#T#W#X#W#X#T#X#T#X#4#X#4#P#X#P#4#P#X#Z#P#Z#P#Z#P#R#Z#M#Z#R#Z#Y#R#Y#R#Y#R#Y#5#J#5#J#Y#1#J#1#J#1#J#1#1#U#1#U#1#3#U#3#U",
"#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k"
"#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.z.A.A.A.za#.A#W.A#W.A#W.A#W#W#W#W#V#W#W#W#L#W#L#W#X#W#X#T#X#T#X#T#X#X#P#X#P#X#P#X#Z#P#Z#P#Z#Z#Z#Z#M#Z#M#Z#Y#Z#Y#R#Y#R#Y#R#Y#Y#J#Y#J#Y#J#J#1#J#1#J#1#1#U#1#E#1#3#U#3#U#3#U#A",
"#C#C#6#C#8#C#F#C#F#C#F#p#F#H#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#D#y#D#z#D#y#D#z#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q"
"#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.x.A.z.A.z.A.za#.z#W.A#W.A#W.A#W.B#W#W#W#W#W#W#W#W#T#W#X#W#X#W#X#T#X#4#X#4#X#4#P#4#P#X#Z#P#Z#P#Z#P#Z#Z#R#Z#R#Z#Y#Z#Y#R#Y#R#Y#5#Y#5#J#Y#J#5#1#J#1#J#1#1#U#1#U#1#3#1a.#U#3#U",
"#9#C#C#C#C#C#C#6#C#8#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#K#F#K#F#K#F#K#F#K#t#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k"
"#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.za#.Aa#.A#W.A#W.A#Wa##W#W#W#W#W#W#V#W#L#W#X#W#X#W#X#T#X#T#X#X#X#4#P#X#P#X#Z#X#Z#P#Z#P#Z#P#0#Z#R#Z#M#Z#M#Z#Y#R#Y#R#Y#Y#J#5#J#Y#J#Y#1#J#1#J#1#1#U#1#E#1#E#1#3#U#3#U#3a.#A",
"#6#C#6#C#6#C#8#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#O#q#O#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#D#y#D#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q"
"#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.x.A.z.A.za#.z.A.A#W.z#W.A#W.A#W#W#W#W#W#W#W#W#T#W#X#W#X#W#X#T#X#4#X#4#X#4#P#4#P#X#Z#X#Z#P#Z#P#Z#Z#Z#Z#R#Z#M#Z#R#Z#Y#R#Y#R#Y#R#J#5#J#5#J#Y#1#J#1#J#1#1#U#1#U#1#U#1#3#U#3a.",
"aa#C#9#C#C#C#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#Q#O#Q#O#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k"
"#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.x.A.z.A.za#.Aa#.A#W.A#W.A#W.A#W.A#W#W#W#W#V#W#W#W#X#W#X#W#X#W#X#T#X#4#X#4#X#X#P#X#Z#X#Z#X#Z#P#Z#P#Z#Z#Z#Z#M#Z#M#Z#Y#Z#Y#R#Y#5#Y#5#J#Y#J#Y#1#Y#1#J#1#J#1#1#E#1#U#1#3#1#3#U#3a.#3a.#A",
"#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#K#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q"
"#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.z.A.z.A.za#.z#W.A#W.A#Wa##W#W#W#W#W#W#W#W#X#W#4#W#X#W#X#T#X#T#X#4#X#4#P#X#Z#4#Z#X#Z#P#Z#P#Z#Z#Z#Z#R#Z#R#Z#Y#Z#Y#R#Y#R#Y#5#J#5#J#Y#1#5#1#J#1#J#1#1#U#1#U#1#3#U#3#U#3a.",
"aa#C#9#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k"
"#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.za#.A.A.za#.A#W.A#W.A#W.A#W#W#W#W#W#W#W#Wab#W#X#W#X#W#X#W#X#4#X#4#X#X#X#4#P#X#P#X#Z#X#Z#P#Z#Z#Z#Z#0#Z#M#Z#Y#Z#Y#R#Y#R#Y#R#Y#Y#J#5#J#Y#1#Y#1#J#1#J#1#1#U#1#E#1#3#1#3#U#3#U#3a.#A",
"#6#C#C#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#O#K#O#K#O#K#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q"
"#j#q#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.z.A.z.A.za#.z#W.A#W.z#Wa##W.A#W#W#W#W#W#W#W#W#T#W#X#W#X#W#X#T#X#4#X#4#X#4#P#4#P#X#Z#X#Z#P#Z#P#Z#Z#Z#Z#R#Z#Y#Z#Y#R#Y#R#Y#5#Y#5#J#5#J#5#1#Y#1#J#1#1#1#1#U#1a.#1a.#U#3#U#3a.",
"aa#Caa#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k"
"#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.z.A.z.A.za#.Aa#.A#W.A#W.A#Wa##W#W#W#W#W#Wab#W#L#W#X#W#X#W#Xac#X#4#X#4#X#4#X#X#P#X#Z#X#Z#X#Z#Z#Z#Z#Z#Z#R#Z#M#Z#Y#Z#Y#R#Y#R#Y#5#Y#5#J#Y#J#Y#1#J#1#J#1#1#1#1#E#1#E#1#3#U#3#U#3a.#3a.#A",
"#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q"
"#j#j#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.x.A.z.A.za#.za#.A#W.z#W.A#W.A#W#W#W#W#W#W#W#W#W#W#X#W#4#W#X#W#X#4#X#4#X#4#X#4#P#X#Z#4#Z#X#Z#P#Z#Z#Z#Z#Z#Z#M#Z#5#Z#Y#R#Y#5#Y#5#Y#5#J#5#J#Y#1#J#1#J#1#1#1#1#U#1#U#1#3#U#3a.#3a.",
"aa#Caa#C#9#C#C#C#C#C#C#F#C#F#C#F#C#F#C#F#7#F#H#F#H#F#F#t#H#K#F#K#F#K#F#K#F#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k"
"#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.z.A.za#.Aa#.za#.A#W.A#Wa##Wa##W#W#W#W#V#W#W#W#X#W#X#W#X#W#X#T#X#4#X#4#X#X#P#X#Z#X#Z#X#Z#P#Z#P#Z#Z#Z#Z#M#Z#M#Z#Y#Z#Y#R#Y#5#Y#5#Y#Y#J#Y#1#Y#1#J#1#1#1#1#1#1#U#1#3#1#3#U#3a.#3a.#A#3#A",
"#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#O#j#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#y#y#z#y#y#y#z#y#y#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j"
"#k#j#j#j#k#j#j#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.x.A.z.A.za#.za#.za#.z#W.A#W.A#Wa##W#W#W#W#W#W#W#W#X#W#4#W#X#W#Xac#X#4#X#4#X#4#X#4#Z#4#Z#X#Z#P#Z#Z#Z#Z#Z#Z#R#Z#R#Z#Y#Z#Y#R#Y#R#Y#5#Y#5#J#Y#1#5#1#J#1#J#1#1#1#1#U#1#3#1a.#U#3a.#3a.",
"aa#Caa#C#9#C#9#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#k#j#Q#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k"
"#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.x.A.x.A.za#.Aa#.za#.A#W.A#Wa##Wa##Wa##W#W#W#W#W#Wab#W#X#W#X#W#X#W#X#4#X#4#X#X#X#4#Z#X#Z#X#Z#X#Z#P#Z#Z#Z#Z#0#Z#R#Z#Y#Z#Y#Z#Y#R#Y#5#Y#5#Y#5#1#Y#1#Y#1#J#1#J#1#1#1#1#3#1#3#1#3#U#3a.#3a.#Aa.ad",
"#C#9#6#C#C#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#K#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#j#Q#O#Q#j#O#j#Q#j#O#q#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j"
"#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.x.A.x.A.za#.za#.za#.za#.A#W.z#Wa##Wa##W#W#W#W#W#Wac#W#4#W#X#W#X#W#X#T#X#4#X#4#X#4#Z#4#Z#X#Z#X#Z#P#Z#Z#Z#Z#Z#Z#R#Z#Y#Z#5#Z#Y#R#Y#5#Y#5#J#5#1#5#1#Y#1#J#1#1#1#1#U#1a.#1a.#1#3#U#3a.#3a.",
"aa#Caa#Caa#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#K#O#x#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k"
"#j#k#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.x.A.x.A.xa#.za#.za#.Aa#.A#Wa##Wa##Wa##W#W#W#W#W#Wab#Wac#W#X#W#X#W#Xac#X#4#X#4#X#4ae#X#Z#X#Z#X#Z#X#Z#Z#Z#Z#Z#Z#Z#Z#Y#Z#Y#Z#Y#Z#Y#R#Y#5#Y#5#J#Y#J#Y#1#Y#1#J#1#1#1#1#E#1#U#1#3#1#3#U#3a.#3a.ad#3ad",
"#C#9#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#p#F#H#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#O#K#O#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#q#O#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j"
"#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.xa#.za#.za#.za#.A#W.z#Wa##Wa##Wa##W#W#W#Wac#Wac#W#X#W#4#W#X#W#X#4#X#4#X#4#X#4#P#X#Z#4#Z#X#Z#P#Z#Z#Z#Z#Z#Z#5#Z#5#Z#Y#Z#Y#5#Y#5#Y#5#J#5#J#Y#1#5#1#1#1#1#1#1#U#1a.#1#3#1#3a.#3a.#3a.",
"aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#8#C#F#C#F#C#F#C#F#H#F#H#F#H#t#H#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#O#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k"
"#j#k#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.x.A.x.A.xa#.za#.za#.Aa#.za#a##W.A#Wa##Wa##W#W#W#Wab#Wac#W#X#W#X#W#Xac#Xac#X#4#X#4#X#X#X#4#Z#X#Z#X#Zae#Z#Z#Z#Z#Z#Z#0#Z#Y#Z#Y#Z#Y#R#Y#5#Y#5#Y#Y#J#5#1#Y#1#Y#1#1#1#1#1#1#U#1#3#1#3#1#3a.#3a.#3a.#Aa.ad",
"#Caa#6#9#C#C#6#C#6#C#8#C#8#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#2#K#K#K#I#K#O#K#O#x#O#K#O#x#O#x#O#Q#O#Q#j#Q#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j"
"#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.xa#.x.A.za#.za#.za#.z#Wa##Wa##Wa##W#W#W#Wac#Wac#Wac#W#4#W#X#W#Xac#X#4#X#4#X#4#X#4#Z#4#Z#X#Z#X#Z#Z#Z#Z#Z#Z#Z#Z#5#Z#Y#Z#Y#R#Y#5#Y#5#Y#5#J#5#1#5#1#Y#1#J#1#1#1#1#U#1a.#1a.#U#3a.#3a.#3a.",
"aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#F#C#F#C#F#C#F#C#F#7#F#H#F#H#F#F#t#H#K#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k"
"#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.x.A.x.A.xa#.xa#.za#.Aa#.za#.A#W.A#Wa##Wa##Wa##W#Wac#Wac#Wab#W#X#W#Xac#Xac#X#4#X#4#X#X#X#4#Z#X#Z#X#Z#X#Z#P#Z#Z#Z#Z#0#Z#R#Z#Y#Z#Y#Z#Y#5#Y#5#Y#5#Y#5#1#Y#1#Y#1#J#1#1#1#1#1#1#3#1a.#1#3#U#3a.#3a.#3a.ad#3ad",
"#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#z#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j"
"#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.xa#.x.A.za#.za#.za#.za#a##Waf#Wa##Wa##W#W#W#W#W#Wac#W#4#W#X#W#Xac#X#4#X#4#X#4#X#4ae#4#Z#X#Z#X#Zae#Z#Z#Z#Z#Z#Z#R#Z#Y#Z#5#Z#Y#R#Y#5#Y#5#Y#5#1#5#1#Y#1#J#1#1#1#1#1#1a.#1a.#1#3#U#3a.#3a.#Aa.",
"aa#Caa#Caa#Caa#C#9#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s"
"#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.x.A.x.A.xa#.xa#.xa#.za#.za#.Aa#.A#Wa##Wa##Wa##Wa#ac#W#W#Wab#Wac#W#X#W#X#W#Xac#X#4#X#4#X#4ae#X#Z#X#Z#X#Z#X#Z#Z#Z#Z#Z#Z#Z#Z#Y#Z#Y#Z#Y#Z#Y#5#Y#5#Y#5ag#Y#1#Y#1#Y#1#J#1#1#1#1#1#1a.#1#3#1#3#U#3a.#3a.ad#3ada.ad",
"#Caa#Caa#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#t#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#x#O#Q#O#Q#j#O#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j"
"#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.x.A.x.A.xa#.za#.za#.za#af#W.z#Wa##Wa##Wa##W#W#W#Wac#Wac#W#X#W#4ac#Xac#X#4#X#4#X#4ae#4#Z#X#Z#4#Zae#Z#Z#Z#Z#Z#Z#Z#Z#5#Z#5#Z#Y#Z#Y#5#Y#5#Y#5ag#5#1#Y#1#5#1#1#1#1#1#1ah#1a.#1#3#1a.a.#3a.#3a.ada.",
"aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s"
"#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.x.y.x.A.xa#.xa#.xa#.za#.za#.Aa#.za#a##Wa##Wa##Wa##W#W#W#Wab#Wac#Wab#W#X#W#Xac#Xac#X#4#X#4ae#Xae#4#Z#X#Z#X#Zae#Z#Z#Z#Z#Z#Zai#Z#5#Z#Y#Z#Y#Z#Y#5#Y#5#Y#Y#J#5#1#Y#1#Y#1#1#1#1#1#1#1#1#3#1#3#1#3a.#3a.#3a.#Aa.ad#3ad",
"#Caa#Caa#C#9#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#p#F#H#F#F#F#H#F#F#F#F#K#F#K#F#K#t#K#F#K#K#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#O#Q#O#Q#j#O#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j"
"#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.x.A.x.A.xa#.xa#.za#.za#.za#.z#Wa##Wa##Wa##W#W#W#Wac#Wac#Wac#W#4ac#Xac#Xac#X#4#X#4ae#4ae#4#Z#4#Z#X#Z#X#Z#Z#Z#Z#Z#Zaj#Z#5#Z#Y#Z#5aj#Y#5#Y#5#Y#5#J#5#1#5#1#Y#1#1#1#1#1#1#U#1a.#1a.ah#3a.#3a.aka.ada.",
"aa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#8#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#I#K#Q#K#O#K#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s"
"#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.x.y.x.A.xa#.xa#.xa#.xa#.za#.Aa#.za#a##Wa##Wa##Wa##Wa##W#Wac#Wac#Wab#W#X#W#Xac#Xac#X#4#X#4ae#4ae#4ae#X#Z#X#Zae#Zae#Z#Z#Z#Zaj#Zaj#Z#Y#Z#Y#Z#Y#5#Y#5#Y#5#Y#5#1#Y#1#Y#1ag#1#1#1#1#1#1#3#1a.#1#3ah#3a.#3a.#3a.ad#3ad#3ad",
"#9aa#Caa#Caa#6#C#C#C#6#C#6#C#8#C#F#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#j#Q#O#Q#j#O#j#Q#j#O#j#O#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j"
"#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.x.A.x.A.xa#.xa#.za#.za#.za#.za#a##Waf#Wa##Wa##W#Wac#Wac#Wac#W#4#W#X#W#Xac#Xac#X#4ae#4ae#4ae#4#Z#X#Z#4#Zae#Z#Z#Z#Zaj#Zaj#Z#5#Z#5aj#Y#5#Y#5#Y#5#Y#5#1#5#1#Y#1#5#1#1#1#1#1#1a.#1a.#1#3#1#3a.#3a.#3a.ada.",
"alaaaa#9aa#9aa#Caa#C#9#C#9#C#C#6#C#C#C#F#C#F#C#F#C#F#C#F#7#F#H#F#H#F#H#t#H#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#I#K#Q#x#O#K#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#z#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#y#q#z#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s"
"#j.s#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.y.x.y.xa#.xa#.xa#.xa#.xa#.za#.za#a#a#a##Wa##Wa##Wa##Wa#ac#Wac#Wab#Wac#W#Xac#Xac#Xac#X#4ae#4#X#4ae#X#Z#X#Zae#Zae#Z#Z#Z#Z#Z#Z#Z#Z#Y#Z#Y#Z#Yaj#Y#5#Y#5#Y#5ag#Y#1#5#1#Y#1ag#1#1#1#1ah#1a.#1#3#1#3a.#3a.#3a.aka.ada.adakad",
"#9aa#9aa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#2#K#K#K#I#K#I#K#O#K#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#O#j#O#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#z#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j"
".s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.A.x.z.xa#.xa#.xa#.za#.za#.za#af#Waf#Wa##Wa##Wa#ac#Wac#Wac#Wac#W#X#W#4ac#Xac#X#4#X#4#X#4ae#4#Z#X#Z#4#Zae#Z#Z#Z#Z#Z#Z#Z#Z#5#Z#5#Z#Y#Z#Y#5#Y#5#Y#5ag#5#1#5#1#5#1#1#1#1#1#1ah#1a.#1a.#1a.a.#3a.#3a.aka.ada.",
"aaaaal#9aa#Caa#Caa#Caa#C#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s"
"#j.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.y.xam.xa#.xa#.xa#.xa#.za#.za#a#a#afa#a##Wa##Wa##Wa#ac#Wac#Wab#Wac#Wabac#Xac#Xac#Xac#X#4#X#4ae#Xae#4#Z#X#Z#X#Zae#Z#Z#Z#Z#Z#Zai#Z#5#Z#Yaj#Yaj#Y#5#Y#5ag#5ag#5#1#Y#1#Y#1#1#1#1#1#1#1#1#3#1#3#1#3a.#3a.#3a.aka.ad#3adakad",
"#9aa#Caa#Caa#Caa#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#K#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j"
".s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.w.x.v.x.v.x.v.x.x.x.x.x.x.x.x.y.x.z.xa#.xa#.xa#.xa#.za#.za#afa#af#Wa##Wa##Wa#ac#Wac#Wac#Wac#Wac#W#4ac#Xac#Xac#X#4#X#4ae#4ae#4#Z#4#Zae#Zae#Z#Z#Z#Z#Z#Zaj#Z#5#Z#Y#Z#5aj#Y#5#Y#5ag#5ag#5#1#5#1ag#1#1#1#1#1#1ah#1a.#1a.ah#3a.#3a.aka.ada.ada.",
"anaaaaaaaa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#t#K#K#K#K#x#K#K#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#O#Q#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s"
"#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.x.y.x.x.xam.xa#.xa#.xa#.xa#.xa#.za#a#a#afa#a##Wa##Wa##Wa#aca#ac#Wac#Wac#Wabac#Xac#Xac#Xac#Xac#X#4ae#4ae#4ae#X#Z#X#Zae#Zae#Z#Z#Z#Zaj#Zaj#Z#Y#Z#Y#Z#Yaj#Y#5#Y#5#Y#5#1#Y#1#Y#1ag#1#1#1#1#1#1ah#1a.#1#3ah#3a.#3a.#3a.ad#3ada.adakad",
"#9aa#9aa#Caa#Caa#C#9#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#z#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j"
".s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.xa#.xa#.xa#.xa#.za#.za#afa#afa#a##Waf#Wa#aca#ac#Wac#Wac#Wac#W#4ac#Xac#4ac#Xac#X#4ae#4ae#4ae#4#Zae#Zao#Zae#Z#Z#Z#Zaj#Zaj#Z#5#Z#5aj#Yaj#Y#5#Y#5#Y#5#1#5#1ag#1ap#1#1#1#1#1#1a.#1a.ah#3aha.a.#3a.aka.ada.adaq",
"alaaalaaaa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#8#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#t#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#O#Q#j#Q#j#Q#j#Q#j#k#j#Q#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s"
"#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.x.y.x.x.xam.xam.xa#.xa#.xa#.xa#.za#afa#afa#a#a#a##Wa##Wa#aca#aca#ac#Wac#Wabacacac#Xac#Xac#Xac#X#4ae#4ae#4ae#X#Z#X#Zae#Zae#Z#Z#Z#Zaj#Zaj#Z#Y#Z#Y#Z#Yaj#Y#5#Y#5#Y#5ag#Y#1#5#1ag#1ag#1#1#1#1ah#1a.#1#3ah#3ah#3a.#3a.aka.ada.adakadadad",
"aaaa#9aa#9aa#Caa#Caa#6#C#C#C#6#C#6#C#8#C#8#C#F#C#F#C#F#H#F#H#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#t#K#K#K#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#O#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#y#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j"
".s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.w.v.v.x.v.x.v.x.v.x.x.x.x.x.x.x.x.x.xa#.xa#.xa#.xa#.xa#.za#afa#afa#a##Waf#Wa#aca#aca#ac#Wac#Wac#Wacac#Xac#4ac#Xac#X#4ae#4ae#4ae#4#Zao#Zao#Zae#Zae#Z#Zaj#Zaj#Z#5#Z#5aj#Yaj#Y#5#Y#5#Y#5ag#5#1#5#1#5#1ag#1#1#1#1ah#1a.#1a.#1a.ah#3a.#3a.aka.adaqadaq",
"anaaaaaaalaaaa#9aa#9aa#Caa#C#9#C#9#C#C#C#C#C#C#F#C#F#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#K#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#O#Q#j#Q#j#Q#j#Q#j#k#j#Q#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s"
"#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.x.x.x.x.xam.xam.xa#.xa#.xa#.xa#.xa#afa#afa#a#a#afa#a##Wa#aca#aca#ac#Wac#Wabacacacabac#Xac#Xac#Xacae#4ae#4ae#Xae#4#Zae#Zae#Zae#Z#Zaj#Zaj#Zai#Z#5#Z#Yaj#Yaj#Y#5#Y#5ag#5ag#5#1#Y#1ag#1#1#1#1ah#1ah#1#3#1a.#1#3a.#3a.aka.aka.adakadakadadad",
"aaaa#9aa#9aa#9aa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#t#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#j#Q#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j"
".s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.x.x.x.x.xam.xaf.xa#.xa#.xa#.za#afa#afa#afa#af#Wa#aca##Wa#ac#Wac#Wac#Wacacacac#4ac#Xac#Xacae#4ae#4ae#4ae#4#Z#4#Zae#Zae#Z#Zaj#Zaj#Zaj#Z#5#Z#Yaj#5aj#Y#5#Y#5ag#5ag#5#1#5#1ag#1#1#1#1ah#1ah#1a.#1a.ah#3a.#3a.aka.aka.ada.adak",
"anaaanaaaaaaal#9aa#Caa#Caa#Caa#C#9#C#9#C#C#6#C#C#C#F#C#F#C#F#C#F#C#F#7#F#H#F#H#F#H#t#H#t#F#K#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#k#j#Q#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s"
".s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.v.x.x.x.xam.xar.xam.xa#.xa#.xa#.xa#ara#afa#a#a#afa#a##Wa#aca#aca#aca#ac#Wacacac#Wabac#Xac#Xac#Xacae#4ae#4ae#4ae#4aeae#Zae#Zae#Zaeaj#Zaj#Zaj#Zaj#Z#Yaj#Yaj#Yaj#Y#5ag#5ag#5#1#Y#1#Y#1ag#1#1#1#1ah#1ah#1a.#1#3ah#3a.#3a.aka.ak#3ada.adakadadas",
"aaaaaaaa#9aa#Caa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#I#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j"
".s#j.s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.xam.xaf.xa#.xa#.xa#.xa#afa#afa#afa#afa#a#acaf#Wa#aca#ac#Wac#Wacacacac#4ac#Xac#4acaeacae#4ae#4ae#4ae#4#Zae#Zao#Zae#Z#Z#Z#Zaj#Zaj#Z#5#Z#5aj#Yaj#Y#5ag#5ag#5#1#5#1ag#1ap#1#1ah#1ah#1ah#1a.ah#3aha.a.aka.aka.ada.adaqadaq",
"anaaanaaanaaaaaaaa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#H#t#H#t#F#K#H#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#k#j#Q#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s"
".s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.xam.xar.xam.xa#.xa#.xa#.xa#ara#afa#afa#afa#a#a#a#aca#aca#aca#ac#Wac#Wac#Wabacacac#Xac#Xacaeacae#4ae#4ae#4aeae#Zae#Zae#Zae#Z#Z#Z#Zaj#Zaj#Z#Yaj#Yaj#Yaj#Y#5ag#5ag#5ag#Y#1#5#1ag#1ag#1#1#1#1ah#1a.#1#3ah#3ah#3a.#3a.aka.ada.adakadakadadas",
"aaaaaaaaaaaa#9aa#Caa#Caa#C#9#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#I#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#j#Q#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#w#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j"
".s#j.s#b.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.v.x.v.x.x.x.x.x.xar.xar.xa#.xa#.xa#.xa#ara#afa#afa#afa#a#acaf#Wa#aca#aca#ac#Wacacacacacac#Xac#4acaeacae#4ae#4ae#4ae#4#Zao#Zao#Zae#Zae#Z#Zaj#Zaj#Zaj#Z#5aj#Yaj#Y#5ag#5ag#5ag#5#1ap#1ap#1ag#1#1#1#1ah#1a.aha.aha.ah#3a.aka.aka.adaqadaqadad",
"ataaanaaalaaalaaaa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#H#t#H#t#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#k#j#Q#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#b.s.s.s.s.s"
".s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.q.q.o.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.v.x.v.x.v.x.v.x.xar.xar.xam.xam.xa#.xa#.xa#ara#ara#afa#afa#a#a#a#aca#aca#aca#aca#ac#Wac#Wabacacac#Xac#Xacaeacaeacae#4ae#4aeaeaeao#Zae#Zae#Zae#Z#Zaj#Zaj#Zaiaj#5aj#Yaj#Yaj#Y#5#Y#5ag#5ag#5#1ag#1ag#1ag#1#1ah#1ah#1#3aha.ah#3ah#3a.aka.aka.adakadakadadadadas",
"aaauaaaaaaaa#9aa#9aa#Caa#Caa#C#9#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#7#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#2#K#K#K#I#K#I#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#Q#j#Q#j#Q#j#O#j#Q#j#j#j#Q#j#j#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j"
".s#b.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.v.x.x.x.xar.xar.xa#.xaf.xa#.xa#ara#afa#afa#afa#afa#af#Wa#aca#aca#ac#Wacacacacacacacac#4ac#Xac#Xacae#4ae#4ae#4aeao#Zao#Zae#Zae#Z#Zaj#Zaj#Zaj#Z#5aj#Yaj#5aj#Y#5#Y#5ag#5ag#5#1ap#1ag#1#1#1#1ah#1ah#1a.#1a.ah#3a.#3a.aka.aka.adaqadakadav",
"ataaanaaanaaaaaaalaaaa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#j#k#j#k#j#j#j#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s"
".s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.v.v.x.v.x.v.x.v.x.var.xar.xam.xam.xam.xa#.xa#ara#ara#afa#afa#a#a#afawa#aca#aca#aca#aca#ac#Wacacacacabac#Xacaeacaeacae#4ae#4aeaoae#4aeae#Zae#Zae#Zaeaj#Zaj#Zaj#Zaj#Z#Yaj#Yaj#Yaj#Y#5ag#5ag#5agag#1ag#1ag#1#1ah#1ah#1ah#1a.ah#3ah#3a.aka.aka.akaqadaqadakadadasadas",
"aaauaaauaaaaaaaa#9aa#9aa#Caa#C#9#6#C#C#C#6#C#6#C#8#C#8#C#F#C#F#C#F#p#F#H#F#F#F#H#F#F#F#F#t#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#K#K#I#K#I#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#Q#j#j#j#Q#j#j#j#k#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b"
".s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.v.x.v.x.var.x.x.xar.xar.xam.xaf.xa#.xa#ara#ara#afa#afa#afa#af#Wa#acafaca#aca#acacac#Wacacacac#4ac#Xac#Xacaeacae#4ae#4aeaoaeao#Zae#Zao#Zaeaj#Zaj#Zaj#Zajaj#5aj#5aj#Yaj#Y#5ag#5ag#5#1#5#1ag#1ap#1#1ah#1ah#1ah#1a.ah#3aha.a.aka.aka.ada.adaqadaqadad",
"ataaataaanaaanaaaaaaalaaaa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#B#C#8#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#F#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#O#K#Q#x#O#x#O#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#Q#j#Q#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s"
".s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.var.xar.xam.xar.xam.xa#.xa#ara#ara#ara#afa#a#a#afawa#awa#aca#aca#aca#ac#Wacacacacabacacacaeac#Xacaeacae#4ae#4ae#4aeae#Zae#Zae#Zaeaj#Zaj#Zaj#Zaj#Z#Yaj#Yaj#Yaj#Y#5ag#5ag#5ag#Y#1ap#1ag#1agah#1ah#1ah#1ah#1#3ah#3ah#3a.aka.aka.ada.adakadakasadasadas",
"aaanaaaaaaaaaaaa#9aa#9aa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#H#F#F#F#H#F#F#F#F#K#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#I#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#Q#j#j#j#Q#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b"
".s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.var.v.x.xar.xar.xam.xaf.xa#.xa#ara#ara#afa#afa#afa#afa#a#acafaca#aca#acacac#Wacacacacacac#Xac#4acaeacae#4ae#4aeaoaeao#Zae#Zao#Zaeaj#Zaj#Zaj#Zajaj#5aj#5aj#Yaj#Y#5ag#5ag#5ag#5#1ap#1ap#1agah#1ah#1ah#1a.aha.aha.ahaka.aka.aka.adaqadaqadadasad",
"atauataaanaaanaaanaaaaaaal#9aa#Caa#Caa#Caa#C#9#C#9#C#C#C#C#C#C#F#C#F#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#I#K#Q#x#O#x#O#x#O#x#O#x#j#x#j#x#j#x#j#Q#j#Q#j#Q#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s"
".s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.v.x.v.x.v.x.v.x.var.var.xam.xar.xam.xam.xa#ara#ara#ara#afa#afa#afawa#awa#aca#aca#aca#aca#acacacacabacacac#Xac#Xacaeacaeacae#4ae#4aeaeaeao#Zae#Zaeajaeaj#Zaj#Zaj#Zaiaj#5aj#Yaj#Yajag#5ag#5ag#5ag#5#1ag#1ag#1ag#1#1ah#1ah#1#3aha.ah#3ah#3a.aka.aka.adakadaqadadasadasadas",
"aaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#H#t#F#F#F#K#F#K#F#K#F#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#I#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#x#j#Q#j#Q#j#Q#j#Q#j#O#j#Q#j#j#j#k#j#j#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s"
".s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.var.v.x.xar.xar.xar.xar.xa#.xa#ara#ara#ara#afa#afa#afa#afacafaca#aca#aca#ac#Wacacacacacac#4ac#4acaeacaeacae#4aeaoaeaoaeao#Zao#Zaeajaeaj#Zaj#Zajajajaj#5aj#Yaj#5ajag#5ag#5ag#5agap#1ap#1ag#1agah#1ah#1ahaha.aha.ah#3ahaka.aka.akaqadaqadakasavasad",
"ataaataaataaanaaanaaalaaaaaaaa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#C#F#C#F#H#F#H#F#H#F#H#F#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#K#K#x#K#x#K#x#K#x#K#I#K#Q#x#O#x#Q#x#O#x#O#x#j#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s"
".s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.var.v.x.var.var.xar.xar.xam.xam.xa#ara#ara#ara#ara#afa#afawa#awafawa#aca#aca#aca#acacacacacacacacabac#Xacaeacaeacae#4ae#4aeaeaeao#Zae#Zaeajaeajaeaj#Zaj#Zajajajaj#Yaj#Yajagajag#5ag#5ag#5agag#1ag#1ag#1#1ah#1ah#1ahaha.ah#3ah#3a.aka.aka.akaqadaqadakadadasadasadax",
"aaataaauaaanaaaaaaaa#9aa#9aa#Caa#Caa#C#9#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#F#K#t#K#t#K#K#K#K#I#K#x#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#Q#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b"
".s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.v.v.w.x.v.x.v.x.var.var.var.xar.xar.xar.xam.xafara#ara#ara#afa#afa#afa#afawafaca#aca#aca#aca#acacacacacacacac#4acaeacaeacae#4aeaoaeaoaeaoaeao#Zaeajaoajaeaj#Zaj#Zaj#Zajaj#5aj#5ajagajag#5ag#5agap#1ap#1ag#1ap#1#1ah#1ah#1ahaha.ah#3aha.a.aka.akaqakaqadaqadaqasadasad",
"atauataaataaataaanaaalaaalaaaa#9aa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#F#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#K#x#K#K#K#x#K#x#K#x#K#x#K#I#K#Q#x#O#x#Q#x#O#x#O#x#j#x#O#x#j#x#j#x#j#Q#j#Q#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p"
".s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.v.v.w.v.v.v.v.v.v.x.var.v.x.var.var.var.xar.xam.xam.xamara#ara#ara#ara#afa#afawa#awafawa#aca#aca#aca#acawacacacacacacabac#Xacaeacaeacaeacae#4aeaoaeaoaeae#Zaeajaeajaeaj#Zaj#Zajajajaj#Yaj#Yajagajag#5ag#5ag#5agag#1ap#1ag#1agah#1ah#1ahahahah#3ah#3ahaka.aka.akaqakaqadakadakasadasadasasax",
"aaataaanaaauaaauaaaaaaaa#9aa#9aa#Caa#Caa#C#9#C#9#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#I#K#x#K#I#K#x#K#I#K#O#K#O#x#O#x#O#x#O#x#O#x#O#x#j#x#j#Q#j#Q#j#Q#j#Q#j#Q#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s"
".s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.v.x.var.var.var.xar.xar.xar.xam.xafara#ara#ara#ara#afawafa#afawafawa#acafaca#aca#acacacacacacacacacacaeacaoacaeacaeaoae#4aeaoaeao#Zaeajaoajaeaj#Zaj#Zaj#Zajaj#5aj#5ajagajag#5ag#5agapagap#1ap#1ap#1agah#1ah#1ah#1a.aha.aha.ahaka.aka.aka.adaqadaqadakasadasas",
"atauatauataaataaanaaanaaaaaaalaaaa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#K#x#K#K#K#x#K#x#K#x#K#x#K#I#K#x#x#O#x#Q#x#O#x#O#x#j#x#O#x#j#x#j#x#j#x#j#Q#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#q#k#q#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p"
".s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.v.v.v.v.v.v.var.var.var.var.var.xar.xamarar.xamara#ara#ara#ara#ara#afawa#awafawa#awa#aca#aca#acawacacacacacacabacacacaeacaeacaeacae#4aeaoaeaoaeaeaeaoajaeajaeajaeaj#Zajajajajaiaj#5aj#Yaj#Yajag#5ag#5agapagap#1ag#1agahagah#1ah#1ahah#3aha.ahakahaka.aka.akaqadakadaqasadasadasasasasay",
"auataaataaanaaauaaauaaaaaaaa#9aa#9aa#Caa#Caa#C#C#C#C#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#I#K#x#K#I#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#O#x#O#x#j#x#j#x#j#Q#j#Q#j#Q#j#Q#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s.s"
".s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.x.w.x.var.var.var.xar.xar.xar.xam.xafara#ara#ara#ara#afawafa#afawafawa#acafaca#aca#acawacacacacacacacacaeacaoacaeacaeacae#4aeaoaeaoaeao#Zao#Zaeajaeaj#Zaj#Zajajajaj#5aj#Yaj#5ajag#5ag#5ag#5agap#1ap#1agah#1ah#1ah#1ahaha.aha.ahakahaka.aka.akaqadaqadakasavasadasas",
"atauatauatauataaataaanaaanaaaaaaalaaaa#9aa#9aa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#C#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#K#x#K#K#K#x#K#x#K#x#K#x#K#x#K#x#x#O#x#Q#x#O#x#O#x#j#x#O#x#j#x#j#x#j#x#j#x#j#Q#j#k#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p"
".s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.u.t.t.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.w.v.v.v.w.v.v.v.var.var.var.var.var.xar.xamarar.xamara#ara#ara#ara#ara#afawafawafawa#awa#aca#aca#acawacawacacacacabacacacaeacaeacaeacaeacaeaoaeaoaeaeaeaoajae#Zaeajaeaj#Zajajajajajajajaj#Yaj#Yajagajag#5agapagap#1ag#1agahagah#1ah#1ah#1ahaha.ah#3ahaka.aka.aka.akaqadaqasakasadasadasasaxasax",
"auatauataaazaaanaaaaaaaaaaaa#9aa#9aa#9aa#Caa#Caa#6#C#C#C#6#C#6#C#6#C#8#C#F#C#F#C#F#C#F#C#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#I#K#x#K#I#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#O#x#O#x#j#x#j#x#j#x#j#Q#j#Q#j#Q#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s"
".s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.t.t.u.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.w.v.v.v.w.x.var.var.var.var.xar.xararar.xarara#ara#ara#ara#arawafa#afawafawafawafaca#aca#acawacacacacacacacacaAacaoacaeacaeacae#4aeaoaeaoaeao#Zao#Zaeajaeajaeaj#Zajajajajajaj#5aj#5ajagajag#5ag#5agap#1ap#1agahapah#1ah#1ahahahaha.ahakahaqa.aka.akaqakaqadaqasaqasadasadasas",
"atazatauatauatauataaataaanaaanaaaaaaalaaaa#Caa#Caa#Caa#Caa#C#9#C#C#C#C#C#C#6#C#7#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#H#t#H#t#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#t#x#K#K#K#x#K#x#K#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#Q#x#O#x#O#x#j#x#j#x#j#x#j#x#j#x#j.s#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p"
".s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.waB.v.v.vaB.var.var.var.var.var.xamarararamaramara#ara#ara#ara#arawafawafawa#awafawa#aca#acawacawacacacacacacacacaCacaeacaeacaeacaeaoaeaoaeaoaeaoaeae#Zaeajaeajaeaj#Zaj#Zajajajaj#Yaj#Yajagajag#5agapagapagag#1ap#1agahagah#1ah#1ahahahah#3ah#3ahaka.aka.akaqakaqadakadakasadasadasasaxasaD",
"auatauatauataaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#B#C#8#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#x#K#I#K#I#K#O#x#O#x#O#x#O#x#O#x#O#x#j#x#j#x#j#x#j#x#j#Q#j#Q#j#k#j#Q#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#j#j#j#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#q#j#j#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s"
".p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.t.t.u.t.w.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.vaB.w.x.var.var.var.var.xar.xararar.xarara#arafara#ara#arawafa#afawafawafawafacawaca#acawacawacacacacacacaAacaoacaeacaeacaeacaeaoaeaoaeaoaeao#Zaeajaoajaeaj#Zajajajajajaj#5aj#5ajagajag#5ag#5agapagap#1apahapahagah#1ahahahaha.aha.aha.ahaka.akaqakaqadaqasaqasakasadasasasas",
"aEatatazatauataaataaataaanaaanaaanaaaaaaal#9aa#Caa#Caa#Caa#Caa#C#9#C#C#C#C#C#C#B#C#7#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#H#t#H#t#F#t#H#K#F#K#F#K#t#K#t#K#t#K#t#K#t#x#K#K#K#x#K#x#K#x#K#x#K#x#K#x#x#I#x#Q#x#O#x#Q#x#O#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p"
".s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.w.v.v.v.waB.v.v.vaB.var.var.var.var.var.xararararamaramara#ara#ara#ara#arawafawafawa#awafawa#aca#acawacawacawacacacacacacaCacaCacaeacaeacaeaAaeaoaeaoaeaoaeae#Zaeajaeajaeajaeaj#Zajajajajaiaj#5ajagajagajag#5ag#5agapagap#1ag#1agah#1ah#1ahahahah#3aha.ahakahaka.akaqakaqadakadaqasadasadasasasasayasaF",
"auatauataaataaataaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#B#C#F#C#F#C#F#C#F#p#F#p#F#F#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#K#K#K#x#K#x#K#I#K#x#K#I#K#I#K#I#x#O#x#O#x#O#x#O#x#O#x#j#x#j#x#j#x#j#x#j#x#j#Q#j#k#j#Q#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s"
".p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.vaB.w.v.var.var.var.var.var.xararararararamarafara#ara#arawarawafawafawafawafawawacafacawacawacacacacacacaAacaAacaeacaoacaeacaeaoaeaoaeaoaeaoaeaoajaoajaeajaeajajajajajajajaj#5ajagajapajag#5agapagapagap#1ap#1agah#1ah#1ahahahaha.aha.ahakahaka.akaqakaqadaqadakasavasasasasaxas",
"aEazaEatatauatauataaataaataaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#H#t#H#t#F#t#H#K#F#K#H#K#t#K#t#K#t#K#t#K#t#x#K#K#K#x#K#x#K#x#K#x#K#x#K#x#x#I#x#x#x#O#x#Q#x#O#x#Q#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.r"
".s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.t.v.v.v.waB.vaB.vaB.var.var.var.var.var.vararararamarararamara#arawara#arawarawafawa#awafawawawa#acawacawacawacacacacacacaCacaAacaeacaeacaeaAaeaAaeaoaeaoaeaeaeaoajaeajaeajaeaj#Zajajajajaiaj#5ajagajagajag#5ag#5agapagap#1ag#1agahagah#1ahahahahahaha.ahakahaka.akaqakaqakaqadaqasakasadasasasasaxasaxasaD",
"azatauatauataaataaataaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#F#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#t#K#K#x#K#x#K#I#K#x#K#I#K#x#K#I#x#I#x#O#x#O#x#O#x#O#x#j#x#O#x#j#x#j#x#j#x#j#x#j#x#j#Q#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s"
".p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.w.v.w.v.w.v.waB.waB.var.var.var.var.var.xararararararamarafara#ara#arawarawafawafawafawafawawacafacawacawacawacacacacaAacaAacaeacaoacaeacaeaAaeaoaeaoaeaoaeaoajaoajaeajaeajajajajajajajaj#5ajagajapajag#5agapagapagap#1ap#1agahapah#1ah#1ahahahaha.ahakahaqa.aka.akaqakaqadaqasaqasasasasasasaFas",
"aEataEazatazatauatauataaataaataaanaaalaaalaaaaaaaa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#F#t#H#t#F#K#H#K#t#K#t#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#x#K#x#K#x#x#x#x#x#x#I#x#Q#x#O#x#Q#x#j#x#k#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p"
".p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.v.t.v.t.v.t.v.t.v.taB.w.v.waB.vaB.vaB.vaB.var.var.varaBar.vararararamarararamara#arawara#arawarawafawafawafawawawa#acawacawacawacawacacacacaCacaAacaCacaeacaeaAaeaAaeaoaeaoaeaeaeaoajaeajaeajaeajaeajajajajajajajajagajagajagajag#5agapagapagag#1apahagahagahahahahahaha.ahakahakahaka.akaqakaqadaqasakasakasasasasasasaxasaDasaD",
"azatazatauatauataaataaanaaauaaauaaaaaaaa#9aa#9aa#Caa#Caa#Caa#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#t#K#K#x#K#x#K#I#K#x#K#I#K#x#K#I#x#I#x#O#x#O#x#O#x#O#x#j#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s"
".p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.waB.w.v.waB.waB.var.war.var.var.var.varararararararararara#ara#arawarawarawafawafawafawaGacafacawacawacawacacacacaAacaAacaAacaoacaeacaeaAaeaoaeaoaeaoaeaoaHaoajaeajaoajaHajajajajajajajajapajapajagajagapagapagapagap#1apahapahagah#1ahahahaha.ahaqahaqahaka.akaqakaqadaqasaqasadasadasasasasaFas",
"aEataEatatazatazatauatauataaataaanaaanaaalaaalaaaaaaaa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#F#t#H#t#F#K#H#K#t#K#t#K#t#K#t#K#t#K#t#K#K#x#K#x#K#x#K#x#K#x#K#x#x#x#x#x#x#I#x#x#x#O#x#Q#x#j#x#Q#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r"
".p.p.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.u.t.t.t.u.t.u.t.u.t.u.t.v.t.v.t.v.t.v.taB.w.v.waB.vaB.waB.vaB.var.var.varaBar.vararararamarararamaramarawarawarawarawarawafawafawawawawawawacawacawacawaAacacacaAacaAacaCacaeacaeaAaeaAaeaAaeaoaeaoaeaoaHaeajaeajaeajaeajajajajajajajajagajagajagajag#5agapagapagag#1apahagahagah#1ah#1ahahahahakahaqahakahaka.akaqakaqadakasaqasadasasasasasasaDasaFasaD",
"azaEazatauatauatauataaataaanaaauaaauaaaaaaaa#9aa#9aa#Caa#Caa#C#9#C#9#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#K#F#K#F#K#t#K#t#K#t#K#t#K#t#K#K#x#K#x#K#I#K#x#K#I#K#x#K#I#x#I#x#I#x#O#x#O#x#O#x#O#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#j#j#k#j#j#j#k#j#j#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s"
".p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.u.t.t.t.v.t.v.t.v.t.v.t.v.t.v.waB.w.v.waB.waB.vaB.war.var.varaBar.varararararararararara#ara#arawarawarawafawafawafawaGawaGacawacawacawacawacacaAacaAacaAacaoacaeacaoaAaeaAaeaoaeaoaeaoaHaoajaeajaoajaHajajajajajajajajaIajapajagajagaIagapagapagap#1apahapahagah#1ahahahahahahaqahaqahaka.akaqakaqakaqasaqasakasavasasasasaxasaFas",
"aJataEataEatatazatauatauatauataaataaanaaanaaalaaalaaaa#9aa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#H#t#H#t#F#K#H#K#t#K#t#K#t#K#t#K#t#K#t#K#t#x#K#x#K#x#K#x#K#x#K#x#x#x#x#x#x#x#x#x#x#I#x#x#x#j#x#Q#x#j#x#k#x#j#x#k#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.p.p.p.r.p.r"
".p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.u.t.v.t.v.taB.t.v.taB.taB.waB.vaB.waB.vaB.var.var.varaBaraBaraBararamarararamaramarawarawarawarawarawafawafawawawaGawawacawacawacawaAawacacaAacaAacaCacaeacaeaAaeaAaeaAaeaoaeaoaeaoaHaeajaeajaeajaeajaHajajajajajajaKajapajagajagajagapagapagapagapahagahagahagah#1ahahahahahaha.ahakahaka.akaqakaqakaqadaqasakasadasasasasaFasaFasaDaFaD",
"azaEazaEazatauatauatauataaataaanaaaaaaaaaaaaaaaa#9aa#9aa#Caa#Caa#C#C#C#C#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#H#F#H#F#H#F#F#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#t#K#t#K#t#K#t#x#K#x#K#x#K#x#K#I#K#x#K#I#x#x#x#I#x#I#x#O#x#O#x#O#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s"
".p.p.p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.t.v.t.v.waB.waB.waB.waB.vaB.war.var.varaBar.varararararararararara#arafarawarawarawafawafawafawaGawaGacawacaGacawacawacacaAacaAacaAacaAacaeacaoaAaeaAaeaoaeaoaeaoaHaoaHaoajaoajaHajaeajajajajajajaIajapajagajapajag#5agapagapagapahapahagahapahahahahahaha.ahaqahakahaqaqakaqakaqadaqasaqasaqasasasasasasaFasaFas",
"aJataEataEazaEatatazatauatauatauataaataaanaaanaaaaaaalaaaa#9aa#9aa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#H#t#H#t#F#K#H#K#t#K#t#K#t#K#t#K#t#K#t#K#t#x#t#x#K#x#K#x#K#x#K#x#x#x#K#x#x#x#x#x#x#x#x#x#x#I#x#x#x#j#x#k#x#j#x#k#x#j#x#k#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r"
".p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.u.t.v.t.v.taB.t.v.taB.taB.waB.vaB.waB.vaB.vaBaBar.varaBaraBaraBararararararamaramarawarawarawarawarawafawafawawawaGawawacawacawacawaAawacacaAacaAacaCacaAacaeaAaeaAaeaAaeaAaeaoaeaoaHaeaHaoajaeajaeajaHajajajajajajaKajaIajagajagajagaIagapagapagapaLagahagahagah#1ahahahahahaha.ahakahakahakaqakaqakaqadaqasakasakasasasasasasaFasaDasaDaFaD",
"ataEazatazatazatauatauatauataaauaaanaaaaaaaaaaaaaaaa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#t#K#t#K#t#K#t#x#K#x#K#x#K#x#K#I#K#x#K#I#x#x#x#I#x#I#x#I#x#O#x#O#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j#k#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p"
".p.p.p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.taB.t.v.taB.waB.waB.waB.vaB.war.var.varaBaraBaraBararararararararaMarafarawarawarawarawafawafawaGawaGawawacaGacawacawacawaAacaAacaAacaAacaeacaoaAaeaAaeaAaeaoaeaoaHaoaHaoajaoajaeajaeajajajajajajaIajaIajapajapajagajagapagapagapaLapahagahapahahahahahahahaha.ahaqahaqaNakaqakaqakaqadaqasaqasasasasasasaOasaFasaFas",
"aJaEaJataEataEazaEatatazatauataaataaataaataaanaaanaaaaaaalaaaa#Caa#Caa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#H#t#H#t#F#t#H#K#t#K#u#K#t#K#t#K#t#K#t#K#t#x#t#x#K#x#K#x#K#x#K#x#x#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x#b#x#x#x#j#x#k#x#j#x#k#x#j#x#k#x#j#x#k.s#j#x#k.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r"
".p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.u.taB.t.v.taB.taB.taB.taB.waB.vaB.waB.vaB.vaBaBar.varaBaraBaraBararararararaMaramaraMarawarawarawarawarawafawawawaGawawawawacawacawaAawaAawaAacaAacaCacaAacaeaAaeaAaeaAaeaAaeaoaeaoaHaeaHaoajaeajaeajaHajaHajajajajaIajaIajagajagajagaIagapagapagapaLagahapahagahagahahahahahahahahakahakahakaqakaqakaqakaqasakasakasasasasasasasasaDasaFasaDaFaD",
"azaEataEazatazatauatauataaataaataaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#t#K#t#K#t#K#t#x#t#K#K#x#K#x#K#x#K#x#K#I#x#x#K#I#x#x#x#I#x#I#x#I#x#O#x#j#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p"
".p.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.t.v.t.v.taB.taB.taB.waB.waB.waB.vaB.waraBar.varaBaraBaraBararararararararaMarafarawarawarawarawafawafawaGawaGawawacaGacawacawacawaAacaAacaAacaAacaAacaoaAaeaAaeaAaeaoaeaoaHaoaHaoaHaoajaeajaoajaHajajajajaIajaIajapajapajagajagapagapagapaLapahapahapahaLahahahahahahahahaqahaqahaka.akaqakaqakaqasaqasakasaPasasasasaFasaFasaFaF",
"aJaEaJataEataEataEazaEatatauatauataaataaataaanaaalaaalaaaaaaalaaaa#Caa#Caa#Caa#Caa#Caa#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#H#t#H#t#F#t#H#K#t#K#u#K#t#K#t#K#t#K#t#K#t#x#t#x#t#x#K#x#K#x#K#x#K#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x#b#x.s#x#b#x#k#x#j#x#k#x#j#x#k.s#j#x#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r"
".p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.t.u.t.u.taB.taB.taB.taB.taB.taB.waB.vaB.waB.vaB.vaBaBaraBaraBaraBaraBaraBararararaMarararaMarawarawarawarawarawafawaGawaGawawawawacawacawaAawaAawaAacaAacaAacaAacaCaAaeaAaeaAaeaAaeaoaeaoaHaoaHaoaHaeajaeajaHajaHajajajajaIajaIajaKajagajagaIagaIagapagapagapaLapahagahagahaLahahahahahahakahaqahakaNakaqakaqakaqasakasaqasaQasasasasasasaFasaFasaDaFaDaFaR",
"aEaEazaEazaEazatazatauatauataaataaataaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#t#F#K#F#K#t#K#t#K#t#K#t#K#t#K#t#x#t#K#t#x#K#x#K#x#K#x#K#I#x#x#K#I#x#x#x#I#x#x#x#I#x#I#x#j#x#O#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p"
".p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.u.t.t.t.u.t.v.taB.t.v.taB.taB.taB.waB.waB.waB.vaB.waBaBar.varaBaraBaraBararararararararaMaraSarawarawarawarawarawafawaGawaGawaGawaGacawacawacawaAacaAacaAacaAacaAacaoaAaeaAaeaAaeaAaeaoaHaoaHaoaHaoajaeajaoajaHajajajajajajajajaIajapajagajapaIagapagapagapaLapahapahagahaLahahahahahahaqahaqahakahaqaqakaqakaqasaqasaqasaqasasasasasasaFasaFaOaDaF",
"aJaEaJataJataEataEatatazatatatauatauataaataaataaanaaalaaalaaaaaaalaaaa#Caa#Caa#Caa#Caa#C#9#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#H#t#H#t#H#t#H#t#F#t#H#t#t#K#u#K#t#K#t#K#t#K#t#K#t#x#t#x#t#x#t#x#K#x#K#x#K#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x.s#x.s#x.s#x#b#x.s#x#b#x#k#x#j#x#k.s#j#x#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#j.s#k.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r"
".p.q.p.q.p.q.p.q.p.q.p.q.p.q.r.q.o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.t.u.t.u.taT.t.u.taB.taB.taB.taB.taB.taB.waB.vaB.waBaBaB.vaBaBaraBaraBaraBaraBaraBararararaMaraSaraMarawarawarawarawaSawarawaGawaGawawawawawawacawaAawaAawaAawaAacaAacaAacaCaAaeaAaeaAaeaAaeaAaeaoaHaoaHaoaHaeajaeajaHajaHajaHajajaIajajajaKajapajagaIagaIagapagapagapagapahagahagahaLahahahahahahaNahaqahakahakaNakaqakaqaQaqasaqasakasasasasasasaOasaFasaDaOaDaFaDaFaR",
"ataJataEazaEazaEazatazatauatauataaataaataaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#6#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#F#t#F#t#F#K#t#K#t#K#t#K#t#K#t#K#t#x#t#K#t#x#K#x#K#x#K#x#K#x#K#x#K#I#x#x#x#I#x#x#x#I#x#x#x#I#x#I#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p"
".r.p.p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.t.taT.t.w.taB.taB.taB.taB.taB.waB.waB.waB.waB.waBaBaraBaraBaraBaraBaraBararararararaSaraSarawarawarawarawaSawafawaGawaGawaGawaGacawaAawacawaAawaAacaAacaAacaAacaAaAaeaAaoaAaeaAaeaoaHaoaHaoaHaoaHaoajaoajaHajaHajajajajajajaIajapajagajapaIagapagapagapagapahapahagahapahahahahahahaNahaqahaqahaqaqakaqakaqaQaqasaqasaqasasasasasasaFasaFaOaFaOaFaF",
"aJaEaJaEaJataJataEataEatatazatatatauatauataaataaataaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#Caa#C#9#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#p#F#C#F#p#F#p#F#p#F#H#F#H#t#H#t#H#t#H#t#H#t#H#t#t#K#u#K#t#K#u#K#t#K#t#K#t#x#t#x#t#x#t#x#t#x#t#x#K#x#K#x#x#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s#b#x.s.s#b#x.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q"
".p.q.p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.u.t.t.taT.t.t.taT.taT.taB.taB.taB.taB.taB.taB.taB.vaB.waBaBaBaBaBaBaraBaraBaraBaraBaraBaSarararaMaraSaraMaraMarawarawarawaSawaSawaGawaGawawawaGawawacawaAawaAawaAawaAacaAacaAacaCaAaAaAaeaAaeaAaeaAaeaoaHaoaHaoaHaeaHaoajaHajaHajaHajajaIajajajaIajaIajagaIagaIagaIagapagapagapaLagahapahaLahaLahahahahaNahaqahakahakahakaqakaqakaqaQaqasakasakasasasasaOasaFasaDaOaFaOaDaFaDaFaR",
"aEaJataEataEazaEazaEazatauatauatauataaataaanaaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#B#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#H#t#F#t#F#K#t#K#t#K#t#K#t#K#t#K#t#K#t#K#t#x#t#x#K#x#K#x#K#x#K#x#K#I#x#x#x#I#x#x#x#I#x#x#x#I#x#x#x#b#x#I#x#b#x#j#x#j#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p"
".p.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.t.t.t.taT.t.w.taB.taB.taB.taB.taB.waB.waB.waBaUaB.waBaBaraUaraBaraBaraBaraBararaSarararaSaraSarawaraGarawarawaSawafawaGawaGawaGawaGacawaAaGacawaAawaAacaAacaAaAaAacaAaAaeaAaoaAaeaAaeaAaHaoaHaoaHaoaHaoajaoajaHajaHajajajajajajaIajaIajapajapaIagaIagapagapagapaLapahapahapahaLahahahahaNahaqahaqahaqaNakaqakaqaQaqasaqasaqasaQasasasasaOasaFasaFaOaFaFaDaF",
"aJaEaJaEaJaEaJataEataEataEatatazatanatauatauataaataaataaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#Caa#C#9#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#p#F#C#F#p#F#p#F#p#F#H#F#H#t#H#t#H#t#H#t#H#t#H#t#t#t#H#K#t#K#u#K#t#K#t#K#t#x#t#K#t#x#t#x#t#x#t#x#t#x#K#x#x#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s.s#x.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q"
".p.q.p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.qaT.t.t.taT.t.6.taT.taT.taB.taB.taB.taB.taB.taB.taBaBaB.waBaBaBaBaBaBaBaBaraBaraBaraBaraBaSarararaMaraSaraMaraMarawarawarawaSawaSawaGawaGawawawaGawawacawaAawaAawaAawaAacaAaAaAacaCaAaAaAaeaAaeaAaeaAaeaAaHaoaHaoaHaeaHaoajaHajaHajaHajaHaIajajajaIajaIajagaIagaIagaIagapagapagapaLagahapahaLahaLahahahahaNahaNahakahaqahakaNakaqakaqakaqasakasaqasaQasasasasaOasaFasaFaOaDaFaDaFaRaFaV",
"aEaJaEaJazaEataEazaEazaEazatauatauatauataaataaanaaauaaanaaaaaaaaaaaa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#B#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#H#F#H#F#F#F#H#t#F#t#H#t#F#t#F#t#t#K#F#K#t#K#t#K#t#K#t#K#t#K#t#x#t#x#t#x#K#x#K#x#K#x#K#x#x#x#x#I#x#x#x#I#x#x#x#I#x#x#x#b#x#x#x#b#x.s#x#b#x#b#x#b#x#j#x#j#x#j#x#j.s#j#x#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#j.s#b.s#b.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.p.p.r.p.p.p.r.p.r.p"
".r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.t.t.t.t.6.t.t.taT.taU.taB.taB.taB.taB.taB.taB.waB.waBaUaBaUaBaBaraUaraBaraBaraBaraBararaSaraSaraSaraSarawaraGarawarawaSawaSawaGawaGawaGawaGawawaAaGaAawaAawaAawaAacaAaAaAacaAaAaoaAaoaAaeaAaeaAaHaoaHaoaHaoaHaoaHaoajaHajaWajaHajajajajaIajaIajapajapaIagaIagapagapagapaLapahapahapahaLahahahahaNahaNahaqahaqaNakaqakaqakaqaQaqasaqasaXasaPasasaOasaOasaFasaFaOaDaFaDaF",
"aJaEaJaEaJaEaJaEaJataEataEataEatatazatauatauatauataaataaanaaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#C#l#C#9#C#C#C#C#C#C#C#C#C#C#B#C#H#C#F#C#F#p#F#C#F#p#F#p#F#p#F#H#F#H#t#H#t#H#t#H#t#H#t#H#t#u#t#H#t#t#K#u#K#t#K#u#K#t#x#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s.s#x.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q"
".p.q.p.q.p.q.p.q.q.q.o.q.q.q.q.q.q.q.q.q.q.q.q.t.q.t.q.t.q.t.q.6.q.t.qaT.t.6.taT.t.6.taT.taT.taT.taB.taB.taB.taB.6aB.taBaBaBaUaBaBaBaBaBaBaBaBaraBaraBaraBaraBaSaBaSaraMaraSaraMaraMarawaSawarawaSawaSawaGawaGawawawaGaYawawawaAawaAawaAawaAawaAaAaAacaCaAaAaAaeaAaeaAaHaAaeaAaHaoaHaoaHaoaHaoaHaHajaHajaHajaHaIajajajaIajaIajaKaIagajagaIagaIagapagapaLapaLapahaLahaLahaLahahahahaNahaNahaqahakaNakaqakaqakaqaQaqasaqasaQasasasasasasaOasaFasaDaOaDaFaRaFaRaFaV",
"aEaJaEaJaEaJazaEataEazatazatazatauatauatauataaataaanaaaaaaalaaaaaaaa#9aa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#B#C#6#C#F#C#F#C#F#C#F#C#F#p#F#p#F#p#F#H#F#H#F#H#t#F#t#H#t#F#t#F#t#t#K#F#K#t#K#t#K#t#K#t#K#t#K#t#x#t#x#t#x#t#x#t#x#K#x#K#x#x#x#K#x#x#x#x#I#x#x#x#I#x#x#x#I#x#x#x#b#x.s#x#b#x.s#x#b#x.s#x#b#x.s#x#b#x#b#x#b.s#b#x#b.s#b.s#b.s#b.s#b.s#b.s#b.s#b.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.q.p"
".q.p.q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.t.q.t.t.6.t.t.t.6.t.6.taT.t.6.taB.taB.taB.taB.taB.taBaUaB.waBaUaBaUaBaBaBaUaraBaraBaraBaraBararaSaraSaraSaraSaraMaraGarawarawaSawaSawaGawaGawaGawaGawawaAaGaAawaAawaAawaAacaAaAaAaAaAaAaAaAaoaAaeaAaeaAaHaoaHaoaHaoaHaoaHaoajaHajaWajaHajajajajaIajaIajaIajapaIagaIapaIagapagapaLapaLapahapahaLahaZahahaNahaNahaqahaqaNakaNaqaqakaqakaqaQaqasaXasaXasasaOasaOasaFasaFaOaDaFaFaFaRaF",
"a0aJaJaEaJaEaJaEaJaEaJataEataEataEatatazatauatauatauataaataaanaaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#C#l#Caa#C#C#C#C#C#C#C#C#C#C#B#C#p#C#F#C#H#C#F#C#F#p#F#p#F#p#F#p#F#H#t#H#t#H#t#H#t#H#t#H#t#u#t#H#t#t#t#u#K#t#K#u#K#t#K#u#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s.s#x.s.s.s#x.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.s.p.p.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q"
".p.q.r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.q.q.t.q.6.q.t.q.6.q.t.q.6.q.6.qaT.t.6.taT.taT.taT.taT.taT.taB.taB.taB.taB.6aB.6aBaBaBaUaBaBaBaBaBaBaBaBaraBaraBaSaBaraBaSaBaSaraMaraSaraMaraMarawaSawarawaSawaSawaSawaGawawawaGaYawaYawaAawaAawaAawaAawaAaAaAaAaAaAaAaAaCaAaeaAaHaAaeaAaHaAaHaoaHaoaHaoaHaHajaHajaHajaHaIaHajajaIajaIajaKajapajagaIagaIagapagapaLapaLapahagahagahaLahahahahahahaNahaqahakaNakaNakaqakaqaQaqasaqasaQasaQasasasasaOasaFasaDaOaDaFaRaFaRaFaVa1aV",
"aEaJaEaJaEaJaEaJazaEataEazatazatauatauataaataaataaataaanaaaaaaalaaaaaaaa#9aa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#B#C#B#C#F#C#F#C#F#C#F#C#F#p#F#p#F#p#F#H#F#H#F#H#t#F#t#H#t#F#t#H#t#t#t#F#K#t#K#t#K#t#K#t#K#t#K#t#x#t#x#t#x#t#x#t#x#t#x#K#x#K#x#K#x#x#x#x#x#x#x#x#x#x#x#x#I#x#x#x#b#x#x#x#b#x.s#x#b#x.s#x#b#x.s#x#b#x.s#x#b.s.s#x#b.s.s#x#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p"
".q.p.q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.t.q.t.q.6.q.t.t.6.t.6.t.6.t.6.taT.t.6.taB.taB.taB.taB.taB.taBaUaBaUaBaUaBaUaBaBaBaUaraBaraBaraBaSaBararaSaraSaraSaraSaraMaraGaSawarawaSawaSawaGawaGawaGawaGawawaAaGaAawaAawaAawaAacaAaAaAaAaAaAaAaAaoaAaeaAaoaAaHaAaHaoaHaoaHaoaHaoajaWajaWajaHajaHajajaIajaIajaIajapaIagaIapaIagapagapaLapaLapahapahaLahaZahahahahaNahaNahaqaNaqaNaqaNakaqakaqaQaqasaqasaXasasasasaOasaOasaFaOaFaOaFaFaRaFaRaF",
"a2aJa0aEaJaEaJaEaJataJataJataEataEataEatatazatauataaataaataaataaanaaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#B#C#p#C#F#C#H#C#F#C#F#p#F#p#F#p#F#p#F#H#t#H#t#H#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#t#K#u#K#t#K#u#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s.s#x.s.s.s#e.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.r.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q"
".r.q.o.q.q.q.o.q.q.q.q.q.q.q.q.m.q.t.q.6.q.t.q.6.q.6.q.6.q.6.qaT.t.6.taT.taT.taT.taT.taT.taB.taB.6aB.taB.6aB.6aBaUaBaUaBaBaBaUaBaBaBaBaraBaraBaSaBaSaBaSaBaSaraSaraSaraMaraMaraMaSawaSawaSawaSawaSawaGawaGawaGaYawaYawaAawaAawaAawaAawaAaAaAaAaAaAaAaAaCaAaeaAaHaAaHaAaHaAaHaoaHaoaHaoaHaHaHaWajaHajaHaIaHajajaIajaIajaKajapajagaIagaIagaIagapaLapaLapaLagahapahaLahaLahahahahaNahaNahakaNakaNakaqakaqaQaqaQaqasaQasaQasasasasaOasaOasaDaOaFaOaDaFaRaFaRaFaVa1aV",
"aEaJaEaJaEaJataJataEazaEataEazatazatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#9aa#9aa#Caa#Caa#Caa#Caa#C#C#C#C#6#C#C#C#B#C#B#C#F#C#F#C#F#C#F#C#F#p#F#p#F#p#F#H#F#H#F#H#t#F#t#H#t#F#t#H#t#F#t#F#t#t#K#t#K#t#K#t#K#t#K#t#x#t#K#t#x#t#x#t#x#t#x#t#x#t#x#K#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#b#x.s#x#b#x.s#x#b#x.s#x#b#x.s#x#b#x.s#x#b.s.s#x#b.s.s#x#b.s.s.s#b.s.s.s#b.s.s.s#b.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p"
".q.p.q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.q.q.t.q.t.q.t.q.6.q.t.q.6.q.6.t.6.t.6.t.6.t.6.taT.t.6.taB.taB.taB.taB.6aB.taBaUaBaUaBaUaBaUaBaBaBaUaraBaraBaraBaSaBaSaBaSaraSaraSaraSaraMaraGaSawarawaSawaSawaSawaGawaGaYaGawaGaYaGaAawaAawaAawaAawaAaAaAaAaAaAaAaAaAaAaeaAaoaAaHaAaHaoaHaoaHaWaHaoaHaWajaWajaHajaHajajaIajaIajaIajaIaIapaIapaIagaIagapaLapaLapaLapahaZahaZahaLahahahahaNahaqahaqahaqaNakaqakaqaQaqaQaqasaqasaQasaPasasaOasaFaOaFaOaFaFaRaFaRaFaVaF",
"a0aJa2aJa0aEaJaEaJaEaJataJataEataEatatatatatatauatauataaataaataaataaanaaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#F#C#H#C#F#C#F#p#F#p#F#p#F#p#F#p#t#H#F#H#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#t#t#u#K#t#K#u#K#t#x#u#K#t#x#u#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s.p.s#e.s.p.s#e.s.p.s#e.s.p.s.p.s.p.s.r.s.p.s.r.s.p.s.r.p.p.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.r.q"
".o.m.q.q.o.m.q.q.q.m.q.q.q.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.qaT.t.6.taT.taT.taT.taT.taB.6aB.taB.6aB.6aB.6aB.6aBaUaBaUaBaBaBaUaBaBaBaBaSaBaraBaSaBaSaBaSaBaSaraSaraSaraMaSaSaraMaSawaSawaSawaSawaSawaGawaGawaGaYawaYawaYawaAawaAawaAawaAaYaAaAaAaAaAaAaCaAaAaAaHaAaHaAaHaAaHaoaHaoaHaoaHaHaHaWajaHajaHaIaHajaHaIajaIajaIajaIajagaIagaIagaIagapaLapaLapaLagahapahaLahaLahahahahaNahaNahakaNaqaNakaNakaqaQaqaQaqasakasaXasaQasasaOasaOasaFaOaFaOaDaFaDaFaRaFaRaFaVaRaV",
"aJaJaEaJaEaJaEaJataJataEazaEataEazatazatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#Caa#C#9#C#C#C#C#6#C#C#C#B#C#B#C#F#C#F#C#F#C#F#C#F#p#F#p#F#p#F#p#F#H#F#H#t#H#t#H#t#F#t#H#t#F#t#H#t#t#t#t#K#t#K#t#K#t#K#t#x#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s.s#x.s.s.s#x.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p"
".q.p.q.p.q.p.q.o.q.o.q.o.q.o.q.q.q.o.q.q.m.q.t.q.6.q.t.q.6.q.6.q.6.q.6.t.6.t.6.t.6.t.6.taT.t.6.taB.taB.6aB.taB.6aB.6aBaUaBaUaBaUaBaUaBaBaBaUaraBaSaBaraBaSaBaSaBaSaraSaraSaraSaraMaraGaSawaSawaSawaSawaSawaGawaGaYaGawaGaYaGaAawaAawaAawaAawaAaAaAaAaAaAaAaAaAaAaHaAaoaAaHaAaHaAaHaoaHaWaHaoaHaWajaWajaHajaHajaHaIajaIajaIajaIaIapaIapaIagaIagapaLapaLapaLapahaZahaZahaLahahahahaNahaNahaqahaqaNakaNakaqaQaqaQaqasaqasaXasaXasasaOasaOasaFaOaFaOaDaFa1aFaRaFaVa1",
"a0aJa0aJa2aJa3aEaJaEaJaEaJataJataEataEatatazatatatauatauataaataaataaataaanaaanaaalaaalaaaaaaal#9aa#Caa#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#F#C#H#C#F#C#F#p#F#p#F#p#F#p#F#p#t#H#F#H#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#u#t#u#t#t#t#u#K#t#K#u#K#t#x#u#x#t#x#u#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s.p.s#h.s.p.s#h.s.p.s#h.s.p.p#h.p.p.p#h.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.r.p.q.p.m.p.q.p.m.p.q.p.m.p.q.p.m.p.q.p.m.q.q.o.m"
".q.q.q.m.q.m.q.m.q.m.q.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.taT.t.6.taT.taT.taT.6aT.taB.6aB.6aB.6aB.6aB.6aB.6aBaUaBaUaBaBaBaUa4aBaBaBaSaBaSaBaSaBaSaBaSaBaSaraSaraSaraMaSaSaraMaSawaSawaSawaSawaSawaSaYaGawaGaYawaYaGaYawaAawaAawaAawaAaYaAaAaAaAaAaAaCaAaAaAaHaAaHaAaHaAaHaAaHaoaHaoaHaHaHaWajaHajaHaIaHaIaHaIajaIajaIaIaIajagaIagaIagaIagaIaLapaLapaLapaLapahaLahaLahaLahahaNahaNahaNaNaqaNakaNakaqaQaqaQaqaQaqasaqasaQasasaOasaOasaOaOaFaOaDaOaDaFaRaFaRaFaVa1aVa1a5",
"aJa2aJaJaEaJaEaJaEaJataEataEazaEataEazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#C#9#C#C#C#C#6#C#C#C#B#C#p#C#B#C#F#C#F#C#F#C#F#p#F#p#F#p#F#p#F#H#F#H#t#H#t#H#t#F#t#H#t#F#t#H#t#t#t#t#t#t#K#t#K#t#K#t#K#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#e#x#e#x#x#x#x#x#x#x#x#x#x#x#x#x#x.s#x#x#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s#x.s.s.s#x.s.s.s#e.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p"
".q.p.q.o.q.o.q.o.q.q.q.o.q.q.m.o.q.q.6.q.t.q.6.q.6.q.6.q.6.q.6.q.6.t.6.t.6.t.6.t.6.taT.t.6.6aB.taB.6aB.6aB.6aB.6aBaUaBaUaBaUaBaUaBaBaBaUaraBaSaBaSaBaSaBaSaBaSaraSaraSaraSaSaMaraSaSawaSawaSawaSawaSawaGawaGaYaGaYaGaYaGaAawaAaGaAawaAawaAaAaAaAaAaAaAaAaAaAaHaAaoaAaHaAaHaAaHaoaHaWaHaWaHaWaHaWajaHajaWajaHaIajaIajaIajaIaIapaIapaIagaIagaIaLapaLapaLapaLaZahaZahaLahaZahahaNahaNahaqahaqaNakaNaqaqaQaqaQaqaQaqasaXasaXasasaOasaOasaFasaFaOaFaOaFaFaRaFaRa1aVa1",
"a0aJa0aJa0aJaJaJa3aEaJaEaJaEaJataJataEataEatatazatatatauatauataaataaataaataaalaaalaaalaaalaaaa#lal#9aa#Caa#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#B#C#H#C#F#C#H#p#F#p#F#p#F#p#F#p#t#p#F#H#t#H#t#H#t#H#t#H#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#t#K#u#K#t#x#u#x#t#x#u#x#t#x#u#x#t#x#u#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#h.s#e.s#h.s#e.s#h.s#e.s#h.s#e.p#h.p#e.p#h.p.r.p#h.p.r.p#h.p.r.p#h.p.r.p#h.p.r.p#h.p.q.p.m.p.q.p.m.p.q.p.m.p.m.p.m.r.m.p.m.q.m.o.m.q.m"
".q.m.q.m.q.m.q.m.q.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.taT.6.6.taT.6aT.taT.6aT.6aB.6aB.6aB.6aB.6aB.6aB.6aBaUaBaUaBaBaBaUa4aBa4aBaSaBaSaBaSaBaSaBaSaBaSaraSaSaSaraMaSaSaSaMaSawaSawaSawaSawaSawaSaYaGaYaGaYawaYaGaYawaAawaAaYaAawaAaYaAaAaAaAaAaAaCaAaAaAaHaAaHaAaHaAaHaAaHaWaHaoaHaWaHaWaHaHajaHaIaHaIaHaIajaIajaIaIaIajaKaIapaIagaIagaIaLapaLapaLapaLapahaLahaLahaLahahaNahaNahaNahaqaNakaNakaNaQaqaQaqaQaqasaqasaQasaQaOasaOasaOasaFaOaDaOaFaOaRaFaRaFaRa1aVa1aVaRa5",
"aJa2aJa2aEaJaEaJaEaJaEaJataEataEazaEataEazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#C#9#C#C#C#C#6#C#C#C#B#C#p#C#B#C#F#C#F#C#F#C#F#p#F#p#F#p#F#p#F#p#F#H#t#H#t#H#t#H#t#H#t#F#t#H#t#t#t#u#t#t#t#t#K#t#K#t#K#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#x#x#x.s#x.s#x.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s.p.s#e.s.p.s#e.s.p.s#e.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.s.p.p.p.p.p.p.r.p.p.p.r.p.p.p.r.p.p.p.r.p.r.p.r.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p.q.p"
".m.o.q.o.m.o.q.q.m.o.q.q.m.o.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.q.6.t.6.t.6.taT.t.6.6aT.t.6.6aB.6aB.6aB.6aB.6aB.6aBaUaBaUaBaUaBaUaBaBa4aUaraBaSaBaSaBaSaBaSaBaSaraSaraSaraSaSaSaSaSaSawaSawaSawaSawaSawaGawaGaYaGaYaGaYaGaYawaAaGaAaYaAawaAaYaAaAaAaAaAaAaAaAa6aAaWaAaHaAaHaAaHaoaHaWaHaWaHaWaHaWajaHajaWajaHaIaHaIajaIajaIaIaIaIapaIagaIapaIaLapaLapaLapaLaZahaZahaLahaZahahaNahaNahaNahaqaNaqaNaqaNaQaqaQaqaQaqasaXasaXasaQaOasaOasaOasaFaOaFaOaFaFaRaFaRa1aVa1aVa1",
"a0aJa0aJa0aJa0aJaJaEaJaEaJaEaJaEaJataEataEataEatatazatatatauatauataaataaataaanaaalaaalaaalaaalaaaa#lal#9aa#Cal#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#B#C#H#C#F#C#H#p#F#p#F#p#F#p#F#p#F#p#F#p#t#H#t#H#t#H#t#H#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#t#x#u#K#t#x#u#x#t#x#u#x#t#x#u#x#t#x#u#x#e#x#u#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#x#e.s#h#x#e.s#h.s#e.s#h.s#e.s#h.s#e.s#h.s#e.s#h.p#e.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.r.m.o.m.q.m.o.m.q.m.q.m"
".q.m.q.m.q.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.m.6.taT.6.6.taT.6aT.6aT.6aT.6aB.6aB.6aB.6aB.6aB.6aB.6aBaUaBaUa4aBaBaUa4aBa4aBa4aBaSaBaSaBaSaBaSaBaSaBaSaSaSaraMaSaSaSaMaSawaSawaSawaSawaSawaSaYaGaYaGaYawaYaGaYawaAawaAaYaAaYaAaYaAaYaAaAaAaAaCaAaAaAa7aAaHaAaHaAaHaAaHaWaHaWaHaWaHaWaHaHajaHaIaHaIaHaIaHaIajaIaIaIajaKaIapaIagaIagaIaLapaLapaLapaLapaLaLahaZahaLahaLaNahaNahaNahaNahakaNakaNakaqaQaqaQaqaQaqasaQasaXasasaOasaOasaOasaFaOaFaOaRaFaRaFaRa1aVa1aVaRaVaVa5",
"aJa0aJa2aJa2aEaJaEaJaEaJaEaJataEataEazaEataEazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#Caa#C#C#C#C#C#C#C#C#B#C#p#C#B#C#F#C#F#C#F#C#F#p#F#p#F#p#F#p#F#p#F#H#t#H#t#H#t#H#t#H#t#F#t#H#t#t#t#H#t#t#t#u#t#t#K#t#K#t#K#t#x#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e#x#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s.p.s#e.s.p.s#e.s.p.s#e.p.p.p#e.p.p.p#h.p.p.p#h.p.p.p#h.p.r.p#h.p.r.p#h.p.q.p.m.p.q.p.m.p.q.p.m.p.q.p.m.p.q.p.m.o"
".q.o.m.o.m.q.m.o.m.q.m.q.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.q.6.t.6.t.6.6aT.t.6.6aT.t.6.6aB.6aB.6aB.6aB.6aB.6aBaUaBaUaBaUa4aUaBaBa4aUaSaBaSaBaSaBaSaBaSaBaSaraSaSaSaraSaSaSaSaSaSawaSaGaSawaSawaSaYaSawaGaYaGaYaGaYaGaYawaAaGaAaYaAaYaAaYaAaAaAaAaAaAaAaAa6aAaWaAaHaAaHaAaHaAaHaWaHaWaHaWaHaWajaWaIaWajaHaIaHaIajaIajaIaIaIaIapaIagaIapaIaLaIaLapaLapaLaZaLaZahaZahaZahaLaNahaNahaNahaqaNaqaNaqaNaQaqaQaqaQaqaQaXasaXasaQaOaPaOasaOasaOaOaFaOaFaOaRaFaRaFaRa1aVa1a5a1",
"a8a2a0aJa0aJa0aJa0aJaJaEaJaEaJaEaJaEaJataEataEataEatatazatatatauatauataaataaataaanaaalaaalaaalaaalaaal#lal#9aa#Cal#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#p#C#H#C#F#C#H#p#F#p#H#p#F#p#F#p#F#p#F#p#t#p#t#H#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#e#t#x#u#x#t#x#u#x#t#x#u#x#t#x#u#x#e#x#u#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#x#e.s#h#x#e.s#h#e#e.p#h.p#e.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.r.m.p.m.r.m.o.m.q.m.q.m.q.m.q.m.q.m"
".q.m.q.m.q.6.q.6.q.6.m.6.q.6.m.6.q.6.m.6.6aT.6.6.6aT.6aT.6aT.6aT.6aB.6aB.6aB.6aB.6aB.6aB.6a4aBaBaUa4aBa4aUa4aBa4aBa4aBaSaBaSaBaSaBaSa4aSaBaSaSaSaSaMaSaSaSaMaSaMaSawaSawaSaYaSawaSaYaGaYaGaYawaYaGaYawaYawaAaYaAaYaAaYaAaYaAaAaAaAa7aAaAaAa7aAaHaAaHaAaHaAaHa6aHaWaHaWaHaWaHaHaHaWaIaHaIaHaIaHaIajaIaIaIaIaKaIaIaIagaIagaIaLaIaLapaLapaLapaLaLahaZahaLahaLaNahaNahaNahaNahakaNaqaNakaNakaqaQaqaQaqasaQasaXasaQasasaOasaOasaFaOaFaOaDaOaRaFaRaFaRa1aVaRaVa1a5aVa5",
"aJa2aJa0aJa2aJa2aEaJaEaJaEaJaEaJataEataEazatatatazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#Caa#C#C#C#C#C#C#C#C#B#C#p#C#B#C#F#C#F#C#F#C#F#p#F#p#F#p#F#p#F#p#F#p#t#H#t#H#t#H#t#H#t#H#t#H#t#t#t#H#t#t#t#u#t#t#t#u#K#t#K#t#K#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e#x#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.p#e.p#e.p#e.p#h.p#e.p#h.p.p.p#h.p.p.p#h.p.r.p#h.p.r.p.m.p.q.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.o.m.o.m.o"
".m.o.m.q.m.o.m.q.m.q.m.q.6.q.6.q.6.q.6.q.6.q.6.q.6.t.6.6.6.t.6.6aT.6.6.6aT.6.6.6aB.6aB.6aB.6aB.6aB.6aBaUaBaUaBaUa4aUaBaBa4aUa4aBaSaBaSaBaSaBaSaBaSaraSaSaSaraSaSaSaSaSaSawaSaGaSawaSawaSaYaSaYaGaYaGaYaGaYaGaYawaAaGaAaYaAaYaAaYaAaAaAaAaAaAaAaAa6aAaWaAaHaAaWaAaHaAaHaWaHaWaHaWaHaWaHaWaIaWaIaHaIaHaIajaIajaIaIaIaIaIaIapaIapaIaLaIaLapaLapaLaZaLaZahaZahaZahaLaNahaNahaNahaNaNaqaNaqaNaQaNaQaqaQaqaQaXasaXasaXasaXaOasaOasaOaOaFaOaFaOaRaFa1aFaRa1aVa1aVa1a5aV",
"a8a2a8a2a0aJa0aJa0aJa0aJaJaEaJaEaJaEaJaEaJataEataEataEatatazatanatauatauataaataaataaanaaalaaalaaalaaalaaal#lal#9aa#Cal#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#p#C#p#C#F#C#H#p#F#p#H#p#F#p#F#p#F#p#F#p#t#p#t#p#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#x#t#x#u#x#t#x#u#x#e#x#u#x#e#x#h#x#e#x#h#x#e#x#h#x#e#x#h#e#e#e#h#e#h#e#h#e#h.p#h#e#h.p#h#e#h.p#h#e#h.p#h#e#h.p#h.p#h.p#h.p#h.p#h.p#h.p#h.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.r.m.p.m.q.m.o.m.q.m.q.m.q.m.q.m.q.m.q.m"
".m.m.q.6.m.6.q.6.m.6.q.6.m.6.maT.m.6.6aT.6.6.6aT.6aT.6aT.6aT.6aB.6aB.6aB.6aB.6a4.6aB.6a4aBa4aUa4aBa4aUa4aBa4aBa4aBaSaBaSaBaSaBaSa4aSaBaSaSaSaSaMaSaSaSaMaSaMaSawaSawaSaYaSaYaSaYaGaYaGaYawaYaGaYaYaYawaAaYaAaYaAaYaAaYaAaAaAaAa6aAa6aAa7aAaHaAaHaAaHaAaHa6aHaWaHaWaHaWaHaHaHaWaIaHaIaHaIaHaIajaIaIaIaIaIaIaIaIagaIagaIaLaIaLapaLapaLapaLaLahaZahaLahaLaNaLaNahaNahaNahakaNaqaNakaNakaqaQaqaQaqaQaXasaXasaQasasaOasaOasaOaOaFaOaDaOaRaFaRaFaRa1aVa1aVa1a5aVa5aVa5",
"a2a0aJa2aJa0aJaJaEaJaEaJaEaJaEaJaEaEataEataEazatatatazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#Caa#C#C#C#C#C#C#C#C#B#C#C#C#B#C#p#C#F#C#F#C#F#p#F#p#F#p#F#p#F#p#F#p#t#p#F#H#t#H#t#H#t#H#t#H#t#H#t#H#t#t#t#u#t#t#t#u#t#t#t#u#t#t#K#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e#x#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.s#e.p#e.p#h.p#e.p#h.p#e.p#h.p#e.p#h.p#e.p#h.p#h.p#h.p#h.p#h.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.o.m.o.m.o.m.q.m.o"
".m.q.m.o.m.q.m.q.6.q.6.q.6.q.6.q.6.m.6.q.6.m.6.t.6.6.6.6.6.6aT.6.6.6aT.6aT.6aB.6aB.6aB.6aB.6aB.6aBaUa4aUaBaUa4aUa4aBa4aUa4aBaSaBaSaBaSaBaSa4aSaBaSaSaSaSaSaSaSaSaSaSawaSaGaSawaSawaSaYaSaYaGaYaGaYaGaYaGaYawaAaGaAaYaAaYaAaYaAaAaAaAaAaAaAaAa6aAa6aAaHaAaWaAaHaAaHaWaHaWaHaWaHaWaHaWaIaWaIaHaIaHaIaHaIajaIaIaIaIaIaIapaIapaIaLaIaLapaLapaLaZaLaZaLaZahaZahaLaNaLaNahaNahaNaNaqaNaqaNakaNaXaqaQaqaQaXaQaXasaXasaXasasaOasaOaOaOaOaFaOa1aOa1aFaRaFaRa1aVa1aVa1a5a9",
"a8a2a8a2a8a2a0aJa0aJa0aJa0aJaJaEaJaEaJaEaJaEaJataEataEataEatatazatanatauatauataaataaataaanaaalaaalaaalaaalaaal#lal#9aa#Cal#Caa#Caa#C#l#C#l#C#l#C#C#C#C#C#C#C#C#p#C#p#C#p#C#p#C#p#C#H#p#F#p#H#p#F#p#H#p#F#p#F#p#t#p#F#p#t#p#t#H#t#H#t#H#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#h#e#u#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h.p#h#e#h.p#h#e#h.p#h#e#h.p.m#e.m.p.m#e.m.p.m#e.m.p.m#e.m.p.m#h.m.p.m#h.m.o.m.m.m.q.m.m.m.q.m.m.m.q.m.m.m.q.m.m.6"
".m.6.m.6.m.6.m.6.m.6.m.6.maT.m.6.6aT.6.6.6aT.6aT.6aT.6aT.6aB.6aB.6a4.6aB.6a4.6a4.6a4aBa4aUa4aBa4aUa4aBa4aBa4aBaSaBaSa4aSaBaSa4aSa4aSaSaSaSaMaSaSaSaMaSaMaSaYaSawaSaYaSaYaSaYaSaYaGaYawaYaGaYaYaYawaAaYaAaYaAaYaAaYaAaAaAaAa6aAa6aAa7aAaHaAaHaAaHaAaHa6aHaWaHaWaHaWaHaHaHaWaIaHaIaHaIaHaIaHaIaIaIaIaIaIaIaIagaIagaIaLaIaLaIaLapaLapaLaZaLaZahaLahaLaNaLaNahaNahaNahaNaNaqaNakaNakaNaQaqaQaqaQaXasaXasaQasaQaOasaOasaOaOaFaOaDaOaFaOaRaFaRaFaRa1aVa1a5aRa5aVa5a5a5",
"a2a0a2a0aJa2aJa2aJaJaEaJaEaJaEaJataJataEataEataEazatatatazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#Caa#C#C#C#C#C#C#C#C#B#C#C#C#B#C#p#C#B#C#F#C#F#p#F#p#F#p#F#p#F#p#F#p#F#p#F#p#t#H#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#t#t#u#t#t#t#u#t#t#t#u#e#t#t#u#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e.s#e#x#e.s#e#x#e.s#e.p#h.p#e.p#h.p#e.p#h.p#e.p#h.p#e.p#h.p#e.p#h.p#h.p#h.p#h.p#h.p#h.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.o.m.o.m.o.m.q.m.o.m.q"
".m.o.m.q.m.q.6.m.6.q.6.m.6.q.6.m.6.q.6.m.6.6.6.6.6.6.6.6aT.6.6.6aT.6aB.6aB.6aB.6aB.6aB.6a4.6aBaUa4aUa4aUa4aUa4aBa4aUa4aBaSaBaSaBaSaBaSa4aSaBaSaSaSaSaSaSaSaSaSaSaMaSaGaSaYaSawaSaYaSaYaGaYaGaYaGaYaGaYaYaYaGaAaYaAaYaAaYaAaYaAaAa6aAaAaAa6aAa6aAaHaAaWaAaHaAaHa6aHaWaHaWaHaWaHaWaIaWaIaHaIaWaIaHaIajaIaIaIaIaIaIapaIapaIaLaIaZaIaLapaLaZaLaZaLaZahaZahaLaNaZaNahaNahaNaNaNaNaqaNaqaNaqaNaQaqaQaqaQaXasaXasaXasaQaOasaOaOaOaOaFaOaFaOa1aFaRaFaRa1aVa1aVa1a5a9a5a5",
"a8a2a8a2a8a2a0a2a0aJa0aJa0aJa0aEaJaEaJaEaJataJataJataEataEataEatatatatanatauatauataaataaataaataaalaaalaaalaaalaaal#lal#9aa#Cal#Caa#Caa#C#l#C#l#C#l#C#C#C#C#C#C#C#C#p#C#p#C#p#C#p#C#p#C#H#p#F#C#H#p#F#p#H#p#F#p#F#p#t#p#F#p#t#p#t#p#t#p#t#H#t#H#t#H#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#h#e#u#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m.p.m#h.m.p.m#h.m.p.m#h.m.n.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.6.m.6"
".m.6.m.6.m.6.m.6.m.6.maT.6.6.6aT.6aT.6aT.6aT.6aT.6aT.6a4.6aB.6a4.6a4.6a4.6a4.6a4aBa4aUa4aBa4aBa4aBa4aBa4a4aSaBaSa4aSa4aSa4aSa4aSaSaSaSaMaSaSaSaMaSaMaSaYaSaYaSaYaSaYaSaYaSaYaGaYawaYaGaYaYaYaYaAaYaAaYaAaYaAaYaAaYaAaAa6aAa6aAa7aAa6aAaHaAaHaAaHa6aHa6aHaWaHaWaHaHaHaWaIaHaIaHaIaHaIaHaIaIaIaIaIaIaIaIaKaIapaIaLaIaLaIaLapaLapaLaZaLaZahaLahaLaNaLaNahaNahaNahaNaNaqaNakaNakaNaQaqaQaqaQaXaQaXasaQasaQaOasaOasaOaOaOaOaFaOaFaOaRaFaRaFaRa1aVa1a5aRa5aVa5a5a5a5b.",
"a2a8a2a0a2a0aJa2aJa2aJaJaEaJaEaJaEaJataJataEataEataEazatatatazatauatauatauataaataaataaanaaanaaaaaaalaaaaaaaa#Caa#Caa#Caa#Caa#C#l#Caa#C#l#C#C#C#C#C#C#p#C#C#C#B#C#p#C#B#C#F#C#F#p#F#p#F#p#F#p#F#p#F#p#F#p#F#p#t#p#t#H#t#H#t#H#t#H#t#H#t#u#t#H#t#u#t#u#t#t#t#u#t#t#t#u#t#t#t#u#e#t#e#u#e#t#e#u#x#t#x#u#x#t#x#t#x#t#x#t#x#e#x#t#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#x#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h.p#e#e#h.p#e#e#h.p#e#e#h.p#e.p#h.p#h.p#h.p#h.p#h.p#h.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.p.m.o.m.o.m.q.m.o.m.m.m.o.m.m.m.q"
".m.m.m.q.6.m.6.q.6.m.6.m.6.m.6.m.6.m.6.6.6.6.6.6.6.6aT.6.6.6aT.6aB.6aB.6aB.6a4.6aB.6a4.6a4aUa4aUa4aUa4aUa4aBa4aUa4aBaSaBaSa4aSaBaSa4aSa4aSaSaSaSaSaSaSaSaSaSaMaSaGaSaYaSaYaSaYaSaYaGaYaGaYaGaYaGaYb#aYb#aAaYaAaYaAaYaAaYaAaAa6aAa6aAa6aAa6aAaHaAaWa6aHaAaHa6aHaWaHaWaHaWaHaWbaaWaIaHaIaWaIaHaIajaIaIaIaIaIaIaIaIapaIaLaIaZaIaLapaLaZaLaZaLaZahaZahaLaNaZaNahaNahaNaNaNaNaqaNaqaNaqaNaQaqaQaqaQaXaQaXasaXasaQaOaPaOasaOaOaOaOaFaOa1aOaRaFaRa1aRa1aVa1a5a1a5a5a5a5",
"bba2a8a2a8a2a8aJa0aJa0aJa0aJa0aJa0aEaJaEaJaEaJataJataJataEataEataEatatatatanatauatanataaataaataaataaalaaalaaalaaalaaal#lalaaaa#Cal#Caa#Cal#C#l#Caa#C#l#C#l#C#C#C#C#C#C#p#C#p#C#p#C#p#C#p#C#p#p#H#C#H#p#F#p#H#p#F#p#H#p#F#p#F#p#t#p#t#p#t#p#t#p#t#p#t#H#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#h#e#u#e#h#e#u#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#h.m#e.m#h.m#e.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.6.m.6.m.6.m.6"
".m.6.m.6.m.6.m.6.maT.6.6.6aT.6aT.6.5.6aT.6.5.6aT.6a4.6a4.6a4.6a4.6a4.6a4.6a4aBa4aUa4aBa4aBa4aBa4aBa4a4aSaBaSa4aSa4aSa4aSa4aSaSaSaSaMaSaSaSbcaSaMaSaYaSaYaSaYaSaYaSaYaSaYaGaYaYaYaGaYaYaYaYaYaYaAaYaAaYaAaYa6aYaAaAa6aAa6aAa7aAa6aAaHa6aHaAaHa6aHa6aHaWaHaWaHaWaHaWbaaHaIaHaIaHaIaHaIaIaIaIaIaIaIaIaKaIapaIaLaIaLaIaLapaLapaLaZaLaZaLaLahaZaNaLaNaLaNahaNahaNaNaNaNakaNaqaNaQaNaQaqaQaXaQaXasaQasaXaOaQaOasaOaOaOaOaFaOaFaOaRaOaRaFaRa1aRa1aVaRa5a1a5a5a5a5a5a5b.",
"a2a8a2a8a2a0aJa0aJa2aJa2aJaJaEaJaEaJaEaJataJataEataEataEazatatatazatauataaatauataaataaataaalaaanaaaaaaalaaaaaaal#Caa#Caa#Caa#Caa#Caa#Caa#C#l#C#C#C#C#C#C#C#C#C#C#B#C#p#C#B#C#p#C#F#p#F#C#F#p#F#p#F#p#F#p#F#p#F#p#t#p#t#p#t#p#t#H#t#H#t#H#t#H#t#H#t#u#t#u#t#u#t#u#t#t#t#u#t#t#t#u#e#t#t#u#e#t#e#u#e#t#e#u#e#t#e#u#e#t#e#u#e#t#e#u#e#e#e#u#e#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h#e#h#e#h.p#h#e#h.p#h#e#h.p#h#e#h.p.m#e.m.p.m#e.m.p.m#e.m.p.m#e.m.p.m#e.m.p.m#e.m.o.m.n.m.o.m.m.m.o.m.m.m.o.m.m.m.m.m.m"
".6.m.6.m.6.m.6.m.6.m.6.m.6.m.6.m.6.6.6.6.6.6.6.6aT.6.6.6aT.6aB.6a4.6aB.6a4.6a4.6a4.6a4aUa4aUa4aUa4aUa4aBa4aUa4aBaSaBaSa4aSa4aSa4aSa4aSaSaSaSaSaSaSaSaSaSbcaSaGaSaYaSaYaSaYaSaYaSaYaGaYaGaYaGaYb#aYb#aAaYaAaYaAaYaAaYaAaAa6aAa6aAa6aAa6aAaHaAaWa6aHaAaHa6aHaWaHaWaHaWaHaWbaaWaIaHaIaWaIaHaIaHaIaIaIaIaIaIaIaIapaIaLaIaZaIaLaIaLaZaLaZaLaZaLaZahaZaNaZaNaLaNahaNaNaNaNaNaNaqaNaqaNaQaqaQaqaQaqaQaXasaXasaXaOaXaOasaOasaOaOaFaOaFaOaRaFa1a1aRa1aVa1a5a1a5aVa5bda5a5",
"bba2bba2a8a2a8a2a8aJa0aJa0aJa0aJa3aJa3aEaJaEaJaEaJataJataEataEataEataEatatatatanatauatanataaataaataaataaalaaalaaalaaalaaal#lalaaaa#Cal#Caa#Cal#C#l#Caa#C#l#C#l#C#C#C#C#C#C#p#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#F#p#H#p#F#p#H#p#F#p#H#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t.l#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#h#e#u#e#h#e#u#e#h#e.l#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#h.m#e.m#h.m#e.m#h.m#e.m.m.m.n.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.6.m.6.m.6.m.6.m.6"
".m.6.maT.m.6.m.5.6.6.6.5.6aT.6.5.6.5.6.5.6.5.6a4.6a4.6a4.6a4.6a4.6a4.6a4aBa4aUa4aBa4aBa4a4a4aBa4a4aSa4aSa4aSa4aSa4aSa4aSaSaSaSaMaSaSaSbcaSbcaSaYaSaYaSaYaSaYaSaYaSaYaGaYaYaYb#aYaYaYaYaYaYaAaYaAaYaAaYa6aYa6aAa6aAa6aAa7aAa6aAaHa6aHa6aHa6aHa6aHaWaHaWbaaWaHaWbaaHaIaHaIaHaIaHaIbaaIaIaIaIaIaIaKaIapaIaLaIaLaIaLaIaLapaLaZaLaZaLaLahaZaNaLaNaLaNahaNahaNaNaNaNakaNaqaNaQaNaQaqaQaXaQaXaQaXasaXaOaQaOasaOasaOaOaOaOaFaOaRaOaRaFaRa1aRa1aVa1aVa1a5aVa5a5a5a5b.a5be",
"a2a8a2a8a2a0aJa0aJa0aJa2aJa2aEaJaEaJaEaJaEaJataJataEataEataEazatatatazatauataaataaataaataaataaalaaanaaalaaalaaaaaaal#laa#Caa#Caa#Caa#Caa#Caa#C#l#C#C#C#C#C#C#C#C#C#C#p#C#p#C#B#C#p#C#B#C#F#C#F#p#F#p#F#p#F#p#F#p#F#p#t#p#F#p#t#p#t#p#t#H#t#H#t#H#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#t#t#u#e#t#e#u#e#t#e#u#e#t#e#u#e#t#e#u#e#t#e#u#e#e#e#u#e#e#e#h#e#e#e#h#e#e#e#h#e#e#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m.n.m.m.m.n.m.m.m.n.m.m.m.m.m.m.m.m.m.m.6.m"
".6.m.6.m.6.m.6.m.6.m.6.m.6.m.6.6.6.6aT.6.6.6aT.6.6.6.5.6aB.6a4.6a4.6a4.6a4.6a4.6a4aUa4aUa4aUa4aUa4aBa4aUa4a4aSaBaSa4aSa4aSa4aSa4aSaSaSaSaSaSaSaSaSaSbcaSb#aSaYaSaYaSaYaSaYaSaYaGaYb#aYaGaYb#aYb#aAaYaAb#aAaYaAaYaAaYa6aAa6aAa6aAa6aAa6aAaWa6aHa6aHa6aHa6aHaWaHaWaHaWbaaWaIaHaIaWaIaHaIaHaIaIaIaIaIaIaIaIaIaIaZaIaZaIaLaIaLaZaLaZaLaZaLaZahaZaNaZaNaLaNahaNaNaNaNaNaNaqaNaqaNaQaNaXaqaQaqaQaXaQaXasaXaOaXaOasaOasaOaOaOaOaFaOa1aOa1a1aRa1aRa1a5a1a5a1a5a9a5a5a5a5",
"bba8bba2bba2a8a2a8a2a8aJa0aJa0aJa0aJa3aJa3aEaJaEaJaEaJataJataEataEataEataEatatatatanatauatanataaataaataaataaalaaalaaalaaalaaal#lalaaal#Cal#Caa#Cal#Caa#Caa#C#l#C#l#C#l#C#C#C#C#p#C#C#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#H#p#F#p#H#p#F#p#H#p#t#p#H#p#t#p#u#p#t#p#t#p#t#p#t#p#t#p#t.l#t#p#t.l#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e.l#e.l#e.l#e.l#e.m#e.l#e.m#e.l#e.m#e.l#e.m#e.l#h.m#e.m#h.m#e.m#h.m#e.m#h.m.n.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.m.6.m.6.m.6.m.k.m.6.m.k.m.6"
".m.5.m.6.6.5.6.k.6.5.6.5.6.5.6.5.6.5.6.5.6a4.6a4.6a4.6a4.6a4.6a4.6a4aBa4aUa4a4a4aBa4a4a4a4a4a4aSa4aSa4aSa4aSa4aSa4aSaSaSaSbcaSaSaSbcaSbcaSaYaSaYaSaYaSaYaSaYaSaYaGaYb#aYb#aYaYaYaYaYaYaAaYaAaYaAaYa6aYa6aAa6aAa6aAa7aAa6aAa7a6aHa6aHa6aHa6aHa6aHaWbaaWbaaWbaaHaIaHaIaHaIaHaIbaaIaIaIaIaIaIaIaIaIaIaLaIaLaIaLaIaLapaLaZaLaZaLaLahaZaNaLaNaLaNaLaNahaNaNaNaNaNaNaqaNaQaNaQaNaQaXaQaXaQaXasaXaOaQaOaQaOasaOaOaOaOaFaOaRaOa1aOaRa1aRa1aRa1aVa1a5aRa5aVa5a5a5a5bea5be",
"a2a8a2a8a2a8a2a0aJa0aJa0aJa2aJa2aEaJaEaJaEaJaEaJataJataEataEataEazatatatauatauataaataaataaataaataaalaaanaaalaaalaaaaaaal#laa#Caa#Caa#Caa#Caa#Caa#C#l#C#l#C#C#C#C#C#C#C#C#p#C#p#C#B#C#p#C#B#C#p#C#F#p#F#p#F#p#F#p#F#p#F#p#F#p#F#p#t#p#t#p#t#p#t#p#t#H#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#h#e#u#e#h#e#u#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e#h#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m.n.m.n.m.n.m.m.m.n.m.m.m.n.m.m.m.m.m.m.m.m.6.m.6.m.6.m"
".6.m.6.m.6.m.6.m.6.m.6.6.6.6.6.6.5.6.6.6.5.6.6.6.5.6a4.6a4.6a4.6a4.6a4.6a4.6a4aUa4aUa4aUa4aUa4a4a4aUa4a4aSa4aSa4aSa4aSa4aSa4aSaSaSaSaSaSbfaSaSaSbcaSb#aSaYaSaYaSaYaSaYaSaYaGaYb#aYb#aYb#aYb#aYaYaAb#aAaYa6aYaAaYa6aAa6aAa6aAa6aAa6aAaWa6aHa6aHa6aHa6aHaWaHaWaHaWbaaWbaaWaIaWaIaHaIaHaIaIaIaIaIaIaIaIaIaIaZaIaZaIaLaIaLaZaLaZaLaZaLaZaLaZaNaZaNaLaNaLaNaNaNaNaNaNaNaNaqaNaQaNaXaqaQaqaQaXaQaXasaXaOaXaOaQaOasaOaOaOaOaFaOa1aOa1aFaRa1aRa1aVa1a5a1a5a9a5a5a5a5b.a5",
"bba8bba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aJa3aEaJaEaJaEaJataJataEataEatatatatatatatatanataaatanataaataaataaataaalaaalaaalaaalaaalaaalaaal#lal#Caa#Cal#Caa#Cal#C#l#C#l#C#l#C#l#C#C#p#C#C#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#p#p#H#p#H#p#F#p#H#p#F#p#H#p#t#p#u#p#t#p#u#p#t#p#t#p#t#p#t#p#t#p#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#e.l#t.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#h.l#e.l#h.m#e.l#h.m#e.l#h.m.n.l.m.m.m.l.m.m.m.l.m.m.m.l.m.m.m.l.m.m.m.l.m.m.m.k.m.6.m.k.m.6.m.k.m.k.m.k.m.k.m.5"
".m.k.6.5.6.5.6.5.6.5.6.5.6.5.6.5.6a4.6a4.6a4.6a4.6a4.6a4.6a4.6a4a4a4aUa4a4a4a4a4a4a4a4a4a4aSa4aSa4aSa4aSa4aSa4bfaSaSaSbcaSbfaSbcaSbcaSbcaSaYaSaYaSaYaSaYbfaYaSaYb#aYb#aYaYaYaYaYaYaAaYa6aYaAaYa6aYa6aYa6aAa6aAa7aAa6aAa7a6aHa6aHa6aHa6aHa6aHaWbaaWbaaWbaaHbaaWaIaHaIaHaIbaaIbaaIaIaIaIbgaIaIaIaLaIaLaIaLaIaLaIaLaZaLaZaLaZaLaZaNaLaNaLaNaLaNahaNaNaNaNaNaNaqaNaQaNaQaNaQaqaQaXaQaXaQaXaOaQaOaQaOasaOasaOaOaOaOa1aOa1aOaRa1aRa1aRa1aVa1a5aRa5aVa5a5a5a5bea5bea5bh",
"a2bba2a8a2a8a2a8a2a0aJa0aJa0aJa2aJa2aEaJaEaJaEaJaEaJataJataEataEataEazatatatauatauataaataaataaataaataaalaaanaaalaaalaaaaaaal#laa#Caa#Caa#Caa#Caa#Caa#C#l#C#l#C#l#C#C#C#C#C#C#p#C#p#C#p#C#p#C#B#C#p#C#B#p#H#p#F#p#F#p#F#p#F#p#F#p#F#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t#H#t#u#t#H#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#h#e#u#e#h#e#u#e#h#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m#e.m.n.m.n.m.m.m.n.m.m.m.n.m.m.m.m.m.m.m.m.m.m.6.m.6.m.6.m.6.m.6.m"
".6.m.6.m.6.m.k.m.6.6.k.6.6.6.5.6.k.6.5.6.5.6.5.6a4.6a4.6a4.6a4.6a4.6a4.6a4aUa4aUa4bia4aUa4a4a4bia4a4aSa4aSa4aSa4aSa4aSa4aSaSaSaSaSaSbfaSbfaSbcaSb#aSaYaSaYaSaYaSaYbfaYaGaYb#aYb#aYb#aYb#aYaYaAb#aAaYa6aYa6aYa6aAa6aAa6aAa6aAa6aAaWa6aHa6aHa6aHa6aHaWbaaWaHaWbaaWbaaWaIaWaIbaaIaHaIbaaIaIaIaIaIaIaIaIaZaIaZaIaLaIaLbgaLaZaLaZaLaZaLaZaNaZaNaLaNaZaNaNaNaNaNaNaNaNaqaNaXaNaXaNaQaqaQaXaQaXasaXaOaXaOaQaOaPaOaOaOaOaFaOa1aOa1aOaRaFaRa1aRa1a5a1a5a1a5a5a5a5a5a5bea5",
"bba8bba8bba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aJa3aEaJaEaJaEaJataJataEataEatatatatatatatatanataaatanataaataaataaataaalaaalaaalaaalaaalaaalaaal#lal#Caa#Cal#Caa#Cal#C#l#C#l#C#l#C#l#C#l#C#C#C#C#p#C#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#H#p#H#p#F#p#H#p#t#p#H#p#t#p#u#p#t#p#u#p#t#p#u#p#t#p#u.l#t#p#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#e.l#t.l#e.l#t.l#e.l#e.l#e.l#e.l#e.l#h.l#e.l#h.l#e.l#h.l#e.l#h.l#e.l#h.l#e.l#h.l#e.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.5.m.k.m.5.6.k"
".6.5.6.5.6.5.6.5.6.5.6.5.6.5.6a4.6a4.6a4.6a4.ka4.6a4.ka4.6a4a4a4bia4a4a4a4a4a4a4a4a4a4aSa4aSa4aSa4aSa4aSa4bfa4aSaSbcaSbfaSbcaSbcaSbcaSaYaSaYaSaYaSaYbfaYbfaYb#aYb#aYaYaYaYaYaYaAaYa6aYa6aYa6aYa6aYa6aAa6aAa7a6a6aAa7a6aHa6aHa6aHa6aHa6aHaWbaaWbaaWbaaHbaaWaIaHaIaHaIbaaIbaaIaIaIaIbgaIaIaIaLaIaLaIaLaIaLaIaLaZaLaZaLaZaLaZaNaLaNaLaNaLaNahaNaNaNaNaNaNaqaNaQaNaQaNaQaqaQaXaQaXaQaXaOaQaOaXaOaQaOasaOaOaOaOa1aOa1aOaRaOaRa1aRa1aRa1a5aRa5a1a5a5a5a5bja5bea5bhbjbh",
"bkbba2bba2a8a2a8a2a8a2a0aJa0aJa0aJa2aJa2aEaJaEaJaEaJaEaJataJataEataEataEazatatatauatauataaataaataaataaataaalaaanaaalaaalaaaaaaal#laa#Cal#Caa#Caa#Caa#Caa#C#l#C#l#C#l#C#C#C#C#C#C#p#C#p#C#p#C#p#C#B#C#p#C#B#p#p#C#F#p#H#p#F#p#F#p#F#p#F#p#t#p#F#p#t#p#t#p#t#p#t#p#t#p#t#p#t.l#t#p#t.l#t.l#t#u#t#u#t#u#t#u#t#u#t#u#t#u#t#u#e#u#t#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e#u#e.l#e.l#e.l#e.l#e.l#e.m#e.l#e.m#e.l#e.m#e.l#e.m#e.l#e.m#e.m#e.m#e.m#e.m#e.m.n.m.m.m.n.m.m.m.n.m.m.m.n.m.m.m.m.m.m.l.m.m.m.k.m.6.m.k.m.6.m.k.m.6.m"
".k.m.6.m.k.6.k.6.k.6.k.6.5.6.k.6.5.6.5.6.5.6a4.6a4.6a4.6a4.6a4.6a4.6a4bia4aUa4bia4bia4a4a4bia4a4aSa4aSa4aSa4aSa4aSa4aSa4bfaSaSaSbfaSbfaSbcaSbfaSaYaSaYaSaYaSaYbfaYaGaYb#aYb#aYb#aYb#aYaYaAb#aAaYa6aYa6aYa6aAa6aAa6aAa6a6a6aAa6a6aHa6aWa6aHa6aHa6baaWbaaWbaaWbaaWaIaWaIbaaIblaIbaaIaIaIaIaIaIaIaIbgaIaZaIaLaIaZbgaLaZaLaZaLaZaLaZaNaZaNaLaNaZaNaNaNaNaNaNaNaNaqaNaXaNaXaNaQaqaQaXaQaXaQaXaOaXaOaXaOaPaOaOaOaOaOaOa1aOa1aOaRaFa1a1aRa1aVa1a5a1a5a5a5bda5a5bea5bebj",
"bba8bba8bbbkbba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aEa3aEaJaEaJaEaJataJataEataEatatatatatatatatatataaatanataaataaataaataaalaaalaaalaaalaaalaaalaaal#lal#lal#Cal#Caa#Cal#C#l#C#o#C#l#C#l#C#l#C#l#C#C#p#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#p#p#p#p#p#p#H#p#H#p#H#p#H#p#t#p#u#p#t#p#u#p#t#p#u#p#t#p#u#p#t#p#u.l#t#p#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#e.l#u.l#e.l#u.l#e.l#h.l#e.l#h.l#e.l#h.l#e.l#h.l#h.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.5.m.k.m.5.6.k.6.5"
".6.5.6.5.6.5.6.5.6.5.6.5.ka4.6a4.ka4.6a4.ka4.ka4.5a4.ka4a4a4bia4a4a4a4a4a4a4a4a4a4aSa4aSa4aSa4bfa4aSa4bfa4bfaSbcaSbfaSbcaSbcaSbcaSaYaSaYbfaYaSaYbfaYbfaYb#aYb#aYaYaYb#aYaYaYaYa6aYa6aYa6aYa6aYa6aAa6aAa7a6a6a6a7a6aHa6aHa6aHa6aHa6aHa6baaWbaaWbaaHbaaWaIbaaIaHaIbaaIbaaIaIaIaIbgaIbgaIbmaIaZaIaLbgaLaIaLaZaLaZaLaZaLaZbnaLaNaLaNaLaNaLaNaNaNaNaNaNaNaNaQaNaXaNaQaNaQaqaQaXaQaXaQaQaOaXaOaQaOasaOaOaOaOaOaOa1aOaRaOaRaFaRa1aRa1a5a1a5a1a5a5a5a5a5a5bea5bebjbebebh",
"a8bbbkbba2bba2a8a2a8a2a8a2a0aJa0aJa0aJa2aJa2aEaJaEaJaEaJaEaJataJataEataEataEazatatatazatauataaataaataaataaataaalaaanaaalaaalaaaaaaal#laaaaal#Caa#Caa#Caa#Caa#C#l#C#l#C#l#C#l#C#C#C#C#p#C#C#C#p#C#p#C#p#C#p#C#B#p#p#C#B#p#p#p#F#p#H#p#F#p#F#p#F#p#F#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t.l#t#p#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#e.l#t.l#e.l#t.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.m#e.l#e.m#e.l#e.m.m.l.n.m.m.l.n.m.m.l.n.m.m.l.m.m.m.l.m.m.m.l.m.6.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m"
".k.m.k.6.k.6.5.6.k.6.5.6.k.6.5.6.5.6a4.6a4.6a4.6a4.6a4.6a4.ka4.6a4bia4bia4a4a4bia4a4a4bia4a4aSa4aSa4aSa4aSa4bfa4aSa4bfaSbfaSbfaSbfaSbcaSbfaSaYaSaYaSaYaSaYbfaYbfaYb#aYb#aYb#aYb#aYaYa6b#aAaYa6aYa6aYa6aYa6aAa6aAa6a6a6a6a6a6aHa6aWa6aHa6aHa6baaWbaaWbaaWbaaWbaaWaIbaaIblaIbaaIaIaIaIbgaIaIaIbgaIaZaIaLaIaZbgaLaZaLaZaLaZaLaZbnaZaNaZaNaZaNbnaNaNaNaNaNaNaNaNaXaNaXaNaQaNaQaXaQaXaQaXaOaXaOaXaOaXaOaOaOaOaOaOa1aOa1aOa1aOa1a1aRa1aRa1a5a1a5boa5bda5a5bja5bea5bebe",
"bbbbbba8bba8bbbkbba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aEa3aEaJaEa3aEaJataJataEataEatatatatatatatatatataaatanataaatalataaataa#iaaalaaalaaalaaalaaalaaal#lal#lal#Cal#Caa#Cal#C#l#C#o#C#l#C#l#C#l#C#l#C#C#p#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#H#p#u#p#H#p#u#p#u#p#t#p#u#p#t#p#u#p#t#p#u#p#t#p#u.l#t#p#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#e.l#u.l#e.l#u.l#h.l#u.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.5.m.k.m.5.m.k.6.5.6.5.6.5.6.5"
".6.5.k.5.6.5.k.5.6a4.ka4.ka4.ka4.ka4.ka4.ka4.5a4bia4a4a4bia4a4a4a4a4a4a4a4a4a4aSa4bfa4aSa4bfa4bfa4bfa4bfaSbcaSbfaSbcaSbcaSbcbfaYaSaYbfaYbfaYbfaYbfaYb#aYb#aYaYaYb#bpaYaYaYa6aYa6aYa6aYa6aYa6aAa6aAa7a6a6a6a7a6a7a6aHa6aHa6baa6aHa6baaWbaaWbaaHbaaWbabaaIbaaIbaaIbaaIbaaIaIbgaIbgaIbmaIaZaIaLbgaLbgaLbgaLaZaLaZaLaZbnaLaNaZaNaLaNaLaNaNaNaNaNaNaNaNaQaNaXaNaQaNaQaqaQaXaQaXaQaXaOaXaOaQaOaQaOaOaOaOaOaOaFaOaRaOaRaFaRa1aRa1bqa1a5a1a5bqa5a5a5a5a5a5bea5bebjbhbebh",
"bkbba8bbbkbba2bba2a8a2a8a2a8a2a0aJa0aJa0aJaJaJaJaEaJaEaJaEaJaEaJataJataEataEataEazatatatazatauataaataaataaataaataaataaanaaalaaalaaalaaal#laaaaal#Caa#Caa#Caa#Caa#C#l#C#l#C#l#C#l#C#l#C#C#p#C#C#C#p#C#p#C#p#C#p#C#p#C#p#C#B#p#p#p#B#p#p#p#F#p#H#p#F#p#F#p#t#p#F#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t.l#t#p#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#e.l#t.l#e.l#t.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l.m.l.n.l.m.l.n.l.m.l.n.l.m.l.m.l.m.l.m.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.6"
".k.6.k.6.5.6.k.6.5.6.k.6.5.6.5.6a4.6a4.ka4.6a4.ka4.6a4.ka4.ka4bia4bia4a4a4bia4a4a4bia4a4aSa4aSa4bfa4aSa4bfa4bfa4bfaSbfaSbfaSbfaSbcaSbfaSaYaSaYbfaYaSaYbfaYbfaYb#aYb#aYb#aYb#aYaYbpb#aAaYa6aYa6aYa6aYa6aAa6aAa6a6a6a6a6a6aHa6aWa6aHa6aHa6baaWbaaWbaaWbaaWbaaWaIbaaIblaIbaaIbaaIaIbgaIaIaIbgaIaZaIaLaIaZbgaLbgaLaZaLaZaLaZbnaZaNaZaNaZaNbnaNaNaNaNaNaNaNaNaXaNaXaNaQaNaXaXaQaXaQaXbraXaOaXaOaXaOaOaOaOaOaObsaOa1aOa1aOa1a1aRa1aRa1a5a1a5a1a5bda5a5bja5bea5bebjbebe",
"btbbbbbbbba8bba8bbbkbba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aEa3aEaJaEa3aEaJataJataEataEatatatatatatatatatatanatanataaatalataaataa#iaaanaaalaaalaaalaaalaaal#lal#lal#lal#Cal#Cal#C#l#C#o#C#l#C#o#C#l#C#l#C#l#p#l#C#C#p#C#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p.l#p#u#p#u#p#u#p#u#p#u#p#u#p#u#p#u#p#u#p#u.l#u#p#u.l#u.l#u.l#u.l#u.l#u.l#u.l#u.l#u.l#u.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.m.l.l.l.m.l.l.l.m.l.l.l.m.l.l.l.m.l.l.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.5.l.k.m.5.l.k.m.5.l.k.6.5.k.5.6.5.k.5.6.5"
".k.5.k.5.k.5.ka4.ka4.ka4.ka4.ka4.ka4.ka4.5a4bia4a4a4a4a4a4a4a4a4a4a4a4.ia4aSa4bfa4bfa4bfa4bfa4bca4bfaSbcaSbfaSbcaSbcaSbcbfaYaSaYbfaYbfaYbfaYbfaYb#aYb#aYaYaYb#bpaYbpaYa6aYa6aYa6aYa6aYa6aYa6aAa7a6a6a6a7a6a7a6aHa6aHa6baa6baa6baaWbaaWbaaHbaaWbabaaIbaaIbaaIbaaIbaaIaIbgaIbgaIbmaIbgaIaLbgaLbgaLbgaLaZaLaZaLaZbnaLaNaZaNaLaNaLaNbnaNaNaNaNaNaNbuaNaXaNaQaNaQaNaQaXaQaXaQaXasaXaOaQaOaQaOaOaOaOaOaOaFaOaRaOa1aOaRa1aRa1bqa1a5a1a5bqa5a5a5a5a5a5bea5bebjbhbebhbebv",
"bbbbbkbba8bba2bba2bba2a8a2a8a2a0a2a0aJa0aJa0aJaJaJaJaEaJaEaJaEaJaEaJataJataEataEataEazatatatazatanataaataaataaataaataaataaanaaalaaalaaalaaalaaaaaaal#laa#Cal#Caa#Caa#C#l#C#l#C#l#C#l#C#l#C#l#C#C#C#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#B#p#p#p#B#p#H#p#F#p#H#p#F#p#H#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t#p#t.l#t#p#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#e.l#t.l#e.l#t.l#e.l#e.l#e.l#e.l#e.l#e.l#e.l#h.l#e.l.m.l.n.l.m.l.n.l.m.l.n.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.6.5.6"
".k.6.5.6.k.6.5.6.5.k.5.6.5.ka4.6a4.ka4.ka4.ka4.ka4.ka4.ka4bia4bia4a4a4bia4a4a4a4a4a4aSa4aSa4bfa4aSa4bfa4bfa4bfaSbfaSbfaSbfaSbcaSbfbfaYaSaYbfaYbfaYbfaYbfaYb#aYb#aYb#aYb#aYaYbpb#a6aYa6aYa6aYa6aYa6aAa6aAa6a6a6a6a6a6a7a6aWa6aHa6aHa6baaWbaaWbaaWbaaWbaaWaIbaaIblaIbaaIbaaIaIbgaIbgaIbgaIbgaIaLaIaZbgaLbgaLaZaLaZaLaZbnaZaNaZaNaZaNbnaNbnaNaNaNaNaNaNbwaNaXaNaQaNaXaXaQaXaQaXbraXaOaXaOaXaObraOaOaOaObsaOa1aOa1aOa1a1aRa1aRa1bqa1a5a1a5boa5a5a5a5bja5bea5bebebhbe",
"btbbbtbbbbbbbba8bba8bbbkbba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aEa3aEaJaEa3aEaJataJataEataEatatatatatatatatatatanatanataaatalataaataa#iaaataaalaaalaaalaaalaaal#lal#lal#lal#lal#Cal#C#l#Cal#C#l#C#o#C#l#C#o#C#l#C#l#C#l#p#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.l.l#p.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.m.k.l.k.m.k.l.k.m.k.l.k.m.5.l.k.m.5.l.k.m.5.l.k.l.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5"
".k.5.k.5.ka4.ka4.ka4.ka4.ka4.5a4.ka4.5a4bia4a4a4a4.ia4a4a4.ia4a4a4bfa4bfa4bfa4bfa4bfa4bfa4bca4bfaSbcaSbfaSbcbfbcaSbcbfaYbfaYbfaYbfaYbfaYbfaYb#aYb#bpaYaYb#bpaYbpaYa6aYa6aYa6aYa6aYa6bpa6aAa7a6a6a6a7a6a7a6aHa6aHa6baa6baa6baaWbaaWbaaHbaaWbabaaIbaaIbaaIbaaIbaaIaIbgaIbgaIbmaIbgaIaLbgaLbgaLbgaLaZaLaZaLaZbnaLbnaZaNaLaNaLaNbnaNaNaNaNaNaNbwaNaXaNaQaNaQaNaQaXaQaXaQaXaQaXaOaQaOaQaOaOaOaOaOaOaOaOa1aOa1aOaRa1aRa1aRa1bqa1a5aRa5boa5a5a5a5bja5bea5bhbebhbebvbebx",
"bbbbbbbbbkbba8bba2a8a2a8a2a8a2a8a2a0a2a0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataEataEataEatatatatazatanataaataaataaataaataaataaanaaalaaalaaalaaalaaalaaal#laa#lal#Caa#Caa#C#l#Caa#C#l#C#l#C#l#C#l#C#l#C#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#B#p#p#p#B#p#p#p#F#p#H#p#t#p#H#p#t#p#u#p#t#p#t#p#t#p#t#p#t#p#t#p#t.l#t#p#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#t.l#e.l#u.l#e.l#u.l.n.l.l.l.n.l.m.l.n.l.m.l.n.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.l.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.m.k.6.k.6.5.6.k.k"
".5.6.k.k.5.6.5.k.5.6.5.ka4.ka4.ka4.ka4.ka4.ka4.ka4bia4bia4bia4a4a4bia4a4.ia4a4a4bfa4aSa4bfa4bfa4bfa4bfa4bfaSbfaSbfaSbfaSbcaSbfbfaYaSaYbfaYbfaYbfaYbfaYb#aYb#aYb#bpb#aYaYbpb#a6aYa6aYa6aYa6aYa6bpa6aAa6a6a6a6a6a6a7a6aWa6baa6aHa6baa6baaWbaaWbaaWbaaWbabaaIblaIbaaIbaaIaIbgaIbgaIbgaIbgaIaZaIaZbgaLbgaLbgaLaZaLaZbnaZbnaZaNaZaNbnaNbyaNaNaNaNaNaNbwaNaXaNaXaNaXbwaQaXaQaXbraXbraXaOaXaObraObzaOaOaOaObsaOa1aOa1bsaRa1a1a1bqa1a5a1a5a1a5a5a5bdbja5bea5bebjbebebAbe",
"btbbbtbbbbbbbbbbbba8bba8bba2bba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aEa3aEaJaEa3aEaJataJataJataEatatatatatatatatatatanatanataaatalataaataaataaataaalaaalaaalaaalaaal#lal#lal#lal#lal#lal#Cal#Cal#C#l#C#o#C#l#C#o#C#l#C#o#C#l#p#l#p#l#p#C#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.l.l#p.l.l.l#p.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.5.l.k.l.5.l.k.l.5.l.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5"
".ka4.ka4.ka4.ka4.ka4.ka4.5a4.ka4.5a4bi.ia4a4a4.ia4.ia4.ia4.ia4bfa4bfa4bfa4bfa4bfa4bfa4bcaSbfaSbcbfbcaSbcbfbcbfbcbfaYbfaYbfaYbfaYbfaYbfaYaYaYb#bpaYbpaYbpaYbpaYa6aYa6aYa6aYa6aYa6bpa6a6a7a6a6a6a7a6a7a6a7a6aHa6baa6baa6baa6baaWbaaHbaaWbabababaaIbaaIbaaIbaaIbabgaIbgaIbmaIbgaIaLbgaLbgaLbgaLbgaLaZaLaZbnaZbnaZaNaLaNaLaNbnaNaNaNaNaNaNbwaNaXaNaQaNaQaNaQaXaQaXaQaXaQaXaOaQaOaXaObraOaOaOaOaOaOa1aOa1aOaRbsaRa1aRa1aRa1a5a1a5a1a5a5a5a5bja5bea5bebjbebebvbebvbebx",
"bbbbbbbbbbbbbkbba8bba2a8a2a8a2a8a2a8a2a0a2a0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataEataEataEatatatatazatanataaatauataaataaataaataaataaalaaalaaalaaalaaalaaal#laa#lal#Caa#Cal#Caa#Caa#C#l#C#l#C#l#C#l#C#l#C#l#p#C#C#C#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#p#p#p#p#B#p#p#p#B#p#p#p#B#p#p#p#t#p#u#p#t#p#u#p#t#p#u#p#t#p#u#p#t#p#u#p#t.l#u#p#t.l#u#p#t.l#u.l#t.l#u.l#t.l#u.l#t.l#u.l#t.l.l.l#m.l.l.l#m.l.l.l#m.l.l.l.n.l.l.l.m.l.l.l.m.l.l.l.m.l.l.l.m.l.l.l.m.l.l.l.m.k.l.k.m.k.l.k.m.k.l.k.m.k.l.k.m.k.l.k.m.k.l.5.6.k.k.5.6.k.k.5.k"
".5.k.5.k.5.k.5.ka4.ka4.ka4.ka4.ka4.ka4.ka4.ka4bia4bia4bia4a4.ibia4a4.ia4a4a4bfa4bfa4bfa4bfa4bfa4bfa4bfaSbfaSbfaSbfbfbcaSbfbfaYbfaYbfaYbfaYbfaYbfaYb#aYb#aYb#bpb#bpaYbpb#a6aYa6aYa6aYa6aYa6bpa6a6a6a6a6a6a6a6a7a6aWa6baa6baa6baa6baaWbaaWbablbaaWbabaaIblaIbaaIbaaIbabgaIbgaIbgaIbgaIaZaIaZbgaLbgaLbgaLaZaLaZbnaZbnaZaNaZaNbnaNbyaNaNaNaNaNaNbwaNaXaNaXaNaXbwaQaXaQaXaQaXbraXaOaXaObBaObBaOaOaOaObsaOa1aOa1bsaRa1a1a1bqa1bqa1a5a1a5boa5bda5a5bea5bebjbebebebebvbe",
"bCbbbtbbbtbbbbbbbbbbbba8bba8bba2bba0a8a2a8a2a8a2a8a2a0aJa0aJa0aJa0aJa3aEa3aEaJaEa3aEaJataJataJataEatatataEatatatatatatatatanataaatalataaatalataaataa#iaaalaaalaaalaaal#lalaaal#lal#lal#lal#lal#Cal#C#o#C#o#C#l#C#o#C#l#C#o#C#l#p#o#C#l#p#l#p#l#p#C#p#C#p#C#p#C#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.l.l#p.l.l.l#p.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.k.l.k.l.k.l.k.l.k.l.k.l.5.l.k.l.5.l.k.l.5.l.k.l.5.l.k.l.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.ka4.ka4"
".ka4.ka4.ka4.ka4.k.i.5a4.k.ia4a4a4.ia4.ia4.ia4.ia4.ia4.ia4bfa4bfa4bfa4bfa4bfa4bfa4bcbfbfaSbcbfbcbfbcbfbcbfbcbfaYbfaYbfaYbfaYbfaYbfbpaYaYb#bpaYbpaYbpaYbpaYbpaYa6aYa6bpa6aYa6bpa6a6a6a6a6a6a7a6a7a6bDa6aHa6baa6baa6baa6baaWbablbaaWbabababaaIbaaIbaaIbaaIbabgaIbgaIbgaIbgaIbmbgaLbgaLbgaLbgaLaZaLaZbnaZbnaZbnaLaNaLaNbnaNbnaNaNaNaNbwaNbwaNaQaNaQaNaQbwaQaXaQaXaQaXbraQaOaXaObraOaOaOaOaOaObsaOa1aOaRbsaRa1aRa1aRa1a5a1a5a1a5bqa5a5bja5bja5bebjbebebvbebvbebxbAbx",
"bbbtbbbbbbbbbbbbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataEataEataEatatatatazatatataaatauataaataaataaataaataaalaaalaaalaaalaaalaaal#laa#lal#laa#Cal#Caa#Caa#C#l#C#l#C#l#C#l#C#l#C#l#C#l#C#l#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#m#p#p#p#m#p.l#p#m#p.l#p#m#p.l#p#m#p.l#p#m#p.l#p#m.l.l#p#m.l.l#p#m.l.l.l#m.l.l.l#m.l.l.l#m.l.l.l#m.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.k.5.k.k.k.5.k.k.k.5.k.5.k"
".5.k.5.k.5.ka4.ka4.ka4.ka4.ka4.ka4.ka4.ka4bia4a4.ibia4a4.ibia4a4.ia4.ia4bfa4bfa4bfa4bfa4bfa4bfaSbfaSbfbfbcaSbfbfbcbfbfbfaYbfaYbfaYbfaYbfaYbfaYb#bpb#aYb#bpb#bpaYbpb#a6aYa6aYa6aYa6aYa6bpa6a6a6a6a6a6a6a6a7a6aWa6baa6baa6baa6baa6baaWbablbaaWbablbablaIbaaIbaaIbabgaIbgaIbgaIbgaIbgaIaZbgaLbgaLbgaLaZaLaZbnaZbnaZbnaZaNbnaNbyaNbnaNaNaNaNbwaNbwaNaXaNaXbwaQbwaQaXaQaXbraXaOaXaObBaObBaOaOaOaOaOaOa1aOa1bsa1bsa1a1bqa1bqa1a5a1a5boa5bda5a5bja5bebjbebjbebebvbebxbA",
"bEbbbCbbbtbbbtbbbbbbbbbbbba8bba8bba0bba0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aEaJaEa3aEaJataJataJataEatatataEatatatatatatatatanatalatalataaatalataaataa#iaa#iaaalaaalaaalaaalaaal#lal#lal#lal#lal#lal#C#o#Cal#C#o#C#o#C#l#C#o#C#l#C#o#C#l#p#o#p#l#p#l#p#l#p#l#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.l.l#p.l.k.l.h.l.k.l.h.l.k.l.k.l.k.l.5.l.k.l.5.l.k.l.5.l.k.l.5.l.k.l.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.5.k.j.k.5.k.i.ka4.k.i"
".ka4.k.i.5a4.k.i.5.i.k.ia4.ia4.ia4.ia4.ia4.ia4.ia4.ia4bfa4bfa4bfa4bfa4bf.ibfa4bc.ibfbfbcbfbcbfbcbfbcbfbcbfaYbfaYbfaYbfbpbfaYbfbpaYbpb#bpaYbpaYbpaYbpaYbpaYa6aYa6bpa6bpa6bpa6a6a6a6a6a6a7a6a7a6bDa6baa6baa6baa6baa6baaWbablbablbabababaaIbaaIbabgbaaIbabgaIbgaIbgaIbgaIbmbgaLbgaLbgaLbgaLbgaLaZbnaZbnaZbnaLaNaLaNbnaNbnaNaNaNaNbwaNbwaNaQaNaXaNaQbwaQaXaQaXaQaXbraXaOaXaObraObraOaOaOaObsaOa1aOaRbsa1bsaRa1aRa1bqa1a5a1a5bqa5a5bja5bja5bebjbebjbhbebvbebvbAbxbAbF",
"bbbEbbbtbbbbbbbbbbbbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataEataEataEatatatatazatatatauatanataaataaataaataaataa#iaaalaaalaaalaaalaaal#lal#lal#laa#lal#Caa#Cal#C#l#C#l#C#l#C#l#C#l#C#l#C#l#C#l#p#l#p#l#p#C#p#C#p#C#p#C#p#C#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p#m#p.l#p#m#p.l#p.l#p.l#p.l.l.l#p.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.5.k.k.k.5.k.k.k.5.k.5.k.5.k.5.k.5.k"
".5.ka4.ka4.ka4.ka4.ka4.ka4.k.i.ka4.k.ibia4a4.ibi.ia4.ibi.ia4.ia4.ia4bfa4bfa4bfa4bfa4bfa4bf.ibfaSbfbfbcbfbfbfbcbfbcbfaYbfaYbfaYbfaYbfbpbfaYb#bpb#bpb#bpb#bpaYbpb#a6aYa6aYa6bpa6aYa6bpa6a6a6a6a6a6a6a6a7a6a6a6baa6baa6baa6baa6baaWbablbablbablbablaIbaaIbaaIbabgbabgaIbgaIbgaIbgaIaZbgaLbgaZbgaLbgaLaZbnaZbnaZbnaZaNbnaNbyaNbnaNaNaNaNbwaNbwaNaXaNaXbwaQbwaXaXaQaXaQaXbraXaObBaObBaOaOaOaOaOaObsaOa1bsa1bsa1a1aRa1bqa1a5a1a5boa5bda5a5bja5bebjbebjbebebvbebvbAbxbA",
"bCbbbEbbbCbbbtbbbtbbbbbbbba8bba8bba8bba0bba0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aEaJaEa3aEaJataJataJataEataEataEatatatatatatatatatatalatalataaatalataaatal#iaa#iaaalaaalaaalaaalaaal#lal#lal#lal#lal#lal#l#o#Cal#C#o#C#o#C#o#C#o#C#l#C#o#C#l#p#o#C#l#p#o#p#l#p#o#p#l#p#l#p#l#p#l#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.h.l.h.l.h.l.h.l.h.l.h.l.j.l.h.l.j.l.k.l.j.l.k.l.j.l.k.l.j.l.k.l.j.l.5.k.j.k.5.k.j.k.5.k.j.k.5.k.j.k.5.k.j.k.5.k.j.ka4.k.i.k.i.k.i.k.i"
".k.i.5.i.k.i.5.ibi.ia4.ia4.ia4.ia4.ia4.ia4.ia4.ia4bfa4bfa4bf.ibfa4bf.ibf.ibcbfbfbfbcbfbcbfbcbfbcbfbcbfaYbfaYbfaYbfbpbfbpbfbpaYbpb#bpaYbpaYbpaYbpaYbpbpa6aYa6bpa6bpa6bpa6bpa7a6a6a6a7a6a7a6bDa6baa6baa6baa6baa6baa6bablbablbabababababaaIbabgbabgbabgbabgaIbgaIbgaIbmbgaLbgaLbgaLbgbnbgaLaZbnaZbnaZbnaLaNaZaNbnaNbnaNbnaNaNbwaNbwaNbuaNaXaNaQbwaQbwaQaXaQaXbraXaOaXaObraObraOaOaOaObsaObsaOaRbsa1bsaRa1aRa1bqa1a5a1a5bqa5a5bja5bja5bebjbebjbhbebvbebvbAbxbAbxbvbF",
"bbbEbbbtbbbtbbbbbbbbbbbbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataEataEataEatatatatatatatatauatanataaataaataaataaataa#iaaalaaalaaalaaalaaal#lalaaal#lal#lal#laa#Cal#C#l#Cal#C#l#C#l#C#l#C#l#C#l#C#l#p#l#C#l#p#l#p#l#p#C#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.l.l#p.l.l.l#p.l.l.l#p.l.l.l.l.l.l.l.l.l.l.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.k.l.5.k.k.k.5.k.k.k.5.k.k.k.5.k.5.k.5.k.5.k.5.ka4.k"
"a4.ka4.k.i.ka4.k.i.ka4.k.i.k.ibi.ibi.ia4.ibi.ia4.ia4.ia4.ia4bfa4bfa4bfa4bfa4bf.ibfa4bf.ibfaSbfbfbcbfbfbfbcbfbcbfaYbfaYbfaYbfaYbfbpbfaYb#bpb#bpb#bpb#bpaYbpb#a6aYa6aYa6bpa6bpa6bpa6a6a6a6a6a6a6a6bDa6a6a6baa6baa6baa6baa6baaWbablbablbablbablaIbaaIbaaIbabgbabgaIbgaIbgbgbgaIbgbgaLbgaZbgaLbgaLaZbnaZbnaZbnaZaNbyaNbyaNbnaNbnaNaNbwaNbwaNbwaNaXbwaQbwaXaXaQaXaQaXbraXaOaXaObBaObraOaOaOaObsaOa1aOa1bsa1bsaRa1bqa1bqa1a5boa5boa5a5bja5bjbjbebjbebjbAbebAbebxbAbxbA",
"bCbtbCbbbEbbbCbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aEa3aEa3aEaJata3ataJataJataEataEatatatatatatatatatatalatalatalatalataaatal#iaa#iaa#iaaalaaalaaalaaal#lal#lal#lal#lal#lal#lal#lal#l#o#C#o#C#o#C#o#C#o#C#o#C#l#C#o#C#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p#l#p#l#p.h#p#l#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h.l.h#p.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.j.l.h.l.j.l.h.l.j.l.h.l.j.l.h.l.j.l.h.l.j.l.j.l.j.l.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k.i.5.i.k.i"
".5.i.k.i.5.ia4.ia4.ia4.ia4.ia4.ia4.ia4.i.ibfa4bf.ibfa4bf.ibf.ibc.ibf.ibcbfbfbfbcbfbcbfbcbfbcbfbcbfaYbfbpbfaYbfbpbfbpbfbpbcbpb#bpaYbpaYbpaYbpaYbpbpa6bpa6bpa6bpa6bpa6bpa7a6a6a6bDa6a7a6bDa6baa6baa6baa6baa6baa6bablbablbabababababaaIbabgbabgbabgbabgaIbgaIbgaIbmbgbmbgaLbgaLbgbnbgaLaZbnaZbnaZbnbnbnaZaNbnaNbnaNbnaNaNbwaNbwaNbuaNaXaNaQbwaQbwaQaXaQaXbraXbraXaObraObraObraOaObsaObsaOa1bsa1bsaRa1aRa1bqa1bqa1a5bqa5boa5a5bja5bjbjbebjbhbjbebebvbebxbAbxbAbxbGbF",
"bbbEbbbtbbbtbbbtbbbba8bba8bbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataEataEataEatatatatatatatatazatanataaataaataaataaataa#iaa#iaaalaaalaaalaaal#lalaaal#lal#lal#laa#lal#Caa#Cal#C#l#C#o#C#l#C#l#C#l#C#l#C#l#C#l#p#l#p#l#p#l#p#l#p#l#p#C#p#C#p#C#p#p#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.l.l.h.l.k.l.h.l.k.l.h.l.k.l.h.l.k.l.h.l.k.l.k.l.k.l.k.l.k.l.5.l.k.k.5.k.k.k.5.k.k.k.j.k.5.k.j.k.5.k.j.k.5.k.i.ka4.k.i.k"
"a4.k.i.k.i.k.i.k.i.k.i.k.ia4.ibi.ia4.ibi.ia4.ia4.ia4.ia4bfa4bfa4bf.ibfa4bf.ibfa4bfbfbfbfbfbfbcbfbfbfbcbfbcbfaYbfaYbfbpbfaYbfbpbfbpbfbpb#bpb#bpb#bpaYbpb#a6bpa6aYa6bpa6bpa6bpa6a6a6a6a6a6a6a6bDa6bHa6baa6baa6baa6baa6baaWbablbablbablbablaIbabgbaaIbabgbabgaIbgaIbgbgbgbgbgbgaLbgaZbgaLbgaLaZbnaZbnaZbnaZbnbyaNbyaNbnaNbnaNaNbwaNbwaNbwaNaXbwaQbwaXbwaQaXaQaXbraXbraXaObBaObraObzaOaObsaObsaOa1bsa1bsaRa1a1a1bqa1a5boa5boa5a5bjbdbja5bebjbebjbAbebAbebvbAbxbAbFbG",
"bCbEbCbtbCbbbEbbbCbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a0a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aEa3aEa3aEaJata3ataJataJataEataEatatatatatatatatatat#iatalatalatalataaatal#iaaatal#iaa#iaaalaaalaaal#lalaaal#lal#lal#lal#lal#lal#l#o#lal#l#o#C#o#C#o#C#o#C#o#C#o#C#o#p#o#C#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p.h#p#o#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h.l.j#p.h.l.j#p.h.l.j.l.h.l.j.l.h.l.j.l.h.l.j.l.h.l.j.l.h.l.j.l.j.l.j.l.j.l.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k.i.k.i.5.i.k.i.5.i"
".k.ia4.ia4.ia4.ia4.i.i.ia4.i.i.ia4.i.ibf.ibf.ibf.ibf.ibf.ibc.ibf.ibcbfbfbfbcbfbcbfbcbfbcbfbIbfaYbfbpbfbpbfbpbfbpbfbpbcbpb#bpaYbpaYbpbpbpaYbpbpa6bpa6bpa6bpa6bpa6bpa7a6a6a6bDa6bDa6bDa6baa6baa6baa6babHbaa6bablbablbabababababaaIbabgbabgbabgbabgaIbgbgbgaIbmbgbgbgaLbgaLbgbnbgbnbgbnaZbnaZbnbnbnbyaNbnaNbnaNbnaNaNbwaNbwaNbwaNaXaNaQbwaQbwaQaXaQaXbraXbraXaObraObraObraOaObsaObsaObsbsa1bsaRbsaRa1bqa1bqa1a5boa5boa5a5bja5bja5bebjbebjbebebvbebvbAbxbAbxbGbFbxbF",
"btbEbbbEbbbtbbbtbbbtbbbba8bba8bbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJaJaJa3aEaJaEaJaEaJaEaJataJataJataEataEatatatatatatatatazatanataaatalataaataaataa#iaaataa#iaaalaaalaaalaaalaaal#lal#lal#lal#lal#laa#lal#C#l#C#o#C#l#C#o#C#l#C#l#C#l#C#l#p#l#C#l#p#l#p#l#p#l#p#l#p#l#p#l#p#l#p#C#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p#p.l#p#p#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l#p.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.k.l.h.l.k.l.h.l.k.l.j.l.k.l.j.k.k.k.j.k.k.k.j.k.5.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k"
".i.k.i.k.i.k.i.k.ibi.ia4.ibi.ia4.ibi.ia4.ia4.ia4.ia4bf.ibfa4bf.ibf.ibf.ibf.ibfbfbfbfbfbfbcbfbfbfbcbfbcbfbpbfaYbfbpbfbpbfbpbfbpbfbpb#bpb#bpb#bpaYbpb#bpbpa6bpa6bpa6bpa6bpa6a6a6a6bHa6a6a6bDa6bHa6baa6baa6baa6babHbaaWbablbablbablbablbababgbabgbabgbabgaIbgaIbgbgbgbgbgbgaLbgaZbgbnbgaLbgbnaZbnaZbnaZbnbyaNbyaNbnaNbnaNaNbwaNbwaNbwaNaXbwaXbwaXbwaQaXaQaXbraXbraXaOaXaObraObBaOaObsaObsaOa1aOa1bsaRbsa1a1bqa1bqboa5boa5bobjbdbja5bjbjbebjbebebAbebvbebxbAbFbAbFbx",
"bCbEbCbEbCbtbCbbbEbbbCbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a0a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aEa3aEa3aEaJata3ataJataJataEataEatatatatatatatatatat#iatalatalatalataaatal#iaaatal#iaa#iaa#iaaalaaalaaalaaal#lal#lal#lal#lal#lal#lal#lal#l#o#l#o#l#o#C#o#C#o#C#o#C#o#C#o#C#o#p#o#p#o#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p.h#p#o#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h.l.j#p.h.l.j#p.j.l.j#p.j.l.j.l.j.l.j.l.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k.i.5.i.k.i.5.i.k.i.j.ia4.i"
".i.ia4.i.i.ia4.i.i.i.i.i.i.i.i.i.ibf.ibf.ibf.ibf.ibf.ibc.ibf.ibcbfbcbfbcbfbcbfbIbfbcbfbIbfbpbfbpbfbpbfbpbfbpbfbpbcbpb#bpaYbpaYbpbpbpbpbpbpa6bpa6bpa6bpa6bpa6bpbDa6a6a6bDa6bDa6bDa6bDa6baa6baa6babHbabHbablbablbababababJbaaIbabgbabgbabgbabgaIbgbgbgbgbmbgbgbgaLbgaLbgbnbgbnbgbnaZbnaZbnbnbnbyaNbnaNbnaNbnaNbnbwaNbwaNbwaNbwaNaQbwaQbwaQbwaQaXbraXbraXbrbraObBaObraOaObsaObsaObsbsa1bsaRbsaRa1bqa1bqa1bqboa5boa5bqa5a5bja5bjbjbebjbebjbvbebvbebxbAbxbAbFbxbFbFbK",
"bEbCbtbEbbbEbbbtbbbtbbbbbbbba8bba8bbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJaJaJa3aEaJaEa3aEaJaEaJataJataJataEataEatatatatatatatatatatatataaatalataaataaataaataaataa#iaa#iaaalaaalaaalaaal#lal#lal#lal#lal#lal#lal#l#l#Cal#C#l#C#o#C#l#C#o#C#l#C#l#C#l#C#l#p#l#p#l#p#l#p#l#p#l#p#l#p#l#p#l#p.h#p#l#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h.l.h#p.h.l.h#p.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.j.l.h.l.j.k.h.k.j.k.h.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k.i.k.i.k"
".i.k.i.k.i.5.ibi.ia4.ibi.ia4.ia4.i.i.ia4.i.ibfa4bf.ibf.ibf.ibf.ibf.ibf.ibfbfbfbfbfbfbcbfbfbfbcbfbcbfbpbfaYbfbpbfbpbfbpbfbpbfbpb#bpb#bpb#bpbpbpb#bpbpa6bpa6bpa6bpa6bpa6bpa6a6bHa6bHa6bDa6bHa6baa6baa6baa6babHbabHbablbablbablbablbababgblbgbabgbabgbabgaIbgbgbgbgbgbgbmbgaZbgbnbgbnbgbnaZbnaZbnaZbnbybnbyaNbnaNbnaNbnbwaNbwaNbwaNbwbwaXbwaXbwaQbwaQaXbraXbraXbraXaObBaObBaOaObsaObsaObsaOa1bsa1bsa1a1bqa1bqboa5boa5bobjbdbja5bja5bebjbebjbAbebvbebvbAbxbAbFbGbFbL",
"bCbEbCbEbCbEbCbtbCbbbEbbbCbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a0a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aEa3aEa3aEaJata3ataJata3ataJataEatatatatatatatatatat#iatatatalatalatalatalataaatal#iaa#ial#iaa#iaa#iaaalaaal#lal#lal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#C#o#C#o#C#o#p#o#C#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p.j#p#o#p.j#p#o#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.i.k.i.h.i.k.i.j.i.k.i.j.i.k.i.j.i.k.i.j.i.i.i.i.i"
".i.i.i.i.i.i.i.i.i.i.i.i.ibf.ibf.ibf.ibf.ibc.ibf.ibc.ibfbfbcbfbcbfbIbfbcbfbIbfbIbfbIbfbpbfbpbfbpbfbpbfbpbfbpbcbpb#bpbpbpaYbpbpbpbpbpbpa6bpa6bpa6bpa6bpa6bpbDa6bHa6bDa6bDa6bDa6bDa6baa6baa6babHbabHbablbablbababablbJbabJbabgbabgbabgbabgbabgbgbgbgbmbgbgbgbmbgaLbgbnbgbnbgbnaZbnaZbnbnbnbybnbnaNbnaNbnaNbnbwaNbwaNbwaNbwaNaQbwaQbwaQbwaQaXbraXbraXbrbraObBaObraObrbsaObsaObsaOa1bsaRbsaRbsbqa1bqa1bqboa5boa5bqa5a5bja5bja5bebjbebjbvbebvbebvbAbxbAbFbxbFbxbFbFbK",
"bEbEbEbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJa3aJa3aJaJaEa3aEaJaEaJataJataJataEataEatatatatatatatatatatatataaatalataaatalataaataaataa#iaa#iaa#iaaalaaalaaal#lalaaal#lal#lal#lal#lal#lal#lal#l#l#C#o#C#l#C#o#C#l#C#o#C#l#C#o#p#l#C#l#p#l#p#l#p#l#p#l#p#l#p#l#p#l#p#l#p.h#p#l#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h.l.h#p.h.l.h#p.h.l.h.l.h.l.h.l.h.l.h.l.h.l.h.l.j.l.h.l.j.l.h.l.j.k.h.k.j.k.h.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k.i.k.i.k.i.k.i.k"
".i.k.ia4.ibi.i.i.ibi.i.i.ia4.i.i.i.i.i.ibf.ibf.ibf.ibf.ibf.ibf.ibf.ibfbfbcbfbfbfbcbfbfbfbIbfbcbfbpbfbpbfbpbfbpbfbpbfbpbfbpb#bpbpbpb#bpbpbpbMbpbpa6bpa6bpa6bpa6bpa6bpa6a6bHa6bHa6bDa6bHa6bDa6baa6baa6babHbabHbablbablbablbablbababgblbgbabgbabgbabgaIbgbgbgbgbgbgbmbgaZbgbnbgbnbgbnaZbnaZbnaZbnbybnbyaNbnaNbyaNbnbwaNbwaNbwaNbwbwaXbwaXbwaQbwaQaXbraXbraXbraXaObBaObBaObrbsaObsaObsaOa1bsa1bsa1a1bqa1bqa1bqboa5boa5bobja5bja5bjbjbebjbebebvbebvbAbxbAbFbAbFbGbFbF",
"bNbEbCbEbCbEbCbtbCbtbCbbbEbbbCbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a0a8aJa8aJa0aJa0aJa0aJa0aJa3aJa3aJa3aEa3aEa3ata3ataJata3ataJataEatatatatatatatatatatatatatat#iatalatalatalatalatal#iaa#ial#iaa#ial#iaa#iaaal#lalaaal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#p#o#C#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p#o#p.j#p#o#p.j#p.j#p.j#p.j#p.j#p.j#p.j#p.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.i.h.i.h.i.h.i.j.i.h.i.j.i.h.i.j.i.h.i.j.i.j.i.i.i.i.i.i.i.i.i"
".i.i.i.i.i.i.i.i.ibf.ibf.ibf.ibf.ibf.ibc.ibf.ibI.ibfbfbIbfbcbfbIbfbIbfbIbfbIbfbIbfbpbfbpbfbpbfbpbfbpbfbpbIbpb#bpbpbpbpbpbpbpbpbpbpa6bpa6bpa6bpbHbpa6bpbDa6bHa6bDa6bDa6bDa6bDa6babHbaa6babHbabHbabHbablbababablbJbabJbabgbabgbabgbabgbabgbgbgbgbmbgbgbgbmbgaLbgbnbgbnbgbnbgbnaZbnbnbnbybnbnaNbnaNbnaNbnbwaNbwaNbwaNbwaNbubwaQbwaQbwaQbwbraXbraXbrbBbrbBaObraObrbsaObsaObsaObsbsaRbsa1bsbqa1bqa1bqbobqboa5bqa5bqbja5bja5bjbjbebjbvbebvbebvbAbxbAbFbvbFbxbFbFbKbFbO",
"bEbCbEbEbEbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bbbkbba8bba2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJa3aJa3aJaJaEa3aEaJaEaJataJataJataJataEatatatatatatatatatatatat#ratalataaatalataaataaataa#iaa#iaa#iaa#iaaalaaalaaalaaal#lal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#C#l#C#o#C#l#C#o#C#l#C#o#p#l#C#o#p#l#p#o#p#l#p#l#p#l#p#l#p#l#p#l#p.h#p#l#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.h#p.j.l.h#p.j.l.h#p.j.l.h#p.j.l.h#p.j.k.h.k.j.k.h.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.j.k.i.k.i.k.i.k.i.k.i.k.i.k.i.h.i.k.i.j.i.k"
".i.i.ibi.i.i.i.i.i.i.i.i.i.i.i.ibf.ibf.ibf.ibf.ibf.ibf.ibf.ibfbfbfbfbIbfbfbfbIbfbcbfbIbfbIbfbpbfbpbfbpbfbpbfbpbfbpbfbpb#bpbpbpbMbpbpbpbpbpbpa6bpa6bpa6bpa6bpbHbpa6a6bHa6bHa6bDa6bHa6bDa6babHbaa6babHbabHbablbablbablbJblbababgblbgbabgbabgbabgaIbgbgbgbgbgbgbmbgaZbgbnbgbnbgbnbgbnaZbnaZbnbybnbyaNbnaNbyaNbnbwaNbwaNbwaNbwbwbwbwaXbwaQbwaQbwbraXbraXbraXaObBaObBaObrbsaObsaObsaObsbsa1bsa1bsbqa1bqa1bqboa5boa5bobja5bja5bjbjbebjbebjbAbebAbAbxbAbxbAbFbGbFbFbKbF",
"bPbEbNbEbCbEbCbtbCbtbCbbbCbbbEbbbCbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a0a8aJa8a2a0aJa0aJa0aJa0aJa3aJa3aJa3aEa3aEa3ata3ataJata3ataJataJatatatatatatatatatatatatatat#iat#iatalatalatalatal#iaaatal#iaa#ial#iaa#ial#iaa#iaaal#lal#lal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.j.h#o.h.j.h#o.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.i.h.i.h.i.h.i.j.i.h.i.j.i.h.i.j.i.h.i.j.i.j.i.j.i.i.i.i.i.i.i.i.i.i.i.i.i"
".i.i.i.i.i.i.ibf.ibf.ibf.i.d.ibf.ibI.ibf.ibI.ibIbfbIbfbIbfbIbfbIbfbIbfbIbfbpbfbpbfbpbfbpbfbpbIbpbfbpbpbpbMbpbpbpbpbpbpbpbpbpbpa6bpa6bpa6bpbHbpbHbpbDa6bHa6bDa6bDa6bDa6bDa6babHbabHbabHbabHbabHbablbababablbJbabJbabgbabgbabgbabgbabgbgbgbgbmbgbgbgbmbgaLbgbnbgbnbgbnbgbnaZbnbybnbybnbnaNbnaNbnaNbnbwbnbwaNbwbwbwbwbubwaXbwaQbwaQbwbraXbraXbrbBbrbBaObraObrbsbrbsaObsaObsbsaRbsa1bsbqbsbqa1bqa1bqboa5bqa5bobja5bja5bjbjbebjbvbjbAbebvbAbvbAbFbAbFbGbFbFbFbFbObFbO",
"bEbCbEbCbEbEbtbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bba8bba8bba2a8a2a8a2a8a2a8aJa8aJa0aJa0aJa0aJa3aJa3aJaJaEa3aEaJaEaJataJataJataJataEatatataEatatatatatatatat#iatatataaatalataaatalataa#iaaataa#iaa#iaa#iaaalaaalaaal#lal#lal#lal#lal#lal#lal#lal#lal#l#o#l#o#l#o#l#o#l#l#C#o#C#l#C#o#p#l#C#o#p#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p.h#p#o#p.h#p#o#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h#p.j#p.h.h.j.h.h.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.j.h.j.k.i.h.i.k.i.h.i.k.i.h.i.k.i.h.i.k.i.h.i.h.i.j.i.g.i.i"
".i.g.i.i.i.i.i.i.i.i.i.i.i.ibf.ibf.ibf.ibf.ibf.ibf.ibf.i.dbfbfbfbIbf.dbfbIbfbIbfbIbfbIbfbpbfbpbfbpbfbpbfbpbfbp.dbpb#bpbpbpbMbpbpbpbpbpbpa6bpa6bpa6bpa6bpbHbpbHa6bHa6bHa6bDa6bHa6bDa6babHbabHbabHbabHbablbablbablbJblbJbabJblbgbabgbabgbJbgbgbgbgbgbgbgbgbgbgaZbgbnbgbnbgbnbgbnaZbnaZbnbybnbyaNbnaNbyaNbnbwbnbwaNbwaNbwbwbwbwaXbwaQbwaXbwbraXbraXbraXbrbBaObBaObrbsaObsaObsaObsbsa1bsa1bsbqa1bqa1bqa1bqboa5boa5bqbjbdbjbjbjbjbebjbAbebAbAbvbAbxbAbFbGbFbFbFbFbObF",
"bPbEbPbEbNbEbCbEbCbtbCbtbCbbbCbbbEbbbQbbbtbbbtbbbba8bba8bba8bba8bba0bba0a8a2a8a0a8aJa8a2a8aJa0aJa0aJa0aJa3aJa3aJa3aJa3aEa3ata3aEaJata3ataJata3atatataEatatatatatatatatatat#iat#iat#iatalatalatalatalatal#iaa#ial#iaa#ial#iaa#ial#i#l#iaaal#lal#lal#lal#lal#lal#lal#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.j.h#o.h.j.h#o.h.j.h#o.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.i.h.i.h.i.h.i.j.i.h.i.j.i.h.i.j.i.h.i.j.i.j.i.j.i.j.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i.i"
".i.i.i.e.ibf.i.d.ibf.ibI.i.d.ibI.i.d.ibIbfbIbfbIbfbIbfbIbfbIbfbIbfbIbfbpbfbpbfbp.dbpbfbpbIbp.dbpbpbpbpbpbpbpbpbpbpbpbpbpbpa6bpbHbpa6bpbHbpbHbpbDa6bHa6bDa6bDa6bDbHbDa6babHbabHbabHbabHbabHbablbJbabababJbabJbabgbabgbabgbJbgbabgbJbgbgbmbgbgbgbRbgbnbgbnbgbnbgbnbgbnaZbnbybnbybnbnbnbnbwbnbwbnbwbnbwaNbwbwbwbwbubwaXbwaQbwaQbwbraXbraXbrbBbrbBaObraObrbsbrbsaObsaObsaObsbsa1bsbqbsbqa1bqa1bqboa5bqa5bobjbqbja5bjbjbjbjbebjbAbebvbAbvbAbxbAbFbAbFbFbFbFbObFbObKbS",
"bEbCbEbCbEbCbtbCbtbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bba8bba8bba2a8a0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa3aJa0aJaJaEa3aEaJaEa3ataJataJataJataJataEataEatatatatatatatatatatatat#ratalataaatalataaataaataa#iaa#iaa#iaa#iaa#iaaal#lalaaal#lal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#p#l#C#o#p#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p#l#p#o#p.h#p#o#p.h#p.j#p.h#p.j#p.h#p.j.h.h.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.j.i.h.i.i.i.g.i.i.i.i"
".i.i.i.i.i.i.i.i.i.i.i.ibf.ibf.ibf.i.d.ibf.i.d.ibf.ibIbf.dbfbIbf.dbfbIbfbIbfbIbfbpbfbpbfbpbfbpbfbp.dbpbfbpbMbpbMbpbpbpbMbpbpbpbpbpbpa6bpa6bpbHbpa6bpbHbpbHa6bHa6bHa6bDa6bHbHbDa6babHbabHbabHbabHbabHbablbablbJblbJbabJblbgbabgbabgbJbgbJbgbgbgbgbgbgbgbgbgbgbnbgbnbgbnbgbnbybnbybnbybnbybnbnbwbyaNbnbwbnbwaNbwaNbwbwbwbwaXbwaQbwaXbwbraXbraXbraXbrbBaObBaObrbsbBbsaObsaObsbsa1bsa1bsbqbsboa1bqa1bqboa5boa5bqbjbdbjbjbjbjbebjbAbjbAbAbvbAbxbAbFbAbFbFbFbFbKbFbObF",
"bPbCbPbEbPbEbCbEbCbEbCbtbCbtbCbbbCbbbtbbbQbbbtbbbtbbbba8bba8bba8bba8bba8bba0a8a0a8a0a8a2a8a0a8aJa0aJa0aJa0aJa3aJa3aJa3aJa3aEa3ata3aEa3ata3ataJata3ataJataEatatatatatatatatatat#iatatat#iat#iatalatalatalatal#ial#ial#iaa#ial#iaa#ial#i#l#ial#i#l#i#oal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.j.h#o.h.j.h#o.h.j.h#o.h.i.h.e.h.i.h.e.j.i.h.e.j.i.h.e.j.i.h.i.j.i.j.e.j.i.j.e.j.i.i.e.i.i.i.e.i.i.i.e.i.i.i.e.i.i.i.e.i.i"
".i.d.i.d.i.d.i.d.ibI.i.d.ibI.i.d.ibIbfbIbfbIbfbIbfbIbfbIbfbIbfbIbfbp.dbpbfbp.dbp.dbpbIbp.dbpbpbpbpbpbpbpbpbpbpbpbpbTbpa6bpbHbpbHbpbHbpbHbpbDa6bHa6bDa6bDa6bDbHbDbHbabHbabHbabHbabHbabHbablbJbabJbabJbabJbabJbabgbabgbJbgbJbgbJbgbgbmbgbgbgbRbgbRbgbnbgbnbgbnbgbnaZbnbybnbybnbnbnbnbwbnbwbnbwbnbwaNbwbwbwbwbubwbwbwaQbwaQbwbrbwbraXbrbBbrbBbrbraObrbsbrbsaObsaObsaObsbsa1bsbqbsbqa1bqa1bqbobqboa5bobjbqbja5bjbjbjbjbebjbAbjbvbAbvbAbvbAbFbAbFbFbFbFbFbFbObFbSbObS",
"bEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bba8bba8bba2a8a0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa3aJa0aJa3aJa3aEaJaEa3ataJataJataJataJataJataEatatatatatatatatatatatat#iat#iataaatalataaatalataa#iaa#iaa#iaa#iaa#iaa#iaa#iaaal#lal#lal#lal#lal#lal#lal#lal#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.j.h#o.h.j.h#o.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.j.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.j.i.h.i.j.i.g.i.i.i.g.i.i.i.i.i.i"
".i.i.i.i.i.i.i.i.d.ibf.i.d.ibf.i.d.ibf.i.d.i.d.ibIbf.dbfbIbf.dbfbIbfbIbfbIbfbpbfbpbfbp.dbpbfbp.dbp.dbpbMbpbMbpbpbpbMbpbpbpbpbpbpbHbpa6bpbHbpbHbpbHbpbHa6bHa6bHa6bDa6bHbHbDbHbabHbabHbabHbabHbabHbJblbablbJblbJbabJblbgbabgbabgbJbgbJbgbgbgbgbgbgbUbgbgbgbnbgbnbgbnbgbnbUbnbybnbybnbybnbnbwbybwbnbwbnbwaNbwaNbwbwbwbwbwbwaQbwaXbwbrbwbraXbraXbrbBbrbBaObrbsbBbsaObsaObsbsbsbsa1bsbqbsboa1bqa1bqboa5boa5bobjbdbjbjbjbjbebjbAbjbAbebvbAbvbAbFbAbFbLbFbLbFbFbObFbObO",
"bPbCbPbCbPbEbPbEbCbEbCbEbCbtbCbtbCbbbCbbbQbbbQbbbtbbbtbbbba8bba8bba8bba8bba8bba0a8a0a8a0a8a2a8a0a8aJa0aJa0aJa0aJa3aJa3aJa3aJa3aJa3aEa3aEa3ata3ataJata3ataJata3atatatatatatatatatatatatatat#iat#iat#iat#iatalatal#ialatal#ial#ial#ial#ial#iaa#ial#i#l#ial#i#l#i#o#i#l#i#oal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.e.h.e.h.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.i.j.e.j.i.j.e.j.i.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.d.i.d"
".i.d.ibI.i.d.ibI.i.d.ibI.ibIbfbIbfbIbfbIbfbIbfbI.dbIbfbI.dbIbfbp.dbp.dbp.dbp.dbpbIbp.dbpbpbpbpbpbpbpbpbTbpbpbpbTbpbHbpbHbpbHbpbHbpbHbpbDa6bHa6bDbHbDa6bDbHbDbHbabHbabHbabHbabHbJbHbablbJbabJbabJbabJbabJbabgbabgbJbgbJbgbJbgbgbmbgbgbgbRbgbRbgbnbgbnbgbnbUbnbgbnbybnbybnbnbnbnbwbnbwbnbwbnbwbnbwbwbwbwbubwbwbwbrbwaQbwbrbwbraXbrbBbrbBbrbraObrbsbrbsbrbsaObsaObsbsa1bsbqbsbqbsbqa1bqa1bqboa5bobjbqbja5bjbjbjbjbebjbAbjbvbebvbAbvbAbxbAbFbvbFbFbFbFbObFbObObObObV",
"bEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bba8bba8bba2a8a0a8a2a8a2a8aJa8a2a0aJa0aJa0aJa0aJa0aJa3aJa3aEaJaEa3ataJaEaJataJataJataJataEatatatatatatatatatatatat#iat#iat#iatalataaatalataa#ialataa#iaa#iaa#iaa#iaa#iaa#i#l#iaaal#lal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.j.h#o.h.j.h#o.h.j.h#o.h.j.h#o.h.j.h.j.h.j.h.j.h.j.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.h.i.j.i.h.i.j.i.h.i.i.i.g.i.i.i.i.i.i.e.i.i.i.e.i"
".i.i.e.ibf.i.d.i.d.i.d.i.d.i.d.i.d.i.d.i.dbfbIbf.dbfbIbfbIbfbIbfbIbfbIbfbp.dbpbfbp.dbp.dbp.dbp.dbpbMbpbMbpbpbpbMbpbpbpbpbpbpbHbpbHbpbHbpbHbpbHbpbHa6bHa6bHbHbDa6bHbHbDbHbabHbabHbabHbabHbabHbJblbJblbJblbJbabJblbgbabgbabgbJbgbJbgbgbgbgbgbgbUbgbUbgbnbgbnbgbnbgbnbUbnbybnbybnbybnbnbWbybwbnbwbnbwbnbwaNbwbwbwbwbwbwbrbwbBbwbrbwbraXbraXbrbBbrbBaObrbsbBbsbrbsaObsbsbsbsa1bsbobsboa1bqa1bqbobqboa5bobjbobjbjbjbjbjbjbAbjbAbjbvbAbvbAbFbAbFbAbFbLbFbFbObFbObObObO",
"bPbPbPbCbPbCbPbEbPbEbCbEbCbEbCbtbCbtbCbbbCbbbQbbbQbbbtbbbtbbbba8bba8bba8bba8bba8bba0a8a0a8a0a8a2a8a0a8aJa8aJa0aJa0aJa3aJa0aJa3aJa3aJa3aEa3aEa3ata3ata3ata3ataJata3at.batatatatatatatatatatatat#iat#iat#iat#iat#iatalatalatal#ial#ial#ial#ial#ial#ial#iaa#ial#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#o#i#lal#o#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o#l#o.h#o.h#o.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.j.e.j.e.j.e.j.e.j.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.d.i.d.i.d.i.d"
".ibI.i.d.ibI.i.d.ibI.ibIbfbI.dbIbfbI.dbIbfbI.dbI.dbI.dbp.dbp.dbp.dbpbIbp.dbpbIbp.dbpbpbpbpbTbpbpbpbTbpbTbpbTbpbHbpbHbpbHbpbHbpbHbpbDbTbHa6bDbHbDbHbDbHbDbHbabHbabHbabHbabHbJbDbJblbJbabJbabJbabJbabJbJbgbabgbJbgbJbgbJbgbgbmbgbgbgbRbgbRbgbnbgbnbgbnbUbnbUbnbybnbybnbnbnbnbWbnbwbnbwbnbwbnbwbwbwbwbubwbwbwbrbwbrbwbrbwbraXbrbBbrbBbrbraObBbsbrbsbrbsaObsaObsbsbsbsbqbsbqbsbqa1bqa1bqbobqbobjbqbjbqbjbjbjbjbjbjbebjbvbjbvbAbvbAbxbAbFbvbFbFbFbFbXbFbObXbObObVbObY",
"bEbPbEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbEbbbtbbbtbbbbbbbba8bba8bba8bba8bba2a8a0a8a2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJa3aJa3aJaJaEa3aEaJaEa3ataJataJataJataJatatatatatatatatatatatat#iatatat#iat#iatalatalataaatalataa#ial#iaa#iaa#iaa#iaa#iaa#iaa#i#l#i#l#i#lal#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.i.h.e.h.i.h.e.h.i.h.e.h.i.h.e.h.i.h.e.h.i.j.e.h.i.j.e.h.i.i.e.g.i.i.e.i.i.i.e.i.i.i.e.i.e.i.e.i.e.i"
".d.i.d.i.d.i.d.i.d.i.d.i.d.i.d.ibI.i.dbfbIbf.dbfbIbfbIbfbIbfbI.dbIbfbp.dbp.dbp.dbp.dbp.dbp.dbpbpbpbMbpbpbpbMbpbpbTbpbpbpbHbpbHbpbHbpbHbpbHbpbHa6bHa6bHbHbDbHbHbHbDbHbabHbabHbabHbabHbabHbJblbJblbJblbJbabJblbgbJbgbJbgbJbgbJbgbJbgbgbgbgbUbgbUbgbRbgbnbgbnbgbnbUbnbybnbybnbybnbybWbybwbnbwbnbwbnbwaNbwbwbwbwbwbwbrbwbBbwbrbwbraXbraXbrbBbrbBaObBbsbBbsbrbsaObsbsbsbsbsbsbobsbobsbqa1bqbobqboa5bobjbobjbjbjbjbjbjbAbjbAbjbvbAbAbAbZbAbFbAbFbLbFbFbXbFbObXbObOb0bO",
"bPbPbPb1bPbCbPbCbPbEbPbEbCbEbCbEbCbtbCbtbCbbbCbbbQbbbQbbbtbbbQbbbba8bba8bba8bba8bba8bba0a8a0a8a0a8a2a8a0a8aJa8a3a0aJa0aJa0aJa0aJa3aJa3aJa3aJa3aEa3ata3ata3ata3ataJata3at.bat#gatatatatatatatatatat#iatatat#iat#iat#iat#iat#iatal#ialatal#ial#ial#ial#ial#ial#ial#i#o#ial#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#o.e#l#i#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e.h.e#o.e.h.e#o.e.h.e#o.e.h.e.j.e.h.e.j.e.h.e.j.e.j.e.j.e.j.e.j.e.j.e.j.e.j.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.d.i.d.i.d.ibI.i.d.ibI"
".i.d.ibI.ebI.ibI.dbIbfbI.dbI.dbI.dbI.dbI.dbI.dbI.dbp.dbp.dbp.dbpbIbp.dbpbIbpbIbTbpbpbpbTbpbTbpbTbpbTbpbTbpbHbpbHbpbHbpbDbpbHbpbDbHbHbHbDbHbDbHbDbHbDbHbDbHbabHbJbHbabHbJbDbJblbJbabJbabJbabJbabJbJbgbJbgbJbgbJbgbJbgbgbRbgbgbgbRbgbRbgbnbgbnbgbnbUbnbUbnbybnbybnbnbnbnbWbnbwbnbwbnbwbnbwbWbwbwbubwbwbwb2bwbrbwbrbwbrbwbrbBbrbBbrbrbrbBbsbrbsbrbsbrbsaObsbsbsbsaRbsbqbsbqbsbqa1bqbobqbobjbqbjbqbjbjbjbjbjbjbebjbvbjbvbAbvbAbvbAbFbvbFbLbFbFbFbFbObXbObObSbObYbObY",
"b1bPbEbPbEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbCbbbtbbbtbbbbbbbba8bba8bba8bba8bba2a8a0a8a2a8a2a8a2a8a2a8aJa0aJa0aJa0aJa0aJa3aJa3aJa3aEa3aEaJaEa3ataJataJataJataJat.bataEatatatatatatatatatatatat#iat#iat#iat#iatalatalataa#ial#iaa#ial#iaa#ial#iaa#iaa#i#l#iaa#i#l#i#l#i#l#i#lal#lal#lal#lal#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h#o.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.j.e.h.e.j.e.h.e.j.e.g.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.d.i"
".d.i.d.i.d.i.d.i.d.i.d.i.d.ibIbf.dbfbIbfbI.dbIbfbI.dbIbfbI.dbp.dbp.dbp.dbp.dbp.dbp.dbp.dbpbpbpbMbpbpbTbpbpbpbTbpbTbpbHbpbHbpbHbpbHbpbHbpbHbHbHa6bHbHbDbHbHbHbDbHbabHbabHbabHbJbHbJbHbJblbJblbJblbJbabJblbgbJbgbJbgbJbgbJbgbJbgbgbgbgbUbgbUbgbRbgbnbgbnbgbnbUbnbUbnbybnbybnbybWbybwbnbwbnbwbnbwbnbwbwbwbwbwbwb3bwbBbwbrbwbrb3brbBbrbBbrbBbrbBbsbBbsbrbsaObsbsbsbsbsbsbobsbobsbqa1bqbobqbobqbobjbobjbqbjbjbjbjb4bjbAbjbAbebAbAbvbAbFbAbFb5bFbFbXbFbObXbObObObObYbO",
"bPbPbPbPbPb1bPbCbPbCbPbEbPbEbCbEbCbEbCbtbCbtbCbbbCbbbQbbbQbbbtbbbQbbbba8bba8bba8bba8bba8bba0a8a0a8a0a8a0a8a0a8aJa8a3a0aJa0aJa0aJa0aJa3aJa3aJa3aJa3aEa3ata3ata3ata3ata3ata3at.bata3at.batb6atatatatatatatatatat#iat#iat#iat#iat#iat#iat#iatal#ial#ial#ial#ial#ial#ial#ial#ial#i#o#ial#i#o#i#o#i#o#i#o#i#l#i#o#i#l#i#o#i#l#i#o.e#l#i#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#o.e#o.e.j.e#o.e.j.e#o.e.j.e#o.e.j.e#o.e.j.e.e.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.e.e.i.e.e.e.i.d.e.d.ibI.e.d.ibI.e.d.ibI.ebI"
".ibI.ebI.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbp.dbp.dbp.dbp.dbpbIbp.dbTbIbpbpbTbpbTbpbTbpbTbpbTbpbTbpbTbpbHbpbHbpbHbpbDbTbHbpbDbHbDbHbDbHbDbHbDbHbDbHbDbHbabHbJbHbJbHbJbDbJbHbJbabJbabJbJbJbabJbJbgbJbgbJbgbJbgbJbgbgbRbgbUbgbRbgbRbgbRbgbnbgbnbUbnbUbnbUbnbybnbnbnbnbWbnbwbnbwbnbwbnbwbWbwbwbwbwbwbwb2bwbrbwbrbwbrbwbrbBbrbBbrbrbrbBbsbrbsbrbsbrbsaObsbsbsbsb7bsbobsbqbsbqa1bqbobqbobjbqbjbqbjb8bjbjbjbjbjbjbvbjbAbjbvbAbvbAbFbAbFbAbFbFbFbFbObFbObXbSbObVbObYb0bY",
"bPbPb1bPbEbPbEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbCbbbtbbbtbbbtbbbba8bba8bba8bba8bba8a8a0a8a2a8a0a8a2a8a2a8aJa0aJa0aJa0aJa0aJa3aJa3aJa3aJa3aEaJaEa3ataJata3ataJataJataJataJatatatatatatatatatatatat#iat#iat#iat#iat#iatalatal#ialataa#ial#iaa#ial#iaa#ial#iaa#iaa#i#l#iaa#i#l#i#l#i#l#i#l#i#l#i#l#i#l#i#lal#l#o#lal#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o#l#o.h#o#l#o.h.e#l.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.g.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.d.i.d.i.d.i"
".d.i.d.i.d.i.d.ibI.i.d.dbIbf.d.dbIbfbI.dbI.dbI.dbI.dbI.dbp.dbp.dbp.dbp.dbp.dbp.dbp.dbpbpbTbMbpbpbTbpbTbpbTbpbTbpbHbpbHbpbHbpbHbpbHbpbHbHbDbHbHbHbDbHbDbHbDbHbabHbabHbabHbJbHbJbHbJblbJblbJblbJbabJbabJbJbgbJbgbJbgbJbgbJbUbgbgbgbUbgbUbgbRbgbnbgbnbgbnbUbnbUbnbybnbybnbybWbybwbnbwbnbwbWbwbWbwbwbwbwbwbwb3bwbBbwbrbwbrbwbrbBbrbBbrbBbrbBbsbBbsbrbsbrbsbsbsbsbsbsbobsbobsbqbsbqbobqbobqbobjbobjbqbjbjbjbjbjbjbAbjbAbjbAbAbvbAbZbAbFbAbFbFbFbFbXbFbObXbObObVbObYb0",
"bPbPbPbPbPbPbPb1bPbCbPbCbPbEbPbCbCbEbCbEbCbtbCbtbCbtbCbbbQbbbQbbbtbbbQbbbba8bba8bba8bba8bba8bba8a8a0a8a0a8a0a8a0a8aJa8a3a8aJa0aJa0aJa0aJa3aJa3aJa3aJa3aJa3ata3aEa3ata3ata3ata3ata3ata3at.bat#gatatatb6atatatatatat#iat#iat#iat#iat#iat#iat#iat#i#i#iat#i#ial#ial#ial#ial#ial#ial#ial#ial#i#o#ial#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o.e#o#i#o.e#o#i#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e#o.e.e.e.e.e.e.e.e.e.e.e.e.e.i.e.e.e.i.e.e.e.i.e.e.e.i.e.e.e.e.d.e.d.ebI.e.d.ebI.e.d.ebI.e.d.ebI.ebI.ebI"
".ebI.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbp.dbp.dbTbIbp.dbTbIbp.dbTbIbTbpbTbpbTbpbTbpbTbpbTbpbTbpbHbpbHbpbHbTbHbpbDbTbHbTbDbHbDbHbDbHbDbHbDbHbDbHb9bHbabHbJbHbJbHbJbDbJbHbJbabJbabJbJbJbJbJbJbgbJbgbJbgbJbgbJbgbJbRbgbUbgbRbgbRbgbRbUbnbgbnbUbnbUbnbUbnbybWbnbnbnbWbnbWbnbwbnbwbnbwbWbwbwbwbwbwbwb2bwbrbwbrbwbrbwbrb3brbBbrbrbrbBc.brbsbrbsbrbsaObsbsbsbsc#bsbobsbqbsbqa1bqbobqbob8bqbjbobjbqbjbjbjbjbjbjbvbjbAbjbvbAbvbAbZbAbFbAbFbFbFbFbXbFbObXbSbObVbObYb0bYcacb",
"bPbPbPbPb1bPbEbPbEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbCbbbtbbbtbbbtbbbba8bba8bba8bba8bba8a8a0a8a2a8a0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa3aJa3aJa3aJa3aJa3aEa3ataJata3ataJataJataJataJat.batatatatatatatatatat#iatatat#iat#iat#iat#iat#iatalatal#ial#iaa#ial#iaa#ial#iaa#ial#i#l#ial#i#l#i#o#i#l#i#l#i#l#i#l#i#l#i#l#i#l#i#l#i#l.e#l#i#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e#l.e.h.e#l.e.h.e#l.e.h.e#l.e.h.e#o.e.h.e.j.e.h.e.j.e.h.e.j.e.h.e.j.e.g.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.e.i.d.i.d.i.d.e.d.i.d.e.d.i"
".d.ebI.i.d.ebI.i.d.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbp.dbp.dbp.dbp.dbp.dbp.dbTbMbpbpbTbMbTbpbTbpbTbpbTbpbTbpbHbpbHbpbHbpbHbTbHbpbHbHbDbHbHbHbDbHbDbHbDbHbabHbJbHbabHbJbHbJbHbJblbJblbJblbJbJbJbabJbJbgbJbgbJbgbJbgbJbUbgbUbgbUbgbUbgbRbgbnbUbnbUbnbUbnbUbnbybnbybnbybWbybWbnbwbnbwbWbwbWbwbwbwbwbwbwb3bwbBbwbrbwbrb3brb3brbBbrbBbrbBc.bBbsbrbsbrbsbsbsbsbsbsccbsbobsbqbsbqbobqbobqbobjbobjbqbjcdbjbjbjbjb4bjbAbjbAbAbvbAbZbAbFbAbFb5bFbLbXbFbObXbObObVbObYb0bYca",
"cebPbPbPbPbPbPbPbPb1bPbCbPbCbPbEbPbCbCbEbCbEbCbtbCbtbCbtbCbbbQbbbCbbbtbbbQbbbta8bba8bba8bba8bba8bba8a8a0a8a0a8a0a8a0a8aJa8a3a8aJa8a3a0aJa0aJa3aJa3aJa3aJa3aJa3aJa3aEa3ata3ata3ata3ata3ata3at.bat#gat.bat#gatatatb6atat#iatatat#iat#iat#iat#iat#iat#iat#iat#i#i#i#i#i#i#i#i#i#ial#ial#ial#ial#ial#ial#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o.e#o#i#o.e#o#i#o.e#o.e#o.e#o.e#o.e#o.e#o.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.d.ebI.e.d.ebI.e.d.ebI.e.d.ebI.ebI.ebI.ebI.ebI.dbI"
".dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbp.dbT.dbp.dbTbIbT.dbTbIbTbIbTbIbTbpbTbpbTbpbTbpbTbpbTbpbTbpbHbTbHbpbHbTbHbTbDbTbHbTbDbHbDbHbDbHbDbHbDbHbDbHb9bHbJbHbJbHbJbHbJbDbJbHbJbabJbabJbJbJbJbJbJbgbJbgbJbgbJbUbJbgbJbRbgbUbgbRbgbRbgbRbUbnbgbnbUbnbUbnbUbnbybWbnbWbnbWbnbWbnbwbnbwbnbwbWbwbwbwbwbwbwb2bwb2bwbrbwbrbwbrb3brbBbrbrbrbBc.brbsbrbsbrbsbrbsbsbsbsb7bsbobsbqbsbqbsbqbobqbob8bqbjbobjbqbjbjbjbjbjbjbAbjbAbjbvbAbvbAbZbAbFbAbFbZbFbFbXbFbObFbSbXbSbObVb0bYb0cbbYcb",
"bPbPbPbPbPbPb1bPbEbPbEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbCbbbtbbbtbbbtbbbba8bba8bba8bba8bba8a8a0a8a2a8a0a8a2a8a2a8aJa8aJa0aJa0aJa0aJa3aJa3aJa3aJa3aJa3aEa3ataJata3ataJata3ataJataJat.bat.batatatatatatatatatatatat#iat#iat#iat#iat#iat#iat#i#i#iatal#ial#ial#ial#iaa#ial#iaa#ial#i#l#ial#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#l#i#l.e#l#i#l.e#l.e#l.e#l.e#l.e#l.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e.h.e#o.e.h.e#o.e.i.e.e.e.i.e.e.e.i.e.e.e.i.e.e.e.i.e.e.e.i.e.e.e.i.e.e.d.i.d.e.d.i.d.e.d.e.d.e.d.e.d.e"
"bI.e.d.ebI.d.d.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbp.dbp.dbp.dbT.dbp.dbT.dbpbIbTbMbTbpbTbMbTbpbTbpbTbpbTbpbHbpbHbpbHbTbHbpbHbTbHbTbHbHbDbHbHbHbDbHbDbHbDbHbabHbJbHbJbHbJbHbJbHbJblbJbabJblbJbJbJbJbJbJbgbJbgbJbgbJbgbJbUbgbUbgbUbgbUbgbRbgbRbUbnbUbnbUbnbUbnbybWbybnbybWbybWbnbwbnbwbWbwbWbwbwbwbwbwbwb3bwb3bwbrbwbrb3brb3brbBbrbBbrbBc.bBbsbrbsbrbsc.bsbsbsbsccbsbobsbqbsbqbobqbobqbob8bobjbobjbobjbjbjbjb4bjbAbjbAb4bvbAbvbAbFbAbFb5bFbLbXbFbObFbObXbVbObVb0bYb0cbbY",
"cebPcfbPbPbPbPbPbPbPbPbCbPbCbPbCbPbEbPbCbCbEbCbEbCbtbCbtbCbtbCbbbCbbbCbbbQbbbQbbbta8cgbbbba8bba8bba8bba8a8a8a8a0a8a0a8a0a8a3a8a3a8aJa8a3a0aJa0aJa3aJa3aJa3aJa3aJa3aJa3aJa3ata3ata3ata3ata3ata3at#gata3at.bat#gat.bat#gatatatb6atat#iat#iat#iat#iat#iat#iat#iat#i#i#iat#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i.e#ial#i#o#ial#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i#o#i.e#i.e#i.e.e.e#i.e.e.e#i.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.d.ebI.e.d.ebI.e.d.ebI.e.d.ebI.ebI.ebI.ebI.ebI.ebI.dbI.dbI.dbI"
".dbI.d.4.dbI.d.4.dbI.d.4.dbT.dbTbIbT.dbTbIbT.dbTbIbTbIbTbpbTbpbTbpbTbpbTbpbTbpbTbTbTbpbHbTbHbTbHbTbHbTbDbTbHbTbDbHbDbHbDbHbDbHb9bHbDbHb9bHbJbHbJbHbJbHbJbDbJbHbJbJbJbabJbJbJbJbJbJbJbJbgbJbgbJbUbJbUbJbRbgbUbgbRbgbRbgbRbUbnbUbnbUbnbUbnbUbnbybWbnbWbnbWbnbWbnbwbWbwbWbwbWbwbWbwbwbwbwb2bwb2bwbrbwbrbwbrb3brbBbrbBbrbBc.brbsbrbsbrbsbrbsbsbsbsb7bsccbsbqbsbqbsbqbobqbob8bob8bobjbqbjb8bjbjbjbjb4bjbAbjbvb4bvbAbZbAbZbAbFbZbFbFbXbFbXbFbObXbObObVbObYb0cbbYcbbYch",
"bPcfbPbPbPbPbPbPb1bPbEbPbEbPbEbNbEbCbEbCbEbCbtbCbtbCbbbEbbbCbbbtbbbtbbbtbbbba8bba8bba8bba8bba8a8a8a8a2a8a0a8a2a8a0a8aJa8aJa8aJa0aJa0aJa3aJa0aJa3aJa3aJa3aJa3aEa3aEa3ataJata3ataJata3at.bataJat.batatatatatatatatatat#iatatat#iat#iat#iat#iat#iat#iat#i#i#iat#i#ial#ial#ial#ial#ial#iaa#ial#i#l#ial#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#o#i#l#i#o#i#l.e#o#i#l.e#o#i#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#l.e#o.e#o.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.e.d.e.d.e.d.e.d.e.d.e.d.e.d.ebI.e.d.ebI.e"
".d.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbI.dbT.dbp.dbT.dbp.dbT.dbT.dbT.dbTbIbTbMbTbpbTbpbTbpbTbpbTbpbTbpbHbTbHbpbHbTbHbTbHbTbHbTbHbHbDbHbHbHbDbHbDbHb9bHbJbHbJbHbJbHbJbHbJbHbJblbJbJbJblbJbJbJbJbJbJbgbJbgbJbUbJbgbJbUbJbUbgbUbgbUbgbRbgbRbUbnbUbnbUbnbUbnbUbWbybWbybWbybWbnbwbnbwbWbwbWbwbWbwbwbwbwb3bwb3bwbrbwbrb3brb3brbBbrbBbrbBc.bBbsbrbsbrbsc.bsbsbsbsccbsccbsbqbsboccbqbobqbob8bobjbobjbobjbjbjbjbjbjbAbjbAb4bvbAbvbAbZbAbFb5bFbLbFbFbXbFbObXb0bOb0b0bYb0bYcacbcb",
"cibPcebPcjbPbPbPbPbPbPbPbPbCbPbCbPbCbPbEbPbCbCbEbCbEbCbtbCbtbCbtbCbbbCbbbCbbbQbbbQbbbtbbcgbbbba8bba8bba8bba8a8a8a8a0a8a0a8a0a8a3a8a0a8aJa8a3a8aJa0a3a3aJa0aJa3aJa3aJa3aJa3aJa3.ba3ata3ata3ata3ata3ata3ata3at#gat#gat.bat#gat.bat#gatat#ib6atat#iat#iat#iat#iat#iat#iat#iat#i#i#iat#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i#i.e#i#i#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e#i.e.e.e#i.e.e.e#i.e.e.e#i.e.e.e#i.e.e.e#i.e.e.eck.e.d.ebI.e.d.ebI.e.d.ebI.e.d.e.4.ebI.e.4.ebI.e.4.ebI.d.4.dbI.d.4.dbI.d.4.dbI"
".d.4.d.4.d.4.d.4.dbT.dbT.dbTbIbT.dbTbIbTbIbTbIbTbIbTbpbTbpbTbpbTbpbTbTbTbpbTbTbTbTbHbTbHbTbDbTbHbTbDbTbHbTbDbHbDbHb9bHbDbHb9bHb9bHbJbHbJbHbJbHbJbHbJb9bJbHbJbJbJbJbJbJbJbJbJbJbJbJbgbJbgbJbUbJbUbJbRbgbUbgbRbgbRbgbRbUbnbUbnbUbnbUbnbUbnbUbWbnbWbnbWbnbWbnbwbWbwbWbwbWbwbWbwbwbwbwb2bwb2bwbrbwbrbwbrb3brb3brbBbrbBc.brc.brbsbrbsc.bsbsbsbsbsbsccbsbqbsbqbsbqccbqbobqbob8bobjbqbjb8bjbjbjbjb4bjbAbjbvb4bvbAbZbAbZbAbFbZbFbZbXbFbXbFbObXbObObVbObVb0bYbVcbbYcbcbcl"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class DeicsOnzeGuiBase: public Ui_DeicsOnzeGuiBase {};
} // namespace Ui

QT_END_NAMESPACE

class DeicsOnzeGuiBase : public QDialog, public Ui::DeicsOnzeGuiBase
{
    Q_OBJECT

public:
    DeicsOnzeGuiBase(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~DeicsOnzeGuiBase();

protected slots:
    virtual void languageChange();

};

#endif // DEICSONZEGUIBASE_H
