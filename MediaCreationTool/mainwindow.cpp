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
#include <adxintrin.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->ui->distro_logo->setPixmap(QPixmap(":/logo.png"));
    QFile inputFile(":/os.conf");
    if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
       qDebug() << "Input file opened.";
       QTextStream in(&inputFile);
       QString line = "";
       if (in.atEnd())
           qDebug() << "File is empty.";
       while (!in.atEnd())
       {
            line = in.readLine();

            if (line.startsWith("#"))
            {
                qDebug() << line << "Comment";
                continue;
            }
            if (line.startsWith("os_name=", Qt::CaseInsensitive))
            {
                this->os_name = line.mid(8);
                qDebug() << line << "OS Name";
            }
            if (line.startsWith("iso_url=", Qt::CaseInsensitive))
            {
                this->iso_url = line.mid(8);
                qDebug() << line << "ISO Url";
            }
            if (line.startsWith("mirrors-aviable=", Qt::CaseInsensitive))
            {
                QString avbl = line.mid(16);
                this->mirrors_aviable = (avbl.compare("True", Qt::CaseInsensitive) == 0);
                qDebug() << line << "Mirror aviability";
            }
            if (line.startsWith("mirrors+=", Qt::CaseInsensitive))
            {
                this->mirrors.append(QUrl::fromUserInput(line.mid(11)));
                qDebug() << line << "Mirror +=";
            }
            if (line.startsWith("md5=", Qt::CaseInsensitive ))
            {
                this->md5 = line.mid(4);
                qDebug() << line << "MD5 Sum";
            }
            if (in.atEnd())
                qDebug() << "At end.";

       }
       qDebug() << os_name << " " << iso_url << " " << mirrors_aviable << " " << mirrors << " " << md5 << endl;
       inputFile.close();
    }
    this->setWindowTitle(os_name + " Media Creation Tool");
    this->setWindowIcon(QPixmap(":/logo.png"));
    this->ui->lblIntroduction->setText(this->ui->lblIntroduction->text().replace("%OS%", this->os_name));
    this->ui->lblSelectUsb->setText(this->ui->lblSelectUsb->text().replace("%OS%", this->os_name));
    connect(this->ui->selectDrive, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::storageCurrentIndexChanged);
    connect(&manager, &QNetworkAccessManager::finished, this, &MainWindow::imageDownloaded);
}

void MainWindow::on_btnBegin_released()
{
    this->drives = this->getUsbDrives();
    this->ui->selectDrive->addItems(this->storageInfoToString(this->drives));
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
        this->ui->lblCurrentAction->setText(this->ui->lblCurrentAction->text().replace("%OS%", this->os_name));
        this->ui->stackedWidget->setCurrentIndex(2);
        this->update();
        this->repaint();
        QUrl fastestUrl = this->testDownloadSpeeds();
        this->ui->pbStatus->setValue(3);
        this->update();
        this->repaint();
        this->prepareDrive(this->selectedDriveInfo.rootPath());
        this->ui->lblCurrentAction->setText(tr("<html><head/><body><p><span style=\" font-size:20pt; font-weight:600; text-decoration: underline;\">Downloading... (Step 2 of 3)</span></p><p><span style=\" font-size:14pt;\">Please wait while the %OS%<br/>Media Creation Tool is downloading the ISO Image...</span></p></body></html>").replace("%OS%", this->os_name));
        this->update();
        this->repaint();
        connect(this, &MainWindow::downloaded, this, &MainWindow::writeImage);
        this->downloadImage(fastestUrl);
    }
    else
    {
        this->close();
    }
}

void MainWindow::on_btnExit_released()
{
    this->close();
}

QList<QStorageInfo> MainWindow::getUsbDrives()
{
    QList<QStorageInfo> driveData;

    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes())
    {
        if (storage.isValid())
        {
            driveData.append(storage);
        }
    }

    return driveData;
}

QStringList MainWindow::storageInfoToString(QList<QStorageInfo> infos)
{
    QString driveInfo;
    QStringList driveData;

    foreach (const QStorageInfo &storage, infos)
    {
        if (storage.isValid())
        {
            driveInfo = storage.displayName();
            driveInfo.append(" - ");
            driveInfo.append(QString::number(((float)storage.bytesTotal() / 1024 / 1024 / 1024),'f' ,2));
            driveInfo.append(" GB");
            driveData.append(driveInfo);
        }
    }
    qDebug() << driveData;

    return driveData;
}

