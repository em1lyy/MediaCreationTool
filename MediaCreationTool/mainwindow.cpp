#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPixmap>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QStorageInfo>
#include <QMessageBox>
#include <iostream>
#include <QtNetwork>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ui->distro_logo->setPixmap(QPixmap(":/logo.png"));
    QFile inputFile(":/os.conf");
    if (inputFile.open(QIODevice::ReadOnly))
    {
       QTextStream in(&inputFile);
       this->os_name = in.readLine().mid(8);
       this->iso_url = in.readLine().mid(8);
       this->iso_mirror = in.readLine().mid(11);
       this->md5 = in.readLine().mid(4);
       qDebug() << os_name << " " << iso_url << " " << iso_mirror << " " << md5 << endl;
       inputFile.close();
    }
    this->setWindowTitle(os_name + " Media Creation Tool");
    this->setWindowIcon(QPixmap(":/logo.png"));
    this->ui->lblIntroduction->setText(QString("<html><head/><body><p><span style=\" font-size:20pt; font-weight:600; text-decoration: underline;\">%OS% Media Creation Tool</span></p><p><span style=\" font-size:14pt;\">Welcome to the %OS% Media Creation Tool!<br/>This program will help you create a<br/>%OS% Boot Medium!</span></p><p>Please click on \"Begin\".</p><p>Program by <a href=\"https://jagudev.net\"><span style=\" text-decoration: underline; color:#2980b9;\">JaguDev</span></a>.</p></body></html>").replace("%OS%", this->os_name));
    this->ui->lblSelectUsb->setText(this->ui->lblSelectUsb->text().replace("%OS%", this->os_name));
}

void MainWindow::on_btnBegin_released()
{
    this->ui->selectDrive->addItems(getUsbDrives());
    this->ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::on_btnStartInstall_released()
{
    QMessageBox *warning = new QMessageBox(this);
    warning->setWindowTitle(tr("Data Loss Warning"));
    warning->setText(tr("WARNING:\nThis action will erase all Data on the selected Drive.\nWe are not responsible for any kind of Data Loss that may occur.\nContinue?"));
    warning->addButton(QMessageBox::Yes);
    warning->addButton(QMessageBox::No);
    int result = warning->exec();
    if (result == QMessageBox::Yes)
    {
        this->ui->stackedWidget->setCurrentIndex(2);
        bool isMirrorFaster = this->testDownloadSpeeds();
        //QFile isoImageFile = downloadImage(isMirrorFaster);
        //this->writeImage(isoImageFile);
    }
    else
    {
        this->close();
    }
}

QStringList MainWindow::getUsbDrives()
{
    QString driveInfo;
    QStringList driveData;

    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid()) {
            driveInfo = storage.displayName();
            driveInfo.append(" - ");
            driveInfo.append(QString::number(((float)storage.bytesTotal() / 1024 / 1024 / 1024),'f' ,2));
            driveInfo.append(" GB");
            driveInfo.append(" - ");
            driveInfo.append(storage.fileSystemType());
            driveData.append(driveInfo);
        }
    }
    qDebug() << driveData;

    return driveData;
}

bool MainWindow::testDownloadSpeeds()
{
    /* TEST MAIN URL */
    bool iso_url_dead = false;
    QStringList parameters;
    bool win = false;
#if defined(Q_OS_WIN)
    parameters << "-n" << "1";
    qDebug() << "OS is Windows";
    win = true;
#else
    parameters << "-c 1";
    qDebug() << "OS is not Windows";
#endif
   parameters << QUrl(this->iso_url).host();
   QProcess *isoping_p = new QProcess();
   isoping_p->start("ping", parameters);
   isoping_p->waitForFinished();
   int exitCode = isoping_p->exitCode();
   QString isoping_s = "";
   if (exitCode == 0) {
       // it's alive, get result
       QString isoping_s = QString(isoping_p->readAllStandardOutput());
       if (win)
           isoping_s = isoping_s.left(isoping_s.indexOf("ms", 0, Qt::CaseInsensitive));
       else
           isoping_s = isoping_s.left(isoping_s.indexOf(" ms", 0, Qt::CaseInsensitive));
       isoping_s = isoping_s.right(isoping_s.length() - (isoping_s.lastIndexOf("=") + 1));
       qDebug() << "Main URL Ping (ms): " << isoping_s;
   } else {
       // it's dead, set iso_url_dead to true
       iso_url_dead = true;
   }

   /* TEST MIRROR URL */
   bool mirror_url_dead = false;
   QStringList parameters2;
   win = false;
#if defined(Q_OS_WIN)
   parameters2 << "-n" << "1";
   qDebug() << "OS is Windows";
   win = true;
#else
   parameters2 << "-c 1";
   qDebug() << "OS is not Windows";
#endif
  parameters2 << QUrl(this->iso_mirror).host();
  QProcess *mirrorping_p = new QProcess();
  mirrorping_p->start("ping", parameters2);
  mirrorping_p->waitForFinished();
  int exitCode2 = isoping_p->exitCode();
  QString mirrorping_s = "";
  if (exitCode2 == 0) {
      // it's alive, get result
      QString mirrorping_s = QString(mirrorping_p->readAllStandardOutput());
      if (win)
          mirrorping_s = mirrorping_s.left(mirrorping_s.indexOf("ms", 0, Qt::CaseInsensitive));
      else
          mirrorping_s = mirrorping_s.left(mirrorping_s.indexOf(" ms", 0, Qt::CaseInsensitive));
      mirrorping_s = mirrorping_s.right(mirrorping_s.length() - (mirrorping_s.lastIndexOf("=") + 1));
      qDebug() << "Mirror Ping (ms): " << mirrorping_s;
  } else {
      // it's dead, set mirror_url_dead to true
      mirror_url_dead = true;
  }

  /* CALCULATE RESULT */
  if (iso_url_dead && mirror_url_dead)
  {
      QMessageBox *error = new QMessageBox(this);
      error->setWindowTitle(tr("Cannot Reach Download Servers"));
      error->setText(tr("ERROR:\nBoth the main download server and the mirror download server\ncan't be reached at this point of time.\nPlease try again later."));
      error->addButton(QMessageBox::Ok);
      error->exec();
      this->close();
  }
  if (iso_url_dead && (!mirror_url_dead))
  {
      return true;
  }
  if ((!iso_url_dead) && mirror_url_dead)
  {
      return false;
  }
  float isoping = isoping_s.toFloat();
  float mirrorping = mirrorping_s.toFloat();
  if (isoping <= mirrorping)
  {
      return false;
  }
  else
  {
      return true;
  }
}

void MainWindow::writeImage(QFile isoImage)
{

}

MainWindow::~MainWindow()
{
    delete ui;
}
