/*
* Copyright (C) 2017  Geetam Chawla <geetam.chawla@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*/

#include "qaria.h"
#include "ui_qaria.h"
#include <QDebug>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <utility>


int downloadEventCallback(aria2::Session* session, aria2::DownloadEvent event,
                        aria2::A2Gid gid, void* userData)
{

    qaria *qaobj = static_cast <qaria *> (userData);
    return qaobj->eventCallback(session, event, gid);
    
}

qaria::qaria(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::qaria)
{
    
    ui->setupUi(this);
    newdialog = new dialogNewDownload(this);

    connect(newdialog, &dialogNewDownload::downloadRequest, this, &qaria::addNewDownload);
    connect(this, &qaria::newProgressAvailable, &qaria::updateProgress);
    connect(this, &qaria::newDownSpeedAvailable, &qaria::updateDownSpeed);

    connect(ui->actionNewDownload, &QAction::triggered , newdialog, &dialogNewDownload::exec ) ;
    connect(ui->actionPause, &QAction::triggered, this, &qaria::pauseButtonClicked);
    connect(ui->actionResume, &QAction::triggered, this, &qaria::resumeButtonClicked);
    connect(ui->actionRemove, &QAction::triggered, this, &qaria::removeButtonClicked);
    
    db =  QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("downloadTable");
    db.open();
    QSqlQuery query(db);
    
    query.exec("create table if not exists downloadTable (url text, name text, progress integer, downloaddir text,  status int, gid text primary key, DownloadSpeed integer);");
   
    downloadTableModel = new QSqlTableModel(this, db);
    downloadTableModel->setTable("downloadTable");
    downloadTableModel->select();
    QStringList labels = QStringList() << QStringLiteral("URL") << QStringLiteral("Name") << 
    QStringLiteral("Progress") << QStringLiteral("Path") << QStringLiteral("Status") <<
    QStringLiteral("GID") << QStringLiteral("Download Speed");
    
    for(int i = 0; i < labels.size(); i++)
    {
        downloadTableModel->setHeaderData(i, Qt::Horizontal, labels[i]);

    }
    

    pd = new progressDelegate(this);
    ui->downloadTable->setModel(downloadTableModel);
    ui->downloadTable->setItemDelegate(pd);
    ui->downloadTable->hideColumn(0);
    ui->downloadTable->hideColumn(5);
    QFontMetrics fm = fontMetrics();

    ui->downloadTable->horizontalHeader()->resizeSection(1, fm.width("archlinux-2017.06.01-x86_64.iso"));
    ui->downloadTable->horizontalHeader()->resizeSection(3, fm.width("/home/fooUser/Downloads"));
    ui->downloadTable->horizontalHeader()->resizeSection(4, fm.width("   Downloading   "));
    ui->downloadTable->horizontalHeader()->resizeSection(6, fm.width("   Download Speed   "));
    
    
    
    aria2::libraryInit();
    ariaSessionconfig.keepRunning = true;
    ariaSessionconfig.userData = this;
    ariaSessionconfig.downloadEventCallback = downloadEventCallback;
    ariaSession = aria2::sessionNew(aria2::KeyVals(), ariaSessionconfig);
     
    
    query.exec("select url, downloaddir, gid from downloadTable where status=2;");
    while( query.next() )
    {
        QUrl qurl = query.value(0).toUrl();
        QString downloaddir = query.value(1).toString();
        QString gid = query.value(2).toString();
        addExistingDownload(qurl, downloaddir, gid);
        qDebug() << "found existing download" << qurl.toString() << downloaddir;;
    }
    ariaFuture = QtConcurrent::run(this, &qaria::ariaLoop);
    setStyleSheet(QLatin1String("QFrame { border: 0px ;margin-top 0px padding-top 0px}"));

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), ui->downloadTable, SLOT(repaint()));
    timer->start(1000);
   
}



