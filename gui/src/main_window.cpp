#include "main_window.hpp"
#include "./ui_main_window.h"
#include <QFile>
#include <QCloseEvent>
#include <QSerialPortInfo>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include "version.hpp"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(WEAVER_VERSION_NAME);

    ui->editWiFiNetwork->setPlaceholderText("WiFi SSID");
    ui->editWiFiPassword->setPlaceholderText("WiFi password");
    ui->editMqttIpAddress->setInputMask("000.000.000.000");
    ui->editMqttIpAddress->setText("192.168.0.218");
    ui->editMqttPort->setInputMask("0000");
    ui->editMqttPort->setText("8080");
    ui->editMqttToken->setPlaceholderText("Authentication token");

    m_serialPort = new QSerialPort(this);
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)),
            this, SLOT(handleSerialPortError(QSerialPort::SerialPortError)));
    connect(m_serialPort, SIGNAL(readyRead()), this, SLOT(readSerialData()));
    connect(ui->cbxSerialPort, SIGNAL(currentIndexChanged(int)), this, SLOT(serialPortChanged(int)));

    connect(ui->actionClearLogs, SIGNAL(triggered()), this, SLOT(clearLogs()));
    connect(ui->actionToggleConnection, SIGNAL(triggered()), this, SLOT(toggleConnection()));
    connect(ui->actionLoadSerialPorts, SIGNAL(triggered()), this, SLOT(loadSerialPorts()));
    connect(ui->actionReadSensors, SIGNAL(triggered()), this, SLOT(readSensors()));
    connect(ui->actionReadConfiguration, SIGNAL(triggered()), this, SLOT(readConfiguration()));
    connect(ui->actionSendConfiguration, SIGNAL(triggered()), this, SLOT(configureDevice()));

    connect(ui->btnExit, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->btnToggleConnection, SIGNAL(clicked()), this, SLOT(toggleConnection()));
    connect(ui->btnReadConfiguration, SIGNAL(clicked()), this, SLOT(readConfiguration()));
    connect(ui->btnReadSensors, SIGNAL(clicked()), this, SLOT(readSensors()));
    connect(ui->btnConfigureDevice, SIGNAL(clicked()), this, SLOT(configureDevice()));
    connect(ui->actionExitApplication, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), this, SLOT(showAboutQt()));
    connect(ui->actionAboutApplication, SIGNAL(triggered()), this, SLOT(showAboutApplication()));

    loadSerialPorts();
    fillPortParameters();
    toggleControls(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_serialPort->isOpen())
    {
        QMessageBox::StandardButton response;
        response = QMessageBox::question(this, WEAVER_VERSION_NAME,
            "Device is connected, are you sure you want to exit?",
            QMessageBox::Yes | QMessageBox::No);

        if (response == QMessageBox::No)
        {
            event->ignore();
            return;
        }
    }

    disconnectDevice();
    event->accept();
}

void MainWindow::showAboutQt()
{
    QMessageBox::aboutQt(this, "About Qt - "
        + QStringLiteral(WEAVER_VERSION_NAME));
}

void MainWindow::showAboutApplication()
{
    QFile aboutFile(":/weaver/html/about.html");
    if (!aboutFile.open(QIODevice::ReadOnly))
        return;

    QString html = aboutFile.readAll();

    html.replace("{APPLICATION_NAME}", WEAVER_VERSION_NAME);
    html.replace("{APPLICATION_VERSION}", WEAVER_VERSION_NUMBER);
    html.replace("{QT_VERSION}", QT_VERSION_NUMBER);
    html.replace("{APPLICATION_COPYRIGHT}", WEAVER_VERSION_COPYRIGHT);

    QMessageBox::about(this, "About "
        + QStringLiteral(WEAVER_VERSION_NAME), html);
}

void MainWindow::clearLogs()
{
    ui->logView->clear();
}

void MainWindow::toggleConnection()
{
    if (m_serialPort->isOpen())
    {
        disconnectDevice();
    }
    else
    {
        connectDevice();
    }

    toggleControls(m_serialPort->isOpen());
}

void MainWindow::readConfiguration()
{
    const QString command = "STATUS\r\n";
    logMessage(MSG_INFORMATION, "Reading configuration...");
    m_serialPort->write(command.toLatin1());
}

void MainWindow::readSensors()
{
    const QString command = "SENSORS\r\n";
    logMessage(MSG_INFORMATION, "Reading sensors...");
    m_serialPort->write(command.toLatin1());
}

void MainWindow::configureDevice()
{
    QString command = "WIFICFG|"
        + ui->editWiFiNetwork->text() + "|"
        + ui->editWiFiPassword->text() + "|"
        + ui->editMqttIpAddress->text() + "|"
        + ui->editMqttPort->text() + "|"
        + ui->editMqttToken->text() + "|\r\n";

    logMessage(MSG_INFORMATION, "Sending configuration...");
    m_serialPort->write(command.toLatin1());
}

