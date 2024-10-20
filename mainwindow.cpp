#include "mainwindow.h"
#include <cctype>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), byteFrequencies(256, 0), charCodeToEncodingStrings(256, ""), charCol(0), symbolCol(1), freqCol(2), huffmansCol(3) {

    setWindowTitle("huffman's encoding files");

    //LAYOUTS
    //root widget
    QWidget *center = new QWidget();
    setCentralWidget(center);

    //set the vertical box
    QVBoxLayout *vertLayout = new QVBoxLayout();
    center->setLayout(vertLayout);

    //add the horizontal box to the top
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    vertLayout->addLayout(buttonLayout);

    //add the buttons to the top horizontal box
    loadFile = new QPushButton("load");
    buttonLayout->addWidget(loadFile);
    connect(loadFile, &QPushButton::clicked, this, &MainWindow::loadFileButtonPushed);
    encodeFile = new QPushButton("encode");
    buttonLayout->addWidget(encodeFile);
    connect(encodeFile, &QPushButton::clicked, this, &MainWindow::encodeFileButtonPushed);
    decodeFile = new QPushButton("decode");
    buttonLayout->addWidget(decodeFile);
    connect(decodeFile, &QPushButton::clicked, this, &MainWindow::decodeFileButtonPushed);

    encodeFile->setEnabled(false);

    //add the table widget to the bottom
    dataTable = new QTableWidget();
    int nCols = 4; int nRows = 256;
    dataTable->setColumnCount(nCols);
    dataTable->setRowCount(nRows);
    dataTable->setHorizontalHeaderLabels(QStringList() << "char code" << "char symbol" << "# occurances" << "encoding");
    dataTable->setSortingEnabled(false);
    vertLayout->addWidget(dataTable);

    QSettings settings("DRT", "huffmans");
    lastDir = settings.value("lastDirectory", "").toString();

}

MainWindow::~MainWindow() {
    QSettings settings("DRT", "huffmans");
    settings.value("lastDirectory", lastDir);
}

void MainWindow::buildTableRow(int iPos) {
    //char code (which byte)
    QTableWidgetItem *charCode = new QTableWidgetItem();
    charCode->setData(Qt::DisplayRole, iPos);
    dataTable->setItem(iPos, charCol, charCode);

    //symbol
    QString charSymbol(1, QChar(iPos));
    if (QChar(iPos).isPrint()) {
        QTableWidgetItem *symbol = new QTableWidgetItem(charSymbol);
        dataTable->setItem(iPos, symbolCol, symbol);
    }

    //frequency
    QTableWidgetItem *freq = new QTableWidgetItem();
    freq->setData(Qt::DisplayRole, byteFrequencies[iPos]);
    dataTable->setItem(iPos, freqCol, freq);
}

//prompt user for a file and open it (read in the file)
void MainWindow::loadFileButtonPushed() {
    QString inName = QFileDialog::getOpenFileName(this, "select file you want to open", lastDir);

    //check for empty file
    if (inName.isEmpty()) return;

    QFile inFile(inName);

    //exit if the file doesn't open
    if (!inFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "file does not open", QString("Can't open file \"%1\"").arg(inName));
        return;
    }

    lastDir = QFileInfo(inName).absolutePath();
    byteArray = inFile.readAll();
    if (byteArray.isEmpty()) {
        QMessageBox::information(this, "empty file", "Your file is empty, no point in encoding it!");
            return;
    }
    inFile.close();

    //loop through the bytes to find freqencies
    orgFileSize = byteArray.size();
    for (int iByte = 0; iByte < orgFileSize; ++iByte) {
        ++byteFrequencies[(unsigned char)byteArray[iByte]];
    }

    dataTable->clearContents();
    for (int iPos = 0; iPos < 256; ++iPos) buildTableRow(iPos);
    dataTable->sortByColumn(freqCol, Qt::DescendingOrder);
    dataTable->hideColumn(huffmansCol);

    encodeFile->setEnabled(true);
}

