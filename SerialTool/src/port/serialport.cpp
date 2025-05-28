﻿#include "serialport.h"
#include "ui_serialport.h"
#include "portsetbox.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QDebug>

SerialPort::SerialPort(QWidget *parent) :
    AbstractPort(parent),
    ui(new Ui::SerialPort)
{
    ui->setupUi(this);
    serialPort = new QSerialPort(this);
    m_scanTimer = new QTimer(this);

    QRegExpValidator *pReg = new QRegExpValidator(QRegExp("^\\d{2,7}$"));
    ui->baudRateBox->lineEdit()->setValidator(pReg);
    //serialPort->setTextModeEnabled(true);
#if defined(Q_OS_LINUX)
    ui->portNameBox->setEditable(true);
#endif

    connect(serialPort, &QSerialPort::readyRead, this, &SerialPort::readyRead);
    connect(m_scanTimer, &QTimer::timeout, this, &SerialPort::onTimerUpdate);
    connect(ui->baudRateBox, &QComboBox::currentTextChanged, this, &SerialPort::setBaudRate);
    connect(ui->portNameBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(portChanged()));
    connect(ui->dtrBox, &QPushButton::toggled, [&](bool checked) {
        if(serialPort->isOpen()) {
            serialPort->setDataTerminalReady(checked);
        }
    });
#if defined(Q_OS_LINUX)
    connect(ui->portNameBox->lineEdit(), SIGNAL(editingFinished()), this, SLOT(onPortTextEdited()));
#endif

    scanPort();
    m_scanTimer->start(1000);
    autoOpenPortName="";
    autoDTR = true;
}

SerialPort::~SerialPort()
{
    delete ui;
}

void SerialPort::retranslate()
{
    ui->retranslateUi(this);
}

void SerialPort::loadConfig(QSettings *config)
{
    config->beginGroup("SerialPort");
    int baudRate = config->value("BaudRate").toInt();
    QString str = QString::number(baudRate);
    autoDTR = config->value("AutoDTR", true).toBool();
    config->endGroup();

    serialPort->setBaudRate(baudRate);
    ui->baudRateBox->setCurrentIndex(ui->baudRateBox->findText(str));
    ui->baudRateBox->setCurrentText(str);
}

void SerialPort::saveConfig(QSettings *config)
{
    config->beginGroup("SerialPort");
    config->setValue("BaudRate", QVariant(ui->baudRateBox->currentText()));
    config->setValue("AutoDTR", autoDTR);
    config->endGroup();
}

void SerialPort::setVisibleWidget(bool visible)
{
    ui->labelPort->setVisible(visible);
    ui->labelBaudRate->setVisible(visible);
    ui->portNameBox->setVisible(visible);
    ui->baudRateBox->setVisible(visible);
    this->setVisible(visible);
}

bool SerialPort::open()
{
    QString name = ui->portNameBox->currentText().section(' ', 0, 0);

#if defined(Q_OS_LINUX)
    if (name.indexOf("/dev/") != 0) {
        name = "/dev/" + name;
    }
#endif
    serialPort->setPortName(name);
    if (serialPort->open(QIODevice::ReadWrite)) {
        ui->portNameBox->setEnabled(false); // 禁止更改串口
        autoOpenPortName = ui->portNameBox->currentText();

        if(autoDTR) ui->dtrBox->setChecked(true);
        return true;
    }

    QMessageBox err(QMessageBox::Critical,
        tr("Error"),
        tr("Can not open the port!\n"
            "Port may be occupied or configured incorrectly!"),
        QMessageBox::Cancel, this);
    err.exec();
    return false;
}

void SerialPort::reset()
{
    serialPort->setRequestToSend(0);
    serialPort->setRequestToSend(1);
    serialPort->setRequestToSend(0);
}

void SerialPort::close()
{
    serialPort->close();
    ui->dtrBox->setChecked(false);
    ui->portNameBox->setEnabled(true); // 允许更改串口
//    autoOpenPortName = "";
}