void MainWindow::storageCurrentIndexChanged(int index)
{
    this->selectedDriveInfo = this->drives.at(index);
    this->selectedDrive = this->selectedDriveInfo.displayName() + " - " + QString::number(((float)this->selectedDriveInfo.bytesTotal() / 1024 / 1024 / 1024),'f' ,2) + " GB";
    qDebug() << this->selectedDrive;
}

QUrl MainWindow::testDownloadSpeeds()
{
    /* TEST MAIN URL */
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
   }

   /* TEST MIRROR URLS */
   QUrl fastestUrl = QUrl(this->iso_url);
   float fastestPing = isoping_s.toFloat();
   QUrl currentUrl;
   float currentPing;
   if (mirrors_aviable)
   {
       foreach (currentUrl, this->mirrors) {
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
          parameters2 << currentUrl.host();
          QProcess *mirrorping_p = new QProcess();
          mirrorping_p->start("ping", parameters2);
          mirrorping_p->waitForFinished();
          int exitCode2 = mirrorping_p->exitCode();
          QString mirrorping_s = "";
          if (exitCode2 == 0) {
              if (win)
              {
                  if (mirrorping_s.indexOf("ms", 0, Qt::CaseInsensitive) == -1)
                  {
                      continue;
                  }
              }
              else
              {
                  if (mirrorping_s.indexOf(" ms", 0, Qt::CaseInsensitive) == -1)
                  {
                      continue;
                  }
              }
              mirrorping_s = QString(mirrorping_p->readAllStandardOutput());
              if (win)
                  mirrorping_s = mirrorping_s.left(mirrorping_s.indexOf("ms", 0, Qt::CaseInsensitive));
              else
                  mirrorping_s = mirrorping_s.left(mirrorping_s.indexOf(" ms", 0, Qt::CaseInsensitive));
              mirrorping_s = mirrorping_s.right(mirrorping_s.length() - (mirrorping_s.lastIndexOf("=") + 1));
              currentPing = mirrorping_s.toFloat();
              if (currentPing < fastestPing)
              {
                  fastestUrl = currentUrl;
                  fastestPing = currentPing;
              }
              qDebug() << "Mirror Ping (ms): " << mirrorping_s;
          }
       }
   }


  /* CALCULATE RESULT */
  if (fastestUrl.isEmpty())
  {
      QMessageBox *error = new QMessageBox(this);
      error->setWindowTitle(tr("Cannot Reach Download Servers"));
      error->setText(tr("ERROR:\nBoth the main download server and the mirror download server\ncan't be reached at this point of time.\nPlease try again later."));
      error->addButton(QMessageBox::Ok);
      error->exec();
      this->close();
  }
  else
  {
      return fastestUrl;
  }
}

void MainWindow::prepareDrive(QString mntPoint)
{
#if defined(Q_OS_WIN)
    QProcess *umount_p = new QProcess();
    umount_p->start("./dd.exe --umount=" + mntPoint);
    umount_p->waitForFinished();
    this->ui->pbStatus->setValue(10);
    this->update();
    this->repaint();
#else
    QProcess *findmnt_p = new QProcess();
    qDebug() << mntPoint << " " << mntPoint.left(1) << " " << mntPoint.replace(" ", "\ ");
    if (mntPoint.left(1) == "/")
    {
        qDebug() << "findmnt cmd 1";
        findmnt_p->start("findmnt -nr -o source -m \"" + mntPoint.replace(" ", "\ ") + "\"");
    }
    else
    {
        qDebug() << "findmnt cmd 2";
        findmnt_p->start("findmnt -nr -o source -T LABEL=" + mntPoint.replace(" ", "\    "));
    }
    findmnt_p->waitForFinished();
    QString mntpnt = QString(findmnt_p->readAllStandardOutput());
    QString partition = QString(mntpnt);
    qDebug() << mntpnt;
    mntpnt = mntpnt.mid(5);
    mntpnt = mntpnt.left(3);
    qDebug() << mntpnt;
    this->selectedDrive = "/dev/" + mntpnt;
    qDebug() << this->selectedDrive;
    this->ui->pbStatus->setValue(6);
    this->update();
    this->repaint();
    QProcess *umount_p = new QProcess();
    umount_p->start("umount " + this->selectedDrive);
    umount_p->waitForFinished();
    this->ui->pbStatus->setValue(8);
    this->update();
    this->repaint();
    QProcess *umount_p2 = new QProcess();
    umount_p2->start("umount " + partition);
    umount_p2->waitForFinished();
    this->ui->pbStatus->setValue(10);
    this->update();
    this->repaint();
#endif
}