void MainWindow::encodeFileButtonPushed() {
    dataTable->sortByColumn(charCol, Qt::AscendingOrder);
    dataTable->showColumn(huffmansCol);
    //determine huffmans encoding based on frequencies (find once per character)
    //put them into the table and store in a QVector for encoding the file later

    //create todo list -- maps int frequency to the QByteArray it corresponds to
    QMultiMap<int, QByteArray> toDo;
    //add each pair (char freq, char)
    for (int iCharCode = 0; iCharCode < 256; ++iCharCode) {
        if (byteFrequencies[iCharCode])
            toDo.insert(byteFrequencies[iCharCode], QByteArray(1, iCharCode));
    }

    //map each sequence of symbols to its two children (left child, right child)
    QMap<QByteArray, QPair<QByteArray, QByteArray> > parentToChildren;

    //toDo: encoding a file with a single symbol in it (write the size)
    if (toDo.size() == 1) {
        QMessageBox::information(this, "error", QString("Huffman encoding does not work for single character"));
        return;
    }

    while (toDo.size() > 1) {
        //take first 2 elements...get their weights and corresponding arrays of bytes for those weights
        int weight0 = toDo.begin().key(); QByteArray char0 = toDo.begin().value();
        toDo.erase(toDo.begin());

        int weight1 = toDo.begin().key(); QByteArray char1 = toDo.begin().value();
        toDo.erase(toDo.begin());

        //add new node to toDo list
        int parentFreq = weight0 + weight1;
        QByteArray parentChar = char0 + char1;
        toDo.insert(parentFreq, parentChar);

        parentToChildren[parentChar] = qMakePair(char0, char1);
    }

    //encoding any character:
    QByteArray root = toDo.begin().value(); //start with the root node

    QByteArray target;

    for (int iChar = 0; iChar < 256; ++iChar) {
        if (! byteFrequencies[iChar]) continue;
        QString encoding = ""; //init an empty string for the encoding !!! technically, put this into a QByteArray 8 bits at a time (binary data -- last step)
        QByteArray current = root;
        target = QByteArray(1, iChar);

        while (current != target) {
            if (parentToChildren[current].first.contains(target)) {
                encoding.append("0");
                current = parentToChildren[current].first;
            }
            else {
                encoding.append("1");
                current = parentToChildren[current].second;
            }
        }
        // qDebug() << "encoding for target" << target << "of char code" << iChar << "is" << encoding;
        charCodeToEncodingStrings[iChar] = encoding;
    }

    //fill encoding column with each char encoding
    for (int iRow = 0; iRow < 256; ++iRow) {
        QTableWidgetItem *encoding = new QTableWidgetItem(QString(charCodeToEncodingStrings[iRow]));
        dataTable->setItem(iRow, huffmansCol, encoding);
    }
    dataTable->sortByColumn(freqCol, Qt::DescendingOrder);

    //prompt for an output file name, and save encoding scheme/binary huffman encoding there
    QString outName = QFileDialog::getSaveFileName(this, "select file to save to");
    if (outName.isEmpty()) return;

    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(this, "error", QString("Can't write to file \"%1\"").arg(outName));
        return;
    }

    QDataStream out(&outFile);
    //the dictionary key for encoding
    out << charCodeToEncodingStrings;

    final = "";
    //the encoded message
    for (int iByte = 0; iByte < orgFileSize; ++iByte) {
        final += charCodeToEncodingStrings[(unsigned char)byteArray[iByte]];
    }

    //turn string into binary
    QByteArray result;
    encodedStringSize = final.size();
    for (int iBit=0; iBit<encodedStringSize; iBit+=8) {
        result.append(final.mid(iBit, 8).toInt(nullptr, 2));
    }

    out << encodedStringSize;

    out.writeRawData(result.data(), result.size());
    QMessageBox::information(this, "file saved", QString("File \"%1\" saved sucessfully. Original file size %2. Embedded file size %3").arg(outName).arg(orgFileSize).arg(outFile.size()));
}

void MainWindow::decodeFileButtonPushed() {

    //load the encoded binary file
    QString inName = QFileDialog::getOpenFileName(this, "select file you want to open to decode", lastDir);
    if (inName.isEmpty()) return;
    QFile inFile(inName);

    //exit if the file doesn't open
    if (!inFile.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "file does not open", QString("Can't open file \"%1\"").arg(inName));
        return;
    }
    lastDir = QFileInfo(inFile).absolutePath();

    QDataStream in(&inFile);
    in >> charCodeToEncodingStrings;
    int encodedStringLen;
    in >> encodedStringLen;

    QByteArray newByteArray((encodedStringLen+7)/8, 0); //size the byte array to string length + 7 divided by 8
    in.readRawData(newByteArray.data(), newByteArray.size()); //pull in bytes for read raw data

    //need length of final to know how many bits to read out of newByteArray
    QString stringToDecode = "";
    for (int iByte=0; iByte<newByteArray.size(); ++iByte) {
        int width = qMin(encodedStringLen, 8);
        stringToDecode.append(QString::number(((unsigned char)newByteArray[iByte]), 2).rightJustified(width, '0'));
        encodedStringLen -= width;
    }

    //make a QMap in the opposite direction QString to Qchar (to decode unambigiously)
    QMap<QString, unsigned char> decodeDict;
    for (int iChar=0; iChar<charCodeToEncodingStrings.length(); ++iChar) {
        decodeDict.insert(charCodeToEncodingStrings[iChar], (unsigned char)iChar);
    }
    QByteArray decodedData;

    //find the char code by looking up in the dictionary
    QString current = "";
    for (int iByte=0; iByte<stringToDecode.length(); ++iByte) {
        current.append(stringToDecode[iByte]);
        if (decodeDict.contains(current)) {
            //add it to the decoded QByteArray
            decodedData.append(decodeDict.value(current));
            //update the current string to nothing
            current = "";
        }
    }

    //fill out the table with its contents
    dataTable->clearContents();
    for (int iRow=0; iRow<256; ++iRow) dataTable->hideRow(iRow);
    dataTable->hideColumn(freqCol);

    for (int iChar = 0; iChar<256; ++iChar) {
        if (! decodedData.contains(iChar)) {
            continue;
        }
        dataTable->showRow(iChar); //show the row you fill
        buildTableRow(iChar);

        //encoding column
        QTableWidgetItem *encoding = new QTableWidgetItem(charCodeToEncodingStrings[iChar]);
        dataTable->setItem(iChar, huffmansCol, encoding);
    }

    //prompt user for output filename
    QString outName = QFileDialog::getSaveFileName(this, "select file to save to");
    if (outName.isEmpty()) return;

    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::information(this, "error", QString("Can't write to file \"%1\"").arg(outName));
        return;
    }

    //write decoded data to this file
    QDataStream out(&outFile);
    out.writeRawData(decodedData.data(), decodedData.size());
}
