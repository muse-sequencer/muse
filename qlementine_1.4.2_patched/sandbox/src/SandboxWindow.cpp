// SPDX-FileCopyrightText: Olivier Cléro <oclero@hotmail.com>
// SPDX-License-Identifier: MIT

#include "SandboxWindow.hpp"

#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/ThemeManager.hpp>
#include <oclero/qlementine/tools/ThemeEditor.hpp>
#include <oclero/qlementine/utils/ColorUtils.hpp>
#include <oclero/qlementine/utils/IconUtils.hpp>
#include <oclero/qlementine/utils/ImageUtils.hpp>
#include <oclero/qlementine/utils/PrimitiveUtils.hpp>
#include <oclero/qlementine/utils/StateUtils.hpp>
#include <oclero/qlementine/utils/WidgetUtils.hpp>
#include <oclero/qlementine/widgets/AboutDialog.hpp>
#include <oclero/qlementine/widgets/ColorEditor.hpp>
#include <oclero/qlementine/widgets/CommandLinkButton.hpp>
#include <oclero/qlementine/widgets/Expander.hpp>
#include <oclero/qlementine/widgets/IconWidget.hpp>
#include <oclero/qlementine/widgets/Label.hpp>
#include <oclero/qlementine/widgets/LineEdit.hpp>
#include <oclero/qlementine/widgets/LoadingSpinner.hpp>
#include <oclero/qlementine/widgets/NavigationBar.hpp>
#include <oclero/qlementine/widgets/PlainTextEdit.hpp>
#include <oclero/qlementine/widgets/Popover.hpp>
#include <oclero/qlementine/widgets/PopoverButton.hpp>
#include <oclero/qlementine/widgets/SegmentedControl.hpp>
#include <oclero/qlementine/widgets/StatusBadgeWidget.hpp>
#include <oclero/qlementine/widgets/Switch.hpp>
#include <oclero/qlementine/widgets/NotificationBadge.hpp>

#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDateTimeEdit>
#include <QDial>
#include <QFileSystemWatcher>
#include <QFontComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QFont>

#include <random>

namespace oclero::qlementine::sandbox {

static QIcon getTestQIcon(const QSize& size = { 16, 16 }, bool colored = false) {
  if (size.height() == 24) {
    return QIcon(":/sandbox/test_image_24x24.svg");
  } else {
    return colored ? QIcon(":/sandbox/test_image_color_16x16.svg") : QIcon(":/sandbox/test_image_16x16.svg");
  }
}

// static QIcon makeQIcon(Icons16 id, const QSize& size = { 16, 16 }) {
//   return oclero::qlementine::makeThemedIcon(id, size);
// }

class ContextMenuEventFilter : public QObject {
private:
  std::function<bool(QContextMenuEvent* evt)> _cb;

public:
  ContextMenuEventFilter(QObject* parent, std::function<bool(QContextMenuEvent*)>&& cb)
    : QObject(parent)
    , _cb(std::move(cb)) {
    parent->installEventFilter(this);
  }

protected:
  virtual bool eventFilter(QObject* watched, QEvent* evt) override {
    if (evt->type() == QEvent::ContextMenu && _cb) {
      auto* const derivedEvent = static_cast<QContextMenuEvent*>(evt);
      return _cb(derivedEvent);
    }

    return QObject::eventFilter(watched, evt);
  }
};

class FontMetricsTestsWidget : public QWidget {
  using QWidget::QWidget;

  void paintEvent(QPaintEvent*) override {
    const auto text = QString("A very long text than can be elided because it is too long.");
    const auto fm = this->fontMetrics();
    const auto totalW = this->width();
    const auto flags = Qt::AlignCenter;
    auto y = 0;

    const auto rect1 = fm.boundingRect(text);
    const auto rect2 = fm.boundingRect(QRect(), Qt::AlignCenter, text, 0, nullptr);
    const auto rect3 = fm.tightBoundingRect(text);
    const auto hAdvance = fm.horizontalAdvance(text);

    const auto textW1 = rect1.width();
    const auto textW2 = rect2.width();
    const auto textW3 = rect3.width();
    const auto textW4 = hAdvance;

    const auto availableW1 = std::min(textW1, totalW);
    const auto availableW2 = std::min(textW2, totalW);
    const auto availableW3 = std::min(textW3, totalW);
    const auto availableW4 = std::min(textW4, totalW);

    const auto textH1 = rect1.height();
    const auto textH2 = rect2.height();
    const auto textH3 = rect3.height();
    const auto textH4 = fm.height();

    QPainter p(this);
    p.fillRect(this->rect(), Qt::red);

    p.setBrush(Qt::NoBrush);

    const auto text1 = fm.elidedText(text, Qt::ElideRight, availableW1, Qt::TextSingleLine);
    p.setPen(Qt::black);
    p.drawText((totalW - availableW1) / 2, y, availableW1, textH1, flags, text1);
    p.setPen(Qt::white);
    p.drawText(0, y, totalW, textH1, 0, QString("boundingRect %1x%2").arg(textW1).arg(textH1));
    y += textH1;

    const auto text2 = fm.elidedText(text, Qt::ElideRight, availableW2, Qt::TextSingleLine);
    p.setPen(Qt::black);
    p.drawText((totalW - availableW1) / 2, y, availableW2, textH2, flags, text2);
    p.setPen(Qt::white);
    p.drawText(0, y, totalW, textH2, 0, QString("boundingRect2 %1x%2").arg(textW2).arg(textH2));
    y += textH2;

    const auto text3 = fm.elidedText(text, Qt::ElideRight, availableW3, Qt::TextSingleLine);
    p.setPen(Qt::black);
    p.drawText((totalW - availableW3) / 2, y, availableW3, textH3, flags, text3);
    p.setPen(Qt::white);
    p.drawText(0, y, totalW, textH3, 0, QString("tightBoundingRect %1x%2").arg(textW3).arg(textH3));
    y += textH3;

    const auto text4 = fm.elidedText(text, Qt::ElideRight, availableW4, Qt::TextSingleLine);
    p.setPen(Qt::black);
    p.drawText((totalW - availableW4) / 2, y, availableW4, textH4, flags, text4);
    p.setPen(Qt::white);
    p.drawText(0, y, totalW, textH4, 0, QString("horizontalAdvance %1x%2").arg(textW4).arg(textH4));
  }
};

class RoundedTriangleWidget : public QWidget {
  using QWidget::QWidget;

public:
  double radius() const {
    return _r;
  }

  void setRadius(double r) {
    _r = r;
    update();
  }

  QSize sizeHint() const override {
    return QSize(100, 100);
  }

  void paintEvent(QPaintEvent*) override {
    const auto rect = this->rect();
    QPainter p(this);
    p.fillRect(rect, Qt::red);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::yellow);
    drawRoundedTriangle(&p, rect, _r);

    p.setBrush(Qt::NoBrush);
    p.setPen(Qt::black);
    p.drawText(rect, Qt::AlignCenter, QString::number(_r));
  }

private:
  double _r{ 4. };
};

class CustomBgWidget : public QWidget {
public:
  using QWidget::QWidget;

  QColor bgColor{ Qt::red };
  QColor borderColor{ Qt::black };
  QSize customSizeHint{ -1, -1 };
  bool showBounds{ true };

  QSize sizeHint() const override {
    if (customSizeHint.isValid())
      return customSizeHint;
    else
      return QWidget::sizeHint();
  }

  QSize minimumSizeHint() const override {
    return QSize(0, 0);
  }

protected:
  void paintEvent(QPaintEvent*) override {
    QPainter p(this);
    p.fillRect(rect(), bgColor);

    if (showBounds) {
      qlementine::drawRectBorder(&p, rect(), borderColor, 1.0);
    }
  }
};

struct SandboxWindow::Impl {
  SandboxWindow& owner;
  QPointer<ThemeManager> themeManager{ nullptr };

  QWidget* windowContent{ nullptr };
  QBoxLayout* windowContentLayout{ nullptr };
  QScrollArea* globalScrollArea{ nullptr };
  QToolBar* toolbar{ nullptr };

  Impl(SandboxWindow& o, ThemeManager* themeManager)
    : owner(o)
    , themeManager(themeManager) {}

  void beginSetupUI() {
    // Create a scrollarea to wrap everything §the window can be quite huge).
    globalScrollArea = new QScrollArea(&owner);
    windowContent = new QWidget(globalScrollArea);
    windowContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    windowContentLayout = new QVBoxLayout(windowContent);

    setupShortcuts();
  }

  void endSetupUI() {
    // Add a spacer at the bottom.
    windowContentLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

    // Set the QMainWindow's central widget.
    globalScrollArea->setWidget(windowContent);
    globalScrollArea->setWidgetResizable(true);
    owner.setCentralWidget(globalScrollArea);
  }

