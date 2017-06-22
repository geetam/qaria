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

#ifndef QARIA_H
#define QARIA_H

#include <QMainWindow>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>
#include "dialognewdownload.h"
#include "progressdelegate.h"
#include <aria2/aria2.h>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

namespace Ui {
    class qaria;
}

class qaria : public QMainWindow
{
    Q_OBJECT
    
signals:
    void newProgressAvailable(const QString &gid, int progress);
    void newDownSpeedAvailable(const QString &gid, int64_t bytesPerSec);
public:
    explicit qaria(QWidget *parent = 0);
    ~qaria();
    enum status{
        stopped = 0,
        paused = 1,
        downloading = 2,
        completed = 3
    };
    
    int eventCallback(aria2::Session* session, aria2::DownloadEvent event,
                        aria2::A2Gid gid);
    void ariaLoop();
  
    QMap <QString, QString> downSpeedMap;
    QSqlTableModel *downloadTableModel;

private:

    
    Ui::qaria *ui;
    
    QSqlDatabase db;
    dialogNewDownload *newdialog;
    progressDelegate *pd;
    
    
    aria2::Session* ariaSession;
    aria2::SessionConfig ariaSessionconfig;
    std::vector<std::string> ariaUris;
    aria2::KeyVals ariaOptions;
    QFuture<void> ariaFuture;
    QString formatDownSpeed(int64_t bytesPerSec);
    
    
    
    
private slots:
    void addNewDownload(const QUrl &url, const QString &downloadDir);
    void addExistingDownload(const QUrl &url, const QString &downloadDir, const QString &gid);
    void updateProgress( const QString &gid, int progress);
    void updateDownSpeed(const QString &gid, int64_t bytesPerSec);
    void pauseButtonClicked();
    void resumeButtonClicked();
    void removeButtonClicked();
};

#endif // QARIA_H
