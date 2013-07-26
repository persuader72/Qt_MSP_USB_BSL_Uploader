#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qintelhexparser.h"
#include <libusb-1.0/libusb.h>

#include <QDebug>
#include <QFile>

#include <unistd.h>

#define CONTROL_REQUEST_TYPE_IN LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE
#define CONTROL_REQUEST_TYPE_OUT LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE

#define HID_GET_REPORT 0x01
#define HID_SET_REPORT 0x09
#define HID_REPORT_TYPE_INPUT 0x01
#define HID_REPORT_TYPE_OUTPUT 0x02
#define HID_REPORT_TYPE_FEATURE 0x03

#define INTERFACE_NUMBER 0
#define TIMEOUT_MS 5000


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::MainWindow),device_handle(0) {
    ui->setupUi(this);

    int r;
    if((r=libusb_init(&ctx))<0) {
        qDebug("MainWindow::MainWindow libusb_init error");
        ui->DeviceStatus->setText(tr("error on libusb_init")+QString(libusb_error_name(r)));
    } else {
    }

    mTimerId = startTimer(500);
}

MainWindow::~MainWindow() {
    delete ui;
    libusb_exit(ctx);
}

void MainWindow::timerEvent(QTimerEvent *) {
    usbDiscovery();
}

void MainWindow::usbDiscovery() {
    int i,r;
    ssize_t device_count;
    libusb_device  **device_list; //to store all found USB devices
    device_count = libusb_get_device_list(ctx, &device_list);
    if((r=device_count<0)) {
        qDebug("MainWindow::MainWindow libusb_get_device_list no device found");
        ui->DeviceStatus->setText(tr("error on libusb_get_device_list")+QString(libusb_error_name(r)));
    } else {
        for(i=0;i<device_count;i++) {
            libusb_device_descriptor device_descriptor;
            if(libusb_get_device_descriptor(device_list[i], &device_descriptor)<0) {
                 qDebug("MainWindow::MainWindow libusb_get_device_descriptor error");
            } else {
                if(device_descriptor.idVendor==0x2047 && device_descriptor.idProduct==0x0200) {
                    if(!device_handle) {
                        if((r=libusb_open(device_list[i],&device_handle))<0) {
                             ui->DeviceStatus->setText(tr("error on libusb_open")+QString(libusb_error_name(r)));
                        } else {
                            libusb_detach_kernel_driver(device_handle, 0);
                            if((r=libusb_claim_interface(device_handle, 0))<0) {
                                ui->DeviceStatus->setText(tr("error on libusb_claim_interface")+QString(libusb_error_name(r)));
                                usbClose();
                            } else {
                                ui->DeviceStatus->setText(tr("USB BSL device OPENED!!!!!"));
                                ui->ProgressStatus->appendPlainText(tr("[>>] Request BSL version"));
                                QByteArray r=bslCommandTXBSLVersion();
                                if(r.size()==0) {
                                    ui->ProgressStatus->appendPlainText(tr("[??] Comunication error"));
                                } else if(r.at(0)==0x3B) {
                                     ui->ProgressStatus->appendPlainText(tr("[<<] limited BSL version"));
                                     ui->ProgressStatus->appendPlainText(tr("[>>] Send unlock password"));
                                     r=bslCommandRXPassword();
                                     if(r.size()==0) {
                                         ui->ProgressStatus->appendPlainText(tr("[??] Comunication error"));
                                     } else if(r.size()>1 && r.at(0)==0x3B && r.at(1)==0x00) {
                                        ui->ProgressStatus->appendPlainText(tr("[<<] BSL unlock successfull"));
                                        usbUploadFullBsl();
                                     }
                                } else if(r.at(0)==0x3A) {
                                    ui->ProgressStatus->appendPlainText(tr("[<<] BSL version: ")+r.mid(1).toHex());
                                    usbUploadProgram();
                                }
                            }
                        }
                    }

                    break;
                }
            }
        }
        if(i==device_count) {
            ui->DeviceStatus->setText(tr("No USB BSL detected!!!"));
        }
        libusb_free_device_list(device_list,true);
    }
}

void MainWindow::usbUploadFullBsl() {
    ui->ProgressStatus->appendPlainText(tr("[>>] Send full BSL"));
    QFile hexfile1(":/firmware/ram_bsl.txt");
    if(hexfile1.open(QIODevice::ReadOnly)) {
        QIntelHexParser parser;
        parser.parseTxtFile(hexfile1);
        for(int i=0;i<parser.segments().size();i++) {
            QIntelHexMemSegment seg = parser.segments().at(i);
            qDebug("MainWindow::MainWindow segment at %04x of %d bytes",seg.address,seg.memory.size());
            for(int offs=0; offs<seg.memory.size(); offs +=52) {
                QByteArray mid = seg.memory.mid(offs,52);
                bslCommandRXDataBlock(device_handle,seg.address+offs,mid,true);
            }
        }
        ui->ProgressStatus->appendPlainText(tr("[>>] Restart BSL"));
        bslCommandLoadPC(device_handle,0x2504);
    } else {

    }
}

