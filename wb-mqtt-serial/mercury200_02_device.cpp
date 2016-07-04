#include <iostream>
#include "mercury200_02_device.h"
#include "crc16.h"

REGISTER_PROTOCOL("mercury200", TMercury20002Device, TRegisterTypes({
            { TMercury20002Device::REG_VALUE_ARRAY, "array", "power_consumption", U32, true },
            { TMercury20002Device::REG_PARAM, "param", "value", U32, true }
        }));

TMercury20002Device::TMercury20002Device(PDeviceConfig device_config, PAbstractSerialPort port)
    : TEMDevice(device_config, port) {}

bool TMercury20002Device::ConnectionSetup(uint8_t slave)
{
    // there is nothing needed to be done in this method
    return true;
}

TEMDevice::ErrorType TMercury20002Device::CheckForException(uint8_t* frame, int len, const char** message)
{
    *message = 0;
    if (len != 4 || (frame[1] & 0x0f) == 0)
        return TEMDevice::NO_ERROR;
    switch (frame[1] & 0x0f) {
    case 1:
        *message = "Invalid command or parameter";
        break;
    case 2:
        *message = "Internal meter error";
        break;
    case 3:
        *message = "Insufficient access level";
        break;
    case 4:
        *message = "Can't correct the clock more than once per day";
        break;
    case 5:
        *message = "Connection closed";
        return TEMDevice::NO_OPEN_SESSION;
    default:
        *message = "Unknown error";
    }
    return TEMDevice::OTHER_ERROR;
}

const TMercury20002Device::TEnergyValues& TMercury20002Device::ReadValueArray(uint32_t slave, uint32_t address)
{
    auto it = EnergyCache.find(slave);
    if (it != EnergyCache.end()) {
        return it->second;
    }

    uint8_t cmdBuf[2];
    cmdBuf[0] = (address >> 4) & 0xff; // high nibble = array number, lower nibble = month
    cmdBuf[1] = (address >> 12) & 0x0f; // tariff
    uint8_t buf[MAX_LEN], *p = buf;
    TValueArray a;
    Talk(slave, 0x05, cmdBuf, 2, -1, buf, 16);
    for (int i = 0; i < 4; i++, p += 4) {
        a.values[i] = ((uint32_t)p[1] << 24) +
                      ((uint32_t)p[0] << 16) +
                      ((uint32_t)p[3] << 8 ) +
                       (uint32_t)p[2];
    }

    return CachedValues.insert(std::make_pair(key, a)).first->second;
}

uint32_t TMercury20002Device::ReadParam(uint32_t slave, uint32_t address)
{
    uint8_t cmdBuf[2];
    cmdBuf[0] = (address >> 8) & 0xff; // param
    cmdBuf[1] = address & 0xff; // subparam (BWRI)
    uint8_t subparam = (address & 0xff) >> 4;
    bool isPowerOrPowerCoef = subparam == 0x00 || subparam == 0x03;
    uint8_t buf[3];
    Talk(slave, 0x08, cmdBuf, 2, -1, buf, 3);
    return (((uint32_t)buf[0] & (isPowerOrPowerCoef ? 0x3f : 0xff)) << 16) +
            ((uint32_t)buf[2] << 8) +
             (uint32_t)buf[1];
}

uint64_t TMercury20002Device::ReadRegister(PRegister reg)
{
    switch (reg->Type) {
    case REG_VALUE_ARRAY:
        return ReadValueArray(reg->Slave->Id, reg->Address).values[reg->Address & 0x03];
    case REG_PARAM:
        return ReadParam(reg->Slave->Id, reg->Address & 0xffff);
    default:
        throw TSerialDeviceException("mercury230: invalid register type");
    }
}

void TMercury20002Device::EndPollCycle()
{
    EnergyCache.clear();
    ParamCache.clear();
}
