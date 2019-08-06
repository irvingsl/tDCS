/**
 * @file mainwindow.cpp
 * @author Irving Souza Lima (irvingsslima@gmail.com)
 * @brief Código principal do QT, controla os botões e principalmente a 
 * comunicação serial com o dispositivo conectado via USB
 * @version 0.1
 * @date 2019-08-02
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QCoreApplication>
#include <QTextStream>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>

static QDate today = QDate::currentDate();
static int totalTime, riseTime;
static double amps;
static qint64 epoch1, epoch2, epochToday;
static QString description, manufacturer, serialNumber, serialPortId;
static QSerialPort serialPort;
static QByteArray readData;
static quint16 vendorId;
/**
 * @brief Construct a new Main Window:: Main Window object
 * 
 * @param parent 
 */
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

/**
 * @brief Destroy the Main Window:: Main Window object
 * 
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief Quando o botão 2 (Conectar/Desconectar) da UI é clicado, essa função é executada
 * 
 * Ela troca as informações na tela, e executa as funções de conectar e ler do dispositivo.
 * Em caso positivo, ela troca o texto do botão para desconectar, e assim fecha a conexão serial.
 * 
 */
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

/**
 * @brief Quando o botão 3 (Limpar) da UI é clicado, essa função é executada
 * 
 * Executa a função initVars, que carrega os valores iniciais pras variáveis e os atualiza nos respectivos campos
 * 
 */
void MainWindow::on_pushButton_3_clicked()//Botão limpar
{
     MainWindow::initVars();
     ui->doubleSpinBox->setValue(amps);
     ui->spinBox->setValue(riseTime);
     ui->spinBox_2->setValue(totalTime);
}

/**
 * @brief Quando o botão 3 (Salvar) da UI é clicado, essa função é executada
 * 
 * Ela envia os valores dos campos para o device, espera uma resposta 
 * e atualiza os respectivos campos em caso da resposta ser a esperada
 * 
 */
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
    MainWindow::on_pushButton_2_clicked();//desconecta e conecta novamente

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

/**
 * @brief Define valores iniciais pras variáveis que são utilizadas na UI
 * 
 * É utilizada principalmente para o funcionamento do botão Limpar,
 * trazendo as variáveis para valores padrões
 * 
 */
void MainWindow::initVars()
{
    totalTime = 20;
    riseTime = 5;
    amps = 1.5;
    ui->dateEdit->setDate(today);//Seta a primeira data com o dia de hoje
    ui->dateEdit_2->setDate(today.addMonths(1));//Seta a segunda data pra daqui a 1 mês
}

/**
 * @brief Busca o dispositvo correto e estabelece conexão serial com ele
 * 
 * Pesquisa dentre os dispositivos conectados os que atendem aos requisitos
 * como nome e fabricante, para assim definir a porta em que está conectado
 * 
 * 
 * @return configSerial função que estabelece a conexão com o dispositivo
 */
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

/**
 * @brief Função que define os parâmetros e faz a conexão serial
 * 
 * @param serialPortName Identificação da porta em que o dispositivo está conectado
 * @return true Em caso de conexão bem sucedida com o device
 * @return false Caso alguma falha ocorra na conexão
 */
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

/**
 * @brief Função escreve uma string na porta serial e verifica se todos os bytes foram escritos
 * 
 * @param msg String que será enviada pela porta serial
 * @return true Caso todos os bytes sejam colocados na serial
 * @return false Caso a string esteja vazia, ou caso algum byte falhe
 */
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

/**
 * @brief Função que envia um request (@R) para que o dispositivo informe sua configuração
 * 
 * O dispositivo está programado para quando receber esse comando (@R) enviar a configuração armazenada
 * no padrão: corrente#tempo_de_subida#tempo_total#data_inicio#data_fim#data_e_hora_atuais#
 * o delimitador é a tralha ('#') e os formatos são int#double#double#epoch#epoch#epoch#
 * 
 * @return true Caso haja mensagem seja recebida
 * @return false Caso não haja mensagem recebida ou estoure o tempo de timeout (2 segundos)
 */
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

/**
 * @brief Função que atualiza o campo central da tela, com os valores do dispositivo
 * 
 * @return true Caso a mensagem seja válida, e os valores atualizados
 * @return false Caso a mensagem seja menor que 30 caracteres, configurando um erro
 */
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

/**
 * @brief Função auxiliar que da um delay de milissegundos
 * 
 * É utilizada para esperar que o device seja capaz de comunicar com o software
 * 
 * @param time Define o número de milissegundos do delay
 */
void MainWindow::delay_ms(int time)//Função auxiliar para delay em milissegundos
{
    QTime dieTime= QTime::currentTime().addMSecs(time);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

/**
 * @brief Função auxiliar que da um delay de segundos
 * 
 * @param time Define o número de segundos do delay
 */
void MainWindow::delay_s(int time)//Função auxiliar para delay em segundos
{
    MainWindow::delay_ms(1000*time);
}