void MainWindow::usbUploadProgram() {
    ui->ProgressStatus->appendPlainText(tr("[>>] Send program file"));
    QFile hexfile("/home/stefano/colibrifw.hex");
    if(hexfile.open(QIODevice::ReadOnly)) {
        QIntelHexParser parser;
        parser.parseFile(hexfile);
        for(int i=0;i<parser.segments().size();i++) {
            QIntelHexMemSegment seg = parser.segments().at(i);
            qDebug("MainWindow::MainWindow segment at %04x of %d bytes",seg.address,seg.memory.size());
            for(int offs=0; offs<seg.memory.size(); offs +=52) {
                QByteArray mid = seg.memory.mid(offs,52);
                bslCommandRXDataBlock(device_handle,seg.address+offs,mid,false);
            }
        }
    } else {

    }

    ui->ProgressStatus->appendPlainText(tr("[>>] Restart MSP"));
    bslCommandLoadPC(device_handle,0x8004);
}

void MainWindow::usbClose() {
    if(device_handle) {
        libusb_release_interface(device_handle, 0);
        libusb_close(device_handle);
        device_handle = 0;
    }

}

void MainWindow::bslCommandLoadPC(libusb_device_handle *device_handle, int address) {
    QByteArray pkt(6,0);
    pkt[0] = 0x3F;
    pkt[1] = 0x04;
    pkt[2] = 0x17;
    pkt[3] = address & 0xFF;
    pkt[4] = (address >> 8) & 0xFF;
    pkt[5] = 0x00;

    QByteArray r = bslReadWrite(device_handle,pkt,false);
}

QByteArray MainWindow::bslCommandTXBSLVersion() {
    QByteArray pkt(35,0xFF);
    pkt[0] = 0x3F;
    pkt[1] = 0x01;
    pkt[2] = 0x19;

    return bslReadWrite(device_handle,pkt,true);
}

QByteArray MainWindow::bslCommandRXPassword() {
    QByteArray pkt(35,0xFF);
    pkt[0] = 0x3F;
    pkt[1] = 33;
    pkt[2] = 0x11;

    return bslReadWrite(device_handle,pkt,true);
}

void MainWindow::bslCommandRXDataBlock(libusb_device_handle *device_handle,int address, const QByteArray &packet,bool fast) {
    QByteArray pkt(6+packet.size(),0);
    pkt[0] = 0x3F;
    pkt[1] = packet.size()+4;
    pkt[2] = fast ? 0x1B : 0x10;
    pkt[3] = address & 0xFF;
    pkt[4] = (address >> 8) & 0xFF;
    pkt[5] = 0x00;
    for(int i=0;i<packet.size();i++) pkt[6+i] = packet.at(i);

    qDebug("MainWindow::bslCommandRXDataBlock %04x %d",address,packet.size());
    QByteArray r = bslReadWrite(device_handle,pkt,!fast);
}

void MainWindow::bslCommandTXDataBlock(libusb_device_handle *device_handle) {
    QByteArray pkt(7,0);
    pkt[0] = 0x3F;
    pkt[1] = 0x05;
    pkt[2] = 0x18;
    pkt[3] = 0x00;
    pkt[4] = 0x80;
    pkt[5] = 0x00;
    pkt[6] = 0x32;
    pkt[7] = 0x00;

    QByteArray r = bslReadWrite(device_handle,pkt,true);
}

