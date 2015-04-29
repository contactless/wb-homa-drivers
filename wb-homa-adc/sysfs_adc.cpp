#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>
#include <ctime>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <math.h>

#include "sysfs_adc.h"
namespace {
        extern "C" int imx233_rd(long offset);
        extern "C" int imx233_wr(long offset, long value);
        extern "C" void usleep(int value);
};
/*
namespace {
    struct ChannelName {
        int n;
        const char* name;
    };

    ChannelName channel_names[] = {
        {0, "adc0"},
        {1, "adc1"},
        {2, "adc2"},
        {3, "adc3"},
        {4, "adc4"},
        {5, "adc5"},
        {6, "adc6"},
        {7, "adc7"},
#if 0
        // later
        {0, "a1"},
        {0, "adc1"},
        {1, "a2"},
        {1, "adc2"},
        {2, "a3"},
        {2, "adc3"},
        {3, "a4"},
        {3, "adc4"},
        {4, "r1"},
        {5, "r4"},
        {6, "r2"},
        {7, "r3"},
#endif
        {-1, 0}
    };
    int GetChannelIndex(const std::string& name)
    {
        std::string locase_name = name;
        std::transform(locase_name.begin(), locase_name.end(), locase_name.begin(), ::tolower);
        for (ChannelName* name_item = channel_names; name_item->n >= 0; ++name_item) {
            if (locase_name == name_item->name)
                return name_item->n;
        }
        throw TAdcException("invalid channel name " + name);
    }

    int GetGPIOFromEnv(const std::string& name)
    {
        char* s = getenv(name.c_str());
        if (!s)
            throw TAdcException("Environment variable not set: " + name);
        try {
            return std::stoi(s);
        } catch (std::exception) {
            throw TAdcException("Invalid value of environment variable '" + name + "': " + s);
        }
    }
};
*/
TSysfsAdc::TSysfsAdc(const std::string& sysfs_dir, bool debug, const TChannel& channel_config)
    : SysfsDir(sysfs_dir),
    ChannelConfig(channel_config),
    MaxVoltage(ChannelConfig.MaxVoltage)
{
    ScaleFactor = ADC_DEFAULT_SCALE_FACTOR;
    AveragingWindow = ChannelConfig.AveragingWindow;
    Debug = debug;
    Initialized = false;
    string path_to_value = SysfsDir + "/bus/iio/devices/iio:device0/in_voltage" + to_string(ChannelConfig.ChannelNumber) + "_raw";
    AdcValStream.open(path_to_value);
    if (AdcValStream < 0) {
        throw TAdcException("error opening sysfs Adc file");
    }
    string scale_prefix = "/sys/bus/iio/devices/iio:device0/in_voltage" + to_string(ChannelConfig.ChannelNumber) + "_scale";
    ifstream scale_file(scale_prefix + "_available");
    if (scale_file.is_open()) {
        vector<string> scales;
        char c;
        string buf = "";
        while(scale_file.get(c)) {
            if (c == ' ') {
                scales.push_back(buf);
                buf = "";
            } else {
                buf += c;
            }
        }
        double max = ADC_DEFAULT_SCALE_FACTOR;
        int i = 0;
        int position = 0; 
        for (const auto& element : scales) {
            double val = stod(element);
            if (val > max) {
                max = val;
                position = i;
            }
            i++;
        }
        scale_file.close();
        ofstream write_scale(scale_prefix);
        if (!write_scale.is_open()) {
            throw TAdcException("error opening sysfs Adc scale file");
        }
        ScaleFactor = max;
        write_scale << scales[position]; 
        write_scale.close();
    }
    NumberOfChannels = channel_config.Mux.size();
}

TSysfsAdc::~TSysfsAdc()
{
    if (AdcValStream.is_open()){
        AdcValStream.close();
    }
}

std::shared_ptr<TSysfsAdcChannel> TSysfsAdc::GetChannel(int i)
{
    std::shared_ptr<TSysfsAdcChannel> ptr(nullptr);
    // TBD: should pass chain_alias also (to be used instead of Name for the channel)
    if (ChannelConfig.Mux[i].Type == OHM_METER)
        ptr.reset (new TSysfsAdcChannelRes(this, ChannelConfig.Mux[i].MuxChannelNumber, ChannelConfig.Mux[i].Id, ChannelConfig.Mux[i].ReadingsNumber, ChannelConfig.Mux[i].DecimalPlaces, ChannelConfig.Mux[i].DischargeChannel, ChannelConfig.Mux[i].Current, ChannelConfig.Mux[i].Resistance1, ChannelConfig.Mux[i].Resistance2));
    else
        ptr.reset(new TSysfsAdcChannel(this, ChannelConfig.Mux[i].MuxChannelNumber, ChannelConfig.Mux[i].Id, ChannelConfig.Mux[i].ReadingsNumber, ChannelConfig.Mux[i].DecimalPlaces, ChannelConfig.Mux[i].DischargeChannel, ChannelConfig.Mux[i].Multiplier));
    return ptr;
}

