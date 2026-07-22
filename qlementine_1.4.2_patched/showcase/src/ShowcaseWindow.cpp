// SPDX-FileCopyrightText: Olivier Cléro <oclero@hotmail.com>
// SPDX-License-Identifier: MIT

#include "ShowcaseWindow.hpp"

#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/ThemeManager.hpp>

#include <oclero/qlementine/utils/IconUtils.hpp>
#include <oclero/qlementine/utils/WidgetUtils.hpp>
#include <oclero/qlementine/utils/LayoutUtils.hpp>

#include <oclero/qlementine/widgets/LineEdit.hpp>
#include <oclero/qlementine/widgets/NavigationBar.hpp>
#include <oclero/qlementine/widgets/SegmentedControl.hpp>
#include <oclero/qlementine/widgets/Switch.hpp>
#include <oclero/qlementine/widgets/StatusBadgeWidget.hpp>
#include <oclero/qlementine/widgets/ColorButton.hpp>
#include <oclero/qlementine/widgets/IconWidget.hpp>
#include <oclero/qlementine/widgets/AboutDialog.hpp>

#include <oclero/qlementine/icons/Icons12.hpp>
#include <oclero/qlementine/icons/Icons16.hpp>
#include <oclero/qlementine/icons/Icons32.hpp>

#include <QPointer>
#include <QBoxLayout>
#include <QScrollArea>
#include <QToolBar>
#include <QDockWidget>
#include <QTreeView>
#include <QCheckBox>
#include <QMenuBar>
#include <QToolButton>
#include <QTabBar>
#include <QSplitter>
#include <QPainter>
#include <QStatusBar>
#include <QPushButton>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QSlider>
#include <QDial>
#include <QDateTimeEdit>
#include <QGroupBox>
#include <QButtonGroup>
#include <QProgressBar>
#include <QActionGroup>
#include <QApplication>
#include <QPlainTextEdit>

#include <random>

namespace oclero::qlementine::showcase {
using Icons16 = oclero::qlementine::icons::Icons16;

static QIcon makeThemedIcon(Icons16 id, const QSize& size = { 16, 16 }) {
  const auto svgPath = oclero::qlementine::icons::iconPath(id);
  if (auto* style = oclero::qlementine::appStyle()) {
    return style->makeThemedIcon(svgPath, size);
  } else {
    return QIcon(svgPath);
  }
}

class DummyWorkspace : public QWidget {
public:
  using QWidget::QWidget;

protected:
  void paintEvent(QPaintEvent* evt) override {
    QPainter p(this);

    QColor backgroundColor;
    if (const auto* qlementine_style = qlementine::appStyle()) {
      const auto theme = qlementine_style->theme();
      backgroundColor = theme.backgroundColorWorkspace;
    }
    p.fillRect(rect(), backgroundColor);
  }
};

static QString getDummyText(const unsigned int minWords = 3, const unsigned int maxWords = 4) {
  static const auto loremIpsumWords = std::array<const QString, 69>{ "Lorem", "Ipsum", "Dolor", "Sit", "Amet",
    "Consectetur", "Adipiscing", "Elit", "Sed", "Do", "Eiusmod", "Tempor", "Incididunt", "Ut", "Labore", "Et", "Dolore",
    "Magna", "Aliqua", "Ut", "Enim", "Ad", "Minim", "Veniam", "Quis", "Nostrud", "Exercitation", "Ullamco", "Laboris",
    "Nisi", "Ut", "Aliquip", "Ex", "Ea", "Commodo", "Consequat", "Duis", "Aute", "Irure", "Dolor", "In",
    "Reprehenderit", "In", "Voluptate", "Velit", "Esse", "Cillum", "Dolore", "Eu", "Fugiat", "Nulla", "Pariatur",
    "Excepteur", "Sint", "Occaecat", "Cupidatat", "Non", "Proident", "Sunt", "In", "Culpa", "Qui", "Officia",
    "Deserunt", "Mollit", "Anim", "Id", "Est", "Laborum" };

  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto randomCountDistrib = std::uniform_int_distribution<>(minWords, maxWords);
  const auto random_word_count = randomCountDistrib(gen);

  auto randomIndexDistrib = std::uniform_int_distribution<>(0, loremIpsumWords.size() - 1 - random_word_count);
  const auto randomWordIndex = randomIndexDistrib(gen);

  auto result = loremIpsumWords.at(randomWordIndex);
  for (auto i = 0; i < random_word_count - 1; ++i) {
    result += ' ' + loremIpsumWords.at(randomWordIndex + 1 + i);
  }
  return result;
}

static QIcon getDummyColoredIcon() {
  static const auto icons = std::array<QIcon, 3>{
    QIcon(":/showcase/icons/cube-green.svg"),
    QIcon(":/showcase/icons/cube-red.svg"),
    QIcon(":/showcase/icons/cube-yellow.svg"),
  };

  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto randomDistrib = std::uniform_int_distribution<>(0, icons.size() - 1);
  const auto randomIndex = randomDistrib(gen);

  return icons.at(randomIndex);
}

static QIcon getDummyMonochromeIcon(const QSize& size = { 16, 16 }) {
  auto rd = std::random_device();
  auto gen = std::mt19937(rd());

  auto randomDistrib =
    std::uniform_int_distribution<std::underlying_type_t<Icons16>>(1, 410 - 1); // TODO use a constexpr variable.
  const auto randomIndex = randomDistrib(gen);
  const auto randomIcon = static_cast<Icons16>(randomIndex);
  return makeThemedIcon(randomIcon, size);
}

struct ShowcaseWindow::Impl {
  ShowcaseWindow& owner;
  QPointer<QlementineStyle> qlementineStyle;
  QPointer<ThemeManager> themeManager;
  QVBoxLayout* rootLayout{ nullptr };
  QMenuBar* menuBar{ nullptr };
  QTabBar* tabBar{ nullptr };
  QToolBar* toolBar{ nullptr };
  QSplitter* splitter{ nullptr };
  QWidget* leftPanel{ nullptr };
  QWidget* rightPanel{ nullptr };
  QWidget* workspace{ nullptr };
  QStatusBar* statusBar{ nullptr };
  oclero::qlementine::Switch* themeSwitch{ nullptr };

