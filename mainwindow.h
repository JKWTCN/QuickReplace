#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private slots:
    void browseSourceFolder();
    void browseTargetFolder();
    void replaceFolders();
    void showAbout();

private:
    void connectSignals();
    bool replaceFolderContents(const QString &sourcePath, const QString &targetPath, bool createBackup, bool confirmDelete);
    bool deleteDirectory(const QString &dirPath);
    bool copyDirectory(const QString &sourcePath, const QString &targetPath);
    bool confirmAction(const QString &message);

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
