#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QCloseEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum MessageType
    {
        MSG_INFORMATION,
        MSG_ACTION,
        MSG_WARNING,
        MSG_ERROR,
    };

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent*);

private slots:
    void showAboutQt();
    void showAboutApplication();
    void clearLogs();
    void toggleConnection();
    void readConfiguration();
    void readSensors();
    void configureDevice();
    void loadSerialPorts();
    void readSerialData();
    void serialPortChanged(int);
    void handleSerialPortError(QSerialPort::SerialPortError);

private:
    Ui::MainWindow *ui;
    QSerialPort *m_serialPort;
    QByteArray m_receivedData;

    void connectDevice();
    void disconnectDevice();
    void fillPortParameters();
    void toggleControls(bool);
    void logMessage(MessageType, QString);
    void parseReceivedData();
    QString wifiStateToString(uint8_t);
};

#endif // MAIN_WINDOW_HPP
