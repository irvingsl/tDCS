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
#include <QThread>
#include <QTimer>


static QDate today = QDate::currentDate();
static int totalTime, riseTime;
static double amps;
static qint64 epoch1, epoch2, epochToday;
static QString description, manufacturer, serialNumber, serialPortId;
static QSerialPort serialPort;
static QByteArray readData;
static quint16 vendorId;

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
        qDebug() << "Conectando" << endl;
        ui->label_7->setText("Conectando...");
        ui->statusBar->showMessage("Conectando...");
        if(MainWindow::connect() && MainWindow::readFromSerial())
        {
            ui->statusBar->showMessage("Conectado!");
            ui->pushButton_2->setText("Desconectar");            
            btn2 = false;
         }
        else
        {
            ui->statusBar->showMessage("Não foi possível conectar");
            ui->label_7->setText("Não foi possível conectar");
            serialPort.close();
            return;
        }
    }
    else
    {

        qDebug() << "Desconectando" << endl;
        serialPort.close();
        ui->label_7->setText("Nenhum dispositivo conectado");
        ui->statusBar->showMessage("Nenhum dispositivo conectado");
        ui->pushButton_2->setText("Conectar");
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
    ui->statusBar->showMessage("Salvando...");
    ui->label_7->setText("Salvando...");
    amps = ui->doubleSpinBox->value();
    riseTime = ui->spinBox->value();
    totalTime = ui->spinBox_2->value();

    QDate myDate1 = ui->dateEdit->date();
    QDateTime DateTime1 = QDateTime(myDate1);
    epoch1 = DateTime1.toSecsSinceEpoch();

    QDate myDate2 = ui->dateEdit_2->date();
    QDateTime DateTime2 = QDateTime(myDate2);
    epoch2 = DateTime2.toSecsSinceEpoch();
    epochToday = QDateTime::currentSecsSinceEpoch();

    auto serialize = QStringLiteral("%1#%2#%3#%4#%5#%6#\n").arg(amps).arg(riseTime).arg(totalTime).arg(epoch1).arg(epoch2).arg(epochToday);
    qDebug() << "Serialize: " << serialize;

    MainWindow::writeToSerial(serialize);
    MainWindow::delay_ms(2000);


    MainWindow::on_pushButton_2_clicked();
    MainWindow::on_pushButton_2_clicked();


    qDebug() << "Dado lido e dado enviado:" << endl << readData << endl << serialize << endl;
    if (readData == serialize)
    {
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

bool  MainWindow::connect()
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

        //if(description=="Virtual Serial Port 9 (Electronic Team)" && manufacturer=="Electronic Team")
        //if(description=="USB-SERIAL CH340" && manufacturer=="wch.cn" && vendorId == "\u0086")
        if(description=="Silicon Labs CP210x USB to UART Bridge" && manufacturer=="Silicon Labs")
        {
            serialPortId = serialPortInfo.portName();
            qDebug()  << "Port: " << serialPortInfo.portName() << endl;
            return(MainWindow::configSerial(serialPortId));
        }
    }

    return false;//To avoid warning (non-void function)
}

bool  MainWindow::configSerial(QString serialPortName)
{
    if(serialPort.isOpen())
    {
        return true;
    }
    else
    {
        serialPort.setPortName(serialPortName);
        serialPort.setBaudRate(QSerialPort::Baud115200);
        serialPort.setDataBits(QSerialPort::Data8);
        serialPort.setParity(QSerialPort::NoParity);
        serialPort.setStopBits(QSerialPort::OneStop);
        serialPort.setFlowControl(QSerialPort::NoFlowControl);
        return serialPort.open(QIODevice::ReadWrite);
    }

}


//TODO: Salvando mesmo quando não há resposta

bool MainWindow::writeToSerial(QString msg)
{
    const qint64 bytesWritten = serialPort.write(msg.toUtf8());

    if (bytesWritten == -1)
    {
        qDebug() << QObject::tr("Failed to write the data to port %1, error: %2")
            .arg(serialPort.portName()).arg(serialPort.errorString()) << endl;
        return false;
    }
    else if (bytesWritten != msg.size())
    {
        qDebug()  << QObject::tr("Failed to write all the data to port %1, error: %2, sizes: written: %3, in msg: %4")
            .arg(serialPort.portName()).arg(serialPort.errorString()).arg(bytesWritten).arg(msg.size()) << endl;
    }
    else
    {
        qDebug() << tr("Message of size %1 succesfully sent.").arg(bytesWritten);
        return true;
    }
    return false; //To avoid warning (non-void function)
}

bool MainWindow::readFromSerial()
{
     MainWindow::writeToSerial("@R\n");
     readData = serialPort.readAll();
     while (serialPort.waitForReadyRead(2000) )
     {
        readData.append(serialPort.readAll());
    }

     if (serialPort.error() == QSerialPort::ReadError) {
         qDebug() << QObject::tr("Failed to read from port %1, error: %2")
                           .arg(serialPort.portName()).arg(serialPort.errorString()) << endl;
         return false;
     } else if (serialPort.error() == QSerialPort::TimeoutError && readData.isEmpty()) {
         qDebug() << QObject::tr("No data was currently available"
                                       " for reading from port %1")
                           .arg(serialPort.portName()) << endl;
         return false;
     }


     qDebug() << QObject::tr("Data successfully received from port %1")
                       .arg(serialPort.portName()) << endl;
     qDebug() << readData << endl;

     return MainWindow::refreshData();
}

bool MainWindow::refreshData()
{
    if(readData.size() < 30)
    {
        return false;
    }
    else
    {
        auto parts = readData.split('#');

        QDateTime parts3, parts4, parts5;
        parts3.setSecsSinceEpoch(parts[3].toInt());
        parts4.setSecsSinceEpoch(parts[4].toInt());
        parts5.setSecsSinceEpoch(parts[5].toInt());

        auto dateStart =  parts3.toString(Qt::SystemLocaleShortDate).split(' ');//Divide as Strings em 2 partes: data/hora
        auto dateEnd =  parts4.toString(Qt::SystemLocaleShortDate).split(' ');

        QString printables = "Dados configurados no dispositivo:\nCorrente: "+parts[0]
               +" mA\nTempo de subida: "+parts[1]
               +" minutos\nTempo total: "+parts[2]
               +" minutos\nData de início: "+dateStart[0]//Utiliza apenas a data
               +"\nData final: "+dateEnd[0]
               +"\n\nSalvo em: "+parts5.toString(Qt::SystemLocaleShortDate);

       ui->label_7->setText(printables);

       return true;
    }
}

void MainWindow::delay_ms(int time = 1000)
{
    QTime dieTime= QTime::currentTime().addMSecs(time);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::delay_s(int time = 1)
{
    MainWindow::delay_ms(1000*time);
}
