#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class QPushButton;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QString lastDir;

    QPushButton *loadFile;
    QPushButton *encodeFile;
    QPushButton *decodeFile;

    QByteArray byteArray; //for encoding file later
    QVector<int> byteFrequencies;

    QTableWidget *dataTable;

    int charCol; int symbolCol; int freqCol; int huffmansCol;
    int orgFileSize;
    int encodedStringSize;

    QString final;

    QVector<QString> charCodeToEncodingStrings;

    void buildTableRow(int iPos);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void loadFileButtonPushed();
    void encodeFileButtonPushed();
    void decodeFileButtonPushed();

};
#endif // MAINWINDOW_H