  void setupUI_toolBar() {
    auto addToolBarIcon = [](QToolBar* toolbar, int btnNum = 1) {
      const auto icon = getTestQIcon();
      for (int i = 0; i < btnNum; i++) {
        auto* toolButton = new QToolButton(toolbar);
        toolButton->setIcon(icon);
        toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        toolbar->addWidget(toolButton);
      }
    };

    auto* contentWidget = new CustomBgWidget(windowContent);
    windowContentLayout->addWidget(contentWidget);

    contentWidget->showBounds = true;
    contentWidget->bgColor = Qt::white;

    contentWidget->setMinimumSize(500, 400);
    auto* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(1, 1, 1, 1);
    contentLayout->setSpacing(0);

    {
      auto* leftToolBar = new QToolBar(contentWidget);
      contentLayout->addWidget(leftToolBar);
      leftToolBar->setOrientation(Qt::Orientation::Vertical);
      leftToolBar->setAllowedAreas(Qt::ToolBarArea::LeftToolBarArea);
      addToolBarIcon(leftToolBar, 3);
    }

    {
      auto* centralWidget = new QWidget(contentWidget);
      contentLayout->addWidget(centralWidget);
      auto* layout = new QVBoxLayout(centralWidget);
      layout->setContentsMargins(0, 0, 0, 0);
      layout->setSpacing(0);

      {
        auto* topToolBar = new QToolBar(centralWidget);
        layout->addWidget(topToolBar);
        topToolBar->setOrientation(Qt::Orientation::Horizontal);
        topToolBar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);
        addToolBarIcon(topToolBar, 4);
      }
      {
        auto* widget = new CustomBgWidget(centralWidget);
        layout->addWidget(widget);
        widget->showBounds = false;
        widget->bgColor = Qt::white;
      }
      {
        auto* middleToolbar = new QToolBar(centralWidget);
        layout->addWidget(middleToolbar);
        middleToolbar->setOrientation(Qt::Orientation::Horizontal);
        middleToolbar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea | Qt::ToolBarArea::BottomToolBarArea);
        addToolBarIcon(middleToolbar, 3);
      }
      {
        auto* widget = new CustomBgWidget(centralWidget);
        layout->addWidget(widget);
        widget->showBounds = false;
        widget->bgColor = Qt::white;
        widget->setStyleSheet("background-color: white");
      }
      {
        auto* bottomToolBar = new QToolBar(centralWidget);
        layout->addWidget(bottomToolBar);
        bottomToolBar->setOrientation(Qt::Orientation::Horizontal);
        bottomToolBar->setAllowedAreas(Qt::ToolBarArea::BottomToolBarArea);
        addToolBarIcon(bottomToolBar, 2);
      }
    }

    {
      auto* rightToolBar = new QToolBar(contentWidget);
      contentLayout->addWidget(rightToolBar);
      rightToolBar->setOrientation(Qt::Orientation::Vertical);
      rightToolBar->setAllowedAreas(Qt::ToolBarArea::RightToolBarArea);
      addToolBarIcon(rightToolBar, 3);
    }
  }

  void setupShortcuts() {
    auto* enableShortcut = new QShortcut(Qt::CTRL | Qt::Key_E, &owner);
    enableShortcut->setAutoRepeat(false);
    enableShortcut->setContext(Qt::ShortcutContext::ApplicationShortcut);
    QObject::connect(enableShortcut, &QShortcut::activated, enableShortcut, [this]() {
      // Disable al widgets.
      if (windowContent) {
        windowContent->setEnabled(!windowContent->isEnabled());
      }
      if (toolbar) {
        toolbar->setEnabled(!toolbar->isEnabled());
      }
    });

    if (themeManager) {
      auto* themeShortcut = new QShortcut(Qt::CTRL | Qt::Key_T, &owner);
      themeShortcut->setAutoRepeat(false);
      themeShortcut->setContext(Qt::ShortcutContext::ApplicationShortcut);
      QObject::connect(themeShortcut, &QShortcut::activated, themeShortcut, [this]() {
        if (themeManager) {
          themeManager->setNextTheme();
        }
      });
    }

    auto* focusShortcut = new QShortcut(Qt::CTRL | Qt::Key_F, &owner);
    focusShortcut->setAutoRepeat(false);
    focusShortcut->setContext(Qt::ShortcutContext::ApplicationShortcut);
    QObject::connect(focusShortcut, &QShortcut::activated, focusShortcut, []() {
      if (auto* w = qApp->focusWidget()) {
        w->clearFocus();
      } else {
        if (const auto* activeWidget = qApp->activeWindow()) {
          const auto widgets = activeWidget->findChildren<QWidget*>();
          for (auto* w : widgets) {
            if (w->isEnabled() && w->focusPolicy() != Qt::NoFocus) {
              w->setFocus();
              break;
            }
          }
        }
      }
    });

    auto* quitShortcut = new QShortcut(Qt::Key_Escape, &owner);
    quitShortcut->setAutoRepeat(false);
    quitShortcut->setContext(Qt::ShortcutContext::ApplicationShortcut);
    QObject::connect(quitShortcut, &QShortcut::activated, quitShortcut, []() {
      QApplication::quit();
    });
    QObject::connect(quitShortcut, &QShortcut::activatedAmbiguously, quitShortcut, []() {
      QApplication::quit();
    });
  }

  void setupUI_label() {
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Headline 1"));
      label->setRole(TextRole::H1);
      windowContentLayout->addWidget(label);
    }
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Headline 2"));
      label->setRole(TextRole::H2);
      windowContentLayout->addWidget(label);
    }
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Headline 3"));
      label->setRole(TextRole::H3);
      windowContentLayout->addWidget(label);
    }
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Headline 4"));
      label->setRole(TextRole::H4);
      windowContentLayout->addWidget(label);
    }
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Headline 5"));
      label->setRole(TextRole::H5);
      windowContentLayout->addWidget(label);
    }
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Press CTRL+E to enable/disable widgets, and CTRL+T to change theme."));
      label->setRole(TextRole::Default);
      windowContentLayout->addWidget(label);
    }
    {
      auto* label = new Label(windowContent);
      label->setText(QStringLiteral("Comment/Uncomment lines in SandbowWindow.cpp to show/hide desired widgets."));
      label->setRole(TextRole::Caption);
      windowContentLayout->addWidget(label);
    }
  }

  void setupUI_button() {
    auto* button = new QPushButton(windowContent);
    button->setText(QStringLiteral("Button with a very long text that can be elided"));
    button->setIcon(getTestQIcon());
    button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    button->setDefault(true);
    windowContentLayout->addWidget(button);
  }

  void setupUI_buttonVariants() {
    {
      // Text, fixed size
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setDefault(true); // Only one can be the default button, so we choose the first.
      windowContentLayout->addWidget(button);
    }
    {
      // Icon, fixed size
      auto* button = new QPushButton(windowContent);
      button->setIcon(getTestQIcon());
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      windowContentLayout->addWidget(button);
    }
    {
      // Text+Icon, fixed size
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setIcon(getTestQIcon());
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      windowContentLayout->addWidget(button);
    }
    {
      // Text+Icon+Menu, fixed size
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setIcon(getTestQIcon());
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

      auto* menu = new QMenu(button);
      for (auto i = 0; i < 3; ++i) {
        menu->addAction(new QAction(QString("Action %1").arg(i), menu));
      }
      button->setMenu(menu);
      windowContentLayout->addWidget(button);
    }
    // ------
    {
      // Text, flat.
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setFlat(true);
      windowContentLayout->addWidget(button);
    }
    {
      // Icon, flat.
      auto* button = new QPushButton(windowContent);
      button->setIcon(getTestQIcon());
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setFlat(true);
      windowContentLayout->addWidget(button);
    }
    {
      // Text+Icon, flat.
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setIcon(getTestQIcon());
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setFlat(true);
      windowContentLayout->addWidget(button);
    }
    {
      // Text+Icon+Menu, flat.
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setIcon(getTestQIcon());
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      button->setFlat(true);

      auto* menu = new QMenu(button);
      for (auto i = 0; i < 3; ++i) {
        menu->addAction(new QAction(QString("Action %1").arg(i), menu));
      }
      button->setMenu(menu);
      windowContentLayout->addWidget(button);
    }
    // ------
    {
      // Text, expanding size.
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      windowContentLayout->addWidget(button);
    }
    {
      // Icon, expanding size.
      auto* button = new QPushButton(windowContent);
      button->setIcon(getTestQIcon());
      windowContentLayout->addWidget(button);
    }
    {
      // Text+Icon, expanding size.
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setIcon(getTestQIcon());
      windowContentLayout->addWidget(button);
    }
    {
      // Text+Icon+Menu, expanding size
      auto* button = new QPushButton(windowContent);
      button->setText(QStringLiteral("Button"));
      button->setIcon(getTestQIcon());

      auto* menu = new QMenu("ButtonMenu");
      for (auto i = 0; i < 3; ++i) {
        menu->addAction(new QAction(QString("Action %1").arg(i), menu));
      }
      button->setMenu(menu);
      windowContentLayout->addWidget(button);
    }
  }

  void setupUI_checkbox() {
    for (auto i = 0; i < 4; ++i) {
      auto* checkbox = new QCheckBox(windowContent);
      const auto checked = i % 2 == 0;
      const auto tristate = i > 1;

      checkbox->setChecked(checked);
      checkbox->setIcon(getTestQIcon());
      checkbox->setText(QString("%1 checkbox %2 with a very long text").arg(tristate ? "Tristate" : "Normal").arg(i));
      checkbox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      checkbox->setTristate(tristate);
      windowContentLayout->addWidget(checkbox);
    }
  }

  void setupUI_radioButton() {
    auto* radioGroup = new QButtonGroup(windowContent);

    for (auto i = 0; i < 2; ++i) {
      auto* radiobutton = new QRadioButton(windowContent);
      radiobutton->setChecked(true);
      radiobutton->setIcon(getTestQIcon());
      radiobutton->setText(QString("RadioButton %1 with a very long text").arg(i));
      radiobutton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      radioGroup->addButton(radiobutton);
      windowContentLayout->addWidget(radiobutton);
    }
  }

  void setupUI_commandLinkButton() {
    {
      auto* button = new CommandLinkButton(windowContent);
      button->setText(QStringLiteral("First Line with a very long text that should be cropped"));
      button->setDescription(QStringLiteral("Second Line that could be very long and should be cropped"));
      button->setIcon(getTestQIcon({ 24, 24 }));
      button->setDefault(true);
      button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      windowContentLayout->addWidget(button);
    }
    {
      auto* button = new CommandLinkButton(windowContent);
      button->setText(QStringLiteral("First Line with a very long text that should be cropped"));
      button->setDescription(QStringLiteral("Second Line that could be very long and should be cropped"));
      button->setIcon(getTestQIcon({ 24, 24 }));
      button->setDefault(false);
      button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      windowContentLayout->addWidget(button);
    }
  }

  void setupUI_sliderAndProgressBar() {
    constexpr auto min = 0;
    constexpr auto max = 100;
    constexpr auto val = 5;
    constexpr auto singleStep = (max - min) / max;
    constexpr auto pageStep = (max - min) / 10;

    auto* progressBar = new QProgressBar(windowContent);
    progressBar->setMaximum(max);
    progressBar->setMinimum(min);
    progressBar->setValue(val);
    progressBar->setTextVisible(true);
    progressBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    windowContentLayout->addWidget(progressBar);

    auto* slider = new QSlider(windowContent);
    slider->setOrientation(Qt::Orientation::Horizontal);
    slider->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    slider->setMinimum(min);
    slider->setMaximum(max);
    slider->setPageStep(pageStep);
    slider->setSingleStep(singleStep);
    slider->setValue(val);
    QObject::connect(slider, &QSlider::valueChanged, progressBar, &QProgressBar::setValue);
    windowContentLayout->addWidget(slider);
  }

  void setupUI_sliderWithTicks() {
    auto* slider = new QSlider(windowContent);
    slider->setOrientation(Qt::Orientation::Horizontal);
    slider->setMinimum(0);
    slider->setMaximum(10);
    slider->setPageStep(1);
    slider->setSingleStep(1);
    slider->setValue(5);
    //slider->setEnabled(false);
    slider->setTickPosition(QSlider::TickPosition::TicksAbove);
    slider->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    windowContentLayout->addWidget(slider);
  }

  void setupUI_lineEdit() {
    auto* lineEdit = new QLineEdit(windowContent);
    lineEdit->setText(QStringLiteral("Text"));
    lineEdit->setPlaceholderText(QStringLiteral("Placeholder"));
    lineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    windowContentLayout->addWidget(lineEdit);
    lineEdit->setClearButtonEnabled(true);
  }

  void setupUI_textEdit() {
    auto* textEdit = new QTextEdit(windowContent);
    textEdit->setTabChangesFocus(true);
    const auto text = QStringLiteral(
      R"(Lorem ipsum <b>dolor sit amet</b>, consectetur <i>adipiscing elit</i>, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.)");
    textEdit->append(text);
    textEdit->setPlaceholderText(QStringLiteral("Placeholder"));
    textEdit->setFixedHeight(84);

    windowContentLayout->addWidget(textEdit);
  }

  void setupUI_plainTextEdit() {
    auto* plainTextEdit = new PlainTextEdit(windowContent); // Also PlainTextEdit
    plainTextEdit->setTabChangesFocus(true);
    plainTextEdit->setFixedHeight(84);
    const auto text = QStringLiteral(
      R"(Lorem ipsum <b>dolor sit amet</b>, consectetur <i>adipiscing elit</i>, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.)");
    plainTextEdit->appendHtml(text);
    plainTextEdit->setPlaceholderText(QStringLiteral("Placeholder"));

    // Use QFrame::Shape::StyledPanel if you want the default look (borders, corner radiuses).
    // Use QFrame::Shape::Panel if you don't want borders neither corner radiuses.
    plainTextEdit->setFrameShape(QFrame::Shape::StyledPanel);

    // Use QPlainTextEdit::setFont() to change the font.
    if (auto* qlementine = qobject_cast<QlementineStyle*>(plainTextEdit->style())) {
      auto font = QFont{ qlementine->theme().fontMonospace };
      font.setPixelSize(font.pointSize() * 2);
      plainTextEdit->setFont(font);
    }

    windowContentLayout->addWidget(plainTextEdit);
  }

  void setupUI_dial() {
    auto* dial = new QDial(windowContent);
    dial->setOrientation(Qt::Orientation::Horizontal);
    dial->setMinimum(0);
    dial->setMaximum(100);
    dial->setPageStep(10);
    dial->setSingleStep(1);
    dial->setValue(5);
    dial->setNotchesVisible(true);
    dial->setFixedSize(48, 48);
    windowContentLayout->addWidget(dial);
  }

  void setupUI_spinBox() {
    auto* spinbox = new QSpinBox(windowContent);
    spinbox->setMinimum(0);
    spinbox->setMaximum(100);
    spinbox->setValue(50);
    spinbox->setSingleStep(1);
    spinbox->setSuffix(QStringLiteral("km/h"));
    spinbox->setPrefix(QStringLiteral("$"));
    spinbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    windowContentLayout->addWidget(spinbox);
  }

  void setupUI_comboBox() {
    auto* combobox = new QComboBox(windowContent);
    combobox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    combobox->setEditable(true);
    for (auto i = 0; i < 4; ++i) {
      combobox->addItem(getTestQIcon(), QString("ComboBox item %1").arg(i));
    }
    windowContentLayout->addWidget(combobox);
  }

  void setupUI_comboBoxVariants() {
    struct ComboBoxTestData {
      bool hasIcons{ false };
      bool isEditable{ false };
    };

    for (const auto& data : std::vector<ComboBoxTestData>{
           { false, false },
           { false, true },
           { true, false },
           { true, true },
         }) {
      auto* combobox = new QComboBox(windowContent);
      combobox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      combobox->setEditable(data.isEditable);

      for (auto i = 0; i < 4; ++i) {
        if (data.hasIcons) {
          combobox->addItem(getTestQIcon(), QString("ComboBox item %1").arg(i));
        } else {
          combobox->addItem(QString("ComboBox item %1").arg(i));
        }
      }
      auto* model = qobject_cast<QStandardItemModel*>(combobox->model());
      auto* item = model->item(2);
      item->setEnabled(false);

      windowContentLayout->addWidget(combobox);
    }
  }

  void setupUI_comboBoxWithTreeView() {
    auto* combobox = new QComboBox(windowContent);
    combobox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    {
      auto* treeWidget = new QTreeWidget(windowContent);
      treeWidget->setMinimumSize(100, 200);
      treeWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
      treeWidget->setAlternatingRowColors(false);
      treeWidget->setColumnCount(1);
      treeWidget->setHeaderHidden(true);
      treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

      for (auto i = 0; i < 3; ++i) {
        auto* root = new QTreeWidgetItem(treeWidget);
        root->setText(0, QString("Root %1").arg(i + 1));
        root->setIcon(0, getTestQIcon({ 16, 16 }));
        root->setText(1, QString("Column 2 of Root %1").arg(i + 1));

        for (auto j = 0; j < 3; ++j) {
          auto* child = new QTreeWidgetItem(root);
          child->setText(0, QString("Child %1 of Root %2").arg(j).arg(i));
          child->setIcon(0, getTestQIcon({ 16, 16 }));
          child->setText(1, QString("Column 2 of Child %1 of Root %2").arg(j).arg(i));

          for (auto k = 0; k < 3; ++k) {
            auto* subChild = new QTreeWidgetItem(child);
            subChild->setText(0, QString("Child %1 of Child %2 of Root %3").arg(k).arg(j).arg(i));
            subChild->setIcon(0, getTestQIcon({ 16, 16 }));
            subChild->setText(1, QString("Column 2 of Child %1 of Child %2 of Root %3").arg(k).arg(j).arg(i));
          }
        }
      }

      treeWidget->topLevelItem(0)->setSelected(true);

      combobox->setModel(treeWidget->model());

      QTimer::singleShot(300, combobox, [combobox, treeWidget]() {
        combobox->setView(treeWidget);
      });
      //combobox->setView(treeWidget);
    }

    windowContentLayout->addWidget(combobox);
  }

  void setupUI_fontComboBox() {
    auto* combobox = new QFontComboBox(windowContent);
    combobox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    combobox->setFocusPolicy(Qt::StrongFocus);
    windowContentLayout->addWidget(combobox);
  }

  void setupUI_listView() {
    auto* listView = new QListWidget(windowContent);
    listView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    //listView->setAlternatingRowColors(true);
    listView->setIconSize(QSize(32, 32));

    for (auto i = 0; i < 2; ++i) {
      auto* item = new QListWidgetItem(
        getTestQIcon(), QString("Item #%1 with very long text that can be elided").arg(i), listView);
      item->setFlags(item->flags() | Qt::ItemFlag::ItemIsUserCheckable);
      item->setCheckState(i % 2 ? Qt ::CheckState::Checked : Qt::CheckState::Unchecked);
      //item->setForeground(i % 2 ? Qt::red : Qt::blue);
      listView->addItem(item);
    }
    listView->item(0)->setSelected(true);
    windowContentLayout->addWidget(listView);

    // Context menu.
    listView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    QObject::connect(listView, &QListView::customContextMenuRequested, listView, [listView](const QPoint& pos) {
      if (const auto item = listView->itemAt(pos)) {
        QMenu contextMenu(listView);
        //contextMenu.setTearOffEnabled(true);

        for (auto i = 0; i < 10; ++i) {
          if (i % 5 == 0) {
            contextMenu.addSeparator();
          } else {
            contextMenu.addAction(QString("Distinctio voluptatum dolorum beatae %1").arg(i));
          }
        }

        const auto globalPos = listView->mapToGlobal(pos);
        contextMenu.exec(globalPos);
      }
    });
  }

  void setupUI_table() {
    auto* tableView = new QTableWidget(windowContent);
    tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tableView->setSortingEnabled(true);

    if (auto* qlementineStyle = qobject_cast<QlementineStyle*>(tableView->style())) {
      qlementineStyle->setAutoIconColor(tableView, oclero::qlementine::AutoIconColor::None);
    }

    constexpr auto columnCount = 3;
    constexpr auto rowCount = 3;
    tableView->setColumnCount(columnCount);
    tableView->setRowCount(rowCount);

    std::vector<Qt::Alignment> columnAlignments;
    columnAlignments.resize(columnCount);
    for (auto col = 0; col < columnCount; ++col) {
      const auto alignment = col % 3 == 0 ? Qt::AlignLeft : (col % 3 == 1 ? Qt::AlignRight : Qt::AlignCenter);
      columnAlignments[col] = alignment;
    }

    for (auto col = 0; col < columnCount; ++col) {
      auto* item = new QTableWidgetItem(QString("Column %1").arg(col + 1));
      item->setIcon(getTestQIcon({ 16, 16 }, true));
      item->setTextAlignment(columnAlignments.at(col));
      tableView->setHorizontalHeaderItem(col, item);
    }

    for (auto row = 0; row < rowCount; ++row) {
      auto* item = new QTableWidgetItem(QString("Row %1").arg(row + 1));
      item->setIcon(getTestQIcon({ 16, 16 }, true));
      tableView->setVerticalHeaderItem(row, item);
    }

    for (auto row = 0; row < rowCount; ++row) {
      for (auto col = 0; col < columnCount; ++col) {
        auto* item = new QTableWidgetItem(QString("Item at %1, %2").arg(row + 1).arg(col + 1));
        item->setIcon(getTestQIcon({ 16, 16 }, true));
        item->setTextAlignment(columnAlignments.at(col));
        item->setFlags(Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled);
        item->setData(Qt::DisplayRole, QVariant::fromValue(true));
        tableView->setItem(row, col, item);
      }
    }

    windowContentLayout->addWidget(tableView);
  }

  void setupUI_treeWidget() {
    auto* treeWidget = new QTreeWidget(windowContent);
    treeWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    treeWidget->setAlternatingRowColors(false);
    treeWidget->setColumnCount(1);
    treeWidget->setHeaderHidden(true);
    treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    oclero::qlementine::appStyle()->setAutoIconColor(treeWidget, oclero::qlementine::AutoIconColor::None);

    for (auto i = 0; i < 3; ++i) {
      auto* root = new QTreeWidgetItem(treeWidget);
      root->setText(0, QString("Root %1").arg(i + 1));
      root->setIcon(0, getTestQIcon({ 16, 16 }, true));
      root->setText(1, QString("Column 2 of Root %1").arg(i + 1));

      for (auto j = 0; j < 3; ++j) {
        auto* child = new QTreeWidgetItem(root);
        child->setText(0, QString("Child %1 of Root %2").arg(j).arg(i));
        child->setIcon(0, getTestQIcon({ 16, 16 }, true));
        child->setText(1, QString("Column 2 of Child %1 of Root %2").arg(j).arg(i));

        for (auto k = 0; k < 3; ++k) {
          auto* subChild = new QTreeWidgetItem(child);
          subChild->setText(0, QString("Child %1 of Child %2 of Root %3").arg(k).arg(j).arg(i));
          subChild->setIcon(0, getTestQIcon({ 16, 16 }, true));
          subChild->setText(1, QString("Column 2 of Child %1 of Child %2 of Root %3").arg(k).arg(j).arg(i));
        }
      }
    }

    treeWidget->topLevelItem(0)->setSelected(true);
    windowContentLayout->addWidget(treeWidget);
  }

  void setupUI_menuBar() const {
    auto* menuBar = owner.menuBar();
    // NB: it looks like MacOS' native menu bar has an issue with QIcon, so we have to force
    // it to generate icons for High-DPI screens.
    const auto icon = getTestQIcon();

    for (auto i = 0; i < 5; ++i) {
      auto* menu = menuBar->addMenu(QString("Menu &%1").arg(i));

      for (auto j = 0; j < 10; ++j) {
        auto* action = menu->addAction(icon, QString("Menu %1 - Action &%2").arg(i).arg(j));

        if (j == 0) {
          auto* subMenu = new QMenu(menuBar);

          auto* subActionGroup = new QActionGroup(subMenu);
          for (auto k = 0; k < 6; ++k) {
            auto* subAction = subMenu->addAction(icon, QString("SubMenu %1 - Action &%2").arg(j).arg(k));

            if (k % 2 == 0) {
              subAction->setEnabled(false);
            }
            subActionGroup->addAction(subAction);
            subAction->setCheckable(true);
          }

          action->setMenu(subMenu);
        } else if (j == 1) {
          action->setCheckable(true);
          action->setChecked(true);
        } else if (j % 2 == 0) {
          const auto keyNumber = (Qt::Key) (Qt::Key_0 + j);
          const auto keySeq = QKeySequence(Qt::CTRL | (Qt::Key_0 + keyNumber));
          action->setShortcut(keySeq);
        } else if (j % 3 == 0) {
          const auto keyNumber = (Qt::Key) (Qt::Key_0 + j);
          const auto keySeq = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::ALT | (Qt::Key_0 + keyNumber));
          action->setShortcut(keySeq);
        } else if (j % 5 == 0) {
          action->setEnabled(false);
        }
      }
    }
  }

  void setupUI_toolButton() {
    auto* toolButton = new QToolButton(toolbar);
    toolButton->setIcon(getTestQIcon());
    toolButton->setText(QStringLiteral("Button with a very long text that can be elided"));
    toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolButton->setCheckable(false);
    toolButton->setChecked(false);

    {
      const auto icon = getTestQIcon();
      auto* subMenu = new QMenu(QStringLiteral("Menu title"), toolButton);
      toolButton->setMenu(subMenu);
      subMenu->addAction(icon, QStringLiteral("Sub Action 1"));
      subMenu->addAction(icon, QStringLiteral("Sub Action 2"));
      toolButton->setMenu(subMenu);
      toolButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
    }

    toolButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    windowContentLayout->addWidget(toolButton);
  }

  void setupUI_toolButtonsVariants() {
    const auto icon = getTestQIcon();

    toolbar = owner.addToolBar(QStringLiteral("ToolBar name"));
    //toolbar->set
    toolbar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonFollowStyle);

    // Button 1: Icon only
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
      toolbar->addWidget(toolButton);
    }
    // Button 2: Text only
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
      toolbar->addWidget(toolButton);
    }
    // Button 3: Icon and Text.
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      toolbar->addWidget(toolButton);
    }
    // Button 4: Icon and Text, checkable.
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      toolButton->setCheckable(true);
      toolButton->setChecked(true);
      toolbar->addWidget(toolButton);
    }
    // Button 5: Icon only + menu
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
      auto* subMenu = new QMenu("Menu title", toolButton);
      toolButton->setMenu(subMenu);
      subMenu->addAction(icon, "Sub Action 1");
      subMenu->addAction(icon, "Sub Action 2");
      toolbar->addWidget(toolButton);
    }

    // Button 6: Text only + menu
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
      toolButton->setPopupMode(QToolButton::InstantPopup);
      auto* subMenu = new QMenu(QStringLiteral("Menu title"), toolButton);
      toolButton->setMenu(subMenu);
      subMenu->addAction(icon, QStringLiteral("Sub Action 1"));
      subMenu->addAction(icon, QStringLiteral("Sub Action 2"));
      toolbar->addWidget(toolButton);
    }

    // Button 6: Icon and Text + menu
    {
      auto* toolButton = new QToolButton(toolbar);
      toolButton->setIcon(icon);
      toolButton->setText(QStringLiteral("Button"));
      toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
      toolButton->setPopupMode(QToolButton::MenuButtonPopup);
      auto* subMenu = new QMenu(QStringLiteral("Menu title"), toolButton);
      toolButton->setMenu(subMenu);
      subMenu->addAction(icon, QStringLiteral("Sub Action 1"));
      subMenu->addAction(icon, QStringLiteral("Sub Action 2"));
      toolbar->addWidget(toolButton);
    }
  }

  void setupUI_tabBar() {
    auto* tabBar = new QTabBar(windowContent);
    tabBar->setFocusPolicy(Qt::NoFocus);
    tabBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    oclero::qlementine::appStyle()->setAutoIconColor(tabBar, oclero::qlementine::AutoIconColor::None);

    // QTabBar features.
    tabBar->setTabsClosable(true);
    tabBar->setMovable(true);
    tabBar->setExpanding(/*true*/ false);
    tabBar->setChangeCurrentOnDrag(true);
    tabBar->setDocumentMode(true);
    tabBar->setUsesScrollButtons(true);

    windowContentLayout->addWidget(tabBar);
    //windowContentLayout->setAlignment(tabBar, Qt::AlignLeft);

    for (auto i = 0; i < 5; ++i) {
      QStringList tabTextList{ "Tab " };
      for (auto j = 0; j < i; ++j) {
        tabTextList.append("Tab");
      }
      const auto tabText = QString(tabTextList.join(" ").append(QString(" %1").arg(i + 1)));

      if (i % 3 == 0) {
        tabBar->addTab(getTestQIcon({ 16, 16 }, true), tabText);
      } else {
        tabBar->addTab(tabText);
      }
      tabBar->setTabToolTip(i, tabText);

      if (i % 2 == 0) {
        auto* leftWidget = new CustomBgWidget();
        const auto extent = leftWidget->style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth);
        leftWidget->customSizeHint = QSize(extent, extent);
        leftWidget->bgColor = QColor(255, 0, 0, 32);
        leftWidget->borderColor = QColor(255, 0, 0);
        tabBar->setTabButton(i, QTabBar::ButtonPosition::LeftSide, leftWidget);
      }
    }

    tabBar->setCurrentIndex(1);

    //windowContentLayout->setContentsMargins(0, 0, 0, 16);

    QObject::connect(tabBar, &QTabBar::tabCloseRequested, tabBar, [tabBar](int index) {
      tabBar->removeTab(index);
    });
  }

  void setupUI_tabWidget() {
    auto* tabWidget = new QTabWidget(windowContent);

    tabWidget->setDocumentMode(false);
    tabWidget->setTabsClosable(true);
    tabWidget->setMovable(true);
    tabWidget->setUsesScrollButtons(true);
    QObject::connect(tabWidget, &QTabWidget::tabCloseRequested, tabWidget, [tabWidget](int index) {
      tabWidget->removeTab(index);
    });

    windowContentLayout->addWidget(tabWidget);

    for (auto i = 0; i < 5; ++i) {
      auto* tabContent = new QWidget(); // Parent will be set by QTabWidget.
      tabContent->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      auto* tabContentLayout = new QVBoxLayout(tabContent);

      // Dummy tab content.
      for (auto j = 0; j < (i + 1); ++j) {
        tabContentLayout->addWidget(new QPushButton(QStringLiteral("Button"), tabContent));
      }
      tabContentLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

      QStringList tabTextList{ "Tab" };
      for (auto j = 0; j < i; ++j) {
        tabTextList.append("Tab");
      }
      const auto tabText = QString(tabTextList.join(" ").append(QString(" %1").arg(i + 1)));
      const auto icon = getTestQIcon();
      tabWidget->addTab(tabContent, icon, tabText);
    }
  }

  void setupUI_groupBox() {
    for (auto i = 0; i < 3; ++i) {
      auto* groupBox = new QGroupBox(windowContent);
      groupBox->setAlignment(Qt::AlignRight);
      groupBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

      // QGroupBox features:
      groupBox->setTitle(QString("Title of the GroupBox %1 that can be very long").arg(i + 1));
      groupBox->setCheckable(true);
      groupBox->setFlat(false);

      auto* radioGroup = new QButtonGroup(groupBox);

      auto* radio1 = new QRadioButton(QStringLiteral("Radio button 1"));
      radio1->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      radioGroup->addButton(radio1);

      auto* radio2 = new QRadioButton(QStringLiteral("Radio button 2"));
      radio2->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
      radioGroup->addButton(radio2);

      auto* button1 = new QPushButton(QStringLiteral("Button 1"));
      button1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

      auto* button2 = new QPushButton(QStringLiteral("Button 2"));
      button2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

      radio1->setChecked(true);

      auto* vbox = new QVBoxLayout(groupBox);
      groupBox->setLayout(vbox);
      //vbox->setContentsMargins(0, 0, 0, 0);
      //vbox->setSpacing(0);
      vbox->addWidget(radio1);
      vbox->addWidget(radio2);
      vbox->addWidget(button1);
      vbox->addWidget(button2);
      vbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

      windowContentLayout->addWidget(groupBox);
    }
  }

  void setupUI_fontMetricsTests() {
    // Widget to test the different bounding boxes returned by QFontMetrics.
    auto* fontMetricsWidget = new FontMetricsTestsWidget(windowContent);
    fontMetricsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    fontMetricsWidget->setMinimumSize(100, 100);
    windowContentLayout->addWidget(fontMetricsWidget);
  }

  void setupUI_messageBox() {
    const auto title = QStringLiteral("Title of the QMessageBox");
    const auto text = QStringLiteral(
      R"(Lorem ipsum dolor sit amet, consectetur <a href="#">adipiscing elit</a>, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.)");
    const auto informativeText = QStringLiteral(
      R"(Vitae ut et dolorem eum. Rerum aut aut quis <a href="#">dolorum facere</a> quod veniam accusantium.
Accusamus quidem sed possimus aut consequatur soluta ut. Soluta ut enim quo reiciendis a tempora dolorum min…)");
    const auto detailedText = QStringLiteral(
      R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.
Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum)");
    const auto buttons = QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel;
    auto* qMessageBox = new QMessageBox(QMessageBox::Icon::Information, title, text, buttons, &owner);
    qMessageBox->setInformativeText(informativeText);
    qMessageBox->setDetailedText(detailedText);
    qMessageBox->show();

    QObject::connect(qMessageBox, &QMessageBox::finished, qMessageBox, []() {
      std::exit(0);
    });
  }

  void setupUI_messageBoxIcons() {
    const auto* qStyle = QApplication::style();
    const auto iconExtent = qStyle->pixelMetric(QStyle::PM_LargeIconSize) * 2;
    const auto iconSize = QSize(iconExtent, iconExtent);

    for (const auto stdIcon : { QStyle::SP_MessageBoxCritical, QStyle::SP_MessageBoxWarning,
           QStyle::SP_MessageBoxInformation, QStyle::SP_MessageBoxQuestion }) {
      auto* label = new QLabel(windowContent);
      label->setFixedSize(iconSize);
      const auto icon1 = qStyle->standardIcon(stdIcon);
      label->setPixmap(icon1.pixmap(iconSize.width()));
      windowContentLayout->addWidget(label);
    }
  }

  void setupUI_loadingSpinner() {
    auto* spinner = new LoadingSpinner(windowContent);
    spinner->setSpinning(true);
    windowContentLayout->addWidget(spinner);

    auto* checkbox = new QCheckBox("Spinning", windowContent);
    checkbox->setChecked(spinner->spinning());
    windowContentLayout->addWidget(checkbox);
    QObject::connect(checkbox, &QCheckBox::clicked, spinner, [spinner](bool checked) {
      spinner->setSpinning(checked);
    });

    auto* checkbox2 = new QCheckBox("Visible", windowContent);
    checkbox2->setChecked(spinner->isVisibleTo(windowContent));
    windowContentLayout->addWidget(checkbox2);
    QObject::connect(checkbox2, &QCheckBox::clicked, spinner, [spinner](bool checked) {
      spinner->setVisible(checked);
    });
  }

  void setupUI_aboutDialog() {
    auto* dialog = new AboutDialog(windowContent);
    dialog->setWindowTitle(QString("About %1").arg(QApplication::applicationDisplayName()));
    dialog->setDescription("An application to showcase Qlementine's capabilities as a QStyle library.");
    dialog->setWebsiteUrl("https://oclero.github.io/qlementine");
    dialog->setLicense("Licensed under MIT license.");
    dialog->setCopyright("© Olivier Cléro");
    dialog->addSocialMediaLink("GitHub", "https://github.com/oclero/qlementine", getTestQIcon());
    dialog->addSocialMediaLink("Mastodon", "https://mastodon.online/@oclero", getTestQIcon());
    dialog->addSocialMediaLink("GitLab", "https://gitlab.com/oclero", getTestQIcon());
    dialog->show();
  }

  void setupUI_notificationBadge() {
    {
      auto* button = new QPushButton("Button", windowContent);
      button->setFocusPolicy(Qt::FocusPolicy::NoFocus);
      windowContentLayout->addWidget(button);

      auto* badge = new NotificationBadge(windowContent);
      badge->setWidget(button);
      badge->setRelativePosition(-4, 4);
      windowContentLayout->addWidget(badge);
    }
    {
      auto* button = new QPushButton("Button", windowContent);
      button->setFocusPolicy(Qt::FocusPolicy::NoFocus);
      windowContentLayout->addWidget(button);

      auto* badge = new NotificationBadge(windowContent);
      badge->setWidget(button);
      badge->setText("3");
      windowContentLayout->addWidget(badge);
    }
  }

  void setupUI_treeView() {
    {
      auto* treeWidget = new QTreeWidget(windowContent);
      treeWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
      treeWidget->setAlternatingRowColors(false);
      treeWidget->setColumnCount(1);
      treeWidget->setHeaderHidden(true);
      treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
      treeWidget->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
      oclero::qlementine::appStyle()->setAutoIconColor(treeWidget, oclero::qlementine::AutoIconColor::None);

      for (auto i = 0; i < 3; ++i) {
        auto* root = new QTreeWidgetItem(treeWidget);
        root->setText(0, QString("Root %1").arg(i + 1));
        root->setIcon(0, getTestQIcon({ 16, 16 }, true));
        root->setText(1, QString("Column 2 of Root %1").arg(i + 1));

        for (auto j = 0; j < 3; ++j) {
          auto* child = new QTreeWidgetItem(root);
          child->setText(0, QString("Child %1 of Root %2").arg(j).arg(i));
          child->setIcon(0, getTestQIcon({ 16, 16 }, true));
          child->setText(1, QString("Column 2 of Child %1 of Root %2").arg(j).arg(i));

          for (auto k = 0; k < 3; ++k) {
            auto* subChild = new QTreeWidgetItem(child);
            subChild->setText(0, QString("Child %1 of Child %2 of Root %3").arg(k).arg(j).arg(i));
            subChild->setIcon(0, getTestQIcon({ 16, 16 }, true));
            subChild->setText(1, QString("Column 2 of Child %1 of Child %2 of Root %3").arg(k).arg(j).arg(i));
          }
        }
      }

      treeWidget->topLevelItem(0)->setSelected(true);
      windowContentLayout->addWidget(treeWidget);
    }

    {
      auto* listView = new QListWidget(windowContent);
      listView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
      //listView->setAlternatingRowColors(true);
      listView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

      for (auto i = 0; i < 3; ++i) {
        auto* item = new QListWidgetItem(
          getTestQIcon(), QString("Item #%1 with very long text that can be elided").arg(i), listView);
        item->setFlags(item->flags() | Qt::ItemFlag::ItemIsUserCheckable);
        item->setCheckState(i % 2 ? Qt ::CheckState::Checked : Qt::CheckState::Unchecked);

        listView->addItem(item);
      }
      listView->item(0)->setSelected(true);
      windowContentLayout->addWidget(listView);
    }

    {
      constexpr auto columnCount = 3;
      constexpr auto rowCount = 3;

      auto* tableView = new QTableWidget(windowContent);
      tableView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
      tableView->setColumnCount(columnCount);
      tableView->setRowCount(rowCount);
      auto icon = getTestQIcon();
      auto* headerItem = new QTableWidgetItem(icon, QStringLiteral("A veeeeeery long header label"));
      tableView->setHorizontalHeaderItem(0, headerItem);
      tableView->setSelectionBehavior(QTableView::SelectionBehavior::SelectRows);
      //tableView->verticalHeader()->hide();
      //tableView->horizontalHeader()->hide();
      //tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
      //tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Interactive);
      tableView->horizontalHeader()->setSectionResizeMode(columnCount - 1, QHeaderView::ResizeMode::Stretch);
      tableView->horizontalHeader()->setSortIndicatorShown(true);
      tableView->setShowGrid(false);

      for (auto i = 0; i < rowCount; ++i) {
        for (auto j = 0; j < columnCount; ++j) {
          auto* item = new QTableWidgetItem(QString("Row %1 / Column %2").arg(i + 1).arg(j + 1));
          item->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
          tableView->setItem(i, j, item);
        }
      }
      tableView->item(0, 0)->setSelected(true);

      windowContentLayout->addWidget(tableView);
    }

    owner.resize(400, 700);
  }

  void setupUI_expander() {
    windowContent->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    {
      auto* container = new CustomBgWidget(windowContent);
      container->bgColor = QColor{ 255, 0, 0, 10 };
      container->borderColor = QColor{ 255, 0, 0, 40 };
      auto* containerLayout = new QVBoxLayout(container);
      containerLayout->setContentsMargins(10, 10, 10, 10);
      container->setLayout(containerLayout);

      auto* expander = new Expander(container);
      expander->setOrientation(Qt::Orientation::Horizontal);
      auto* expanderContent = new CustomBgWidget(expander);
      expanderContent->bgColor = QColor{ 0, 0, 255, 40 };
      expanderContent->borderColor = QColor{ 0, 0, 255, 127 };
      expanderContent->customSizeHint = QSize{ 150, 100 };
      expanderContent->showBounds = true;
      expander->setContent(expanderContent);

      auto* checkBox = new QCheckBox(QStringLiteral("Expanded"), container);
      checkBox->setChecked(expander->expanded());
      QObject::connect(checkBox, &QCheckBox::toggled, &owner, [expander](bool checked) {
        expander->setExpanded(checked);
      });

      auto* vLayout = new QVBoxLayout();
      auto* buttonGroup = new QButtonGroup(windowContent);
      buttonGroup->setParent(windowContent); // to make clang-analyzer happy.
      for (auto orientation : { Qt::Vertical, Qt::Horizontal }) {
        auto* radioButton = new QRadioButton(
          orientation == Qt::Vertical ? QStringLiteral("Vertical") : QStringLiteral("Horizontal"), container);
        radioButton->setChecked(orientation == expander->orientation());
        buttonGroup->addButton(radioButton);
        vLayout->addWidget(radioButton);
        QObject::connect(radioButton, &QRadioButton::toggled, &owner, [expander, orientation](bool checked) {
          if (checked) {
            expander->setOrientation(orientation);
          }
        });
      }

      auto* button = new QPushButton(QStringLiteral("Increase content animated dimension"), container);
      QObject::connect(button, &QPushButton::clicked, &owner, [expanderContent, expander]() {
        if (expander->orientation() == Qt::Vertical) {
          expanderContent->customSizeHint.rheight() += 20;
        } else {
          expanderContent->customSizeHint.rwidth() += 20;
        }
        expanderContent->updateGeometry();
      });

      containerLayout->addWidget(checkBox);
      containerLayout->addWidget(button);
      containerLayout->addLayout(vLayout);
      containerLayout->addWidget(expander);

      windowContentLayout->addWidget(container);
    }
  }

  void setupUI_popover() {
#if 0
    auto* popoverButton = new PopoverButton(QStringLiteral("Open popup"), windowContent);
    popoverButton->popover()->setPreferredPosition(Popover::Position::Top);
    popoverButton->popover()->setPreferredAlignment(Popover::Alignment::Center);
    popoverButton->popover()->setVerticalSpacing(10);
    auto* dummyWidget = new CustomBgWidget();
    dummyWidget->bgColor = Qt::transparent;
    dummyWidget->customSizeHint = QSize{ 150, 150 };
    popoverButton->setPopoverContentWidget(dummyWidget);
    windowContentLayout->addWidget(popoverButton);
    owner.setCentralWidget(windowContent);
    return;
#endif

    auto* anchorLabel =
      new QLabel(QStringLiteral("The popover positions itself relatively to this widget:"), windowContent);
    anchorLabel->setWordWrap(true);
    windowContentLayout->addWidget(anchorLabel);

    auto* anchorWidget = new CustomBgWidget(&owner);
    anchorWidget->showBounds = false;
    anchorWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    anchorWidget->customSizeHint = QSize(100, 100);
    anchorWidget->bgColor = Qt::blue;
    windowContentLayout->addWidget(anchorWidget, 1, Qt::AlignCenter);

    auto* popoverCheckBox = new QCheckBox(QStringLiteral("Popup is opened"), windowContent);
    windowContentLayout->addWidget(popoverCheckBox, 0, Qt::AlignBottom);

    auto* positionLabel = new QLabel(QStringLiteral("Position:"), windowContent);
    windowContentLayout->addWidget(positionLabel);

    auto* comboBoxPosition = new QComboBox(&owner);
    comboBoxPosition->addItem(QStringLiteral("Left"), static_cast<int>(Popover::Position::Left));
    comboBoxPosition->addItem(QStringLiteral("Right"), static_cast<int>(Popover::Position::Right));
    comboBoxPosition->addItem(QStringLiteral("Top"), static_cast<int>(Popover::Position::Top));
    comboBoxPosition->addItem(QStringLiteral("Bottom"), static_cast<int>(Popover::Position::Bottom));
    windowContentLayout->addWidget(comboBoxPosition);

    auto* alignmentLabel = new QLabel("Alignment:", windowContent);
    windowContentLayout->addWidget(alignmentLabel);

    auto* comboBoxAlignment = new QComboBox(&owner);
    comboBoxAlignment->addItem(QStringLiteral("Begin"), static_cast<int>(Popover::Alignment::Begin));
    comboBoxAlignment->addItem(QStringLiteral("Center"), static_cast<int>(Popover::Alignment::Center));
    comboBoxAlignment->addItem(QStringLiteral("End"), static_cast<int>(Popover::Alignment::End));
    windowContentLayout->addWidget(comboBoxAlignment);

    auto* popover = new Popover(anchorWidget);
    popover->setPadding({ 0, 0, 0, 0 });
    popover->setHorizontalSpacing(0);
    popover->setVerticalSpacing(0);

    auto* hSpacingLabel = new QLabel(QStringLiteral("Horizontal Spacing:"), windowContent);
    windowContentLayout->addWidget(hSpacingLabel);

    auto* hSpacingSpinBox = new QSpinBox(&owner);
    hSpacingSpinBox->setRange(-100, 100);
    windowContentLayout->addWidget(hSpacingSpinBox);

    auto* vSpacingLabel = new QLabel(QStringLiteral("Vertical Spacing:"), windowContent);
    windowContentLayout->addWidget(vSpacingLabel);

    auto* vSpacingSpinBox = new QSpinBox(&owner);
    vSpacingSpinBox->setRange(-100, 100);
    windowContentLayout->addWidget(vSpacingSpinBox);

#if 1
    auto* popoverContent = new QWidget();
    auto* popoverContentLayout = new QVBoxLayout(popoverContent);
    popoverContentLayout->setContentsMargins(16, 16, 16, 16);
    {
      for (auto i = 0; i < 3; ++i) {
        auto* btn = new QPushButton(QString("QPushButton %1").arg(i + 1), popoverContent);
        popoverContentLayout->addWidget(btn);
      }
    }
#else
    auto* popoverContent = new CustomBgWidget();
    popoverContent->showBounds = true;
    popoverContent->customSizeHint = QSize(100, 200);

#endif
    popover->setContentWidget(popoverContent);
    popover->setAnchorWidget(anchorWidget);

    // Synchronize opened state.
    popoverCheckBox->setChecked(false);
    QObject::connect(popoverCheckBox, &QAbstractButton::clicked, &owner, [popover](bool checked) {
      popover->setOpened(checked);
    });
    QObject::connect(popover, &Popover::openedChanged, &owner, [popover, popoverCheckBox]() {
      QSignalBlocker b{ popoverCheckBox };
      popoverCheckBox->setChecked(popover->isOpened());
    });

    // Synchronize position.
    comboBoxPosition->setCurrentIndex(comboBoxPosition->findData(static_cast<int>(popover->preferredPosition())));
    QObject::connect(comboBoxPosition, qOverload<int>(&QComboBox::currentIndexChanged), &owner,
      [comboBoxPosition, popover](int index) {
        const auto pos = static_cast<Popover::Position>(comboBoxPosition->itemData(index).toInt());
        popover->setPreferredPosition(pos);
      });
    QObject::connect(popover, &Popover::preferredPositionChanged, &owner, [popover, comboBoxPosition]() {
      QSignalBlocker b{ comboBoxPosition };
      comboBoxPosition->setCurrentIndex(comboBoxPosition->findData(static_cast<int>(popover->preferredPosition())));
    });

    // Synchronize alignment.
    comboBoxAlignment->setCurrentIndex(comboBoxAlignment->findData(static_cast<int>(popover->preferredAlignment())));
    QObject::connect(comboBoxAlignment, qOverload<int>(&QComboBox::currentIndexChanged), &owner,
      [comboBoxAlignment, popover](int index) {
        const auto align = static_cast<Popover::Alignment>(comboBoxAlignment->itemData(index).toInt());
        popover->setPreferredAlignment(align);
      });
    QObject::connect(popover, &Popover::preferredAlignmentChanged, &owner, [popover, comboBoxAlignment]() {
      QSignalBlocker b{ comboBoxAlignment };
      comboBoxAlignment->setCurrentIndex(comboBoxAlignment->findData(static_cast<int>(popover->preferredAlignment())));
    });

    // Synchronize horizontal spacing.
    hSpacingSpinBox->setValue(popover->horizontalSpacing());
    QObject::connect(hSpacingSpinBox, qOverload<int>(&QSpinBox::valueChanged), &owner, [popover](int v) {
      popover->setHorizontalSpacing(v);
    });
    QObject::connect(popover, &Popover::horizontalSpacingChanged, &owner, [hSpacingSpinBox, popover]() {
      hSpacingSpinBox->setValue(popover->horizontalSpacing());
    });

    // Synchronize vertical spacing.
    vSpacingSpinBox->setValue(popover->verticalSpacing());
    QObject::connect(vSpacingSpinBox, qOverload<int>(&QSpinBox::valueChanged), &owner, [popover](int v) {
      popover->setVerticalSpacing(v);
    });
    QObject::connect(popover, &Popover::verticalSpacingChanged, &owner, [vSpacingSpinBox, popover]() {
      vSpacingSpinBox->setValue(popover->verticalSpacing());
    });
  }

  void setupUI_navigationBar() {
    const auto dummyIcon = getTestQIcon();

    auto* navBar = new NavigationBar(windowContent);
    for (auto i = 0; i < 3; ++i) {
      navBar->addItem(QString("Item %1").arg(i), dummyIcon, QString("%1").arg((i + 1) * 10));
    }
    windowContentLayout->addWidget(navBar);

    auto* segmCtrl = new SegmentedControl(windowContent);
    for (auto i = 0; i < 3; ++i) {
      segmCtrl->addItem(QString("Item %1").arg(i), dummyIcon, QString("%1").arg((i + 1) * 10));
    }
    windowContentLayout->addWidget(segmCtrl);
  }

  void setupUI_switch() {
    const auto dummyIcon = getTestQIcon();
    auto* switchWidget = new Switch(windowContent);
    switchWidget->setText(QStringLiteral("Label of the Switch"));
    switchWidget->setIcon(dummyIcon);
    switchWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    windowContentLayout->addWidget(switchWidget);
  }

  static QPixmap getInputPixmap() {
    constexpr auto w = 100;
    QPixmap pixmap(w, w);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red);
    p.drawEllipse(QRect(0, 0, w, w));
    return pixmap;
  }

  static QPixmap getInputWithShadow(const QPixmap& input, double blurRadius) {
    const auto shadow = qlementine::getDropShadowPixmap(input, blurRadius);
    QPixmap result(shadow.size());
    result.fill(Qt::transparent);

    QPainter p(&result);
    p.drawPixmap((result.width() - shadow.width()) / 2, (result.height() - shadow.height()) / 2, shadow);
    p.drawPixmap((result.width() - input.width()) / 2, (result.height() - input.height()) / 2, input);

    return result;
  }

  void setupUI_blur() {
    //constexpr auto extendImage = true;
    constexpr auto initialBlur = 1;

    const auto inputPixmap = getInputPixmap();
    auto* labelBefore = new QLabel(windowContent);
    labelBefore->setPixmap(inputPixmap);
    labelBefore->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    windowContentLayout->addWidget(labelBefore, 0, Qt::AlignRight);

    const auto outputPixmap = getInputWithShadow(inputPixmap, initialBlur);
    auto* labelAfter = new QLabel(windowContent);
    labelAfter->setPixmap(outputPixmap);
    labelAfter->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    windowContentLayout->addWidget(labelAfter, 0, Qt::AlignLeft);

    auto* slider = new QSlider(windowContent);
    slider->setOrientation(Qt::Horizontal);
    slider->setRange(0, 10);
    slider->setValue(initialBlur);
    slider->setMinimumWidth(200);
    QObject::connect(slider, &QSlider::valueChanged, &owner, [inputPixmap, labelAfter](int value) {
      const auto outputPixmap = getInputWithShadow(inputPixmap, value);
      labelAfter->setPixmap(outputPixmap);
    });
    windowContentLayout->addWidget(slider, 0, Qt::AlignLeft);
  }

  void setupUI_focus() {
    auto* button1 = new QPushButton(QStringLiteral("Button 1"));
    button1->setObjectName(QStringLiteral("button1"));
    windowContentLayout->addWidget(button1);

    auto* button2 = new QPushButton(QStringLiteral("Button 2"));
    button2->setObjectName(QStringLiteral("button2"));
    windowContentLayout->addWidget(button2);
  }

  void setupUI_badge() {
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Info, StatusBadgeSize::Medium, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Error, StatusBadgeSize::Medium, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Success, StatusBadgeSize::Medium, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Warning, StatusBadgeSize::Medium, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Info, StatusBadgeSize::Small, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Error, StatusBadgeSize::Small, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Success, StatusBadgeSize::Small, windowContent));
    windowContentLayout->addWidget(new StatusBadgeWidget(StatusBadge::Warning, StatusBadgeSize::Small, windowContent));
  }

  void setupUI_specialProgressBar() {
    {
      auto* progressBar = new QProgressBar(windowContent);
      progressBar->setTextVisible(false);
      progressBar->setRange(0, 100);
      progressBar->setValue(30);
      progressBar->setInvertedAppearance(true);
      windowContentLayout->addWidget(progressBar);
    }
    {
      auto* progressBar = new QProgressBar(windowContent);
      progressBar->setTextVisible(false);
      progressBar->setRange(0, 0);
      windowContentLayout->addWidget(progressBar);
    }
  }

  void setupUI_lineEditStatus() {
    const auto dummyIcon = getTestQIcon();

    auto* lineEdit = new LineEdit(windowContent);
    lineEdit->setText(QStringLiteral("Label of the Switch"));
    lineEdit->setIcon(dummyIcon);
    lineEdit->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    lineEdit->setClearButtonEnabled(true);
    windowContentLayout->addWidget(lineEdit);

    auto* comboBoxStatus = new QComboBox(&owner);
    comboBoxStatus->addItem(QStringLiteral("Default"), static_cast<int>(Status::Default));
    comboBoxStatus->addItem(QStringLiteral("Info"), static_cast<int>(Status::Info));
    comboBoxStatus->addItem(QStringLiteral("Success"), static_cast<int>(Status::Success));
    comboBoxStatus->addItem(QStringLiteral("Warning"), static_cast<int>(Status::Warning));
    comboBoxStatus->addItem(QStringLiteral("Error"), static_cast<int>(Status::Error));
    windowContentLayout->addWidget(comboBoxStatus);
    QObject::connect(
      comboBoxStatus, qOverload<int>(&QComboBox::currentIndexChanged), &owner, [comboBoxStatus, lineEdit](int index) {
        const auto value = static_cast<Status>(comboBoxStatus->itemData(index).toInt());
        lineEdit->setStatus(value);
      });
  }

  void setupUI_colorButton() {
    auto* colorEditor = new ColorEditor(Qt::red, &owner);
    windowContentLayout->addWidget(colorEditor);
  }

  void setupUI_themeEditor() {
    auto* themeEditorDialog = new QWidget(&owner);
    themeEditorDialog->setWindowFlag(Qt::WindowType::Tool);
    auto* themeEditorDialogLayout = new QVBoxLayout(themeEditorDialog);
    themeEditorDialogLayout->setContentsMargins(0, 0, 0, 0);
    auto* themeEditorScrollView = new QScrollArea(themeEditorDialog);
    themeEditorDialogLayout->addWidget(themeEditorScrollView, 1);

    auto* themeEditor = new ThemeEditor(themeEditorScrollView);
    themeEditor->setTheme(qobject_cast<const QlementineStyle*>(owner.style())->theme());
    themeEditorScrollView->setWidget(themeEditor);

    auto* qlementineStyle = qobject_cast<QlementineStyle*>(owner.style());
    QObject::connect(themeEditor, &ThemeEditor::themeChanged, &owner, [qlementineStyle](const Theme& theme) {
      qlementineStyle->setTheme(theme);
    });
    QObject::connect(qlementineStyle, &QlementineStyle::themeChanged, &owner, [qlementineStyle, themeEditor]() {
      themeEditor->setTheme(qlementineStyle->theme());
    });

    themeEditorDialog->installEventFilter(&owner);
    auto* closeShortcut = new QShortcut(Qt::Key_Escape, themeEditorDialog);
    QObject::connect(closeShortcut, &QShortcut::activated, &owner, [themeEditorDialog]() {
      themeEditorDialog->close();
    });

    themeEditorDialog->resize(themeEditorDialog->sizeHint().width(), 600);
    themeEditorDialog->move(themeEditorDialog->x() + 300, themeEditorDialog->y() + 300);
    themeEditorDialog->show();
  }

  void setupUI_dateTimeEdit() {
    auto* dateTimeEdit = new QDateTimeEdit(windowContent);
    dateTimeEdit->setMinimumDate(QDate::currentDate().addDays(-365));
    dateTimeEdit->setMaximumDate(QDate::currentDate().addDays(365));
    dateTimeEdit->setDisplayFormat(QStringLiteral("yyyy.MM.dd"));
    dateTimeEdit->setCalendarPopup(true);

    windowContentLayout->addWidget(dateTimeEdit);
  }

  static int getRandomInt(int min, int max) {
    static std::random_device randomDevice;
    static std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<> distribution(min, max);
    return distribution(generator);
  }

  void setupUI_contextMenu() {
    auto* plainWidget = new CustomBgWidget(windowContent);
    plainWidget->customSizeHint = QSize(200, 200);
    plainWidget->bgColor = QColor(230, 230, 230);
    plainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* contextMenuEvtFilter = new ContextMenuEventFilter(plainWidget, [plainWidget](QContextMenuEvent* e) {
      // Create menu.
      QMenu menu;

      const auto cb = []() {
        qDebug() << "Clicked";
      };

      const auto& clickPos = e->pos();
      const auto clickPosStr = QString("(%1, %2)").arg(clickPos.x()).arg(clickPos.y());

      menu.addAction(QString("Pos: %1").arg(clickPosStr), Qt::CTRL | Qt::Key_A, &menu, cb);

      const auto randomCount = getRandomInt(1, 10);
      for (auto i = 0; i < randomCount; ++i) {
        const auto textLength = i * 4;
        QStringList textList;
        for (auto n = 0; n < textLength; ++n) {
          textList.append("A");
        }

        menu.addAction(textList.join("") + QString(" %1").arg(i), Qt::ALT | Qt::SHIFT | Qt::Key_0 + i, &menu, cb);
      }

      // Show menu.
      menu.exec(plainWidget->mapToGlobal(clickPos));

      // Mark as handled.
      e->setAccepted(true);
      return true;
    });

    plainWidget->installEventFilter(contextMenuEvtFilter);

    windowContentLayout->addWidget(plainWidget);
  }
};

