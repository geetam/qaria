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

#include "dialognewdownload.h"
#include "ui_dialognewdownload.h"
#include <QFileDialog>

dialogNewDownload::dialogNewDownload(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialogNewDownload)
{
    ui->setupUi(this);
    ui->downloadDir->setText("/tmp");
    connect(ui->browseButton, &QPushButton::clicked , this, &dialogNewDownload::browse );
    connect(ui->okButton, &QDialogButtonBox::accepted, this, &dialogNewDownload::sendDownloadRequest );
}


void dialogNewDownload::browse()
{
    QString downloadDir = QFileDialog::getExistingDirectory(this,
                                         tr ("Please select the destination"),
                                         ui->downloadDir->text(),
                                         QFileDialog::ShowDirsOnly);
    ui->downloadDir->setText(downloadDir);
}


void dialogNewDownload::sendDownloadRequest()
{
    if( !ui->url->text().isEmpty() && !ui->downloadDir->text().isEmpty() )
    {
        
        emit downloadRequest( QUrl(ui->url->text() ), ui->downloadDir->text() );
    }
}

dialogNewDownload::~dialogNewDownload()
{
    delete ui;

}
