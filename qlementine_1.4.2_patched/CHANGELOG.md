# Changelog

## v1.4.2

WIP

## v1.4.1

Bugfixes:

- Fix `QCommandLinkbutton` icon size.
- Fix typo in docs (#124).
- Handle correctly right-to-left in `LineEditButtonEventFilter`.

## v1.4.0

New features:

- Add tristate mode to `Switch` (#93).

Bugfixes:

- Fix antialiased carret in text fields. It is now always aligned to pixel grid (by @anydream).
- Fix memory leak for the `FlashActionHelper` (by @thierryba).
- Fix `QTabBar` scroll buttons enabled state (by @christophe-thiery-fw).

## v1.3.0

New features:

- Add `NotificationBadge` widget (#109).
- Switch to Ninja as the default CMake generator on macOS instead of Xcode.
- Replace `emit` by `Q_EMIT`for better compatibility.

Bugfixes:

- Fix `Popover`'s drop shadow and position pixel ratios on high-DPI screens.
- Fix returned `QAction` by `QMenu::exec` (by @thierryba).
- Fix missing border positions for `QToolBar` (by Иванов Павел Геннадьевич).
- Fix loss `Popover` binding on multiscreen (by @polamedev).
- Fix loop animation restart when it is stopped (by @polamedev).

## v1.2.2

- Fix installing includes with double-slashes

## v1.2.1

- Fix the drop shadows: they now use correctly the device pixel ratio.
- Delay even more the creation of `QFocusFrame`.
- Add methods to customize `AboutDialog`.

## v1.2.0

The lib is now used by two more apps: Solarus Launcher and Solarus Editor!

- Fix tab bar scroll buttons enabled state.
- Improve Popover widget.
- Fix Font not found "InterDisplay" (#96).
- Fix Warning about negative minimum sizes (0,-1) (#98).
- Add missing widgets in docs (#95).
- Fix QLineEdit position in editable QComboBox (#63).
- Make clang-analyzer happy.
- Fix LoadingSpinner when not visible.
- QSplitter: don't draw too large lines in splitters
- Add support for QLineEdit without borders.
- Add new Qt6 PixelMetric sizes.
- Fix flat QPushButton background color.
- Improve QTableView/QTreeView headers aesthetics.
- Fix QToolButton icon size: it now uses the set size.
- Fix forwarding the AutoIconColor property to QMenu's child menus.
- Customize QPlainTextEdit's standard context menu icons.
- Fix compressed QCheckBox in QFormLayout.
- Fix huge fonts on Windows.

## v1.1.2

- Fix compilation on Windows.

## v1.1.1

- Fix click on website link in `AboutDialog`.

## v1.1.0

- Add `LoadingSpinner` widget (#27).
- Add `AboutDialog` widget (#90).
- Showcase app uses Qlementine Icons 1.7.0.

## v1.0.3

- Git tagging issue.

## v1.0.2

- Fix relative path in CMake.
- Update Inter fonts to v4.1.

## v1.0.1

- The lib can now be installed properly!
- Lots of bugfixes not necessarily listed as issues on Github.
- Fix treeview QComboBox highlight visual glitch (#72).
- Fix Sonarcube warnings (#70).
- Fix misaligned and cut-off text in context QMenu (#71).
- Fix QComboBox popup thats stays too small when items are added (#75).
- Fix memory leak of FlashActionHelper (#83).
- Fix unscrollable QComboBox when a lot of items (#76).
- Fix popup QMenu QAction triggered when the menu appears (#77).
- Obsolete resources removed: abandonned CSD files, and Windows Inter fonts(#82).

Thanks to @christophe-thiery-fw, @nlogozzo, @thierryba and @Pieter-Dewachter.

## v1.0.0

- Initial release. Enjoy!