int qaria::eventCallback(aria2::Session* session, aria2::DownloadEvent event, aria2::A2Gid gid)
{
            
    switch (event) {
        case aria2::EVENT_ON_DOWNLOAD_COMPLETE:{
            QSqlQuery query(db);
            query.prepare("update downloadTable set progress=?, status=? where gid=?;");
            query.bindValue(0, 100);
            query.bindValue(1, status::completed);
            QString qgidstr = QString::fromStdString(aria2::gidToHex(gid));
            query.bindValue(2, qgidstr );
            query.exec();
            QModelIndexList indexList = downloadTableModel->match(downloadTableModel->index(0, 5), Qt::DisplayRole, qgidstr);
            if( !indexList.isEmpty() )
            {
                qDebug() << "index list valid";
                downloadTableModel->selectRow(indexList[0].row());
            }
            else qDebug() << "empty index list";
            break;
        }
        case aria2::EVENT_ON_DOWNLOAD_ERROR:
            qDebug() << "ERROR";
            break;
        default:
            return 0;
    }
    qDebug() << " [" << QString::fromStdString(aria2::gidToHex(gid)) << "] ";
    aria2::DownloadHandle* dh = aria2::getDownloadHandle(session, gid);
    if (!dh)
        return 0;
    if (dh->getNumFiles() > 0) {
        aria2::FileData f = dh->getFile(1);
        // Path may be empty if the file name has not been determined yet.
        if (f.path.empty()) {
        if (!f.uris.empty()) {
            qDebug() << QString::fromStdString(f.uris[0].uri);
        }
        }
        else {
        qDebug() << QString::fromStdString(f.path);
        }
    }
    aria2::deleteDownloadHandle(dh);

    return 0;
}

qaria::~qaria()
{
    delete ui;
    aria2::sessionFinal(ariaSession);
    aria2::libraryDeinit();
}



void qaria::ariaLoop()
{
    qDebug() << "aria loop thread created";
    auto start = std::chrono::steady_clock::now();

    for( ; ; )
    {
        
        int rv = aria2::run(ariaSession, aria2::RUN_ONCE);
        if(rv != 1) {
            break;
        }
//         the application can call aria2 API to add URI or query progress
//         here
        
       
        auto now = std::chrono::steady_clock::now();
        auto count =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start)
                .count();
        // Print progress information once per 500ms
        if (count >= 500)
        {
            start = now;
            aria2::GlobalStat gstat = aria2::getGlobalStat(ariaSession);
            qDebug() << "Overall #Active:" << gstat.numActive
                        << " #waiting:" << gstat.numWaiting
                        << " D:" << gstat.downloadSpeed / 1024 << "KiB/s"
                        << " U:" << gstat.uploadSpeed / 1024 << "KiB/s ";
            std::vector<aria2::A2Gid> gids = aria2::getActiveDownload(ariaSession);
            for (const auto& gid : gids)
            {
                aria2::DownloadHandle* dh = aria2::getDownloadHandle(ariaSession, gid);
                if (dh)
                {

                    int progress = 0;
                    if(dh->getTotalLength() != 0)
                    {
                        progress = 100 * dh->getCompletedLength() /
                                               dh->getTotalLength();
                    }                    
                   
                   
                   QString qGid = QString::fromStdString(aria2::gidToHex(gid ) );
                   if(progress != 0)
                   {
                        emit newProgressAvailable(qGid, progress) ;
                   }
                   
                   emit newDownSpeedAvailable(qGid, dh->getDownloadSpeed());
                   
                   aria2::deleteDownloadHandle(dh);
                }
            }
        }
    
    }
    
    qDebug() << "aria loop thread dead";
   // ariaSession = aria2::sessionNew(aria2::KeyVals(), ariaSessionconfig);
    
}

void qaria::addNewDownload(const QUrl &url, const QString& downloadDir)
{
    aria2::KeyVals options;
    aria2::A2Gid gid;
    options.push_back(std::make_pair("dir", downloadDir.toStdString()));
    std::vector<std::string> ariaUris = {url.toString().toStdString()};
    int rv = aria2::addUri(ariaSession, &gid, ariaUris, options);
    if(rv < 0)
    {
        //TODO handle failure in adding uri
        qDebug() << "adding uri failed";
    }
    
    else
    {
        QSqlRecord record = downloadTableModel->record();
        record.setValue("url", QVariant( url ) );
        record.setValue("downloaddir", QVariant(downloadDir) );
        record.setValue("name", QVariant( url.fileName() ) );
        record.setValue("status", status::downloading);
        record.setValue("gid", QVariant( QString::fromStdString(aria2::gidToHex(gid) ) ) );
        record.setValue("progress", QVariant::fromValue(0));
        downloadTableModel->insertRecord(-1, record);

    }
}



void qaria::addExistingDownload(const QUrl& url, const QString& downloadDir, const QString& gid)
{
    
    aria2::KeyVals options;
    options.push_back(std::make_pair("gid", gid.toStdString() ) );
    options.push_back(std::make_pair("dir", downloadDir.toStdString()));
    options.push_back(std::make_pair("continue", "true") );
    std::vector<std::string> ariaUris = {url.toString().toStdString()};
    int rv = aria2::addUri(ariaSession, nullptr, ariaUris, options);
    if(rv < 0)
    {
        //TODO handle failure in adding uri
        qDebug() << "adding uri failed";
    }

}