bool MainWindow::checkMD5Sum()
{
    QProcess *md5_p = new QProcess();
    md5_p->start("md5sum ./os-image.iso");
    md5_p->waitForFinished();
    QString md5_s = md5_p->readAllStandardOutput();
    md5_s = md5_s.left(md5_s.indexOf(" ./os-image.iso"));
    qDebug() << "MD5 Check: Expected: " << this->md5 << ", Read: " << md5_s;
    return (this->md5.compare(md5_s, Qt::CaseInsensitive) == 0);
}

void MainWindow::downloadImage(QUrl imageUrl)
{
    QNetworkRequest request(imageUrl);
    QNetworkReply *reply = manager.get(request);
}

void MainWindow::imageDownloaded(QNetworkReply *pReply)
{
    QUrl url = pReply->url();
    if (pReply->error()) {
        QMessageBox *error = new QMessageBox(this);
        error->setWindowTitle(tr("Download of Image failed"));
        error->setText(tr("ERROR:\nThe download of the image failed.\nPlease try again later.\nIf you don't see this error for the first time,\nplease contact us at support@jagudev.net\nand provide the following error message:\n") + pReply->errorString());
        error->addButton(QMessageBox::Ok);
        error->exec();
        this->close();
    } else {
        QString filename = "./os-image.iso";
        if (saveToDisk(filename, pReply)) {
            this->ui->pbStatus->setValue(54);
            this->update();
            this->repaint();
            bool valid = this->checkMD5Sum();
            this->ui->pbStatus->setValue(60);
            this->update();
            this->repaint();
            if (!valid)
            {
                QMessageBox *verror = new QMessageBox(this);
                verror->setWindowTitle(tr("Image file invalid"));
                verror->setText(tr("ERROR:\nThe download of the image failed.\nPlease try again later.\nIf you don't see this error for the first time,\nplease contact us at support@jagudev.net\nand provide the following error message:\n") + pReply->errorString());
                verror->addButton(QMessageBox::Ok);
                verror->exec();
                this->close();
            }
            emit downloaded(new QFile(filename));
        }
    }
}

bool MainWindow::saveToDisk(const QString &filename, QIODevice *data)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox *error = new QMessageBox(this);
        error->setWindowTitle(tr("Download of Image failed"));
        error->setText(tr("ERROR:\nThe download of the image failed.\nPlease try again later.\nIf you don't see this error for the first time,\nplease contact us at support@jagudev.net\nand provide the following error message:\njd-mct_403: Something's wrong with this computer's file permissions. Really Wrong."));
        error->addButton(QMessageBox::Ok);
        error->exec();
        this->close();
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

void MainWindow::writeImage(QFile *isoImage)
{
    this->ui->lblCurrentAction->setText(tr("<html><head/><body><p><span style=\" font-size:20pt; font-weight:600; text-decoration: underline;\">Writing... (Step 3 of 3)</span></p><p><span style=\" font-size:14pt;\">Please wait while the %OS%<br/>Media Creation Tool is writing the<br />Image to the Media...</span></p></body></html>").replace("%OS%", this->os_name));
    this->update();
    this->repaint();
#if defined(Q_OS_WIN)
    QProcess *dd_p = new QProcess();
    dd_p->start("./dd.exe bs=4M if=./os-image.iso of=" + this->selectedDriveInfo.rootPath());
    dd_p->waitForFinished();
    this->ui->pbStatus->setValue(100);
    this->update();
    this->repaint();
#elif defined(Q_OS_MACOS)
    QProcess *dd_p = new QProcess();
    dd_p->start("osascript -e do shell script \"dd bs=4M if=./os-image.iso of=" + this->selectedDrive + "\" with administrator privileges");
    dd_p->waitForFinished();
    this->ui->pbStatus->setValue(100);
    this->update();
    this->repaint();
#else
    QProcess *dd_p = new QProcess();
    qDebug() << "pkexec dd bs=4M if=./os-image.iso of=" + this->selectedDrive;
    dd_p->start("pkexec dd bs=4M if=./os-image.iso of=" + this->selectedDrive);
    dd_p->waitForFinished();
    this->ui->pbStatus->setValue(100);
    this->update();
    this->repaint();
#endif
    this->ui->lblDone->setText(this->ui->lblDone->text().replace("%OS%", this->os_name));
    this->ui->stackedWidget->setCurrentIndex(3);
}

MainWindow::~MainWindow()
{
    delete ui;
}

