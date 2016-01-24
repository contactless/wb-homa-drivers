#include "testlog.h"
#include "fake_serial_port.h"
#include "ivtm_protocol.h"

namespace {
    PRegister Dev1Temp(new TRegister(0x01, 0, 0, Float));
    PRegister Dev1Humidity(new TRegister(0x01, 0, 4, Float));
    PRegister Dev2Temp(new TRegister(0x0a, 0, 0, Float));
};

class TIVTMProtocolTest: public TSerialProtocolTest
{
protected:
    void SetUp();
    PIVTMProtocol Proto;
};

void TIVTMProtocolTest::SetUp()
{
    TSerialProtocolTest::SetUp();
    // Protocol should not depend on slave id from DeviceConfig
    Proto = std::make_shared<TIVTMProtocol>(
        std::make_shared<TDeviceConfig>("ivtm", 0x0001, "ivtm"),
        SerialPort);
    SerialPort->Open();
}

TEST_F(TIVTMProtocolTest, IVTM7MQuery)
{
    // >> 24 30 30 30 31 52 52 30 30 30 30 30 34 41 44 0d
    // << 21 30 30 30 31 52 52 43 45 44 33 44 31 34 31 35 46 0D 
    // temperature == 26.228420

    SerialPort->Expect(
        {
            '$', '0', '0', '0', '1', 'R', 'R', '0', '0', '0', '0', '0', '4', 'A', 'D', 0x0d
        },
        {
            // Session setup response
            '!',                  // header
            '0', '0', '0', '1',   // slave addr
            'R', 'R',             // read response
            'C', 'E', 'D', '3', 'D', '1', '4', '1', //temp data CE D3 D1 41 (little endian)
            '5', 'F',             //CRC
            0x0d                  // footer
        });

    ASSERT_EQ(0x41D1D3CE, Proto->ReadRegister(Dev1Temp)); //big-endian

	// >> 24 30 30 30 31 52 52 30 30 30 34 30 34 42 31 0d
	// << 21 30 30 30 31 52 52 33 30 39 41 45 42 34 31 34 46 0D
    // humidity == 29.450287

    SerialPort->Expect(
        {
            '$', '0', '0', '0', '1', 'R', 'R', '0', '0', '0', '4', '0', '4', 'B', '1', 0x0d
        },
        {
            // Session setup response
            '!',                  // header
            '0', '0', '0', '1',   // slave addr
            'R', 'R',             // read response
            '3', '0', '9', 'A', 'E', 'B', '4', '1', //hum data 30 9A EB 41 (little endian)
            '4', 'F',             //CRC
            0x0D                  // footer
        });

    ASSERT_EQ(0x41EB9A30, Proto->ReadRegister(Dev1Humidity)); //big-endian

    Proto->EndPollCycle();

    // Test upper-case hex letters

    // >> 24 30 30 30 41 52 52 30 30 30 30 30 34 42 44 0d
    // << 21 30 30 30 41 52 52 35 45 38 35 43 37 34 31 35 43 0D
    // temperature == 24.940121

    SerialPort->Expect(
        {
            '$', '0', '0', '0', 'A', 'R', 'R', '0', '0', '0', '0', '0', '4', 'B', 'D', 0x0d
        },
        {
            // Session setup response
            '!',                  // header
            '0', '0', '0', 'A',   // slave addr
            'R', 'R',             // read response
            '5', 'E', '8', '5', 'C', '7', '4', '1', //temp data 5E 85 C7 41 (little endian)
            '5', 'C',             //CRC
            0x0d                  // footer
        });

    ASSERT_EQ(0x41C7855E, Proto->ReadRegister(Dev2Temp)); //big-endian
    SerialPort->Close();
}
