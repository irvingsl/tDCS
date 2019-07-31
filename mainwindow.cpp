#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <string>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QTextStream>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFile>

#define blankString ""

QDate today = QDate::currentDate();
int totalTime, riseTime;
double amps;
qint64 epoch1, epoch2, epochToday;
bool connected = false;
QString description, manufacturer, serialNumber, vendorId, serialPort;
char* charVendorId;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    MainWindow::initVars();

    ui->statusBar->showMessage("Nenhum dispositivo conectado");
    ui->doubleSpinBox->setValue(amps);
    ui->spinBox->setValue(riseTime);
    ui->spinBox_2->setValue(totalTime);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_2_clicked()//Botão conectar
{
    static bool btn2 = true;
    if(btn2)
    {
        auto printable = QStringLiteral("Dados configurados no dispositivo:\nCorrente: %3 mA\nTempo de subida: %1 minutos\nTempo total: %2 minutos\nData de início: 29/07/2019\nData final: 31/12/2019\n\nSalvo em: 29/07/2019").arg(riseTime).arg(totalTime).arg(amps);
        ui->label_7->setText(printable);
        connected = MainWindow::readUSB();
        if(connected)//&& MainWindow::refreshData())
        {
            ui->statusBar->showMessage("Conectado!");
            ui->pushButton_2->setText("Desconectar");
            btn2 = false;
         }
        else
        {
            ui->statusBar->showMessage("Não foi possível conectar");
            ui->label_7->setText("Não foi possível conectar");
            return;
        }
    }
    else
    {
        ui->label_7->setText("Nenhum dispositivo conectado");
        ui->statusBar->showMessage("Nenhum dispositivo conectado");
        ui->pushButton_2->setText("Conectar");
        connected = false;
                btn2= true;
    }
}

void MainWindow::on_pushButton_3_clicked()//Botão limpar
{
     MainWindow::initVars();
     ui->doubleSpinBox->setValue(amps);
     ui->spinBox->setValue(riseTime);
     ui->spinBox_2->setValue(totalTime);
}

void MainWindow::on_pushButton_4_clicked()//Botão salvar
{
    amps = ui->doubleSpinBox->value();
    riseTime = ui->spinBox->value();
    totalTime = ui->spinBox_2->value();

    QDate myDate1 = ui->dateEdit->date();
    //a = myDate1.toString(Qt::RFC2822Date);
    QDateTime DateTime1 = QDateTime(myDate1);
    epoch1 = DateTime1.toSecsSinceEpoch();

    QDate myDate2 = ui->dateEdit_2->date();
    QDateTime DateTime2 = QDateTime(myDate2);
    epoch2 = DateTime2.toSecsSinceEpoch();
    epochToday = QDateTime::currentSecsSinceEpoch();

    auto serialize = QStringLiteral("%1#%2#%3#%4#%5#%6\n").arg(amps).arg(riseTime).arg(totalTime).arg(epoch1).arg(epoch2).arg(epochToday);

    if(MainWindow::configSerial(serialPort, serialize))
    {
    //auto printable = QStringLiteral("Rise time: %1 Total time: %2 Current: %3\nData inicio: %4 | Data fim: %5 \nEpoch %6").arg(riseTime).arg(totalTime).arg(amps).arg(a).arg(b).arg(epoch2);
    QMessageBox::information(this, "tDCS", "Dados salvos com sucesso!");
    }
    else
    {
    QMessageBox::warning(this, "tDCS", "Não foi possível salvar os dados, favor conferir a conexão.");
    }
}


void MainWindow::initVars()
{
    totalTime = 20;
    riseTime = 5;
    amps = 1.5;
    ui->dateEdit->setDate(today);//Seta a primeira data com o dia de hoje
    ui->dateEdit_2->setDate(today.addMonths(1));//Seta a segunda data pra daqui a 1 mês
}

bool  MainWindow::readUSB()
{

    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &serialPortInfo : serialPortInfos)
    {
        description = serialPortInfo.description();
        manufacturer = serialPortInfo.manufacturer();
        serialNumber = serialPortInfo.serialNumber();
        vendorId = serialPortInfo.vendorIdentifier();

        auto serialScan = QStringLiteral("Description: %1\n  Manufacturer: %2\n    serialNumber: %3 \n vendorId: %4\n").arg(description).arg(manufacturer).arg(serialNumber).arg(vendorId);
        qDebug() << serialScan << endl;

        if(description=="USB-SERIAL CH340" && manufacturer=="wch.cn" && vendorId == "\u0086")
        {
            serialPort = serialPortInfo.portName();
            //qDebug() << endl << serialPort << endl;
            qDebug()  << "Port: " << serialPortInfo.portName() << endl;
            connected = true;
            return true;
        }
    }
    connected = false;
    return false;
}


bool  MainWindow::configSerial(QString port, QString msg)
{

   if(connected)
   {
        qDebug() << endl << msg << endl;
        QSerialPort serialPort;
        const QString serialPortName = port;
        serialPort.setPortName(serialPortName);
        serialPort.setBaudRate(QSerialPort::Baud115200);
        serialPort.setDataBits(QSerialPort::Data8);
        serialPort.setParity(QSerialPort::NoParity);
        serialPort.setStopBits(QSerialPort::OneStop);

        serialPort.open(QIODevice::ReadWrite);
        const qint64 bytesWritten = serialPort.write(msg.toUtf8());

        qDebug()  << QObject::tr("Data written to port %1, error: %2, sizes: written: %3, in msg: %4")
                .arg(serialPortName).arg(serialPort.errorString()).arg(bytesWritten).arg(msg.size()) << endl;

        if (bytesWritten == -1)
        {
            qDebug() << QObject::tr("Failed to write the data to port %1, error: %2")
                .arg(serialPortName).arg(serialPort.errorString()) << endl;
            return false;
        }
        else if (bytesWritten != msg.size())
        {
            qDebug()  << QObject::tr("Failed to write all the data to port %1, error: %2, sizes: written: %3, in msg: %4")
                .arg(serialPortName).arg(serialPort.errorString()).arg(bytesWritten).arg(msg.size()) << endl;
        }
        else
        {
            return true;
        }
    }
    else
    {
        ui->label_7->setText("Nenhum dispositivo conectado");
        ui->statusBar->showMessage("Nenhum dispositivo conectado");
        return false;
    }
}

/*

bool MainWindow::refreshData()
{

    auto printable = QStringLiteral("Dados configurados no dispositivo:\nCorrente: %3 mA\nTempo de subida: %1 minutos\nTempo total: %2 minutos\nData de início: 29/07/2019\nData final: 31/12/2019\n\nSalvo em: 29/07/2019").arg(riseTime).arg(totalTime).arg(amps);
    ui->label_7->setText(printable);

}
*/
