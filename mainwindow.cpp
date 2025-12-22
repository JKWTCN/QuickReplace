#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("QuickReplace", "Settings", this))
{
    ui->setupUi(this);
    connectSignals();
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::connectSignals()
{
    connect(ui->sourceBrowseButton, &QPushButton::clicked, this, &MainWindow::browseSourceFolder);
    connect(ui->targetBrowseButton, &QPushButton::clicked, this, &MainWindow::browseTargetFolder);
    connect(ui->replaceButton, &QPushButton::clicked, this, &MainWindow::replaceFolders);

    connect(ui->action_Exit, &QAction::triggered, this, &QWidget::close);
    connect(ui->action_About, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::loadSettings()
{
    settings->beginGroup("Paths");
    ui->sourcePathEdit->setText(settings->value("sourcePath", "").toString());
    ui->targetPathEdit->setText(settings->value("targetPath", "").toString());
    settings->endGroup();

    settings->beginGroup("Options");
    ui->createBackupCheck->setChecked(settings->value("createBackup", true).toBool());
    ui->confirmDeleteCheck->setChecked(settings->value("confirmDelete", true).toBool());
    settings->endGroup();
}

void MainWindow::saveSettings()
{
    settings->beginGroup("Paths");
    settings->setValue("sourcePath", ui->sourcePathEdit->text());
    settings->setValue("targetPath", ui->targetPathEdit->text());
    settings->endGroup();

    settings->beginGroup("Options");
    settings->setValue("createBackup", ui->createBackupCheck->isChecked());
    settings->setValue("confirmDelete", ui->confirmDeleteCheck->isChecked());
    settings->endGroup();
}

void MainWindow::browseSourceFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
        tr("选择源文件夹"),
        ui->sourcePathEdit->text().isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation) : ui->sourcePathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!folderPath.isEmpty()) {
        ui->sourcePathEdit->setText(folderPath);
    }
}

void MainWindow::browseTargetFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this,
        tr("选择目标文件夹"),
        ui->targetPathEdit->text().isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation) : ui->targetPathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!folderPath.isEmpty()) {
        ui->targetPathEdit->setText(folderPath);
    }
}

void MainWindow::replaceFolders()
{
    QString sourcePath = ui->sourcePathEdit->text().trimmed();
    QString targetPath = ui->targetPathEdit->text().trimmed();

    if (sourcePath.isEmpty() || targetPath.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请选择源文件夹和目标文件夹！"));
        return;
    }

    QFileInfo sourceInfo(sourcePath);
    QFileInfo targetInfo(targetPath);

    if (!sourceInfo.exists() || !sourceInfo.isDir()) {
        QMessageBox::warning(this, tr("错误"), tr("源文件夹不存在或不是有效的文件夹！"));
        return;
    }

    if (!targetInfo.exists() || !targetInfo.isDir()) {
        QMessageBox::warning(this, tr("错误"), tr("目标文件夹不存在或不是有效的文件夹！"));
        return;
    }

    if (sourceInfo.absoluteFilePath() == targetInfo.absoluteFilePath()) {
        QMessageBox::warning(this, tr("错误"), tr("源文件夹和目标文件夹不能相同！"));
        return;
    }

    bool createBackup = ui->createBackupCheck->isChecked();
    bool confirmDelete = ui->confirmDeleteCheck->isChecked();

    QString confirmMessage = tr("确定要用文件夹 A 的内容替换文件夹 B 的内容吗？\n\n")
                            + tr("源文件夹: %1\n").arg(sourcePath)
                            + tr("目标文件夹: %2").arg(targetPath);

    if (createBackup) {
        confirmMessage += tr("\n\n目标文件夹将被备份为 .bak 后缀。");
    }

    if (confirmDelete && !confirmAction(confirmMessage)) {
        return;
    }

    if (replaceFolderContents(sourcePath, targetPath, createBackup, confirmDelete)) {
        QMessageBox::information(this, tr("成功"), tr("文件夹替换完成！"));
    } else {
        QMessageBox::critical(this, tr("错误"), tr("文件夹替换失败，请检查权限或磁盘空间！"));
    }
}

bool MainWindow::replaceFolderContents(const QString &sourcePath, const QString &targetPath, bool createBackup, bool confirmDelete)
{
    QDir targetDir(targetPath);

    // 创建备份
    if (createBackup) {
        QString backupPath = targetPath + ".bak";
        if (QDir(backupPath).exists()) {
            if (!deleteDirectory(backupPath)) {
                QMessageBox::warning(this, tr("警告"), tr("无法删除旧的备份文件夹！"));
                return false;
            }
        }

        if (!copyDirectory(targetPath, backupPath)) {
            QMessageBox::warning(this, tr("警告"), tr("创建备份失败！"));
            return false;
        }
    }

    // 删除目标文件夹内容
    targetDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    QFileInfoList entries = targetDir.entryInfoList();

    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            if (!deleteDirectory(entry.absoluteFilePath())) {
                return false;
            }
        } else {
            if (!QFile::remove(entry.absoluteFilePath())) {
                return false;
            }
        }
    }

    // 复制源文件夹内容到目标文件夹
    if (!copyDirectory(sourcePath, targetPath)) {
        return false;
    }

    return true;
}

bool MainWindow::deleteDirectory(const QString &dirPath)
{
    QDir dir(dirPath);

    if (!dir.exists()) {
        return true;
    }

    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    QFileInfoList entries = dir.entryInfoList();

    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            if (!deleteDirectory(entry.absoluteFilePath())) {
                return false;
            }
        } else {
            if (!QFile::remove(entry.absoluteFilePath())) {
                return false;
            }
        }
    }

    return dir.rmdir(dirPath);
}

bool MainWindow::copyDirectory(const QString &sourcePath, const QString &targetPath)
{
    QDir sourceDir(sourcePath);
    QDir targetDir(targetPath);

    if (!targetDir.exists()) {
        if (!targetDir.mkpath(targetPath)) {
            return false;
        }
    }

    sourceDir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    QFileInfoList entries = sourceDir.entryInfoList();

    for (const QFileInfo &entry : entries) {
        QString sourceFilePath = entry.absoluteFilePath();
        QString targetFilePath = targetPath + QDir::separator() + entry.fileName();

        if (entry.isDir()) {
            if (!copyDirectory(sourceFilePath, targetFilePath)) {
                return false;
            }
        } else {
            if (!QFile::copy(sourceFilePath, targetFilePath)) {
                return false;
            }
        }
    }

    return true;
}

bool MainWindow::confirmAction(const QString &message)
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("确认操作"),
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    return reply == QMessageBox::Yes;
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("关于 QuickReplace"),
        tr("<h3>QuickReplace - 文件夹替换工具</h3>"
           "<p>版本 1.0</p>"
           "<p>一个简单易用的文件夹内容替换工具。</p>"
           "<p>使用方法：</p>"
           "<ul>"
           "<li>选择源文件夹（文件夹 A）</li>"
           "<li>选择目标文件夹（文件夹 B）</li>"
           "<li>选择是否创建备份</li>"
           "<li>点击开始替换</li>"
           "</ul>"
           "<p><b>注意：</b>此操作将用文件夹 A 的内容完全替换文件夹 B 的内容，请谨慎操作！</p>"));
}
