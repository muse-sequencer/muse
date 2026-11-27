#include "musemcpdialog.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace MusEGui {

MuseMcpDialog::MuseMcpDialog(const QString& serverScript, const QString& documentation,
                             bool pythonBridgeSupported, bool startAtLaunch,
                             const QString& bindAddress, const QString& nameserverPort,
                             QWidget* parent)
  : QDialog(parent),
    m_serverScript(QFileInfo(serverScript).absoluteFilePath()),
    m_documentation(QFileInfo(documentation).absoluteFilePath()),
    m_nameserverPort(nameserverPort.trimmed()),
    m_pythonBridgeSupported(pythonBridgeSupported),
    m_processTimedOut(false),
    m_processPurpose(ProcessPurpose::None),
    m_startAtLaunch(new QCheckBox(tr("Start the Python bridge automatically when MusE launches"), this)),
    m_bindAddress(new QComboBox(this)),
    m_profile(new QComboBox(this)),
    m_pythonExecutable(new QLineEdit(this)),
    m_requirementsStatus(new QLabel(this)),
    m_connectionStatus(new QLabel(this)),
    m_profileDescription(new QLabel(this)),
    m_configurationPreview(new QPlainTextEdit(this)),
    m_checkRequirementsButton(new QPushButton(tr("Check requirements"), this)),
    m_testConnectionButton(new QPushButton(tr("Test read-only connection"), this)),
    m_installRequirementsButton(new QPushButton(tr("Install dependencies"), this)),
    m_process(new QProcess(this)),
    m_processTimeout(new QTimer(this))
{
  setWindowTitle(tr("Connect AI Agent (MCP)"));
  setMinimumWidth(560);

  auto* layout = new QVBoxLayout(this);
  auto* introduction = new QLabel(
    tr("MusE exposes selected song and transport operations to an MCP client. "
       "The client and AI model are configured separately; this dialog prepares "
       "the server entry and checks the local bridge."), this);
  introduction->setWordWrap(true);
  layout->addWidget(introduction);

  auto* form = new QFormLayout;
  m_startAtLaunch->setObjectName("mcpStartAtLaunch");
  m_startAtLaunch->setToolTip(tr("This preference takes effect the next time MusE starts."));
  form->addRow(m_startAtLaunch);

  m_bindAddress->setObjectName("mcpBindAddress");
  m_bindAddress->addItem(tr("This computer only (127.0.0.1)"), QStringLiteral("127.0.0.1"));
  for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces()) {
    if (!interface.flags().testFlag(QNetworkInterface::IsUp) ||
        interface.flags().testFlag(QNetworkInterface::IsLoopBack))
      continue;
    for (const QNetworkAddressEntry& entry : interface.addressEntries()) {
      const QString address = entry.ip().toString();
      if (!isSafeBindAddress(address) || address.startsWith("127."))
        continue;
      if (m_bindAddress->findData(address) < 0)
        m_bindAddress->addItem(tr("%1 (%2)").arg(address, interface.humanReadableName()), address);
    }
  }
  if (isSafeBindAddress(bindAddress) && m_bindAddress->findData(bindAddress) < 0)
    m_bindAddress->addItem(tr("%1 (saved address)").arg(bindAddress), bindAddress);
  const int selectedAddress = m_bindAddress->findData(bindAddress);
  m_bindAddress->setCurrentIndex(selectedAddress >= 0 ? selectedAddress : 0);
  form->addRow(tr("Pyro interface"), m_bindAddress);

  auto* networkNote = new QLabel(
    tr("Loopback is the default. A private IPv4 interface makes the bridge reachable "
       "from devices that can access that network. Pyro has no authentication; use "
       "this only on a trusted local network."), this);
  networkNote->setWordWrap(true);
  form->addRow(networkNote);

  m_pythonExecutable->setObjectName("mcpPythonExecutable");
  const QString mcpPython = QDir::home().filePath(QStringLiteral(".venvs/muse-mcp/bin/python"));
  const bool mcpPythonAvailable = QFileInfo(mcpPython).isExecutable();
  QString python = mcpPythonAvailable ? QFileInfo(mcpPython).absoluteFilePath()
                                      : QStandardPaths::findExecutable("python3");
  if (python.isEmpty())
    python = QStringLiteral("python3");
  m_pythonExecutable->setText(python);
  auto* pythonRow = new QWidget(this);
  auto* pythonLayout = new QHBoxLayout(pythonRow);
  pythonLayout->setContentsMargins(0, 0, 0, 0);
  pythonLayout->addWidget(m_pythonExecutable, 1);
  auto* browsePython = new QPushButton(tr("Browse..."), pythonRow);
  pythonLayout->addWidget(browsePython);
  connect(browsePython, &QPushButton::clicked, this, [this]() {
    const QString file = QFileDialog::getOpenFileName(this, tr("Select Python interpreter"),
                                                       m_pythonExecutable->text());
    if (!file.isEmpty())
      m_pythonExecutable->setText(file);
  });
  form->addRow(tr("Python interpreter (with MCP packages)"), pythonRow);

  m_profile->setObjectName("mcpProfile");
  m_profile->addItem(tr("Read only"), QStringLiteral("read_only"));
  m_profile->addItem(tr("Transport control"), QStringLiteral("transport"));
  m_profile->addItem(tr("Edit the open project"), QStringLiteral("project_edit"));
  m_profile->setCurrentIndex(m_profile->findData(QStringLiteral("project_edit")));
  form->addRow(tr("Permission profile"), m_profile);

  m_profileDescription->setObjectName(QStringLiteral("mcpProfileDescription"));
  m_profileDescription->setWordWrap(true);
  form->addRow(m_profileDescription);
  layout->addLayout(form);

  auto* statusLayout = new QHBoxLayout;
  statusLayout->addWidget(m_checkRequirementsButton);
  statusLayout->addWidget(m_testConnectionButton);
  statusLayout->addWidget(m_installRequirementsButton);
  layout->addLayout(statusLayout);

  m_requirementsStatus->setObjectName("mcpRequirementsStatus");
  m_requirementsStatus->setWordWrap(true);
  m_connectionStatus->setObjectName("mcpConnectionStatus");
  m_connectionStatus->setWordWrap(true);
  layout->addWidget(m_requirementsStatus);
  layout->addWidget(m_connectionStatus);

  m_configurationPreview->setObjectName("mcpConfigurationPreview");
  m_configurationPreview->setReadOnly(true);
  m_configurationPreview->setMaximumHeight(150);
  layout->addWidget(m_configurationPreview);

  auto* actions = new QHBoxLayout;
  auto* copyConfiguration = new QPushButton(tr("Copy client configuration"), this);
  copyConfiguration->setObjectName("mcpCopyConfiguration");
  auto* openReadme = new QPushButton(tr("Open instructions"), this);
  actions->addWidget(copyConfiguration);
  actions->addWidget(openReadme);
  actions->addStretch(1);
  layout->addLayout(actions);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
  layout->addWidget(buttons);

  m_startAtLaunch->setChecked(startAtLaunch);
  m_startAtLaunch->setEnabled(m_pythonBridgeSupported);
  m_processTimeout->setSingleShot(true);
  m_processTimeout->setObjectName(QStringLiteral("mcpProcessTimeout"));
  m_checkRequirementsButton->setObjectName(QStringLiteral("mcpCheckRequirements"));
  m_testConnectionButton->setObjectName(QStringLiteral("mcpTestConnection"));
  m_installRequirementsButton->setObjectName(QStringLiteral("mcpInstallRequirements"));
  connect(m_processTimeout, &QTimer::timeout, this, [this]() {
    if (m_process->state() != QProcess::NotRunning) {
      m_processTimedOut = true;
      m_process->kill();
    }
  });
  connect(m_process, &QProcess::finished, this,
          [this](int exitCode, QProcess::ExitStatus) { finishProcess(exitCode); });
  connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart)
      finishProcess(-1);
  });

  connect(m_checkRequirementsButton, &QPushButton::clicked, this, &MuseMcpDialog::checkRequirements);
  connect(m_testConnectionButton, &QPushButton::clicked, this, &MuseMcpDialog::testConnection);
  connect(m_installRequirementsButton, &QPushButton::clicked, this, &MuseMcpDialog::installRequirements);
  connect(m_profile, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int) { updateProfileDescription(); updatePreview(); });
  connect(m_bindAddress, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this](int) { updatePreview(); });
  connect(m_pythonExecutable, &QLineEdit::textChanged, this, [this](const QString&) { updatePreview(); });
  connect(copyConfiguration, &QPushButton::clicked, this, [this]() {
    QApplication::clipboard()->setText(clientConfiguration());
    m_connectionStatus->setText(tr("MCP client configuration copied to the clipboard."));
  });
  connect(openReadme, &QPushButton::clicked, this, &MuseMcpDialog::openDocumentation);

  updateProfileDescription();
  updatePreview();
  if (!m_pythonBridgeSupported)
    m_requirementsStatus->setText(tr("This MusE build does not include Python Bridge support."));
  else if (!QFileInfo::exists(m_serverScript))
    m_requirementsStatus->setText(tr("MCP server script was not found: %1").arg(m_serverScript));
  else if (mcpPythonAvailable)
    m_requirementsStatus->setText(tr("Using the dedicated MCP Python environment. Check requirements to verify its packages."));
  else
    m_requirementsStatus->setText(tr("No dedicated MCP Python environment was found. The selected Python may not include MCP packages; check requirements or open the instructions to set one up."));
}

