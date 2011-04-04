/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CALLGRINDVISUALISATION_H
#define CALLGRINDVISUALISATION_H

#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {
class Function;
class DataModel;
}
}

namespace Callgrind {
namespace Internal {

class Visualisation : public QGraphicsView
{
    Q_OBJECT

public:
    explicit Visualisation(QWidget *parent = 0);
    virtual ~Visualisation();

    void setModel(Valgrind::Callgrind::DataModel *model);

    const Valgrind::Callgrind::Function *functionForItem(QGraphicsItem *item) const;
    QGraphicsItem *itemForFunction(const Valgrind::Callgrind::Function *function) const;

    void setFunction(const Valgrind::Callgrind::Function *function);
    const Valgrind::Callgrind::Function *function() const;

public slots:
    void setText(const QString &message);

signals:
    void functionActivated(const Valgrind::Callgrind::Function *);
    void functionSelected(const Valgrind::Callgrind::Function *);

protected slots:
    void populateScene();

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

protected:
    virtual void resizeEvent(QResizeEvent *event);

private:
    class Private;
    Private *d;
};

} // Internal
} // Callgrind

#endif // VALGRIND_CALLGRIND_CALLGRINDVISUALISATION_H
