#include "musemcpdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QtTest/QtTest>

class MuseMcpDialogTests : public QObject
{
  Q_OBJECT

private slots:
  void clientConfigurationUsesProjectEditProfileByDefault();
  void projectEditProfileWarnsAboutHostFileAccess();
  void defaultInterpreterPrefersMcpVirtualEnvironment();
  void clientConfigurationReflectsSelectedProfileAndAddress();
  void startupSettingCanBeChanged();
  void startupSettingIsDisabledWithoutPythonBridgeSupport();
  void bindAddressPolicyAllowsOnlyLoopbackOrPrivateIpv4();
  void processChecksUsePurposeSpecificTimeouts();
};

void MuseMcpDialogTests::clientConfigurationUsesProjectEditProfileByDefault()
{
  MusEGui::MuseMcpDialog dialog("/opt/muse/musemcp.py", "/opt/muse/README.mcp.md",
                                true, false, "127.0.0.1", "");
  const QJsonObject root = QJsonDocument::fromJson(dialog.clientConfiguration().toUtf8()).object();
  const QJsonObject server = root.value("mcpServers").toObject().value("muse").toObject();
  const QJsonObject environment = server.value("env").toObject();
  const QJsonArray arguments = server.value("args").toArray();

  QCOMPARE(server.value("command").toString(), dialog.pythonExecutable());
  QCOMPARE(arguments.size(), 1);
  QCOMPARE(arguments.at(0).toString(), QFileInfo("/opt/muse/musemcp.py").absoluteFilePath());
  QCOMPARE(environment.value("MUSE_PYRO_NS_HOST").toString(), QString("127.0.0.1"));
  QCOMPARE(environment.value("MUSE_MCP_PROFILE").toString(), QString("project_edit"));
}

void MuseMcpDialogTests::projectEditProfileWarnsAboutHostFileAccess()
{
  MusEGui::MuseMcpDialog dialog("/opt/muse/musemcp.py", "/opt/muse/README.mcp.md",
                                true, false, "127.0.0.1", "");
  auto* description = dialog.findChild<QLabel*>("mcpProfileDescription");
  QVERIFY(description);
  QVERIFY(description->text().contains("anywhere"));
  QVERIFY(description->text().contains("project folder"));
}

void MuseMcpDialogTests::defaultInterpreterPrefersMcpVirtualEnvironment()
{
  MusEGui::MuseMcpDialog dialog("/opt/muse/musemcp.py", "/opt/muse/README.mcp.md",
                                true, false, "127.0.0.1", "");
  const QString mcpPython = QDir::home().filePath(QStringLiteral(".venvs/muse-mcp/bin/python"));

  if (QFileInfo(mcpPython).isExecutable())
    QCOMPARE(dialog.pythonExecutable(), QFileInfo(mcpPython).absoluteFilePath());
  else
    QVERIFY(!dialog.pythonExecutable().isEmpty());
}

void MuseMcpDialogTests::clientConfigurationReflectsSelectedProfileAndAddress()
{
  MusEGui::MuseMcpDialog dialog("/opt/muse/musemcp.py", "/opt/muse/README.mcp.md",
                                true, false, "192.168.1.10", "9090");
  auto* address = dialog.findChild<QComboBox*>("mcpBindAddress");
  auto* profile = dialog.findChild<QComboBox*>("mcpProfile");
  QVERIFY(address);
  QVERIFY(profile);

  const int privateAddressIndex = address->findData("192.168.1.10");
  QVERIFY(privateAddressIndex >= 0);
  address->setCurrentIndex(privateAddressIndex);
  profile->setCurrentIndex(profile->findData("project_edit"));

  const QJsonObject root = QJsonDocument::fromJson(dialog.clientConfiguration().toUtf8()).object();
  const QJsonObject server = root.value("mcpServers").toObject().value("muse").toObject();
  const QJsonObject environment = server.value("env").toObject();
  QCOMPARE(environment.value("MUSE_MCP_PROFILE").toString(), QString("project_edit"));
  QCOMPARE(environment.value("MUSE_PYRO_NS_HOST").toString(), QString("192.168.1.10"));
  QCOMPARE(environment.value("MUSE_PYRO_NS_PORT").toString(), QString("9090"));
  QCOMPARE(dialog.permissionProfile(), QString("project_edit"));
}