MuseMcpDialog::~MuseMcpDialog()
{
  m_processTimeout->stop();
  QObject::disconnect(m_process, nullptr, this, nullptr);
  if (m_process->state() != QProcess::NotRunning) {
    m_process->kill();
    m_process->waitForFinished();
  }
}

bool MuseMcpDialog::startAtLaunch() const
{
  return m_startAtLaunch->isChecked();
}

QString MuseMcpDialog::bindAddress() const
{
  const QString address = m_bindAddress->currentData().toString();
  return isSafeBindAddress(address) ? address : QStringLiteral("127.0.0.1");
}

QString MuseMcpDialog::pythonExecutable() const
{
  return m_pythonExecutable->text().trimmed();
}

QString MuseMcpDialog::permissionProfile() const
{
  return m_profile->currentData().toString();
}

QString MuseMcpDialog::clientConfiguration() const
{
  QJsonObject environment;
  environment.insert(QStringLiteral("MUSE_PYRO_NS_HOST"), bindAddress());
  environment.insert(QStringLiteral("MUSE_MCP_PROFILE"), permissionProfile());
  if (!m_nameserverPort.isEmpty())
    environment.insert(QStringLiteral("MUSE_PYRO_NS_PORT"), m_nameserverPort);

  const QJsonObject server {
    { QStringLiteral("command"), pythonExecutable() },
    { QStringLiteral("args"), QJsonArray { m_serverScript } },
    { QStringLiteral("env"), environment }
  };
  const QJsonObject servers { { QStringLiteral("muse"), server } };
  return QString::fromUtf8(QJsonDocument(QJsonObject {
    { QStringLiteral("mcpServers"), servers }
  }).toJson(QJsonDocument::Indented));
}

