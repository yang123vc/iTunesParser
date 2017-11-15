#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql/QSqlDatabase>
#include <QtSql/QtSql>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QTextBrowser>

#include "instruction.h"

#define SIZE_UNIQ_URL 6

#define WHATSAPP_IDENTITY 0
#define WECHAT_IDENTITY 1
#define TELEGRAM_IDENTITY 2
#define VIBER_IDENTITY 3

#define UUID_LENTH 40



namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QString getGUID_FILE_PATH() const;
    void setGUID_FILE_PATH(const QString &value);

public slots:
    void  finishFetchProfile(int, QProcess::ExitStatus);
    void  finishModifyApp(int, QProcess::ExitStatus);

private slots:
    void on_startBtn_clicked();
    void sendReportToServerReply(QNetworkReply* reply);

    //void on_pushButton_clicked();

    void on_chooseFileBtn_clicked();

    void on_action_triggered();

    void on_btnUpdate_clicked();

    void on_actionOpenTasksDir_triggered();

    void on_actionOpenCurrentTaskDir_triggered();

    void on_btnOpenReadyIpaDir_clicked();

private:
    Ui::MainWindow *ui;
    Instruction instructionForm;
    QString pathToAllBackup;
    QString currentBackup;
    QString UUID; //use for name tmpFolder
    QTextBrowser* logTextBrowser;

    void runTest();
    void setBackUpRootPath();
    void showAllBackups();
    bool process(QString backupFullPath);
    bool connectDatabase(const QString& database);

    QString processWechatBackup(QString backupFullPath);
    QString processWhatsappBackup (QString backupFullPath);
    QString processViberBackup (QString backupFullPath);
    void showAlertWithMsg(QString message);

    QSqlDatabase db;
    bool copyFilesToTmpDir(QMap<QString, QString>* mapIosPathAndLocalPath, int lenghForCut);
    bool getMediaFilesPaths(QMap<QString, QString>* mapIosPathAndLocalPath, QString sql);

    QMap<QString, QString>* getMapForViber(QMap<QString, QString>* mapIosPathAndLocalPath);

    QString copyStorageFile();
    QString getBackupTmpDirFullPath();
    QString zip(QString zipFileName);
    QString zipForMac(QString zipFileName);
    QString zipForWin(QString zipFileName);

    QString getWinRarExePath();
    QString getLstFile();
    bool clear();
    bool sendZip(QString zipFullPath, QString uuid);
    QString getNameFromFullPath(QString fullPath);
    void sendPost();
    void sendMultiPart();
    bool saveLog(QString fileFullPath);

    bool sendAllBackup (QMap <QString, QString>* mapPostAndFilePath, QString uuid);
    QByteArray getAppList();
    quint64 dir_size(const QString & str);
    void showAlertWithInfoMsg(QString message);

    QString getBackupTmpDirFullPath_ForResult();
    void moveBackupsToResultFolder();

    void createLocalIpa(QMap <QString, QString>* mapPostAndFilePath, QString uuid);
    void createRemoteIpa(QMap <QString, QString>* mapPostAndFilePath, QString uuid);

    bool copyScripts();
    bool createGuidFile();
    bool copyBackupsToWorkDir();
    bool getProfiles();
    bool runVgModifyScript();
    bool openIpaInFileManager();

    void informPathOfReadyIpa();



    QString getCurrentWorkDir();

     QMap<QString, QString> getMapForCheckedApps();

     QString getAppLetterFromAppId(QString appId);

     QString APPLE_ID;
     QString APPLE_ID_PASS;
     QString KEYCHAIN_PASS;

    int PROCESS_FETCH_PROFILE_COUNTS;
     int PROCESS_MODIFY_APP;

    QString CURRENT_WORK_DIR;
    QString GUID_FILE_PATH;

    QString ALERT_STR;
    qint64 MAX_BYTE_PERMIT_FOR_BACKUP;
    qint64 SIZE_OF_BACKUP_VERY_BIG;

    QByteArray GUID_BYTEARR;
    QByteArray URL_BYTEARR;
    QByteArray ALIAS_BYTEARR;

    QString WHATSAPP_DB_FILENAME;
    QString WHATSAPP_MEDIA_ZIP_FILENAME;
    QString WHATSAPP_MEDIA_ZIP_POST_NAME ;
    QString WHATSAPP_DB_POST_NAME ;

     QString WECHAT_DB_FILE ;
      QString WECHAT_POST_NAME ;

      QString VIBER_DB_FILE ;
       QString VIBER_POST_NAME ;

     QString IPA_BUILDER_MAC_SERVER_IP ;

     QString INSTALL_URL;


};

#endif // MAINWINDOW_H
