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
** Qt_MSPBSL_Uploader is distributed in the hope that it will be useful,
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

#ifndef QINTELHEXPARSER_H
#define QINTELHEXPARSER_H

#include <QObject>
#include <QIODevice>

struct QIntelHexMemSegment {

    QIntelHexMemSegment(int addr) {
        address=addr;
    }

    int lastAddress() const { return address+memory.size(); }

    int address;
    QByteArray memory;
};

class QIntelHexParser {
public:
    QIntelHexParser();
    QIntelHexParser(const QString &filename);
    void parseFile(QIODevice &device);
    void parseFile(const QString &filename);
    void parseTxtFile(QIODevice &device);
    bool endOfFile() const { return mEndOfFile; }
    int sumTotalMemory() const;
    const QList<QIntelHexMemSegment> &segments() const { return mSegments; }
private:
    void handleRecord(quint8 recordType,quint16 address,QByteArray payload,int lineNum);
    quint8 decodeHexChar(QChar hex);
    quint8 parseHexByte(QChar hexH,QChar hexL);
    quint16 parseHexWord(QChar hexHH, QChar hexHL,QChar hexLH, QChar hexLL);
private:
    bool mEndOfFile;
    QList<QIntelHexMemSegment> mSegments;
};

#endif // QINTELHEXPARSER_H
