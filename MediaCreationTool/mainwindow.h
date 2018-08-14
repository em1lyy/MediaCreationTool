#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QStorageInfo>
#include <QtNetwork/QNetworkReply>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QString os_name;
    QString iso_url;
    bool mirrors_aviable;
    QList<QUrl> mirrors;
    QString md5;
    QStorageInfo selectedDriveInfo;
    QList<QStorageInfo> drives;
    QString selectedDrive;

    QNetworkAccessManager manager;

    QList<QStorageInfo> getUsbDrives();
    QStringList storageInfoToString(QList<QStorageInfo> infos);
    void storageCurrentIndexChanged(int index);

    QUrl testDownloadSpeeds();
    bool checkMD5Sum();
    void downloadImage(QUrl imageUrl);
    void prepareDrive(QString mntPoint);
    void writeImage(QFile *isoImage);
    bool saveToDisk(const QString &filename, QIODevice *data);

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void downloaded(QFile *file);
    void creationDone();

private:
    Ui::MainWindow *ui;

private slots:
    void on_btnBegin_released();
    void on_btnStartInstall_released();
    void on_btnExit_released();

    void imageDownloaded(QNetworkReply* pReply);
};

#endif // MAINWINDOW_H