void SerialPort::setAutoOpen(bool enable)
{
    if(enable && serialPort->isOpen()){
       autoOpenPortName= ui->portNameBox->currentText();
    }else{
       autoOpenPortName="";
    }
}

QByteArray SerialPort::readAll()
{
    return serialPort->readAll();
}

void SerialPort::write(const QByteArray &data)
{
    serialPort->write(data);
}

bool SerialPort::portStatus(QString *string)
{
    bool status;
    static const QString parity[] {
        "None", "Even", "Odd", "Space", "Mark", "Unknown"
    };
    static const QString flowControl[] {
        "None", "RTS/CTS", "XON/XOFF", "Unknown"
    };

    // 获取端口名
    *string = ui->portNameBox->currentText().section(" ", 0, 0) + " ";
    if (*string == " ") { // 端口名是空的
        *string = "COM Port ";
    }
    if (serialPort->isOpen()) {
        string->append("OPEND, " + QString::number(serialPort->baudRate()) + "bps, "
            + QString::number(serialPort->dataBits()) + "bit, "
            + parity[serialPort->parity()] + ", "
            + QString::number(serialPort->stopBits()) + ", "
            + flowControl[serialPort->flowControl()]);
        status = true;
    } else {
        string->append("CLOSED");
        status = false;
    }
    return status;
}

bool SerialPort::isOpen()
{
    return serialPort->isOpen();
}

void SerialPort::portSetDialog()
{
    PortSetBox portSet(serialPort, this);

    portSet.setAutoDTR(autoDTR);
    portSet.exec();
    autoDTR = portSet.autoDTR();
}

// 扫描端口
void SerialPort::scanPort()
{
    bool sync = false;
    QComboBox *box = ui->portNameBox;
    QVector<QSerialPortInfo> vec;

    //查找可用的串口
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        // 检测端口列表变更
        QString str = info.portName() + " (" + info.description() + ")";
        if (box->findText(str) == -1) {
            sync = true;
        }
        vec.append(info);
    }
    // 需要同步或者ui->portNameBox存在无效端口
    if (sync || box->count() != vec.count()) {
        int len = 0;
        QFontMetrics fm(box->font());
        QString text = box->currentText();
        bool edited = box->findText(text) == -1; // only edit enable (linux)
        box->clear();
        for (int i = 0; i < vec.length(); ++i) {
            QString name = vec[i].portName() + " (" + vec[i].description() + ")";
            box->addItem(name);
            int width = fm.boundingRect(name).width(); // 计算字符串的像素宽度
            if (width > len) { // 统计最长字符串
                len = width;
            }
        }
        // 设置当前选中的端口
        if (!text.isEmpty() && (box->findText(text) != -1 || edited)) {
            box->setCurrentText(text);
        } else {
            box->setCurrentIndex(0);
        }
#if defined(Q_OS_LINUX)
        box->lineEdit()->setCursorPosition(0);
#endif
        // 自动控制下拉列表宽度
        box->view()->setMinimumWidth(len + 16);
    }

    if(!serialPort->isOpen() && autoOpenPortName.length()>1){
        if ( (box->findText(autoOpenPortName) != -1 )) {
            box->setCurrentText(autoOpenPortName);
            open();
        }
    }
}

void SerialPort::onTimerUpdate()
{
    if (serialPort->isOpen()) {
        if (serialPort->error() == QSerialPort::NoError) {
            return;
        }
        emit portError();
    }
    scanPort();
}

void SerialPort::setBaudRate(const QString &string)
{
    serialPort->setBaudRate(string.toInt());
}

#if defined(Q_OS_LINUX)
void SerialPort::onPortTextEdited()
{
    QComboBox *box = ui->portNameBox;
    QString text = box->currentText();
    if (box->findText(text) == -1) { // can't find text
        qDebug() << "edit text edited";
    }
    box->lineEdit()->setCursorPosition(0);
}
#endif
