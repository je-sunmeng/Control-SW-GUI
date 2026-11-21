#include "net/FrameDecoder.h"

#include <QtTest>

class TstFrameDecoder : public QObject {
    Q_OBJECT
private slots:
    void single();
    void partial_byteByByte();
    void merged_multiple();
    void emptyPayload();
    void oversize_resets();
};

void TstFrameDecoder::single()
{
    FrameDecoder d(1024);
    const QByteArray f = MessageHeader::frame(0x21920001u, QByteArray("hello"));
    const auto frames = d.feed(f);
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames[0].header.opcode, 0x21920001u);
    QCOMPARE(frames[0].payload, QByteArray("hello"));
    QCOMPARE(frames[0].raw, f);
    QCOMPARE(d.buffered(), 0);
}

void TstFrameDecoder::partial_byteByByte()
{
    FrameDecoder d(1024);
    const QByteArray f = MessageHeader::frame(0xB1210100u, QByteArray(37, 'z'));
    int got = 0;
    for (int i = 0; i < f.size(); ++i) {
        const auto frames = d.feed(f.mid(i, 1));
        if (i < f.size() - 1)
            QVERIFY(frames.isEmpty());
        got += int(frames.size());
    }
    QCOMPARE(got, 1);
}

void TstFrameDecoder::merged_multiple()
{
    FrameDecoder d(1024);
    QByteArray stream;
    for (int i = 0; i < 5; ++i)
        stream += MessageHeader::frame(quint32(i), QByteArray(i * 3, char('a' + i)));
    // 3.5 프레임 + 나머지로 분할 전송
    const int cut = MessageHeader::Size * 3 + 0 + 3 + 6 + 4;
    auto first = d.feed(stream.left(cut));
    QCOMPARE(first.size(), 3);
    auto second = d.feed(stream.mid(cut));
    QCOMPARE(second.size(), 2);
    QCOMPARE(second[1].payload, QByteArray(12, 'e'));
    QCOMPARE(d.buffered(), 0);
}

void TstFrameDecoder::emptyPayload()
{
    FrameDecoder d(16);
    const auto frames = d.feed(MessageHeader::frame(0x21B10012u, {}) + MessageHeader::frame(0x21920012u, {}));
    QCOMPARE(frames.size(), 2);
    QVERIFY(frames[0].payload.isEmpty());
}

void TstFrameDecoder::oversize_resets()
{
    FrameDecoder d(100);
    const QByteArray bad = MessageHeader{0x21920001u, 101}.encode() + QByteArray(10, 'x');
    QVERIFY(d.feed(bad).isEmpty());
    QVERIFY(d.hadError());
    QCOMPARE(d.buffered(), 0);
    // 리셋 후 정상 프레임 수신 가능
    QCOMPARE(d.feed(MessageHeader::frame(1, "ok")).size(), 1);
}

QTEST_GUILESS_MAIN(TstFrameDecoder)
#include "tst_FrameDecoder.moc"
