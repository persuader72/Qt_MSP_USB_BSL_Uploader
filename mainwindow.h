#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class libusb_context;
class libusb_device_handle;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
protected:
    void timerEvent(QTimerEvent *);
private slots:
    void on_UploadFirmware_clicked();

    void on_SelectFirmwareFile_clicked();

private:
    void usbDiscovery();
    void usbUploadFullBsl();
    void usbUploadProgram(const QString &filename);
    void usbClose();
private:
    void bslCommandLoadPC(libusb_device_handle *device_handle,int address);
    QByteArray bslCommandTXBSLVersion();
    QByteArray bslCommandRXPassword();
    void bslCommandRXDataBlock(libusb_device_handle *device_handle, int address, const QByteArray &packet, bool fast);
    void bslCommandTXDataBlock(libusb_device_handle *device_handle);
    QByteArray bslReadWrite(libusb_device_handle *device_handle, const QByteArray &packet, bool reply);
private:
    Ui::MainWindow *ui;
    int mTimerId;
    bool mIsFullBsl;
    libusb_context *ctx;
    libusb_device_handle *device_handle;
};

#endif // MAINWINDOW_H