bool MuseMcpDialog::isSafeBindAddress(const QString& address)
{
  QHostAddress parsed;
  if (!parsed.setAddress(address.trimmed()) || parsed.protocol() != QAbstractSocket::IPv4Protocol)
    return false;

  const quint32 ip = parsed.toIPv4Address();
  if ((ip & 0xff000000U) == 0x7f000000U)
    return true;
  if ((ip & 0xff000000U) == 0x0a000000U)
    return true;
  if ((ip & 0xfff00000U) == 0xac100000U)
    return true;
  return (ip & 0xffff0000U) == 0xc0a80000U;
}

void MuseMcpDialog::checkRequirements()
{
  startProcess(ProcessPurpose::Requirements);
}

void MuseMcpDialog::testConnection()
{
  startProcess(ProcessPurpose::Connection);
}

void MuseMcpDialog::installRequirements()
{
  startProcess(ProcessPurpose::Installation);
}

void MuseMcpDialog::startProcess(ProcessPurpose purpose)
{
  if (m_process->state() != QProcess::NotRunning)
    return;
  if (!m_pythonBridgeSupported) {
    m_connectionStatus->setText(tr("This MusE build does not include Python Bridge support."));
    return;
  }
  if (!QFileInfo::exists(m_serverScript)) {
    m_requirementsStatus->setText(tr("MCP server script was not found: %1").arg(m_serverScript));
    return;
  }
  if (pythonExecutable().isEmpty()) {
    m_requirementsStatus->setText(tr("Select a Python interpreter first."));
    return;
  }
  const QString requirementsFile = QFileInfo(m_serverScript).dir().filePath(QStringLiteral("requirements-mcp.txt"));
  if (purpose == ProcessPurpose::Installation && !QFileInfo::exists(requirementsFile)) {
    m_requirementsStatus->setText(tr("MCP requirements file was not found: %1").arg(requirementsFile));
    return;
  }

  m_processPurpose = purpose;
  m_processTimedOut = false;
  m_checkRequirementsButton->setEnabled(false);
  m_testConnectionButton->setEnabled(false);
  m_installRequirementsButton->setEnabled(false);
  m_process->setProgram(pythonExecutable());
  m_process->setWorkingDirectory(QFileInfo(m_serverScript).absolutePath());
  m_process->setProcessChannelMode(QProcess::MergedChannels);

  QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
  if (purpose == ProcessPurpose::Connection) {
    environment.insert(QStringLiteral("MUSE_PYRO_NS_HOST"), bindAddress());
    if (m_nameserverPort.isEmpty())
      environment.remove(QStringLiteral("MUSE_PYRO_NS_PORT"));
    else
      environment.insert(QStringLiteral("MUSE_PYRO_NS_PORT"), m_nameserverPort);
    environment.insert(QStringLiteral("MUSE_PYRO_TIMEOUT"), QStringLiteral("10"));
    m_process->setArguments({m_serverScript, QStringLiteral("--check-connection")});
    m_connectionStatus->setText(tr("Testing a read-only connection to the open MusE song..."));
  } else if (purpose == ProcessPurpose::Requirements) {
    m_process->setArguments({QStringLiteral("-c"),
      QStringLiteral("import Pyro5, mcp.server.fastmcp, pydantic, musepyroconfig; "
                     "print('MCP dependencies are available')")});
    m_requirementsStatus->setText(tr("Checking Python dependencies..."));
  } else {
    m_process->setArguments({QStringLiteral("-m"), QStringLiteral("pip"),
      QStringLiteral("install"), QStringLiteral("-r"), requirementsFile});
    m_requirementsStatus->setText(tr("Installing MCP dependencies into the selected Python environment..."));
  }
  m_process->setProcessEnvironment(environment);
  const int timeout = purpose == ProcessPurpose::Connection ? ConnectionCheckTimeoutMs
    : purpose == ProcessPurpose::Installation ? InstallationTimeoutMs : RequirementsCheckTimeoutMs;
  m_processTimeout->start(timeout);
  m_process->start();
}

