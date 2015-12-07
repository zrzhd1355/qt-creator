/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "testresultmodel.h"

#include <QFontMetrics>
#include <QIcon>

namespace Autotest {
namespace Internal {

/********************************* TestResultItem ******************************************/

TestResultItem::TestResultItem(TestResult *testResult)
    : m_testResult(testResult)
{
}

TestResultItem::~TestResultItem()
{
    delete m_testResult;
}

static QIcon testResultIcon(Result::Type result) {
    static QIcon icons[11] = {
        QIcon(QLatin1String(":/images/pass.png")),
        QIcon(QLatin1String(":/images/fail.png")),
        QIcon(QLatin1String(":/images/xfail.png")),
        QIcon(QLatin1String(":/images/xpass.png")),
        QIcon(QLatin1String(":/images/skip.png")),
        QIcon(QLatin1String(":/images/blacklisted_pass.png")),
        QIcon(QLatin1String(":/images/blacklisted_fail.png")),
        QIcon(QLatin1String(":/images/benchmark.png")),
        QIcon(QLatin1String(":/images/debug.png")),
        QIcon(QLatin1String(":/images/warn.png")),
        QIcon(QLatin1String(":/images/fatal.png")),
    }; // provide an icon for unknown??

    if (result < 0 || result >= Result::MessageInternal) {
        switch (result) {
        case Result::MessageTestCaseSuccess:
            return icons[Result::Pass];
        case Result::MessageTestCaseFail:
            return icons[Result::Fail];
        case Result::MessageTestCaseWarn:
            return icons[Result::MessageWarn];
        default:
            return QIcon();
        }
    }
    return icons[result];
}

QVariant TestResultItem::data(int column, int role) const
{
    if (role == Qt::DecorationRole)
        return m_testResult ? testResultIcon(m_testResult->result()) : QVariant();

    return Utils::TreeItem::data(column, role);
}

void TestResultItem::updateDescription(const QString &description)
{
    QTC_ASSERT(m_testResult, return);

    m_testResult->setDescription(description);
}

void TestResultItem::updateResult()
{
    if (m_testResult->result() != Result::MessageTestCaseStart)
        return;

    Result::Type newResult = Result::MessageTestCaseSuccess;
    foreach (Utils::TreeItem *child, children()) {
        const TestResult *current = static_cast<TestResultItem *>(child)->testResult();
        if (current) {
            switch (current->result()) {
            case Result::Fail:
            case Result::MessageFatal:
            case Result::UnexpectedPass:
                m_testResult->setResult(Result::MessageTestCaseFail);
                return;
            case Result::ExpectedFail:
            case Result::MessageWarn:
            case Result::Skip:
            case Result::BlacklistedFail:
            case Result::BlacklistedPass:
                newResult = Result::MessageTestCaseWarn;
                break;
            default: {}
            }
        }
    }
    m_testResult->setResult(newResult);
}

/********************************* TestResultModel *****************************************/

TestResultModel::TestResultModel(QObject *parent)
    : Utils::TreeModel(parent),
      m_widthOfLineNumber(0),
      m_maxWidthOfFileName(0)
{
}

QVariant TestResultModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    if (role == Qt::DecorationRole)
        return itemForIndex(idx)->data(0, Qt::DecorationRole);

