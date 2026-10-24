// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidpp/report.h"

#include <QTest>

#include <vector>

using namespace hidpp;

namespace
{

Report report(std::vector<std::uint8_t> bytes)
{
    const auto parsed = Report::fromBytes(bytes);
    Q_ASSERT(parsed);
    return *parsed;
}

} // namespace

class ReportTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void acceptsShortAndLongReports()
    {
        const auto shortReport = Report::fromBytes(std::vector<std::uint8_t>{0x10, 0xff, 0x81, 0x02, 0x00, 0x02, 0x00});
        QVERIFY(shortReport);
        QCOMPARE(shortReport->params().size(), 3u);
        QCOMPARE(shortReport->params()[1], 0x02);

        std::vector<std::uint8_t> longBytes(LongReportSize, 0);
        longBytes[0] = LongReportId;
        const auto longReport = Report::fromBytes(longBytes);
        QVERIFY(longReport);
        QCOMPARE(longReport->params().size(), 16u);
    }

    void rejectsOtherReports_data()
    {
        QTest::addColumn<std::vector<std::uint8_t>>("bytes");
        QTest::newRow("empty") << std::vector<std::uint8_t>{};
        QTest::newRow("short truncated") << std::vector<std::uint8_t>{0x10, 0xff, 0x81};
        QTest::newRow("short too long") << std::vector<std::uint8_t>(20, 0x10);
        QTest::newRow("long truncated") << std::vector<std::uint8_t>(7, 0x11);
        QTest::newRow("long too long") << std::vector<std::uint8_t>(21, 0x11);
        QTest::newRow("very long report id 0x12") << std::vector<std::uint8_t>(64, 0x12);
        QTest::newRow("keyboard report") << std::vector<std::uint8_t>{0x01, 0, 0, 0, 0, 0, 0, 0};
    }

    void rejectsOtherReports()
    {
        QFETCH(std::vector<std::uint8_t>, bytes);
        QVERIFY(!Report::fromBytes(bytes));
    }

    void buildsShortRequests()
    {
        const Report request = Report::shortRequest(0x02, 0x00, functionByte(1), {0, 0, 0x5a});
        const std::vector<std::uint8_t> expected{0x10, 0x02, 0x00, 0x1a, 0x00, 0x00, 0x5a};
        QVERIFY(std::ranges::equal(request.bytes(), expected));
    }

    void matchesReplies()
    {
        const Report request = Report::shortRequest(0x02, 0x08, functionByte(1));

        QCOMPARE(matchReply(request, report({0x11, 0x02, 0x08, 0x1a, 85, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})), ReplyMatch::Reply);
        QCOMPARE(matchReply(request, report({0x11, 0x02, 0xff, 0x08, 0x1a, 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})), ReplyMatch::Error20);
        QCOMPARE(matchReply(request, report({0x10, 0x02, 0x8f, 0x08, 0x1a, 0x09, 0x00})), ReplyMatch::Error10);
        QCOMPARE(report({0x10, 0x02, 0x8f, 0x08, 0x1a, 0x09, 0x00}).errorCode(), 0x09);
    }

    void ignoresOtherTraffic()
    {
        const Report request = Report::shortRequest(0x02, 0x08, functionByte(1));

        // Another device index.
        QCOMPARE(matchReply(request, report({0x10, 0x01, 0x08, 0x1a, 0, 0, 0})), ReplyMatch::None);
        // Another software id: a reply to Solaar or logiops.
        QCOMPARE(matchReply(request, report({0x10, 0x02, 0x08, 0x1b, 0, 0, 0})), ReplyMatch::None);
        // Another function of the same feature.
        QCOMPARE(matchReply(request, report({0x10, 0x02, 0x08, 0x0a, 0, 0, 0})), ReplyMatch::None);
        // An error for another tool's request.
        QCOMPARE(matchReply(request, report({0x10, 0x02, 0xff, 0x08, 0x1b, 0x05, 0})), ReplyMatch::None);
        // A notification without a software id.
        QCOMPARE(matchReply(request, report({0x10, 0x02, 0x08, 0x00, 0, 0, 0})), ReplyMatch::None);
    }

    void matchesReceiverRegisters()
    {
        const Report request = Report::shortRequest(ReceiverIndex, 0x81, 0x02);
        QCOMPARE(matchReply(request, report({0x10, 0xff, 0x81, 0x02, 0x00, 0x02, 0x00})), ReplyMatch::Reply);
        QCOMPARE(matchReply(request, report({0x10, 0xff, 0x8f, 0x81, 0x02, 0x02, 0x00})), ReplyMatch::Error10);
        QCOMPARE(matchReply(request, report({0x10, 0xff, 0x81, 0x03, 0x00, 0x02, 0x00})), ReplyMatch::None);
    }
};

QTEST_GUILESS_MAIN(ReportTest)

#include "reporttest.moc"