void MuseMcpDialog::finishProcess(int exitCode)
{
  if (m_processPurpose == ProcessPurpose::None)
    return;

  m_processTimeout->stop();
  const ProcessPurpose purpose = m_processPurpose;
  m_processPurpose = ProcessPurpose::None;
  m_checkRequirementsButton->setEnabled(true);
  m_testConnectionButton->setEnabled(true);
  m_installRequirementsButton->setEnabled(true);

  const QString output = QString::fromLocal8Bit(m_process->readAll()).trimmed();
  if (m_processTimedOut) {
    if (purpose == ProcessPurpose::Requirements)
      m_requirementsStatus->setText(tr("The dependency check timed out."));
    else if (purpose == ProcessPurpose::Installation)
      m_requirementsStatus->setText(tr("Dependency installation timed out. Check your network connection and try again."));
    else
      m_connectionStatus->setText(tr("The check timed out. Verify that MusE is running and the bridge is enabled."));
    return;
  }

  if (purpose == ProcessPurpose::Installation) {
    if (exitCode == 0)
      m_requirementsStatus->setText(tr("MCP dependencies installed successfully. Check requirements to verify them."));
    else
      m_requirementsStatus->setText(tr("Could not install MCP dependencies: %1").arg(output.left(500)));
    return;
  }

  if (purpose == ProcessPurpose::Requirements) {
    if (exitCode == 0)
      m_requirementsStatus->setText(tr("The MCP Python dependencies are available."));
    else
      m_requirementsStatus->setText(tr("Python dependencies are missing or could not be loaded: %1")
                                      .arg(output.left(500)));
    return;
  }

  QJsonParseError parseError;
  const QJsonDocument response = QJsonDocument::fromJson(output.toUtf8(), &parseError);
  const QJsonObject result = response.object();
  if (exitCode == 0 && result.value(QStringLiteral("ok")).toBool()) {
    const int trackCount = result.value(QStringLiteral("tracks")).toArray().size();
    m_connectionStatus->setText(tr("Read-only connection succeeded. The open song has %1 track(s).")
                                  .arg(trackCount));
  } else if (!result.value(QStringLiteral("error")).toString().isEmpty()) {
    m_connectionStatus->setText(tr("Connection failed: %1").arg(result.value(QStringLiteral("error")).toString()));
  } else {
    m_connectionStatus->setText(tr("Connection failed: %1").arg(output.left(500)));
  }
}

void MuseMcpDialog::updatePreview()
{
  m_configurationPreview->setPlainText(clientConfiguration());
}

void MuseMcpDialog::updateProfileDescription()
{
  const QString profile = permissionProfile();
  if (profile == QLatin1String("transport"))
    m_profileDescription->setText(tr("Allows song reads and transport controls; it cannot edit tracks or MIDI parts."));
  else if (profile == QLatin1String("project_edit"))
    m_profileDescription->setText(tr("Allows transport controls and project edits. It can read user-supplied MIDI and .med files and save .med files anywhere the MusE user account has access, not only in the project folder."));
  else
    m_profileDescription->setText(tr("Allows only reading song state and track names. This is the recommended default."));
}

void MuseMcpDialog::openDocumentation()
{
  if (!QFileInfo::exists(m_documentation)) {
    m_connectionStatus->setText(tr("Instructions were not found: %1").arg(m_documentation));
    return;
  }
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(m_documentation)))
    m_connectionStatus->setText(tr("Could not open the MCP instructions."));
}

} // namespace MusEGui
