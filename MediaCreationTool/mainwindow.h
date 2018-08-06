#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFile>
#include <QStringList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QString os_name;
    QString iso_url;
    QString iso_mirror;
    QString md5;

    QStringList getUsbDrives();
    bool testDownloadSpeeds();
    QFile downloadImage(bool useMirror);
    void writeImage(QFile isoImage);

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void on_btnBegin_released();
    void on_btnStartInstall_released();
};

#endif // MAINWINDOW_H
