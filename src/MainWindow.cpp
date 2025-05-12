#include "MainWindow.h"
#include "src/ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // TODO: Сделать открытие прошлой базы данных
    ui->stackedWidget->setCurrentIndex(START_PAGE);

    data = QSqlDatabase::addDatabase("QSQLITE");

    // создание папки с базами данных, если она не существует.
    QDir dir(QFileInfo(folderPath + '/').absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "Не удалось создать директории по пути" << dir.absolutePath();
            this->hide();
            QMessageBox::critical(this,
                                  tr("Create Directory"),
                                  tr("Could not create database directory."));
            delete ui; // Очистка ресурсов
            exit(1);   // Завершение программы
        }
    }

    connect(ui->pushButton_openFile, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(ui->pushButton_createFile, &QPushButton::clicked, this, &MainWindow::createFile);
    connect(ui->pushButton_add, &QPushButton::clicked, this, &MainWindow::addRow);
    connect(ui->pushButton_delete, &QPushButton::clicked, this, &MainWindow::deleteRow);

    timerAutosave = new QTimer(this);
    timerAutosave->setInterval(60000); // 1 минута
    timerAutosave->start();
    connect(timerAutosave, &QTimer::timeout, [this]() {
        saveDB();
        timerAutosave->start();
    });

    /*
1.✅Инициализация SQL: data = QSqlDatabase::addDatabase("QSQLITE")
2.✅Открыть базу по пути, чтобы SQL создал файл: setDatabaseName() перед open()
3.✅Создание таблицы: Создать таблицу в базе данных: query.exec().
4.✅Заполнить model данными из базы: model->select();
5.✅Настроить model.
6.✅Настройка QTableView.
7.Сохранение: model->submitAll();
     */
}
void MainWindow::createFile()
{
    log("Button: Create File");
    QString path = QFileDialog::getSaveFileName(this,
                                                tr("Create File in"),
                                                folderPath,
                                                tr("База данных (*.db)"));
    if (path.isEmpty()) // Если отменили выбор
        return;

    if (!path.endsWith(".db"))
        path += ".db";

    log("База данных создаётся по пути " + path);
    if (openDB(path)) {
        // Создаем таблицу notes1 в новой базе
        QSqlQuery query(data);
        if (!query.exec("CREATE TABLE notes1 (id INTEGER PRIMARY KEY "
                        "AUTOINCREMENT, title TEXT, content TEXT)")) {
            log("Ошибка создания таблицы:" + query.lastError().text());
            QMessageBox::critical(this, tr("Create File"), tr("SQL Ошибка создания файла."));
            return;
        }

        customizeModel();
        model->select();
        customizeTable();

        log("Попытка сохранить...");
        saveDB();
        ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
    } else
        QMessageBox::critical(this, tr("Open File"), tr("Could not open created database."));
}
void MainWindow::openFile()
{
    log("Button: Open File");
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open File"),
                                                folderPath,
                                                tr("База данных (*.db)"));
    if (path.isEmpty()) // Если отменили выбор
        return;

    if (openDB(path)) {
        customizeModel();
        model->select();
        customizeTable();

        log("Попытка сохранить...");
        saveDB();
        ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
    }
}
bool MainWindow::openDB(QString path)
{
    data.setDatabaseName(path);
    if (!data.open()) {
        qDebug() << "Ошибка открытия:" << data.lastError().text();
        return false;
    }
    log("База данных открыта по пути " + data.databaseName());

    if (model != nullptr)
        delete model;
    model = new QSqlTableModel(this, data);
    return true;
}
void MainWindow::customizeModel()
{
    // Настройка model
    model->setTable("notes1");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Title"));
    model->setHeaderData(2, Qt::Horizontal, tr("Content"));
}
void MainWindow::customizeTable()
{
    // Настройка QTableView
    ui->tableView->setModel(model);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->setEditTriggers(QAbstractItemView::SelectedClicked
                                   | QAbstractItemView::EditKeyPressed);
    ui->tableView->setShowGrid(true);
    ui->tableView->setStyleSheet("QTableView {"
                                 "  gridline-color: #FFFFFF;" // Ваш цвет границ
                                 "}"
                                 "QTableView::item {"
                                 "  padding: 5px;"
                                 "}"
                                 "QHeaderView::section {"
                                 "  background-color: #000000;" // Серый фон заголовков
                                 "  padding: 4px;"
                                 "  font-weight: bold;"
                                 "}");

    // Настройка размеров столбцов и строк
    QHeaderView *header = ui->tableView->horizontalHeader();
    header->setMinimumSectionSize(100);
    ui->tableView->setColumnWidth(0, 50);
    ui->tableView->setColumnWidth(1, 200);
    ui->tableView->setColumnWidth(2, 300);
    ui->tableView->verticalHeader()->setMinimumSectionSize(30);
    ui->tableView->resizeRowsToContents();
    header->setSectionResizeMode(QHeaderView::Interactive);

    // Скрытие ID
    ui->tableView->hideColumn(0);
}
bool MainWindow::saveDB()
{
    QFile file(data.databaseName());
    if (!file.exists()) {
        log("По данному пути не удалось найти и сохранить файл: " + data.databaseName());
        QMessageBox::critical(this,
                              tr("Сохранение не удалось"),
                              tr("Не удалось создать путь и сохранить по нему файл"));
        return false;
    }

    if (!model->isDirty())
        log("Изменений не найдено. Сохранять не нужно.");
    else if (model->submitAll())
        log("Изменения успешно сохранены в базе данных");
    else {
        log("Ошибка сохранения изменений: " + model->lastError().text());
        return false;
    }
    return true;
}
void MainWindow::addRow()
{
    int row = model->rowCount();
    if (model->insertRow(row)) {
        model->setData(model->index(row, 1), "ПУСТО");
        model->setData(model->index(row, 2), "ПУСТО");
        ui->tableView->scrollToBottom();
    } else {
        log("Ошибка добавления строки:" + model->lastError().text());
        QMessageBox::warning(this, tr("Добавить строку"), tr("Ошибка добавления строки."));
    }
}

void MainWindow::deleteRow()
{
    QModelIndexList selected = ui->tableView->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("Delete Row"), tr("No row selected."));
        return;
    }
    int row = selected.at(0).row();
    if (model->removeRow(row)) {
        if (model->submitAll()) {
            log("Строка удалена");
        } else {
            qDebug() << "Ошибка сохранения после удаления:" << model->lastError().text();
            QMessageBox::warning(this, tr("Delete Row"), tr("Failed to save changes."));
        }
    } else {
        qDebug() << "Ошибка удаления строки:" << model->lastError().text();
        QMessageBox::warning(this, tr("Delete Row"), tr("Failed to delete row."));
    }
    customizeTable();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (model != nullptr && model->isDirty()) { // Если есть изменения
        QMessageBox::StandardButton reply
            = QMessageBox::question(this,
                                    "Сохранение",
                                    "Сохранить изменения перед выходом?",
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (reply == QMessageBox::Yes) {
            if (!saveDB()) {
                QMessageBox::warning(this, "Ошибка", "Не удалось сохранить!", QMessageBox::Ok);
                event->ignore();
                return;
            };
        } else if (reply == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    event->accept();
}
MainWindow::~MainWindow()
{
    if (data.isOpen()) {
        data.close();
    }
    if (model != nullptr)
        delete model;
    if (timerAutosave != nullptr) {
        timerAutosave->stop();
        delete timerAutosave;
    }
    delete ui;
}