void qaria::updateProgress(const QString &gid, int progress)
{
   
    QModelIndexList indexList = downloadTableModel->match(downloadTableModel->index(0, 5), Qt::DisplayRole, gid);
    if( !indexList.isEmpty() )
    {
        QSqlQuery query(db);
        query.prepare("update downloadTable set progress=? where gid=?;");
        query.bindValue(0, progress);
        query.bindValue(1, gid );
        query.exec();
        downloadTableModel->selectRow(indexList[0].row());

    }
    
    else qDebug() << "empty index list";
}

void qaria::updateDownSpeed(const QString &gid, int64_t bytesPerSec)
{
    QString formattedDownSpeed = formatDownSpeed(bytesPerSec);
    downSpeedMap[gid] = formattedDownSpeed;
}


void qaria::pauseButtonClicked()
{
    QModelIndexList indexList = ui->downloadTable->selectionModel()->selectedRows();
    int row;
    foreach (QModelIndex index, indexList)
    {
        row = index.row();
        QModelIndex statusIndex = downloadTableModel->index(row, 4);
        if(statusIndex.data().toInt() == status::paused)
        {
            continue;
        }
        
        QModelIndex gidIndex = downloadTableModel->index(row, 5);
        QString gid = gidIndex.data().toString();
        int rv = aria2::pauseDownload(ariaSession, aria2::hexToGid(gid.toStdString()));
        if( rv == 0 )
        {
            QSqlQuery query(db);
            query.prepare("update downloadTable set status=? where gid=?;");
            query.bindValue(0, status::paused);
            query.bindValue(1, gid);
            query.exec();
            downloadTableModel->selectRow(row);
        }
    }
}




void qaria::resumeButtonClicked()
{
    QModelIndexList indexList = ui->downloadTable->selectionModel()->selectedRows();
    int row;
    foreach (QModelIndex index, indexList)
    {
        
        row = index.row();
        QModelIndex statusIndex = downloadTableModel->index(row, 4);
        QSqlQuery query(db);

        if( statusIndex.data().toInt() != status::paused)
        {
            continue;
        }
        QModelIndex gidIndex = downloadTableModel->index(row, 5);
        QString gid = gidIndex.data().toString();
        
        aria2::A2Gid gidUnHexed = aria2::hexToGid(gid.toStdString());
        aria2::DownloadHandle *dh = aria2::getDownloadHandle(ariaSession, gidUnHexed);
        
        
        
        if(dh == NULL)
        {
            query.prepare("select url, downloaddir from downloadTable where gid=?;");
            query.bindValue(0, gid);
            query.exec();
            query.first();
            QUrl qurl = query.value(0).toUrl();
            QString downloaddir = query.value(1).toString();
            addExistingDownload(qurl, downloaddir, gid);

        }
        //TODO handle a failure in resuming download
        aria2::unpauseDownload(ariaSession, gidUnHexed);
    
        query.prepare("update downloadTable set status=? where gid=?;");
        query.bindValue(0, status::downloading);
        query.bindValue(1, gid);
        query.exec();
        downloadTableModel->selectRow(row);
       
    }
}

void qaria::removeButtonClicked()
{
    QModelIndexList indexList = ui->downloadTable->selectionModel()->selectedRows();
    int row;
    foreach (QModelIndex index, indexList)
    {
        row = index.row();
        QModelIndex gidIndex = downloadTableModel->index(row, 5);
        QString gid = gidIndex.data().toString();
        
        aria2::A2Gid gidUnHexed = aria2::hexToGid(gid.toStdString());
        aria2::DownloadHandle *dh = aria2::getDownloadHandle(ariaSession, gidUnHexed);
        if(dh != NULL)
        {
            int rv = aria2::removeDownload(ariaSession, gidUnHexed);
            if(rv != 0) qDebug() << "aria2: removing download failed";
            
        }
        downloadTableModel->removeRow(row);
        downloadTableModel->select();
    }
}

QString qaria::formatDownSpeed(int64_t bytesPerSec)
{
    QString res;
    if(bytesPerSec / 1073741824 != 0)
    {
        float val = ( (float) bytesPerSec) / 1073741824;
        
        res = QString::number(val) + " GiB/s";
    }
    
    else if (bytesPerSec / 1048576 != 0)
    {
        float val =  ( (float) bytesPerSec) / 1048576;
        
        res = QString::number(val) + " MiB/s";
    }
    
    else if (bytesPerSec / 1024 != 0)
    {
        float val =  ( (float) bytesPerSec) / 1024;
        
        res = QString::number(val) + " KiB/s";
    }
    
    else
    {
        res = QString::number(bytesPerSec) + " B/s";
    }
    
    
    return res;
    
}
