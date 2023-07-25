#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QDebug>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <packdef.h>
#include <mytablewidgetitem.h>
#include <QProcess>
#include <QStringList>
#include <QInputDialog>

class cKernel;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent* event)override;
    void getFileSize(int len);

private slots:
    void on_pb_addfile_clicked();
    void slot_addFolder(bool flag);
    void slot_uploadFile(bool flag);
    void slot_uploadFolder(bool flag);
    void slot_getShare(bool flag);
    void slot_refreshPageInfo(bool flag);
    void slot_insertUploadFile(FileInfo& info);
    void slot_insertDownloadFile(FileInfo& info);
    void slot_insertUploadFileComplete(FileInfo& info);
    void slot_updateUploadFileProgress(int,int);
    void slot_deleteUploadFileByRow(int row);
    void slot_insertFileInfo(FileInfo& info);
    void slot_insertDownloadComplete(FileInfo &info);
    void slot_openPath(bool flag);
    void slot_deleteAllFileInfo();
    void slot_updateDownloadFileProgress(int,int);
    void slot_deleteDownloadFileByRow(int row);
    void slot_insertShareFileInfo(QString name, int size, QString time, int shareLink);
    void slot_deleteAllShareInfo();
    void slot_uploadPause(bool flag);
    void slot_uploadResume(bool flag);
    void slot_downloadPause(bool flag);
    void slot_downloadResume(bool flag);
    void slot_setName(QString name);
    FileInfo slot_getDownloadFileInfoByTimestamp(int timestamp);
    FileInfo slot_getUploadFileInfoByTimestamp(int timestamp);










    void on_pb_file_clicked();

    void on_pb_tranmit_clicked();



    //void on_table_file_clicked(const QModelIndex &index);

    void on_table_file_cellClicked(int row, int column);

    void on_table_file_customContextMenuRequested(const QPoint &pos);

    void slot_downloadFile(bool);
    void slot_shareFile(bool);
    void slot_deleteFile(bool);

    //void on_table_download_clicked(const QModelIndex &index);

    void on_table_download_cellClicked(int row, int column);

    void on_table_file_cellDoubleClicked(int row, int column);

    void on_pb_prev_clicked();

    void on_pb_share_clicked();


    void on_table_upload_cellClicked(int row, int column);

signals:
    void SIG_uploadFile(QString,QString);    //第一个QString是需要上传的文件的路径， 第二个是希望保存到服务器的哪个目录下
    void SIG_close();
    void SIG_downloadFile(int ,QString);
    void SIG_downloadFolder(int ,QString);
    void SIG_addFolder(QString, QString);
    void SIG_changeDir(QString);
    void SIG_uploadFolder(QString, QString);
    void SIG_refreshPageInfo(QString);  //刷新当前页面信号
    void SIG_shareFile(QVector<int> ,QString);
    void SIG_getShareByLink(int,QString);
    void SIG_deleteFile(QVector<int> , QString);
    //设置上传暂停  0 开始  1暂停
    void SIG_setUploadPause(int timestamp, int isPause);
    //设置下载暂停 0开始  1暂停
    void SIG_setDownloadPause(int timestamp , int isPause);
private:
    Ui::MainWindow *ui;

    QMenu m_menuAddfile;
    QMenu m_menuFileInfo;
    QMenu m_menuDownload;
    QMenu m_menuUpload;

    friend class cKernel;
};

#endif // MAINWINDOW_H