  Impl(ShowcaseWindow& o, ThemeManager* themeManager)
    : owner(o)
    , themeManager(themeManager) {}

  void setupUI() {
    setupMenuBar();
    setupTabBar();
    setupToolBar();
    setupLeftPanel();
    setupRightPanel();
    setupWorkspace();
    setupSplitter();
    setupStatusBar();
    setupLayout();
  }

  void setTheme(const QString& theme) {
    if (themeManager) {
      themeManager->setCurrentTheme(theme);
    }
  }

  void switchTheme() {
    if (themeManager) {
      themeManager->setNextTheme();
    }
  }

  void updateThemeSwitch() {
    themeSwitch->blockSignals(true);
    themeSwitch->setChecked(themeManager ? themeManager->currentTheme() == "Dark" : false);
    themeSwitch->blockSignals(false);
  }

  void setupMenuBar() {
    menuBar = new QMenuBar(nullptr);
    const auto cb = []() {
      // Just for the example.
    };

    {
      auto* menu = menuBar->addMenu("File");
      {
        // TODO: Use the enum provided by Qt6 instead of strings for icon IDs.
        menu->addAction(makeThemedIcon(Icons16::Document_New), "New", QKeySequence::StandardKey::New, cb);
        menu->addAction(makeThemedIcon(Icons16::Document_Open), "Open...", QKeySequence::StandardKey::Open, cb);

        auto* recentFilesMenu = menu->addMenu(makeThemedIcon(Icons16::Document_OpenRecent), "Recent Files");
        for (auto i = 0; i < 5; ++i) {
          recentFilesMenu->addAction(
            makeThemedIcon(Icons16::File_File), QString("Recent File %1").arg(i + 1), QKeySequence{}, cb);
        }

        menu->addSeparator();
        menu->addAction(makeThemedIcon(Icons16::Action_Save), "Save", QKeySequence::StandardKey::Save, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_Close), "Close", QKeySequence::StandardKey::Close, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_Print), "Print...", QKeySequence::StandardKey::Print, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_PrintPreview), "Print Preview...", QKeySequence{}, cb);

        menu->addSeparator();
        menu->addAction(
          makeThemedIcon(Icons16::Navigation_Settings), "Preferences...", QKeySequence::StandardKey::Preferences, cb);

