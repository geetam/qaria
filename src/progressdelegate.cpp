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



#include "progressdelegate.h"
#include <QStyle>
#include <QApplication>
#include "qaria.h"

progressDelegate::progressDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
}


//Refer to https://doc.qt.io/qt-5/qtnetwork-torrent-mainwindow-cpp.html
void progressDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.column() == 2)
    {
        
        // Set up a QStyleOptionProgressBar to precisely mimic the
        // environment of a progress bar.
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.state = QStyle::State_Enabled;
        progressBarOption.direction = QApplication::layoutDirection();
        progressBarOption.rect = option.rect;
        progressBarOption.fontMetrics = QApplication::fontMetrics();
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.textAlignment = Qt::AlignCenter;
        progressBarOption.textVisible = true;

        // Set the progress and text values of the style option.
//         qaria *qaobj = qobject_cast<qaria *>(parent());
//         int progress = qaobj->prog();
        int progress = index.data().toInt();
        progressBarOption.progress = progress < 0 ? 0 : progress;
    //    qDebug() << "delegate called with progress = " << progress;
        progressBarOption.text = QString::asprintf("%d%%", progressBarOption.progress);

        // Draw the progress bar onto the view.
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
    
    }
    
    
    else if(index.column() == 4)
    {
        
        QStyleOptionViewItem statOption = option;
        statOption.state = QStyle::State_Enabled;
        statOption.direction = QApplication::layoutDirection();
        statOption.rect = option.rect;
        statOption.fontMetrics = QApplication::fontMetrics();
    
        switch( index.data().toInt() )
        {
            case qaria::status::paused:
                statOption.text = QStringLiteral("Paused");
                break;
            case qaria::status::downloading:
                statOption.text = QStringLiteral("Downloading");
                break;
            case qaria::status::completed:
                statOption.text = QStringLiteral("Completed");
                break;
            case qaria::status::stopped:
                statOption.text = QStringLiteral("Stopped");
                break;
            default:
                statOption.text = QStringLiteral("nope");
        }
        
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &statOption, painter);
    }
    
    else if(index.column() == 6)
    {
        QStyleOptionViewItem speedOption = option;
        speedOption.state = QStyle::State_Enabled;
        speedOption.direction = QApplication::layoutDirection();
        speedOption.rect = option.rect;
        speedOption.fontMetrics = QApplication::fontMetrics();
        qaria *qaobj = qobject_cast<qaria *>(parent());
        QModelIndex gidIndex = qaobj->downloadTableModel->index(index.row(), 5);
        QString gid = gidIndex.data().toString();
        
        QModelIndex statusIndex = qaobj->downloadTableModel->index(index.row(), 4);
        if(statusIndex.data().toInt() != qaria::status::downloading)
        {
            speedOption.text = QStringLiteral(" ");
        }
        else if(qaobj->downSpeedMap.count(gid) )
        {
            speedOption.text = qaobj->downSpeedMap[gid];
        }
        else
        {
            speedOption.text = QStringLiteral("0 B/s");
        }
        
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &speedOption, painter);
    }

    
    else
        QStyledItemDelegate::paint(painter, option, index);
}