int TSysfsAdc::ReadValue()
{
    int val;
    AdcValStream.seekg(0);
    AdcValStream >> val;
    return val;
}

bool TSysfsAdc::CheckVoltage(int value)
{
    float voltage = ScaleFactor * value;
    if (voltage > MaxVoltage) {
        return false;
    }
    return true;
}

void TSysfsAdc::SetMuxABC(int n)
{
    return;
}

TSysfsAdcMux::TSysfsAdcMux(const std::string& sysfs_dir, bool debug, const TChannel& channel_config)
    : TSysfsAdc(sysfs_dir, debug, channel_config)
{
    MinSwitchIntervalMs = ChannelConfig.MinSwitchIntervalMs;
    CurrentMuxInput = -1;
    GpioMuxA = ChannelConfig.Gpios[0];
    GpioMuxB = ChannelConfig.Gpios[1];
    GpioMuxC = ChannelConfig.Gpios[2];
}


int TSysfsAdcMux::GetRawValue(int index)
{
    SetMuxABC(index);
    return ReadValue(); 
}

void TSysfsAdcMux::InitMux()
{
    if (Initialized)
        return;
    InitGPIO(GpioMuxA);
    InitGPIO(GpioMuxB);
    InitGPIO(GpioMuxC);
    Initialized = true;
}

void TSysfsAdcMux::InitGPIO(int gpio)
{
    std::string gpio_direction_path = GPIOPath(gpio, "/direction");
    std::ofstream setdirgpio(gpio_direction_path);
    if (!setdirgpio) {
        std::ofstream exportgpio(SysfsDir + "/class/gpio/export");
        if (!exportgpio)
            throw TAdcException("unable to export GPIO " + std::to_string(gpio));
        exportgpio << gpio << std::endl;
        setdirgpio.clear();
        setdirgpio.open(gpio_direction_path);
        if (!setdirgpio)
            throw TAdcException("unable to set GPIO direction" + std::to_string(gpio));
    }
    setdirgpio << "out";
}

void TSysfsAdcMux::SetGPIOValue(int gpio, int value)
{
    std::ofstream setvalgpio(GPIOPath(gpio, "/value"));
    if (!setvalgpio)
        throw TAdcException("unable to set value of gpio " + std::to_string(gpio));
    setvalgpio << value << std::endl;
}

std::string TSysfsAdcMux::GPIOPath(int gpio, const std::string& suffix) const
{
    return std::string(SysfsDir + "/class/gpio/gpio") + std::to_string(gpio) + suffix;
}

void TSysfsAdcMux::MaybeWaitBeforeSwitching()
{
    if (MinSwitchIntervalMs <= 0)
        return;

    struct timespec tp;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp) < 0)
        throw TAdcException("unable to get timer value");

    if (CurrentMuxInput >= 0) { // no delays before the first switch
        double elapsed_ms = (tp.tv_sec - PrevSwitchTS.tv_sec) * 1000 +
            (tp.tv_nsec - PrevSwitchTS.tv_nsec) / 1000000;
        if (Debug)
            std::cerr << "elapsed: " << elapsed_ms << std::endl;
        if (elapsed_ms < MinSwitchIntervalMs) {
            if (Debug)
                std::cerr << "usleep: " << (MinSwitchIntervalMs - (int)elapsed_ms) * 1000 <<
                    std::endl;
            usleep((MinSwitchIntervalMs - (int)elapsed_ms) * 1000);
        }
    }
    PrevSwitchTS = tp;
}

void TSysfsAdcMux::SetMuxABC(int n)
{
    InitMux();
    if (CurrentMuxInput == n)
        return;
    if (Debug)
        std::cerr << "SetMuxABC: " << n << std::endl;
    SetGPIOValue(GpioMuxA, n & 1);
    SetGPIOValue(GpioMuxB, n & 2);
    SetGPIOValue(GpioMuxC, n & 4);
    CurrentMuxInput = n;
    usleep(MinSwitchIntervalMs * 1000);
}

TSysfsAdcPhys::TSysfsAdcPhys(const std::string& sysfs_dir, bool debug, const TChannel& channel_config)
    : TSysfsAdc(sysfs_dir, debug, channel_config)
{
}