        menu->addSeparator();
#ifdef Q_OS_WIN
        // QKeySequence::Quit is empty on Windows.
        const auto quitShortcut = QKeySequence(Qt::CTRL | Qt::Key_Q);
#else
        const auto quitShortcut = QKeySequence(QKeySequence::Quit);
#endif
        menu->addAction(makeThemedIcon(Icons16::Action_Close), "Quit", quitShortcut, []() {
          qApp->quit();
        });
      }
    }
    {
      auto* menu = menuBar->addMenu("Edit");
      {
        menu->addAction(makeThemedIcon(Icons16::Action_Undo), "Undo", QKeySequence::StandardKey::Undo, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_Redo), "Redo", QKeySequence::StandardKey::Redo, cb);

        menu->addSeparator();
        menu->addAction(makeThemedIcon(Icons16::Action_Cut), "Cut", QKeySequence::StandardKey::Cut, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_Copy), "Copy", QKeySequence::StandardKey::Copy, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_Paste), "Paste", QKeySequence::StandardKey::Paste, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_Trash), "Delete", QKeySequence::StandardKey::Delete, cb);
      }
    }
    {
      auto* menu = menuBar->addMenu("View");
      {
        menu->addAction(makeThemedIcon(Icons16::Action_ZoomIn), "Zoom In", QKeySequence::StandardKey::ZoomIn, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_ZoomOut), "Zoom Out", QKeySequence::StandardKey::ZoomOut, cb);
        menu->addAction(makeThemedIcon(Icons16::Action_ZoomFit), "Fit", QKeySequence{}, cb);

        menu->addSeparator();
        menu->addAction(
          makeThemedIcon(Icons16::Action_Fullscreen), "Full Screen", QKeySequence::StandardKey::FullScreen, cb);

        if (themeManager) {
          auto* themeMenu = menu->addMenu("Theme");
          themeMenu->setIcon(makeThemedIcon(Icons16::Misc_PaintPalette));

          auto* themeActionGroup = new QActionGroup(themeMenu);
          themeActionGroup->setExclusive(true);

          const auto& themes = themeManager->themes();
          const auto currentTheme = themeManager->currentTheme();

          for (const auto& theme : themes) {
            const auto name = theme.meta.name;
            const auto icon = name == "Dark" ? makeThemedIcon(Icons16::Misc_Moon) : makeThemedIcon(Icons16::Misc_Sun);
            auto* action = themeMenu->addAction(icon, name);
            action->setCheckable(true);
            themeActionGroup->addAction(action);
            action->setChecked(name == currentTheme);

            QObject::connect(action, &QAction::triggered, action, [this, name](auto checked) {
              if (checked) {
                setTheme(name);
              }
            });
            QObject::connect(
              themeManager, &oclero::qlementine::ThemeManager::currentThemeChanged, action, [this, name, action]() {
                action->setChecked(name == themeManager->currentTheme());
              });
          }

          themeMenu->addSeparator();
          themeMenu->addAction(
            makeThemedIcon(Icons16::Action_Swap), "Switch Theme", { Qt::CTRL | Qt::Key_T }, [this]() {
              switchTheme();
            });
        }
      }
    }
    {
      auto* menu = menuBar->addMenu("Help");
      {
        menu->addAction(makeThemedIcon(Icons16::Misc_Mail), "Contact", QKeySequence{}, cb);
        menu->addAction(makeThemedIcon(Icons16::Misc_Info), "About...", QKeySequence{}, [this]() {
          auto* dialog = new oclero::qlementine::AboutDialog(&owner);
          dialog->setWindowTitle(QString("About %1").arg(QApplication::applicationDisplayName()));
          dialog->setDescription("An application to showcase Qlementine's capabilities as a QStyle library.");
          dialog->setWebsiteUrl("https://oclero.github.io/qlementine");
          dialog->setLicense("Licensed under MIT license.");
          dialog->setCopyright("© Olivier Cléro");
          dialog->addSocialMediaLink(
            "GitHub", "https://github.com/oclero/qlementine", makeThemedIcon(Icons16::Brand_GithubFill));
          dialog->addSocialMediaLink(
            "Mastodon", "https://mastodon.online/@oclero", makeThemedIcon(Icons16::Brand_MastodonFill));
          dialog->addSocialMediaLink("GitLab", "https://gitlab.com/oclero", makeThemedIcon(Icons16::Brand_GitlabFill));

          dialog->show();
        });
      }
    }
  }

  void setupTabBar() {
    tabBar = new QTabBar(&owner);
    tabBar->setDocumentMode(true);
    tabBar->setFocusPolicy(Qt::NoFocus);
    tabBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    tabBar->setTabsClosable(true);
    tabBar->setMovable(false);
    tabBar->setExpanding(false);
    tabBar->setChangeCurrentOnDrag(true);
    tabBar->setUsesScrollButtons(true);

    qlementineStyle->setAutoIconColor(tabBar, oclero::qlementine::AutoIconColor::ForegroundColor);

    for (auto i = 0; i < 4; ++i) {
      tabBar->addTab(makeThemedIcon(Icons16::File_File), getDummyText());
    }

    QObject::connect(tabBar, &QTabBar::tabCloseRequested, tabBar, [this](int index) {
      tabBar->removeTab(index);
    });
  }

  void setupToolBar() {
    const auto defaultIconSize = owner.style()->pixelMetric(QStyle::PM_SmallIconSize);

    toolBar = new QToolBar("App ToolBar", &owner);
    toolBar->setBackgroundRole(QPalette::ColorRole::Window);
    toolBar->setAutoFillBackground(false);
    toolBar->setAllowedAreas(Qt::ToolBarArea::TopToolBarArea);
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonFollowStyle);
    toolBar->setIconSize(QSize(defaultIconSize, defaultIconSize));

    const auto addButton = [this](const Icons16 icon, const QString& tooltip, const QString& text = {}) {
      auto* toolButton = new QToolButton(toolBar);
      toolButton->setFocusPolicy(Qt::NoFocus);
      toolButton->setIcon(makeThemedIcon(icon));
      toolButton->setToolTip(tooltip);
      if (!text.isEmpty()) {
        toolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolButton->setText(text);
      }
      toolBar->addWidget(toolButton);
      return toolButton;
    };

    addButton(Icons16::Action_Save, "Save");
    addButton(Icons16::Action_Print, "Print");
    toolBar->addSeparator();
    addButton(Icons16::Action_Undo, "Undo");
    addButton(Icons16::Action_Redo, "Redo");

    auto* resetButton = addButton(Icons16::Action_Reset, "Reset");
    {
      auto* menu = new QMenu(resetButton);
      for (auto i = 0; i < 10; ++i) {
        menu->addAction(new QAction(getDummyMonochromeIcon(), getDummyText(2, 3), menu));
      }
      resetButton->setMenu(menu);
      resetButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
    }

    toolBar->addSeparator();
    addButton(Icons16::Action_Copy, "Copy");
    addButton(Icons16::Action_Paste, "Paste");
    addButton(Icons16::Action_Cut, "Cut");
    toolBar->addSeparator();
    addButton(Icons16::Media_SkipBackward, "Skip Backward");
    addButton(Icons16::Media_Play, "Play");
    addButton(Icons16::Media_SkipForward, "Skip Forward");
    toolBar->addSeparator();
    auto* exportButton = addButton(Icons16::Action_Export, "Export", "Export");
    {
      auto* menu = new QMenu(exportButton);
      menu->addAction(new QAction(makeThemedIcon(Icons16::File_Movie), "Movie", menu));
      menu->addAction(new QAction(makeThemedIcon(Icons16::File_Picture), "Picture", menu));
      menu->addSeparator();
      menu->addAction(new QAction(makeThemedIcon(Icons16::File_Archive), "Archive", menu));

      exportButton->setMenu(menu);
      exportButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
    }

    // Spacer.
    auto* spacer_widget = new QWidget(toolBar);
    spacer_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    spacer_widget->setMinimumSize(0, 0);
    spacer_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Ignored);
    spacer_widget->setUpdatesEnabled(false); // No paint events.
    toolBar->addWidget(spacer_widget);

    // Theme switch.
    auto* themeWidget = new QWidget(toolBar);
    {
      const auto hSpacing = getLayoutHSpacing(themeWidget) / 2;
      themeWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      auto* themeLayout = new QHBoxLayout(themeWidget);
      themeLayout->setSpacing(hSpacing);
      themeLayout->setContentsMargins(0, 0, 0, 0);
      themeWidget->setLayout(themeLayout);

      auto* lightIconWidget = new oclero::qlementine::IconWidget(makeThemedIcon(Icons16::Misc_Sun), themeWidget);
      themeLayout->addWidget(lightIconWidget);

      themeSwitch = new oclero::qlementine::Switch(toolBar);
      themeSwitch->setToolTip("Switch between light and dark theme");
      QObject::connect(themeSwitch, &oclero::qlementine::Switch::clicked, themeSwitch, [this](auto checked) {
        setTheme(checked ? "Dark" : "Light");
      });
      QObject::connect(themeManager, &oclero::qlementine::ThemeManager::currentThemeChanged, themeSwitch, [this]() {
        updateThemeSwitch();
      });
      themeLayout->addWidget(themeSwitch);

      auto* darkIconWidget = new oclero::qlementine::IconWidget(makeThemedIcon(Icons16::Misc_Moon), themeWidget);
      themeLayout->addWidget(darkIconWidget);

      updateThemeSwitch();
    }
    toolBar->addWidget(themeWidget);
  }

  void setupLeftPanel() {
    auto* widget = new QWidget(&owner);
    leftPanel = widget;
    leftPanel->setMinimumWidth(200);
    leftPanel->setMaximumWidth(400);

    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins({ 0, 0, 0, 0 });
    layout->setSpacing(0);

    {
      auto* topBar = new QWidget(widget);
      layout->addWidget(topBar);
      auto* topBarLayout = new QHBoxLayout(topBar);
      topBarLayout->setContentsMargins({ 12, 8, 12, 8 });

      auto* lineEdit = new LineEdit(widget);
      lineEdit->setIcon(makeThemedIcon(Icons16::Navigation_Search));
      lineEdit->setClearButtonEnabled(true);
      lineEdit->setPlaceholderText("Search...");
      topBarLayout->addWidget(lineEdit, 1);

      auto* button = new QPushButton(makeThemedIcon(Icons16::Action_Filter), "", widget);
      button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      topBarLayout->addWidget(button);
    }

    auto* navBar = new NavigationBar(widget);
    {
      layout->addWidget(navBar);
      navBar->setItemsShouldExpand(true);
      navBar->addItem("Objects", QIcon(), QString("%1").arg(12));
      navBar->addItem("Materials", QIcon(), QString("%1").arg(3));
    }

    layout->addWidget(makeHorizontalLine(widget));

    auto* stackedWidget = new QStackedWidget(widget);
    {
      stackedWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

      {
        auto* treeWidget = new QTreeWidget(widget);
        stackedWidget->addWidget(treeWidget);

        qlementineStyle->setAutoIconColor(treeWidget, oclero::qlementine::AutoIconColor::None);

        treeWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        treeWidget->setAlternatingRowColors(false);
        treeWidget->setColumnCount(1);
        treeWidget->setHeaderHidden(true);
        treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

        for (auto i = 0; i < 24; ++i) {
          auto* root = new QTreeWidgetItem(treeWidget);
          root->setText(0, getDummyText());
          root->setIcon(0, getDummyColoredIcon());

          for (auto j = 0; j < 4; ++j) {
            auto* child = new QTreeWidgetItem(root);
            child->setText(0, getDummyText());
            child->setIcon(0, getDummyColoredIcon());

            for (auto k = 0; k < 3; ++k) {
              auto* subChild = new QTreeWidgetItem(child);
              subChild->setText(0, getDummyText());
              subChild->setIcon(0, getDummyColoredIcon());
            }
          }
        }

        treeWidget->topLevelItem(0)->setSelected(true);
        navBar->setItemBadge(0, QString::number(treeWidget->topLevelItemCount()));
      }

      {
        class CustomDelegate : public QStyledItemDelegate {
          using QStyledItemDelegate::QStyledItemDelegate;

          QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
            const auto result = QStyledItemDelegate::sizeHint(option, index);
            return { 0, result.height() };
          }
        };


        auto* listWidget = new QListWidget(widget);
        stackedWidget->addWidget(listWidget);

        listWidget->setItemDelegate(new CustomDelegate(listWidget));
        listWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
        listWidget->setSizeAdjustPolicy(QListView::SizeAdjustPolicy::AdjustIgnored);
        listWidget->setAlternatingRowColors(true);
        listWidget->setIconSize(QSize(32, 32));
        qlementineStyle->setAutoIconColor(listWidget, AutoIconColor::None);

        for (auto i = 0; i < 15; ++i) {
          const auto itemText = QString("Item #%1 with very long text that can be elided").arg(i);
          auto* item = new QListWidgetItem(getDummyColoredIcon(), itemText, listWidget);
          item->setFlags(item->flags() | Qt::ItemFlag::ItemIsUserCheckable);
          item->setCheckState(i % 3 == 0 ? Qt ::CheckState::Checked : Qt::CheckState::Unchecked);
          listWidget->addItem(item);
        }
        listWidget->item(0)->setSelected(true);
        navBar->setItemBadge(1, QString::number(listWidget->count()));
      }

      layout->addWidget(stackedWidget, 1);
    }

    {
      stackedWidget->setCurrentIndex(navBar->currentIndex());
      QObject::connect(navBar, &NavigationBar::currentIndexChanged, stackedWidget, [stackedWidget, navBar]() {
        stackedWidget->setCurrentIndex(navBar->currentIndex());
      });
    }
  }

  void setupRightPanel() {
    auto* widget = new QWidget(&owner);
    rightPanel = widget;
    rightPanel->setMinimumWidth(200);
    rightPanel->setMaximumWidth(400);

    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins({ 0, 0, 0, 0 });
    layout->setSpacing(0);

    {
      auto* topBar = new QWidget(widget);
      layout->addWidget(topBar);
      auto* topBarLayout = new QHBoxLayout(topBar);
      topBarLayout->setContentsMargins({ 12, 8, 12, 8 });

      {
        auto* segmentedControl = new SegmentedControl(topBar);
        topBarLayout->addWidget(segmentedControl);
        segmentedControl->setItemsShouldExpand(false);
        segmentedControl->addItem(
          "Properties", makeThemedIcon(Icons16::Navigation_SlidersVertical), QString("%1").arg(4));
        segmentedControl->addItem("Scene", makeThemedIcon(Icons16::Misc_Globe), QString("%1").arg(2));
      }
    }

    layout->addWidget(makeHorizontalLine(widget));

    {
      auto* scrollArea = new QScrollArea(widget);
      layout->addWidget(scrollArea);

      auto* content = new QWidget(scrollArea);
      content->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
      scrollArea->setWidget(content);
      scrollArea->setWidgetResizable(true);

      const auto margins = getLayoutMargins(content);
      const auto vMargin = static_cast<int>(margins.top() * .75);
      auto* contentLayout = new QFormLayout(content);
      contentLayout->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy::ExpandingFieldsGrow);
      contentLayout->setContentsMargins({ margins.left(), vMargin, margins.right(), vMargin });
      {
        {
          auto* groupBox = new QGroupBox(content);
          groupBox->setAlignment(Qt::AlignRight);
          groupBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
          groupBox->setTitle(getDummyText());
          groupBox->setCheckable(true);
          groupBox->setFlat(false);
          auto* groupBoxLayout = new QFormLayout(groupBox);
          groupBoxLayout->setFieldGrowthPolicy(QFormLayout::FieldGrowthPolicy::ExpandingFieldsGrow);

          {
            auto* switchWidget = new qlementine::Switch(groupBox);
            switchWidget->setChecked(true);
            groupBoxLayout->addRow(getDummyText(2, 2) + ":", switchWidget);
          }
          {
            auto* tristateSwitchWidget = new qlementine::Switch(groupBox);
            tristateSwitchWidget->setTristate(true);
            tristateSwitchWidget->setCheckState(Qt::PartiallyChecked);
            groupBoxLayout->addRow(getDummyText(2, 2) + ":", tristateSwitchWidget);
          }
          {
            auto* spinBox = new QSpinBox(groupBox);
            spinBox->setRange(0, 1000);
            spinBox->setSuffix("cm");
            groupBoxLayout->addRow(getDummyText(2, 2) + ":", spinBox);
          }
          {
            auto* comboBox = new QComboBox(groupBox);
            for (auto i = 0; i < 5; ++i) {
              comboBox->addItem(getDummyMonochromeIcon(), getDummyText(1, 1));
            }
            comboBox->setCurrentIndex(0);
            groupBoxLayout->addRow(getDummyText(1, 1) + ":", comboBox);
          }
          {
            auto* comboBox = new QComboBox(groupBox);
            comboBox->setEditable(true);
            comboBox->setPlaceholderText("Placeholder");
            comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            for (auto i = 0; i < 5; ++i) {
              comboBox->addItem(getDummyMonochromeIcon(), getDummyText(1, 1));
            }
            comboBox->setCurrentIndex(0);
            groupBoxLayout->addRow(getDummyText(1, 1) + ":", comboBox);
          }
          {
            auto* dateTimeEdit = new QDateTimeEdit(groupBox);
            //dateTimeEdit->setSizePolicy(QSizepol)
            dateTimeEdit->setCalendarPopup(true);
            groupBoxLayout->addRow(getDummyText(1, 1) + ":", dateTimeEdit);
          }
          {
            auto* lineEdit = new QLineEdit(groupBox);
            lineEdit->setPlaceholderText("Enter text...");
            lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            lineEdit->setClearButtonEnabled(true);
            groupBoxLayout->addRow(getDummyText(1, 1) + ":", lineEdit);
          }
          contentLayout->addRow(groupBox);
        }
        {
          auto* groupBox = new QGroupBox(content);
          groupBox->setAlignment(Qt::AlignRight);
          groupBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
          groupBox->setTitle(getDummyText());
          groupBox->setCheckable(true);
          groupBox->setFlat(false);
          auto* groupBoxLayout = new QVBoxLayout(groupBox);
          {
            auto* radioGroup = new QButtonGroup(groupBox);
            for (auto i = 0; i < 3; ++i) {
              auto* radioButton = new QRadioButton(getDummyText(), groupBox);
              radioButton->setChecked(i == 0);
              radioButton->setIcon(getDummyMonochromeIcon());
              radioButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
              radioGroup->addButton(radioButton);
              groupBoxLayout->addWidget(radioButton);
            }
          }
          contentLayout->addRow(groupBox);
        }
        {
          auto* groupBox = new QGroupBox(content);
          groupBox->setAlignment(Qt::AlignRight);
          groupBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
          groupBox->setTitle(getDummyText());
          groupBox->setCheckable(true);
          groupBox->setFlat(false);
          auto* groupBoxLayout = new QFormLayout(groupBox);
          {
            auto* slider = new QSlider(groupBox);
            slider->setRange(0, 100);
            slider->setValue(30);
            groupBoxLayout->addRow(getDummyText(1, 1) + ":", slider);
          }
          {
            auto* slider = new QSlider(groupBox);
            slider->setOrientation(Qt::Orientation::Horizontal);
            slider->setRange(0, 10);
            slider->setPageStep(1);
            slider->setSingleStep(1);
            slider->setValue(7);
            slider->setTickPosition(QSlider::TickPosition::TicksAbove);
            slider->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

            groupBoxLayout->addRow(getDummyText(1, 1) + ":", slider);
          }
          contentLayout->addRow(groupBox);
        }
        {
          auto* plainTextEdit = new QPlainTextEdit(content);
          plainTextEdit->setFrameShape(QFrame::StyledPanel);
          plainTextEdit->setFrameShadow(QFrame::Shadow::Raised);
          contentLayout->addRow(plainTextEdit);
        }
      }
    }
  }

  void setupWorkspace() {
    workspace = new DummyWorkspace(&owner);
    workspace->setFocusPolicy(Qt::StrongFocus);
    workspace->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  }

  void setupSplitter() {
    splitter = new QSplitter(&owner);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    splitter->setOrientation(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(workspace);
    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 6);
    splitter->setStretchFactor(2, 2);
  }

  void setupStatusBar() {
    statusBar = new QStatusBar(&owner);
    statusBar->setSizeGripEnabled(false);

    const auto margins = getLayoutMargins(statusBar);
    statusBar->setContentsMargins(margins.left(), 0, margins.right(), 0);
    {
      auto* progressBar = new QProgressBar(statusBar);
      progressBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      progressBar->setTextVisible(false);
      progressBar->setRange(0, 0);
      statusBar->addPermanentWidget(progressBar);
    }
  }

  void setupLayout() {
    rootLayout = new QVBoxLayout(&owner);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setMenuBar(menuBar);
    rootLayout->addWidget(tabBar);
    rootLayout->addWidget(toolBar);
    rootLayout->addWidget(splitter);
    rootLayout->addWidget(statusBar);
    workspace->setFocus(Qt::NoFocusReason);
  }
};

ShowcaseWindow::ShowcaseWindow(ThemeManager* themeManager, QWidget* parent)
  : QWidget(parent)
  , _impl(new Impl(*this, themeManager)) {
  setWindowIcon(QIcon(":/showcase/qlementine_icon.ico"));
  _impl->setupUI();
  setMinimumSize(600, 400);
  resize(800, 600);
  oclero::qlementine::centerWidget(this);

  this->ensurePolished();
  _impl->qlementineStyle = qobject_cast<QlementineStyle*>(this->style());
}

ShowcaseWindow::~ShowcaseWindow() = default;
} // namespace oclero::qlementine::showcase