void MainWindow::loadSerialPorts()
{
    ui->cbxSerialPort->clear();

    const auto infoList = QSerialPortInfo::availablePorts();
    if (infoList.length() == 0)
    {
        logMessage(MSG_WARNING, "No serial port detected on this computer, "
                   "connect the device and restart application.");
        ui->actionToggleConnection->setEnabled(false);
        ui->btnToggleConnection->setEnabled(false);
        return;
    }

    for (const QSerialPortInfo &info : infoList)
    {
        ui->cbxSerialPort->addItem(info.portName(), info.portName());
        logMessage(MSG_INFORMATION, "Detected serial port "
                   + info.portName() + " - " + info.description());
    }

    ui->actionToggleConnection->setEnabled(true);
    ui->btnToggleConnection->setEnabled(true);
}

void MainWindow::readSerialData()
{
    const QByteArray data = m_serialPort->readAll();
    m_receivedData.append(data);

    if (m_receivedData.at(m_receivedData.size() - 1) == 0x0A)
    {
        parseReceivedData();
        m_receivedData.clear();
    }
}

void MainWindow::serialPortChanged(int index)
{
    if (index == -1)
        return;

    const QString portName = ui->cbxSerialPort->itemData(index).toString();
    m_serialPort->setPortName(portName);
}

void MainWindow::handleSerialPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        QMessageBox::critical(this, "Error", m_serialPort->errorString());
    }
}

void MainWindow::connectDevice()
{
    if (m_serialPort->portName().isEmpty())
        return;

    m_serialPort->setBaudRate(static_cast<QSerialPort::BaudRate>(
                            ui->cbxBaudRate->itemData(ui->cbxBaudRate->currentIndex()).toInt()));
    m_serialPort->setParity(static_cast<QSerialPort::Parity>(
                            ui->cbxParity->itemData(ui->cbxParity->currentIndex()).toInt()));
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(
                            ui->cbxDataBits->itemData(ui->cbxDataBits->currentIndex()).toInt()));
    m_serialPort->setStopBits(static_cast<QSerialPort::StopBits>(
                            ui->cbxStopBits->itemData(ui->cbxStopBits->currentIndex()).toInt()));
    m_serialPort->setFlowControl(static_cast<QSerialPort::FlowControl>(
                            ui->cbxFlowControl->itemData(ui->cbxFlowControl->currentIndex()).toInt()));

    if (!m_serialPort->open(QIODevice::ReadWrite))
    {
        logMessage(MSG_ERROR, "Unable to open serial port "
                   + m_serialPort->portName());
        return;
    }

    logMessage(MSG_ACTION, "Connected on serial port "
               + m_serialPort->portName());
}

void MainWindow::disconnectDevice()
{
    if (m_serialPort->isOpen())
    {
        m_serialPort->close();
        logMessage(MSG_ACTION, "Disconnected from serial port "
                   + m_serialPort->portName());
    }
}

void MainWindow::fillPortParameters()
{
    ui->cbxBaudRate->addItem(QStringLiteral("1200"), QSerialPort::Baud1200);
    ui->cbxBaudRate->addItem(QStringLiteral("1400"), QSerialPort::Baud2400);
    ui->cbxBaudRate->addItem(QStringLiteral("4800"), QSerialPort::Baud4800);
    ui->cbxBaudRate->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->cbxBaudRate->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->cbxBaudRate->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->cbxBaudRate->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->cbxBaudRate->setCurrentIndex(6);

    ui->cbxParity->addItem(tr("None"), QSerialPort::NoParity);
    ui->cbxParity->addItem(tr("Even"), QSerialPort::EvenParity);
    ui->cbxParity->addItem(tr("Odd"), QSerialPort::OddParity);
    ui->cbxParity->addItem(tr("Mark"), QSerialPort::MarkParity);
    ui->cbxParity->addItem(tr("Space"), QSerialPort::SpaceParity);
    ui->cbxParity->setCurrentIndex(0);

    ui->cbxDataBits->addItem(QStringLiteral("5"), QSerialPort::Data5);
    ui->cbxDataBits->addItem(QStringLiteral("6"), QSerialPort::Data6);
    ui->cbxDataBits->addItem(QStringLiteral("7"), QSerialPort::Data7);
    ui->cbxDataBits->addItem(QStringLiteral("8"), QSerialPort::Data8);
    ui->cbxDataBits->setCurrentIndex(3);

    ui->cbxStopBits->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    ui->cbxStopBits->addItem(tr("1.5"), QSerialPort::OneAndHalfStop);
#endif
    ui->cbxStopBits->addItem(QStringLiteral("2"), QSerialPort::TwoStop);
    ui->cbxStopBits->setCurrentIndex(0);

    ui->cbxFlowControl->addItem(tr("None"), QSerialPort::NoFlowControl);
    ui->cbxFlowControl->addItem(tr("RTS/CTS"), QSerialPort::HardwareControl);
    ui->cbxFlowControl->addItem(tr("XON/XOFF"), QSerialPort::SoftwareControl);
    ui->cbxFlowControl->setCurrentIndex(0);
}

