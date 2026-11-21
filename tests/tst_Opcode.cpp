#include "net/MessageHeader.h"
#include "net/Opcode.h"

#include <QtTest>

class TstOpcode : public QObject {
    Q_OBJECT
private slots:
    void packUnpack();
    void toString_format();
    void header_bigEndian();
    void header_decodeShort();
};

void TstOpcode::packUnpack()
{
    constexpr quint32 op = Opcode::make(0x21, 0x92, 0x0010);
    static_assert(op == 0x21920010u);
    static_assert(Opcode::src(op) == 0x21);
    static_assert(Opcode::dst(op) == 0x92);
    static_assert(Opcode::cmd(op) == 0x0010);
    QCOMPARE(Opcode::make(0xB1, 0x21, 0xFFFF), 0xB121FFFFu);
}

void TstOpcode::toString_format()
{
    QCOMPARE(Opcode::toString(0x21920010u), QStringLiteral("0x2192 0010"));
    QCOMPARE(Opcode::toString(0xB1210100u), QStringLiteral("0xB121 0100"));
}

void TstOpcode::header_bigEndian()
{
    const MessageHeader h{0x21B10020u, 0x00000124u};
    QCOMPARE(h.encode(), QByteArray::fromHex("21B1002000000124"));
    const auto back = MessageHeader::decode(QByteArray::fromHex("21B1002000000124FFFF"));
    QVERIFY(back);
    QCOMPARE(back->opcode, 0x21B10020u);
    QCOMPARE(back->dataSize, 0x124u);
    QCOMPARE(MessageHeader::frame(0x21920012u, QByteArray()), QByteArray::fromHex("2192001200000000"));
    QCOMPARE(MessageHeader::frame(1, QByteArray("ab")).size(), 10);
}

void TstOpcode::header_decodeShort()
{
    QVERIFY(!MessageHeader::decode(QByteArray(7, '\0')));
}

QTEST_GUILESS_MAIN(TstOpcode)
#include "tst_Opcode.moc"
