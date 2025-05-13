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
    connect(ui->pushButton_addRow, &QPushButton::clicked, this, &MainWindow::addRow);
    connect(ui->pushButton_deleteRow, &QPushButton::clicked, this, &MainWindow::deleteRow);
    connect(ui->pushButton_addColumn, &QPushButton::clicked, this, &MainWindow::addColumn);
    connect(ui->pushButton_deleteColumn, &QPushButton::clicked, this, &MainWindow::deleteColumn);
    connect(ui->pushButton_save, &QPushButton::clicked, this, &MainWindow::saveDB);

    timerAutosave = new QTimer(this);
    timerAutosave->setInterval(60000); // 1 минута
    connect(timerAutosave, &QTimer::timeout, [this]() {
        log("Autosave");
        saveDB();
        timerAutosave->start();
    });
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
        timerAutosave->start();
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
        timerAutosave->start();
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
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->setEditTriggers(QAbstractItemView::CurrentChanged
                                   | QAbstractItemView::AnyKeyPressed);
    ui->tableView->setStyleSheet("QTableView {"
                                 "  gridline-color: #FFFFFF;"
                                 "}"
                                 "QTableView::item {"
                                 "  padding: 5px;"
                                 "}"
                                 "QTableView::item:selected {"
                                 "  background-color: rgb(150,150,150);"
                                 "  color: black;"
                                 "}"
                                 "QTableView::item:selected:!active {"
                                 "  background-color: rgb(150,150,150);" // Цвет строки
                                 "  color: black;"
                                 "}"
                                 "QTableView QLineEdit {"
                                 "  border: none;"
                                 "  background: transparent;"
                                 "  padding: 0px;"
                                 "  margin: 0px;"
                                 "}"
                                 "QTableView QLineEdit:focus {"
                                 "  border: none;"
                                 "  background: transparent;"
                                 "}"
                                 "QHeaderView::section {"
                                 "  background-color: #000000;"
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
    QSqlQuery query(data);
    QString tableName = model->tableName();

    // Получаем список столбцов
    if (!query.exec("PRAGMA table_info(" + tableName + ")")) {
        log("Ошибка выполнения PRAGMA table_info: " + query.lastError().text());
        QMessageBox::warning(this, tr("Добавить строку"), tr("Ошибка получения структуры таблицы."));
        return;
    }
    QStringList columns;
    while (query.next()) {
        int cid = query.value("cid").toInt();
        QString columnName = query.value("name").toString();
        if (cid != 0) { // Пропускаем столбец id
            columns.append(columnName);
        }
    }

    // Формируем SQL-запрос
    QStringList values;
    for (int i = 0; i < columns.size(); ++i) {
        values.append("'Пусто'");
    }
    QString insertQuery = QString("INSERT INTO " + tableName + " (" + columns.join(", ")
                                  + ") VALUES (" + values.join(", ") + ")");
    if (query.exec(insertQuery)) {
        log("Строка добавлена.");

        // Обновляем модель
        model->setTable(model->tableName());
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        customizeModel();
        model->select();

        // Обновляем отображение
        customizeTable();
        ui->tableView->scrollToBottom();
    } else {
        log("Ошибка добавления строки через SQL: " + query.lastError().text());
        QMessageBox::warning(this,
                             tr("Добавить строку"),
                             tr("Ошибка добавления строки: %1").arg(query.lastError().text()));
    }
}
void MainWindow::deleteRow()
{
    QModelIndex currentIndex = ui->tableView->selectionModel()->currentIndex();
    int row = model->rowCount() - 1;
    if (currentIndex.isValid())
        row = currentIndex.row();
    else {
        log("Удаление строки: Не выделена ячейка. Удаление последней строки...");
    }

    if (row <= 0)
        return;

    saveDB();
    QSqlQuery query(data);
    QString request = QString("DELETE FROM " + model->tableName() + " WHERE id = "
                              + QString::number(model->data(model->index(row, 0)).toInt()));
    if (query.exec(request)) {
        log("Строка удалёна, сохранена база данных");
        // Переинициализируем модель
        model->setTable(model->tableName());
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        customizeModel(); // Устанавливаем заголовки
        model->select();

        // Обновляем отображение
        customizeTable();
    } else {
        log("Ошибка удаления строки: " + query.lastError().text());
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("Не возможно удалить: %1").arg(query.lastError().text()));
    }
}
void MainWindow::addColumn()
{
    int column = model->columnCount();
    saveDB();
    QSqlQuery query(data);
    QString request = QString("ALTER TABLE " + model->tableName() + " ADD content"
                              + QString::number(column) + " TEXT DEFAULT 'Пусто'");
    if (query.exec(request)) {
        log("Столбец добавлен и сохранен в базе данных");
        // Переинициализируем модель
        model->setTable(model->tableName());
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        customizeModel(); // Устанавливаем заголовки
        model->select();

        // Обновляем отображение
        customizeTable();
    } else {
        log("Ошибка добавления столбца в таблицу:" + query.lastError().text());
        QMessageBox::warning(this, tr("Добавить столбец"), tr("Ошибка добавления столбца."));
    }
}
void MainWindow::deleteColumn()
{
    int column = model->columnCount() - 1;
    QModelIndex currentIndex = ui->tableView->selectionModel()->currentIndex();
    if (currentIndex.isValid())
        column = currentIndex.column();
    else
        log("Удаление столбца: Не выделена ячейка. Удаление последнего столбца...");

    if (column <= 1)
        return;

    saveDB();
    QSqlQuery query(data);
    QString request = QString("ALTER TABLE " + model->tableName() + " DROP COLUMN "
                              + getColumnName(column));
    if (query.exec(request)) {
        log("Столбец удалён, сохранена база данных");
        // Переинициализируем модель
        model->setTable(model->tableName());
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        customizeModel(); // Устанавливаем заголовки
        model->select();

        // Обновляем отображение
        customizeTable();
    } else {
        log("Ошибка удаления столбца из таблицы:" + query.lastError().text());
        QMessageBox::warning(this, tr("Удалить столбец"), tr("Ошибка удаления столбца."));
    }
}
QString MainWindow::getColumnName(int columnIndex)
{
    QSqlQuery query(data);
    QString tableName = model->tableName(); // "notes1"
    if (!query.exec("PRAGMA table_info(" + tableName + ")")) {
        log("Ошибка выполнения PRAGMA table_info: " + query.lastError().text());
        return QString();
    }

    while (query.next()) {
        int cid = query.value("cid").toInt();
        if (cid == columnIndex) {
            return query.value("name").toString(); // Возвращаем имя столбца
        }
    }

    log("Столбец с индексом " + QString::number(columnIndex) + " не найден.");
    return QString();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (model != nullptr && (model->isDirty())
        || !columnsToDelete.isEmpty()) { // Если есть изменения
        QMessageBox::StandardButton reply
            = QMessageBox::question(this,
                                    tr("Сохранение"),
                                    tr("Сохранить изменения перед выходом?"),
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