SandboxWindow::SandboxWindow(ThemeManager* themeManager, QWidget* parent)
  : QMainWindow(parent)
  , _impl(new Impl(*this, themeManager)) {
  setWindowIcon(QIcon(QStringLiteral(":/sandbox/qlementine_icon.ico")));

  _impl->beginSetupUI();
  {
    // Uncomment the line to show the corresponding widget.
    // _impl->setupUI_label();
    // _impl->setupUI_button();
    // _impl->setupUI_buttonVariants();
    // _impl->setupUI_checkbox();
    // _impl->setupUI_radioButton();
    // _impl->setupUI_commandLinkButton();
    // _impl->setupUI_sliderAndProgressBar();
    // _impl->setupUI_sliderWithTicks();
    // _impl->setupUI_lineEdit();
    // _impl->setupUI_textEdit();
    // _impl->setupUI_plainTextEdit();
    // _impl->setupUI_dial();
    // _impl->setupUI_spinBox();
    // _impl->setupUI_comboBox();
    // _impl->setupUI_comboBoxVariants();
    // _impl->setupUI_comboBoxWithTreeView();
    // _impl->setupUI_fontComboBox();
    // _impl->setupUI_listView();
    // _impl->setupUI_treeWidget();
    // _impl->setupUI_table();
    // _impl->setupUI_menuBar();
    // _impl->setupUI_toolButton();
    // _impl->setupUI_toolButtonsVariants();
    // _impl->setupUI_tabBar();
    // _impl->setupUI_tabWidget();
    // _impl->setupUI_groupBox();
    // _impl->setupUI_treeView();
    // _impl->setupUI_focus();
    // _impl->setupUI_specialProgressBar();
    // _impl->setupUI_lineEditStatus();
    // _impl->setupUI_dateTimeEdit();
    // _impl->setupUI_contextMenu();

    // _impl->setupUI_switch();
    // _impl->setupUI_expander();
    // _impl->setupUI_popover();
    // _impl->setupUI_navigationBar();
    // _impl->setupUI_badge();
    // _impl->setupUI_colorButton();
    // _impl->setupUI_messageBoxIcons();
    // _impl->setupUI_loadingSpinner();
    // _impl->setupUI_aboutDialog();
    // _impl->setupUI_notificationBadge();

    // _impl->setupUI_fontMetricsTests();
    // _impl->setupUI_blur();
    // _impl->setupUI_themeEditor();
    // _impl->setupUI_messageBox();
    // _impl->setupUI_toolBar();
  }
  _impl->endSetupUI();
  oclero::qlementine::centerWidget(this);
}

SandboxWindow::~SandboxWindow() = default;

bool SandboxWindow::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::Type::Close) {
    qApp->closeAllWindows();
  }
  return QMainWindow::eventFilter(watched, event);
}
} // namespace oclero::qlementine::sandbox