void MuseMcpDialogTests::startupSettingCanBeChanged()
{
  MusEGui::MuseMcpDialog dialog("/opt/muse/musemcp.py", "/opt/muse/README.mcp.md",
                                true, false, "127.0.0.1", "");
  auto* startAtLaunch = dialog.findChild<QCheckBox*>("mcpStartAtLaunch");
  QVERIFY(startAtLaunch);
  QVERIFY(!dialog.startAtLaunch());

  startAtLaunch->setChecked(true);
  QVERIFY(dialog.startAtLaunch());
}

void MuseMcpDialogTests::startupSettingIsDisabledWithoutPythonBridgeSupport()
{
  MusEGui::MuseMcpDialog dialog("/opt/muse/musemcp.py", "/opt/muse/README.mcp.md",
                                false, false, "127.0.0.1", "");
  auto* startAtLaunch = dialog.findChild<QCheckBox*>("mcpStartAtLaunch");
  QVERIFY(startAtLaunch);
  QVERIFY(!startAtLaunch->isEnabled());
}

void MuseMcpDialogTests::bindAddressPolicyAllowsOnlyLoopbackOrPrivateIpv4()
{
  QVERIFY(MusEGui::MuseMcpDialog::isSafeBindAddress("127.0.0.1"));
  QVERIFY(MusEGui::MuseMcpDialog::isSafeBindAddress("10.0.0.5"));
  QVERIFY(MusEGui::MuseMcpDialog::isSafeBindAddress("172.16.2.3"));
  QVERIFY(MusEGui::MuseMcpDialog::isSafeBindAddress("192.168.1.10"));
  QVERIFY(!MusEGui::MuseMcpDialog::isSafeBindAddress("0.0.0.0"));
  QVERIFY(!MusEGui::MuseMcpDialog::isSafeBindAddress("8.8.8.8"));
  QVERIFY(!MusEGui::MuseMcpDialog::isSafeBindAddress("localhost"));
}

void MuseMcpDialogTests::processChecksUsePurposeSpecificTimeouts()
{
  const QString existingScript = QCoreApplication::applicationFilePath();
  {
    MusEGui::MuseMcpDialog dialog(existingScript, "/opt/muse/README.mcp.md",
                                  true, false, "127.0.0.1", "");
    auto* python = dialog.findChild<QLineEdit*>("mcpPythonExecutable");
    auto* check = dialog.findChild<QPushButton*>("mcpCheckRequirements");
    auto* timeout = dialog.findChild<QTimer*>("mcpProcessTimeout");
    QVERIFY(python);
    QVERIFY(check);
    QVERIFY(timeout);
    python->setText("/nonexistent/mcp-test-python");
    check->click();
    QCOMPARE(timeout->interval(), MusEGui::MuseMcpDialog::RequirementsCheckTimeoutMs);
    QTRY_VERIFY(!timeout->isActive());
  }
  {
    MusEGui::MuseMcpDialog dialog(existingScript, "/opt/muse/README.mcp.md",
                                  true, false, "127.0.0.1", "");
    auto* python = dialog.findChild<QLineEdit*>("mcpPythonExecutable");
    auto* check = dialog.findChild<QPushButton*>("mcpTestConnection");
    auto* timeout = dialog.findChild<QTimer*>("mcpProcessTimeout");
    QVERIFY(python);
    QVERIFY(check);
    QVERIFY(timeout);
    python->setText("/nonexistent/mcp-test-python");
    check->click();
    QCOMPARE(timeout->interval(), MusEGui::MuseMcpDialog::ConnectionCheckTimeoutMs);
    QTRY_VERIFY(!timeout->isActive());
  }
}

QTEST_MAIN(MuseMcpDialogTests)
#include "test_musemcpdialog.moc"
