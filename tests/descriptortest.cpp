// SPDX-FileCopyrightText: 2026 Tobias Kaminski
// SPDX-License-Identifier: MIT

#include "hidpp/reportdescriptor.h"

#include <QRandomGenerator>
#include <QTest>

#include <vector>

using namespace hidpp;

namespace
{

std::vector<std::uint8_t> fromHex(const char *hex)
{
    const QByteArray bytes = QByteArray::fromHex(hex);
    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}

// The report descriptors of the four hidraw nodes of the Logi Bolt Receiver
// on the desk, read from sysfs on 2026-10-24.
const char BoltHidpp[] = "0600ff0901a101851095067508150026ff000901810009019100c00600ff0902a101851195137508150026ff000902810009029100c0";
const char BoltKeyboard[] = "05010906a1019508750115002501050719e029e7810281039505050819012905910295017503910395067508150026ff00050719002aff008100c0";
const char BoltMouse[] =
    "05010902a10185020901a100951075011500250105091901291081029502751016018026ff7f0501093009318106950175081581257f093881069501"
    "050c0a38028106c0c0050c0901a101850395027510150126ff0219012aff028100c005010980a101850495017502150125030982098109838100750115"
    "002501099b810675058103c005010913a101850b950175011500250109e18106750f8103c0";
// Uses push and pop, and a report id in the vendor page that is not HID++.
const char BoltDigitizer[] =
    "050d0905a10185280922a102950275011500250109470942810295017506250409518102750826ff0009308102a4750c26d70a3500469204550e6511"
    "05010930810226fa0646f30209318102b4c00922a102950275011500250109470942810295017506250409518102750826ff0009308102a4750c26d70a"
    "3500469204550e651105010930810226fa0646f30209318102b4c0750725050954810275012501050909018102751027ffff000047ffff0000550c6601"
    "10050d095681026500550085297508250f0955b102852a96000126ff000600ff09c5b102c0";

} // namespace

class DescriptorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void findsTheHidppNodeOfTheBoltReceiver()
    {
        QVERIFY(isHidppReportDescriptor(fromHex(BoltHidpp)));
        QVERIFY(!isHidppReportDescriptor(fromHex(BoltKeyboard)));
        QVERIFY(!isHidppReportDescriptor(fromHex(BoltMouse)));
        QVERIFY(!isHidppReportDescriptor(fromHex(BoltDigitizer)));
    }

    void needsBothReportsInTheVendorPage()
    {
        // Only the short report.
        QVERIFY(!isHidppReportDescriptor(fromHex("0600ff0901a101851095067508c0")));
        // Both report ids, but in the generic desktop page.
        QVERIFY(!isHidppReportDescriptor(fromHex("05010901a10185108511c0")));
        // The vendor page restored by pop.
        QVERIFY(isHidppReportDescriptor(fromHex("0600ff a4 0501 b4 8510 8511")));
    }

    void rejectsMalformedDescriptors()
    {
        // Truncated usage page.
        QVERIFY(!isHidppReportDescriptor(fromHex("0600")));
        // Pop without push.
        QVERIFY(!isHidppReportDescriptor(fromHex("0600ffb485108511")));
        // A long item running past the end.
        QVERIFY(!isHidppReportDescriptor(fromHex("0600ff85108511fe20")));
        std::vector<std::uint8_t> oversized = fromHex(BoltHidpp);
        oversized.resize(MaxReportDescriptorSize + 1, 0);
        QVERIFY(!isHidppReportDescriptor(oversized));
    }

    void survivesRandomDescriptors()
    {
        QRandomGenerator random(2026);
        for (int round = 0; round < 20000; ++round) {
            std::vector<std::uint8_t> bytes(random.bounded(300));
            for (std::uint8_t &byte : bytes) {
                byte = static_cast<std::uint8_t>(random.bounded(256));
            }
            (void)isHidppReportDescriptor(bytes);
        }
    }
};

QTEST_GUILESS_MAIN(DescriptorTest)

#include "descriptortest.moc"
