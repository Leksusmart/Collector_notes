#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QStandardPaths>
#include <QTimer>
static unsigned short int START_PAGE = 0;
static unsigned short int MAIN_PAGE = 1;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QSqlDatabase data;
    const QString folderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).section('/', 0, -2)
          + "/Leksusmart Games/Collector notes";
    QSqlTableModel *model = nullptr;
    QTimer *timerAutosave = nullptr;
private slots:
    void closeEvent(QCloseEvent *event) override;
    void log(QString message) { qDebug() << message; }
    bool openDB(QString path);
    void customizeModel();
    void customizeTable();
    bool saveDB();
    // Buttons
    void openFile();
    void createFile();
    void addRow();
    void deleteRow();
};
#endif // MAINWINDOW_H
