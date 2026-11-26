//=========================================================
//  MusE
//  Linux Music Editor
//=========================================================

#ifndef MUSEMCPDIALOG_H
#define MUSEMCPDIALOG_H

#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTimer;

namespace MusEGui {

class MuseMcpDialog : public QDialog
{
public:
  static constexpr int RequirementsCheckTimeoutMs = 10000;
  static constexpr int ConnectionCheckTimeoutMs = 30000;

  MuseMcpDialog(const QString& serverScript, const QString& documentation,
                bool pythonBridgeSupported, bool startAtLaunch,
                const QString& bindAddress, const QString& nameserverPort,
                QWidget* parent = nullptr);
  ~MuseMcpDialog() override;

  bool startAtLaunch() const;
  QString bindAddress() const;
  QString pythonExecutable() const;
  QString permissionProfile() const;
  QString clientConfiguration() const;

  static bool isSafeBindAddress(const QString& address);

private:
  enum class ProcessPurpose { None, Requirements, Connection };

  void checkRequirements();
  void testConnection();
  void startProcess(ProcessPurpose purpose);
  void updatePreview();
  void updateProfileDescription();
  void openDocumentation();
  void finishProcess(int exitCode);

  QString m_serverScript;
  QString m_documentation;
  QString m_nameserverPort;
  bool m_pythonBridgeSupported;
  bool m_processTimedOut;
  ProcessPurpose m_processPurpose;

  QCheckBox* m_startAtLaunch;
  QComboBox* m_bindAddress;
  QComboBox* m_profile;
  QLineEdit* m_pythonExecutable;
  QLabel* m_requirementsStatus;
  QLabel* m_connectionStatus;
  QLabel* m_profileDescription;
  QPlainTextEdit* m_configurationPreview;
  QPushButton* m_checkRequirementsButton;
  QPushButton* m_testConnectionButton;
  QProcess* m_process;
  QTimer* m_processTimeout;
};

} // namespace MusEGui

#endif