void MainWindow::toggleControls(bool connected)
{
    const QString connectionLabel = connected ? "Disconnect" : "Connect";
    const QString connectionIcon = connected ?
                ":/weaver/icons/plug-disconnect.png" :
                ":/weaver/icons/plug-connect.png";

    ui->actionToggleConnection->setIcon(QIcon(connectionIcon));
    ui->actionToggleConnection->setText(connectionLabel);
    ui->btnToggleConnection->setText(connectionLabel);

    ui->actionReadSensors->setEnabled(connected);
    ui->actionReadConfiguration->setEnabled(connected);
    ui->actionSendConfiguration->setEnabled(connected);
    ui->actionLoadSerialPorts->setEnabled(!connected);
    ui->btnReadSensors->setEnabled(connected);
    ui->btnReadConfiguration->setEnabled(connected);
    ui->btnConfigureDevice->setEnabled(connected);
    ui->cbxSerialPort->setEnabled(!connected);
    ui->cbxBaudRate->setEnabled(!connected);
    ui->cbxParity->setEnabled(!connected);
    ui->cbxDataBits->setEnabled(!connected);
    ui->cbxStopBits->setEnabled(!connected);
    ui->cbxFlowControl->setEnabled(!connected);
    ui->gbxConfiguration->setEnabled(connected);
    ui->gbxSensors->setEnabled(connected);
}

void MainWindow::logMessage(MessageType type, QString message)
{
    QString iconPath = ":/weaver/icons/";

    switch(type)
    {
    case MSG_INFORMATION:
        iconPath += "info.png";
        break;
    case MSG_ACTION:
        iconPath += "tick.png";
        break;
    case MSG_WARNING:
        iconPath += "warning.png";
        break;
    case MSG_ERROR:
        iconPath += "error.png";
        break;
    }

    QListWidgetItem *item = new QListWidgetItem(ui->logView);
    item->setIcon(QIcon(iconPath));
    item->setText(message);
    ui->logView->addItem(item);
    ui->logView->setCurrentRow(ui->logView->count());
    ui->logView->scrollToBottom();
}

void MainWindow::parseReceivedData()
{
    qDebug() << QString(m_receivedData);
    QJsonParseError jsonError;
    QJsonDocument json = QJsonDocument::fromJson(m_receivedData, &jsonError);

    if (jsonError.error != 0)
    {
        logMessage(MSG_ERROR, jsonError.errorString());
        return;
    }

    QJsonObject root = json.object();
    if (root.contains("status"))
    {
        QJsonValue status = root.value("status");
        if (status.toString() == "OK")
            logMessage(MSG_ACTION, "Configuration sent.");
        else
            logMessage(MSG_ERROR, " Error sending configuration.");
    }
    else if (root.contains("wifi_state"))
    {
        QJsonValue wifi_state = root.value("wifi_state");
        ui->editWiFiStatus->setText(wifiStateToString(wifi_state.toInt()));
        QJsonValue wifi_ssid = root.value("wifi_ssid");
        ui->editWiFiNetwork->setText(wifi_ssid.toString());
        QJsonValue wifi_password = root.value("wifi_password");
        ui->editWiFiPassword->setText(wifi_password.toString());
        QJsonValue broker_address = root.value("broker_address");
        ui->editMqttIpAddress->setText(broker_address.toString());
        QJsonValue broker_port = root.value("broker_port");
        ui->editMqttPort->setText(broker_port.toString());
        QJsonValue broker_token = root.value("broker_token");
        ui->editMqttToken->setText(broker_token.toString());
        logMessage(MSG_ACTION, "WiFi configuration received.");
    }
    else if (root.contains("temperature"))
    {
        QJsonValue temperature = root.value("temperature");
        ui->editTemperature->setText(temperature.toString());
        QJsonValue humidity = root.value("humidity");
        ui->editHumidity->setText(humidity.toString());
        QJsonValue pressure = root.value("pressure");
        ui->editPressure->setText(pressure.toString());
        QJsonValue tvoc = root.value("tvoc");
        ui->editTVoc->setText(tvoc.toString());
        QJsonValue eco2 = root.value("eco2");
        ui->editECo2->setText(eco2.toString());
        logMessage(MSG_ACTION, "Sensors values received.");
    }
}

QString MainWindow::wifiStateToString(uint8_t state)
{
    static const QStringList wifiStateStrings =
    {
        "WIFI_NONE",
        "WIFI_INITIALIZE",
        "WIFI_RESTARTING",
        "WIFI_MODE_CONFIGURE",
        "WIFI_MODE_CONFIGURING",
        "WIFI_NETWORK_CONNECT",
        "WIFI_NETWORK_CONNECTING",
        "WIFI_NETWORK_CONNECTED",
        "WIFI_MQTT_CONNECT",
        "WIFI_MQTT_CONNECTING",
        "WIFI_MQTT_CONNECTED",
        "WIFI_MQTT_PUBLISH_START",
        "WIFI_MQTT_PUBLISH",
        "WIFI_MQTT_PUBLISH_WAIT_REPLY",
        "WIFI_ERROR_INITIALIZE",
        "WIFI_ERROR_NETWORK",
        "WIFI_ERROR_MQTT_BROKER",
        "WIFI_ERROR_MQTT_PUBLISH"
    };

    Q_ASSERT(state <= wifiStateStrings.count());
    return wifiStateStrings[state];
}
