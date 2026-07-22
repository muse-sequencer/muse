// SPDX-FileCopyrightText: Olivier Cléro <oclero@hotmail.com>
// SPDX-License-Identifier: MIT

#include <QApplication>

#include <oclero/qlementine/style/QlementineStyle.hpp>
#include <oclero/qlementine/style/ThemeManager.hpp>
#include <oclero/qlementine/icons/QlementineIcons.hpp>

#include "ShowcaseWindow.hpp"

#define USE_CUSTOM_STYLE 1

int main(int argc, char* argv[]) {
  // Must be set before creating a QApplication.
  QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  QApplication qApplication(argc, argv);

  // Must be set after creating a QApplication.
  QGuiApplication::setApplicationDisplayName("Showcase");
  QCoreApplication::setApplicationName("Showcase");
  QGuiApplication::setDesktopFileName("Showcase");
  QCoreApplication::setOrganizationName("oclero");
  QCoreApplication::setOrganizationDomain("olivierclero.com");
  QCoreApplication::setApplicationVersion("1.0.0");
  QApplication::setWindowIcon(QIcon(QStringLiteral(":/showcase/qlementine_icon.ico")));

#if USE_CUSTOM_STYLE
  // Custom QStyle.
  auto* style = new oclero::qlementine::QlementineStyle(&qApplication);
  style->setAnimationsEnabled(true);
  style->setAutoIconColor(oclero::qlementine::AutoIconColor::TextColor);
  style->setIconPathGetter(oclero::qlementine::icons::fromFreeDesktop);
  qApplication.setStyle(style);

  // Custom icon theme.
  oclero::qlementine::icons::initializeIconTheme();
  QIcon::setThemeName("qlementine");

  // Theme manager.
  auto* themeManager = new oclero::qlementine::ThemeManager(style);
  themeManager->loadDirectory(":/showcase/themes");

  // Define theme on QStyle.
  themeManager->setCurrentTheme("Light");
#endif

  auto window = std::make_unique<oclero::qlementine::showcase::ShowcaseWindow>(themeManager);
  window->show();

  return qApplication.exec();
}