int TSysfsAdcPhys::GetRawValue(int index)
{
   return ReadValue();
}


TSysfsAdcChannel::TSysfsAdcChannel(TSysfsAdc* owner, int index, const std::string& name, int readings_number, int decimal_places, int discharge_channel)
    : DecimalPlaces(decimal_places)
    , d(new TSysfsAdcChannelPrivate())
{
    d->Owner.reset(owner);
    d->Index = index;
    d->Name = name;
    d->ReadingsNumber = readings_number;
    d->ChannelAveragingWindow = readings_number * d->Owner->AveragingWindow;
    d->Buffer = new int[d->ChannelAveragingWindow](); // () initializes with zeros
    d->DischargeChannel = discharge_channel;
}

TSysfsAdcChannel::TSysfsAdcChannel(TSysfsAdc* owner, int index, const std::string& name, int readings_number, int decimal_places, int discharge_channel, float multiplier)
    :TSysfsAdcChannel(owner, index, name, readings_number, decimal_places, discharge_channel)
{
    Multiplier = multiplier;
}

int TSysfsAdcChannel::GetAverageValue()
{
    if (!d->Ready) {
        for (int i = 0; i < d->ChannelAveragingWindow; ++i) {
            int v = d->Owner->GetRawValue(d->Index);
            d->Buffer[i] = v;
            d->Sum += v;
        }
        d->Ready = true;
    } else {
        for (int i = 0; i < d->ReadingsNumber; i++) {
            int v = d->Owner->GetRawValue(d->Index);
            d->Sum -= d->Buffer[d->Pos];
            d->Sum += v;
            d->Buffer[d->Pos++] = v;
            d->Pos %= d->ChannelAveragingWindow;
            this_thread::sleep_for(chrono::milliseconds(DELAY));
        }
    }
    return round(d->Sum / d->ChannelAveragingWindow);
}

const std::string& TSysfsAdcChannel::GetName() const
{
    return d->Name;
}

float TSysfsAdcChannel::GetValue()
{
    float result = -1;
    int value = GetAverageValue();
    if (value < ADC_VALUE_MAX) {
        if (d->Owner->CheckVoltage(value)) {
            result = (float) value * Multiplier / 1000; // set voltage to V from mV
            result *= d->Owner->ScaleFactor / ADC_DEFAULT_SCALE_FACTOR;
        }
    }
    if (result < 0) {
        result = std::nan("");
    }
    return result;
}
std::string TSysfsAdcChannel::GetType()
{
    return "voltage";
}

TSysfsAdcChannelRes::TSysfsAdcChannelRes(TSysfsAdc* owner, int index, const std::string& name, int readings_number, int decimal_places, int discharge_channel, int current, int resistance1, int resistance2)
    : TSysfsAdcChannel(owner, index, name, readings_number, decimal_places, discharge_channel)
{
    Current = current;
    Resistance1 = resistance1;
    Resistance2 = resistance2;
    Type = OHM_METER;
    Ctrl2_val = (current / 20) << 4;
}


float TSysfsAdcChannelRes::GetValue()
{
    if (d->DischargeChannel != -1) {
        d->Owner->SetMuxABC(d->DischargeChannel);
    }
    d->Owner->SetMuxABC(d->Index);
    SetUpCurrentSource(); 
    this_thread::sleep_for(chrono::milliseconds(DELAY));
    int value = GetAverageValue(); 
    float result = -1;
    if (value < ADC_VALUE_MAX) {
        if (d->Owner->CheckVoltage(value)) {
            float voltage = d->Owner->ScaleFactor * value / 1000;// get voltage in V (from mV)
            result = 1.0/ ((Current / 1000000.0) / voltage - 1.0/Resistance1) - Resistance2;
            result = round(result);
        }
    }
    if (result < 0) {
        result = std::nan("");
    }
    SwitchOffCurrentSource();
    return result;
}

std::string TSysfsAdcChannelRes::GetType()
{
        return "resistance";
}
void TSysfsAdcChannelRes::SetUpCurrentSource()
{
    imx233_wr(HW_LRADC_CTRL2_SET, 0x0200); //set TEMP_SENSOR_IENABLE1
    imx233_wr(HW_LRADC_CTRL2_CLR, 0xF0); //clear TEMP_ISRC1
    imx233_wr(HW_LRADC_CTRL2_SET, Ctrl2_val); //set TEMP_ISRC1
}

void TSysfsAdcChannelRes::SwitchOffCurrentSource()
{
    imx233_wr(HW_LRADC_CTRL2_CLR, 0x0200); //set TEMP_SENSOR_IENABLE1=0
}
