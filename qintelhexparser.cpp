/****************************************************************************
** Copyright (c) 2012 Stefano Pagnottelli
** Web: http://github.com/persuader72/Qt_MSPBSL_Uploader
**
** This file is part of Qt_MSPBSL_Uploader.
**
** Nome-Programma is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Nome-Programma is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Nome-Programma.  If not, see <http://www.gnu.org/licenses/>.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/

#include "qintelhexparser.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

QIntelHexParser::QIntelHexParser() {
}

QIntelHexParser::QIntelHexParser(const QString &filename) {
    mEndOfFile=false;
    parseFile(filename);
}

void QIntelHexParser::parseFile(QIODevice &device) {
    int lineCounter=0;
    QTextStream ts(&device);

    mEndOfFile=false;
    mSegments.clear();

    while(!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        lineCounter++;

        if(mEndOfFile) {
            qDebug("QIntelHexParser::parseFile: Warning line beyond end of file record");
            continue;
        }

        if(line.length()<10) {
            qDebug("QIntelHexParser::parseFile: Invalid line too short");
            continue;
        }
        if(line.at(0)!=':') {
            qDebug("QIntelHexParser::parseFile: Missing start charctar");
            continue;
        }

        quint8 bytesCount = parseHexByte(line.at(1),line.at(2));
        quint16 address = parseHexWord(line.at(3),line.at(4),line.at(5),line.at(6));
        quint8 recordType = parseHexByte(line.at(7),line.at(8));
        QByteArray data(bytesCount,0);
        if(bytesCount>0) {
            for(int i=0;i<bytesCount;i++) {
                data[i]=parseHexByte(line.at(9+i*2),line.at(9+i*2+1));
            }
        }

        quint16 calcLength = 11 + bytesCount*2;
        if(calcLength != line.length()) {
            qDebug("QIntelHexParser::parseFile: String length mismatch");
            continue;
        }

        quint8 readChecksum = parseHexByte(line.at(calcLength-2),line.at(calcLength-1));
        quint8 calcCheckSum = 0;

        for(int i=1;i<line.length()-2;i+=2) {
            calcCheckSum += parseHexByte(line.at(i),line.at(i+1));
        }

        calcCheckSum = ~calcCheckSum+1;

        if(calcCheckSum != readChecksum) {
            qDebug("QIntelHexParser::parseFile: Checksum verification failed calc:%02X readed:%02X",calcCheckSum,readChecksum);
            continue;
        }

        handleRecord(recordType,address,data,lineCounter);
    }

    qDebug("QIntelHexParser::parseFile: parsed %d lines with %d memory segments",lineCounter,mSegments.size());
}

void QIntelHexParser::parseFile(const QString &filename) {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        parseFile(filename);
    } else {
        qDebug("QIntelHexParser::parseFile: Can't open file %s",file.errorString().toAscii().constData());
    }
}

void QIntelHexParser::parseTxtFile(QIODevice &device) {
    int lineCounter=0;
    QTextStream ts(&device);

    mEndOfFile=false;
    mSegments.clear();

    while(!ts.atEnd()) {
        QString line = ts.readLine().trimmed();
        lineCounter++;

        if(mEndOfFile) {
            qDebug("QIntelHexParser::parseFile: Warning line beyond end of file record");
            continue;
        }

        if(line.length()<3) {
            qDebug("QIntelHexParser::parseFile: Invalid line too short");
            continue;
        }

        if(line.at(0)=='@') {
            bool ok = 0;
            int address = line.mid(1).toInt(&ok,16);
             mSegments.append(QIntelHexMemSegment(address));
        } else if(line.at(0)=='q') {

        } else {
            line = line.replace(" ","");
            mSegments.last().memory.append(QByteArray::fromHex(line.toAscii()));
            qDebug() << line;
        }
    }

    qDebug("QIntelHexParser::parseFile: parsed %d lines with %d memory segments",lineCounter,mSegments.size());
}

int QIntelHexParser::sumTotalMemory() const {
    int mem=0;
    for(int i=0;i<mSegments.count();i++) mem += mSegments.at(i).memory.size();
    return mem;
}

void QIntelHexParser::handleRecord(quint8 recordType, quint16 address, QByteArray payload, int lineNum) {
    switch(recordType) {
    case 0x00:
        if(mSegments.size()==0 || mSegments.last().lastAddress()!=address) {
            qDebug("QIntelHexParser::parseFile: New mwmory segment (%04X) at line %d",address,lineNum);
            mSegments.append(QIntelHexMemSegment(address));
        }
        mSegments.last().memory.append(payload);
        break;
    case 0x01:
        mEndOfFile=true;
        break;
    default:
        qDebug("QIntelHexParser::parseFile: Unkonow record type %02X at line %d",recordType,lineNum);
        break;
    }
}

quint8 QIntelHexParser::decodeHexChar(QChar hex) {
    if(hex>='0'&&hex<='9') return hex.toAscii()-'0';
    else if(hex>='A'&&hex<='F') return hex.toAscii()-'A'+10;
    else if(hex>='a'&&hex<='f') return hex.toAscii()-'a'+10;
    else return 0xFF;
}

quint8 QIntelHexParser::parseHexByte(QChar hexH, QChar hexL) {
    return (decodeHexChar(hexH)<<4) + decodeHexChar(hexL);
}

quint16 QIntelHexParser::parseHexWord(QChar hexHH, QChar hexHL,QChar hexLH, QChar hexLL) {
    return (parseHexByte(hexHH,hexHL)<<8) + parseHexByte(hexLH,hexLL);
}