    return QVariant();
}

void TestResultModel::addTestResult(TestResult *testResult, bool autoExpand)
{
    const bool isCurrentTestMssg = testResult->result() == Result::MessageCurrentTest;

    QVector<Utils::TreeItem *> topLevelItems = rootItem()->children();
    int lastRow = topLevelItems.size() - 1;
    TestResultItem *newItem = new TestResultItem(testResult);
    // we'll add the new item, so raising it's counter
    if (!isCurrentTestMssg) {
        int count = m_testResultCount.value(testResult->result(), 0);
        m_testResultCount.insert(testResult->result(), ++count);
    } else {
        // MessageCurrentTest should always be the last top level item
        if (lastRow >= 0) {
            TestResultItem *current = static_cast<TestResultItem *>(topLevelItems.at(lastRow));
            const TestResult *result = current->testResult();
            if (result && result->result() == Result::MessageCurrentTest) {
                current->updateDescription(testResult->description());
                emit dataChanged(current->index(), current->index());
                return;
            }
        }

        rootItem()->appendChild(newItem);
        return;
    }

    // FIXME this might be totally wrong... we need some more unique information!
    for (int row = lastRow; row >= 0; --row) {
        TestResultItem *current = static_cast<TestResultItem *>(topLevelItems.at(row));
        const TestResult *result = current->testResult();
        if (result && result->className() == testResult->className()) {
            current->appendChild(newItem);
            if (autoExpand)
                current->expand();
            if (testResult->result() == Result::MessageTestCaseEnd) {
                current->updateResult();
                emit dataChanged(current->index(), current->index());
            }
            return;
        }
    }
    // if we have a MessageCurrentTest present, add the new top level item before it
    if (lastRow >= 0) {
        TestResultItem *current = static_cast<TestResultItem *>(topLevelItems.at(lastRow));
        const TestResult *result = current->testResult();
        if (result && result->result() == Result::MessageCurrentTest) {
            rootItem()->insertChild(current->index().row(), newItem);
            return;
        }
    }

    rootItem()->appendChild(newItem);
}

void TestResultModel::removeCurrentTestMessage()
{
    QVector<Utils::TreeItem *> topLevelItems = rootItem()->children();
    for (int row = topLevelItems.size() - 1; row >= 0; --row) {
        TestResultItem *current = static_cast<TestResultItem *>(topLevelItems.at(row));
        if (current->testResult()->result() == Result::MessageCurrentTest) {
            delete takeItem(current);
            break;
        }
    }
}

void TestResultModel::clearTestResults()
{
    clear();
    m_testResultCount.clear();
    m_processedIndices.clear();
    m_maxWidthOfFileName = 0;
    m_widthOfLineNumber = 0;
}

TestResult TestResultModel::testResult(const QModelIndex &idx)
{
    if (idx.isValid())
        return *(static_cast<TestResultItem *>(itemForIndex(idx))->testResult());

    return TestResult();
}

int TestResultModel::maxWidthOfFileName(const QFont &font)
{
    if (font != m_measurementFont) {
        m_processedIndices.clear();
        m_maxWidthOfFileName = 0;
        m_measurementFont = font;
    }

    const QFontMetrics fm(font);
    const QVector<Utils::TreeItem *> &topLevelItems = rootItem()->children();
    const int count = topLevelItems.size();
    for (int row = 0; row < count; ++row) {
        int processed = row < m_processedIndices.size() ? m_processedIndices.at(row) : 0;
        const QVector<Utils::TreeItem *> &children = topLevelItems.at(row)->children();
        const int itemCount = children.size();
        if (processed < itemCount) {
            for (int childRow = processed; childRow < itemCount; ++childRow) {
                const TestResultItem *item = static_cast<TestResultItem *>(children.at(childRow));
                if (const TestResult *result = item->testResult()) {
                    QString fileName = result->fileName();
                    const int pos = fileName.lastIndexOf(QLatin1Char('/'));
                    if (pos != -1)
                        fileName = fileName.mid(pos + 1);
                    m_maxWidthOfFileName = qMax(m_maxWidthOfFileName, fm.width(fileName));
                }
            }
            if (row < m_processedIndices.size())
                m_processedIndices.replace(row, itemCount);
            else
                m_processedIndices.insert(row, itemCount);
        }
    }
    return m_maxWidthOfFileName;
}

int TestResultModel::maxWidthOfLineNumber(const QFont &font)
{
    if (m_widthOfLineNumber == 0 || font != m_measurementFont) {
        QFontMetrics fm(font);
        m_measurementFont = font;
        m_widthOfLineNumber = fm.width(QLatin1String("88888"));
    }
    return m_widthOfLineNumber;
}

/********************************** Filter Model **********************************/

TestResultFilterModel::TestResultFilterModel(TestResultModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sourceModel(sourceModel)
{
    setSourceModel(sourceModel);
    enableAllResultTypes();
}

void TestResultFilterModel::enableAllResultTypes()
{
    m_enabled << Result::Pass << Result::Fail << Result::ExpectedFail
              << Result::UnexpectedPass << Result::Skip << Result::MessageDebug
              << Result::MessageWarn << Result::MessageInternal
              << Result::MessageFatal << Result::Invalid << Result::BlacklistedPass
              << Result::BlacklistedFail << Result::Benchmark
              << Result::MessageCurrentTest << Result::MessageTestCaseStart
              << Result::MessageTestCaseSuccess << Result::MessageTestCaseWarn
              << Result::MessageTestCaseFail << Result::MessageTestCaseEnd;
    invalidateFilter();
}

void TestResultFilterModel::toggleTestResultType(Result::Type type)
{
    if (m_enabled.contains(type)) {
        m_enabled.remove(type);
        if (type == Result::MessageInternal)
            m_enabled.remove(Result::MessageTestCaseEnd);
    } else {
        m_enabled.insert(type);
        if (type == Result::MessageInternal)
            m_enabled.insert(Result::MessageTestCaseEnd);
    }
    invalidateFilter();
}

void TestResultFilterModel::clearTestResults()
{
    m_sourceModel->clearTestResults();
}

bool TestResultFilterModel::hasResults()
{
    return rowCount(QModelIndex());
}

TestResult TestResultFilterModel::testResult(const QModelIndex &index) const
{
    return m_sourceModel->testResult(mapToSource(index));
}

bool TestResultFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = m_sourceModel->index(sourceRow, 0, sourceParent);
    if (!index.isValid())
        return false;
    return m_enabled.contains(m_sourceModel->testResult(index).result());
}

} // namespace Internal
} // namespace Autotest
