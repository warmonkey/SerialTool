#ifndef __PORTSETBOX_H
#define __PORTSETBOX_H

#include <QtWidgets/QDialog>
#include <QtSerialPort/QSerialPort>

namespace Ui {
class PortSetBox;
}

class PortSetBox : public QDialog {
    Q_OBJECT

public:
    PortSetBox(QSerialPort *port, QWidget *parent = Q_NULLPTR);
    ~PortSetBox();

    bool autoDTR();

public slots:
    void setAutoDTR(bool on);

private slots:
    void setDataBits(int index);
    void setParity(int index);
    void setStopBits(int index);
    void setFlowControl(int index);

private:
    Ui::PortSetBox *ui;
    QSerialPort *serialPort;
};

#endif