QByteArray MainWindow::bslReadWrite(libusb_device_handle *device_handle,const QByteArray &packet,bool reply) {
    int r;

    static const int INTERRUPT_IN_ENDPOINT = 0x81;
    static const int INTERRUPT_OUT_ENDPOINT = 0x01;

    static const int MAX_INTERRUPT_IN_TRANSFER_SIZE = 64;
    static const int MAX_INTERRUPT_OUT_TRANSFER_SIZE = 64;

    unsigned char data_in[MAX_INTERRUPT_IN_TRANSFER_SIZE];
    unsigned char data_out[MAX_INTERRUPT_IN_TRANSFER_SIZE];

    int bytes_transferred;
    memcpy(data_out,packet.constData(),packet.size());

    r = libusb_interrupt_transfer(
                device_handle,
                INTERRUPT_OUT_ENDPOINT,
                data_out,
                MAX_INTERRUPT_OUT_TRANSFER_SIZE,
                &bytes_transferred,
                TIMEOUT_MS);

    if(r < 0) {
        qDebug("MainWindow::MainWindow libusb_control_transfer error %d %s",r,libusb_error_name(r));
    } else {
        if(reply) {
            r = libusb_interrupt_transfer(
                            device_handle,
                            INTERRUPT_IN_ENDPOINT,
                            data_in,
                            MAX_INTERRUPT_OUT_TRANSFER_SIZE,
                            &bytes_transferred,
                            TIMEOUT_MS);
            if(r < 0) {
                qDebug("MainWindow::MainWindow libusb_control_transfer error %d %s",r,libusb_error_name(r));
            } else {
                QByteArray hex((const char *)data_in,MAX_INTERRUPT_IN_TRANSFER_SIZE);

                if(hex.size()>2 && hex.at(0)==0x3F) {
                    int len = hex.at(1);
                    hex = hex.mid(2,len);
                }

                qDebug("%s",hex.toHex().constData());
                return hex;
            }
        }
    }
    return QByteArray();
}


/*
    if(libusb_init(&ctx)<0) {
        qDebug("MainWindow::MainWindow libusb_init error");
    } else {
        ssize_t device_count;
        libusb_device  **device_list; //to store all found USB devices
        device_count = libusb_get_device_list(ctx, &device_list);
        if(device_count<=0) {
            qDebug("MainWindow::MainWindow libusb_get_device_list no device found");
        } else {
            for(int i=0;i<device_count;i++) {
                libusb_device_descriptor device_descriptor;
                if(libusb_get_device_descriptor(device_list[i], &device_descriptor)<0) {
                     qDebug("MainWindow::MainWindow libusb_get_device_descriptor error");
                } else {
                    if(device_descriptor.idVendor==0x2047 && device_descriptor.idProduct==0x0200) {
                        qDebug("MainWindow::MainWindow found device %04X:%04X",device_descriptor.idVendor,device_descriptor.idProduct);

                        libusb_device_handle *device_handle;
                        if((r=libusb_open(device_list[i],&device_handle))<0) {
                            qDebug("MainWindow::MainWindow libusb_open error %d %s",r,libusb_error_name(r));
                        } else {
                            libusb_detach_kernel_driver(device_handle, 0);
                            if((r=libusb_claim_interface(device_handle, 0))<0) {
                                qDebug("MainWindow::MainWindow libusb_claim_interface error %d %s",r,libusb_error_name(r));
                            } else {
                                bslCommandTXBSLVersion(device_handle);
                                bslCommandRXPassword(device_handle);
                                //bslCommandTXDataBlock(device_handle);

                                QFile hexfile1(":/firmware/ram_bsl.txt");
                                if(hexfile1.open(QIODevice::ReadOnly)) {
                                    QIntelHexParser parser;
                                    parser.parseTxtFile(hexfile1);
                                    for(int i=0;i<parser.segments().size();i++) {
                                        QIntelHexMemSegment seg = parser.segments().at(i);
                                        qDebug("MainWindow::MainWindow segment at %04x of %d bytes",seg.address,seg.memory.size());
                                        for(int offs=0; offs<seg.memory.size(); offs +=52) {
                                            QByteArray mid = seg.memory.mid(offs,52);
                                            bslCommandRXDataBlock(device_handle,seg.address+offs,mid,true);
                                        }
                                    }
                                } else {

                                }


                                bslCommandLoadPC(device_handle,0x2504);

                                libusb_close(device_handle);
                                sleep(3);
                                device_handle = libusb_open_device_with_vid_pid(ctx,0x2047,0x0200);
                                bslCommandTXBSLVersion(device_handle);

                                QFile hexfile("/home/stefano/colibrifw.hex");
                                if(hexfile.open(QIODevice::ReadOnly)) {
                                    QIntelHexParser parser;
                                    parser.parseFile(hexfile);
                                    for(int i=0;i<parser.segments().size();i++) {
                                        QIntelHexMemSegment seg = parser.segments().at(i);
                                        qDebug("MainWindow::MainWindow segment at %04x of %d bytes",seg.address,seg.memory.size());
                                        for(int offs=0; offs<seg.memory.size(); offs +=52) {
                                            QByteArray mid = seg.memory.mid(offs,52);
                                            bslCommandRXDataBlock(device_handle,seg.address+offs,mid,false);
                                        }
                                    }
                                } else {

                                }

                                bslCommandLoadPC(device_handle,0x8000);

                                libusb_release_interface(device_handle, 0);
                            }

                            libusb_close(device_handle);
                        }
                    }
                }
            }
            libusb_free_device_list(device_list,true);
        }

        libusb_exit(ctx);
    }

 */

void MainWindow::on_UploadFirmware_clicked() {

}
