Table of Contents
=================

  * [Table of Contents](#table-of-contents)
  * [wb-homa-drivers](#wb-homa-drivers)
  * [wb-homa-adc](wb-homa-adc/README.md)
  * [wb-homa-modbus](#wb-homa-modbus)
    * [Устройства Uniel](#%D0%A3%D1%81%D1%82%D1%80%D0%BE%D0%B9%D1%81%D1%82%D0%B2%D0%B0-uniel)
    * [Таблица шаблонов device_type](#%D0%A2%D0%B0%D0%B1%D0%BB%D0%B8%D1%86%D0%B0-%D1%88%D0%B0%D0%B1%D0%BB%D0%BE%D0%BD%D0%BE%D0%B2-device_type)
  * [wb-homa-gpio драйвер](#wb-homa-gpio-%D0%B4%D1%80%D0%B0%D0%B9%D0%B2%D0%B5%D1%80)



wb-homa-drivers
===============
MQTT drivers for compatible with HomA conventions
    * Kernel 1-wire interface driver (currently thermometers only)
    * GPIO-drivern switches driver
    * Ninja Cloud (ninjablocks.com) bridge


Lanuage: C++11, g++-4.7 is required




wb-homa-modbus
==============

Драйвер Modbus запускается командой `/etc/init.d/wb-homa-modbus start`
По умолчанию запуск драйвера происходит при загрузке системы при
наличии конфигурационного файла `/etc/wb-homa-modbus.conf`. При
установке пакета создаётся пример конфигурационного файла -
`/etc/wb-homa-modbus.conf.sample`. Для начала работы с
Modbus-устройствами необходимо создать корректный файл конфигурации и
запустить демона:

```
# cp /etc/wb-homa-modbus.conf.sample /etc/wb-homa-modbus.conf
# sensible-editor /etc/wb-homa-modbus.conf
# service wb-homa-modbus start
```

Возможен запуск демона вручную, что может быть полезно
для работы в отладочном режиме:

```
# service wb-homa-modbus stop
# wb-homa-modbus -c /etc/wb-homa-modbus.conf -d
```

Конфигурационный файл построен по трёхуровневой схеме:
порты (ports) -> устройства (devices) -> каналы (channels).
Конфигурация устройства device может быть задана двумя способами:
вручную прописать все параметры или задать только пять параметров:
```
{
    // по DeviceType драйвер будет искать  параметры в папке
    // /usr/share/wb-homa-modbus/templates
    "device_type" : "DeviceType",

    // отображаемое имя устройства. Публикуется как
    // .../meta/name в MQTT
    // По умолчанию name берется из шаблона и добавляется slave_id, т.е.
    // "name" + " " + "slave_id"
    "name" : "somename",

    // уникальный идентификатор устройства в MQTT.
    // каждое элемент в devices должен иметь уникальный id
    // topic'и, относящиеся в MQTT к данному устройству,
    // имеют общий префикс /devices/<идентификатор топика>/...
    // также по умолчанию берется из шаблона с добавлением slave_id:
    // "deviceID" + "_" + slave_id
    "id" : "deviceID",

    // идентификатор Modbus slave
    "slave_id" : slaveID,

    "enabled" : true
    // включить/выключить устройство. В случае задания
    // "enabled": false опрос устройства и запись значений
    // его каналов не происходит. По умолчанию - true.
}
```
Структура папки *templates* такова, что в каждом файле
приведены параметры для одного типа устройств.
Также можно совместить первый способ со вторым, к вышеприведенным
5 параметрам дописать конфигурацию для каналов, которые не прописаны
в соответствующем файле в папке templates.
См. также: [пример конфигурационного файла с использованием шаблонов](wb-homa-modbus/config.json).

Ниже приведён пример конфигурационного файла /etc/wb-homa-modbus.conf

```
{
    // опция debug включает или выключает отладочную печать.
    // Опция -d командной строки wb-homa-modbus также
    // включает отладочную печать и имеет приоритет над
    // данной опцией.
    "debug": false,

    // список портов
    "ports": [
        {
            // устройство, соответствующее порту RS-485
            "path" : "/dev/ttyNSC0",

            // скорость порта
            "baud_rate": 9600,

            // паритет - N, O или E (по умолчанию - N)
            "parity": "N",

            // количество бит данных (по умолчанию - 8)
            "data_bits": 8,

            // количество стоп-бит
            "stop_bits": 2,

            // интервал опроса устройств на порту в миллисекундах
            "poll_interval": 10,

            // включить/выключить порт. В случае задания
            // "enabled": false опрос порта и запись значений
            // каналов в устройства на данном порту не происходит.
            // По умолчанию - true.
            "enabled": true,

            // список устройств на данном порту
            "devices" : [
                {
                    // отображаемое имя устройства. Публикуется как
                    // .../meta/name в MQTT
                    "name": "MSU34+TLP",

                    // уникальный идентификатор устройства в MQTT.
                    // каждое элемент в devices должен иметь уникальный id
                    // topic'и, относящиеся в MQTT к данному устройству,
                    // имеют общий префикс /devices/<идентификатор топика>/...
                    "id": "msu34tlp",

                    // идентификатор Modbus slave
                    "slave_id": 2,

                    // включить/выключить устройство. В случае задания
                    // "enabled": false опрос устройства и запись значений
                    // его каналов не происходит. По умолчанию - true.
                    "enabled": true,

                    // список каналов устройства
                    "channels": [
                        {
                            // имя канала. topic'и, соответствующие каналу,
                            // публикуются как
                            // /devices/<идентификатор канала>/controls/<имя канала>
                            "name" : "Temp 1",

                            // тип регистра Modbus.
                            // возможные значения:
                            // "coil" - 1 бит, чтение/запись
                            // "discrete" - 1 бит, только чтение
                            // "holding" - 16 бит, чтение/запись
                            // "input" - 16 бит, только чтение
                            "reg_type" : "input",

                            // адрес регистра Modbus
                            "address" : 0,

                            // тип элемента управления в homA, например,
                            // "temperature", "text", "switch"
                            // Тип wo-switch задаёт вариант switch,
                            // для которого не производится опрос регистра -
                            // для таких каналов возможна только запись.
                            "type": "temperature",

                            // формат канала. Задаётся для регистров типа
                            // "holding" и "input". Возможные значения:
                            // "u16" - беззнаковое 16-битное целое
                            //         (используется по умолчанию)
                            // "s16" - знаковое 16-битное целое
                            // "u8" - беззнаковое 8-битное целое
                            // "s8" - знаковое 8-битное целое
                            "format": "s8"

                            // для регистров типа coil и discrete
                            // с типом отображения switch/wo-swich
                            // также допускается задание on_value -
                            // числового значения, соответствующего
                            // состоянию "on" (см. ниже)
                        },
                        {
                            // Ещё один канал
                            "name" : "Illuminance",
                            "reg_type" : "input",
                            "address" : 1,
                            "type": "text"
                        },
                        {
                            "name" : "Pressure",
                            "reg_type" : "input",
                            "address" : 2,
                            "type": "text",
                            "scale": 0.075
                        },
                        {
                            "name" : "Temp 2",
                            "reg_type" : "input",
                            "address" : 3,
                            "type": "temperature",
                            "format": "s8"
                        }
                    ]
                },
                {
                    // ещё одно устройство на канале
                    "name": "DRB88",
                    "id": "drb88",
                    "enabled": true,
                    "slave_id": 22,

                    // секция инициализации
                    "setup": [
                        {
                            // название регистра (для отладки)
                            // Выводится в случае включённой отладочной печати.
                            "title": "Input 0 type",
                            // адрес holding-регистра
                            "address": 1,
                            // значение для записи
                            "value": 1
                        },
                        {
                            "title": "Input 0 module",
                            "address": 3,
                            "value": 3 // was: 11
                        }
                    ],
                    "channels": [
                        {
                            "name" : "Relay 1",
                            "reg_type" : "coil",
                            "address" : 0,
                            "type": "switch"
                        },
                        {
                            "name" : "Relay 2",
                            "reg_type" : "coil",
                            "address" : 1,
                            "type": "switch"
                        },
                        // ...
                        {
                            "name" : "Input 2",
                            "reg_type" : "input",
                            "address" : 1,
                            "type": "switch",
                            // значение, соответствующее состоянию "on"
                            "on_value": 101
                        },
                        {
                            "name" : "Input 3",
                            "reg_type" : "input",
                            "address" : 2,
                            "type": "switch",
                            "on_value": 101
                        },
                        // ...
                    ]
                }
            ]
        },
        {
            // ещё один порт со своим набором устройств
            "path" : "/dev/ttyNSC1",
            "baud_rate": 9600,
            "parity": "N",
            "data_bits": 8,
            "stop_bits": 1,
            "poll_interval": 100,
            "enabled": true,
            "devices" : [
                {
                    "name": "tM-P3R3",
                    "id": "tmp3r3",
                    "enabled": true,
                    "slave_id": 1,
                    "channels": [
                        {
                            "name" : "Relay 0",
                            "reg_type" : "coil",
                            "address" : 0,
                            "type": "switch"
                        },
                        // ...
                    ]
                },
                // ...
            ]
        }
    ]
}
```


Устройства Uniel
-------------
В драйвере wb-homa-modbus реализована поддержка некоторых устройств Uniel (smart.uniel.ru).
Эти устройства используют собственный протокол, отличный от Modbus-RTU.

Поддерживаются устройства:
UCH-M111RX/0808 (модуль реле), UCH-M131RC/0808 (фазовый диммер на 220В), UCH-M141RC/0808 (диммер светодиодных лент),
UCH-M121RX/0808 (модуль реле с встроенной логикой, работа не проверялась).


Для работы устройств необходимо выставить тип "uniel" для порта, работа с устройствами Modbus на одном порте не поддерживается.

Все типы регистров (input, holding, coil) равнозначны и соответствуют однобайтным регистрам ("параметрам") протокола Uniel,
чтение и запись которых производится через команды 0x05 и 0x06. Допустимые значения адреса регистров: 0-255 ("0x00"-"0xff").

Дополнительно поддерживаются регистры, доступные на запись через команду 0x0A, а на чтение через команду 0x05.
Такие регистры обозначаются следующим образом:
```
"address": "0x0100WWRR"
```
здесь WW - шестнадцатиречный адрес регистра (параметра) для записи с помощью команды 0x0A,
RR - шестнадцатиречный адрес регистра (параметра) для чтения командой 0x05.

Пример (чтение по адресу 0x41, запись командой 0x0A по адресу 0x01):

```
                        {
                            "name" : "LED 1",
                            "reg_type": "holding",
                            "address": "0x01000141",
                            "type": "range",
                            "max": "0xff"
                        },
```
Для устройств типа "uniel" также существуют шаблоны конфигурации, для примера в приведенном ниже файле приведены разные варианты записи параметров,
первое устройство задано через шаблон, второй device через шаблон, но параметры "name" и "id" заданы, и также можно добавить конфиг для канала, который
добавится к тем, что есть в шаблоне(но он не должен описывать тот же канал что уже есть в шаблоне), и параметры третьего устройства записаны явно. Шаблоны для устройств типа "uniel" также находятся
в папке /usr/share/wb-homa-modbus/templates/

См. также: [пример конфигурационного файла](wb-homa-modbus/config-uniel.json).




Таблица шаблонов device_type
-------------

**Modbus-RTU**

|                          Device                         | device_type | id_prefix | name_prefix |
|:-------------------------------------------------------:|:-----------:|:---------:|:-----------:|
| "Разумный дом" четырёхканальный диммер светодиодов DDL4 |    DDL24    |   ddl24   |    DDL24    |
|           "Разумный дом" релейный модуль DRB88          |    DRB88    |   drb88   |    DRB88    |
|      "ICP DAS" модуль управления освещением LC-103      |    LC-103   |   lc-103  |    LC-103   |
|                   "Разумный дом" MSU24                  |    MSU24    |   msu24   |    MSU24    |
|                   "Разумный дом" MSU21                  |    MSU21    |   msu21   |    MSU21    |
|                   "Разумный дом" MSU34                  |    MSU34    |   msu34   |    MSU34    |
|          "ICP DAS" модуль ввода-вывода TM-P3R3          |   TM-P3R3   |  TM-P3R3  |    tmp3r3   |
|            "Бесконтактные устройства" RGB-диммер WB-MRGB|   WB-MRGB   |  WB-MRGB  |   wb-mrgb   |
|  "Бесконтактные устройства" Релейный модуль WB-MRM2     |   WB-MRM2   |  WB-MRM2  |   wb-mrm2   |


**Uniel**


|                     Device                     | device_type | id_prefix | name_prefix      |
|:----------------------------------------------:|:-----------:|:---------:|:----------------:|
| Модуль управления освещением UCH-M111RX/0808   | UCH-M111RX  | uchm111rx | UCH-M111RX 0808  |
|  Модуль управления автоматикой UCH-M121RX/0808 | UCH-M121RX  | uchm121rx | UCH-M121RX 0808  |
|    Диммер светодиодных ламп UCH-M141RC/0808    | UCH-M141RC  | uchm141rc | UCH-M141RC 0808  |

wb-homa-gpio драйвер
====================
Драйвер wb-homa-gpio получает необходимые параметры для запуска с файла /etc/wb-homa-gpio.conf, в этом файле должны находиться
описания GPIO, с которыми драйвер должен работать. Примеры конфигурационных файлов находятся в папке /usr/share/wb-homa-gpio/.
Конфигурационный файл построен по двухуровневой схеме :
```
{
    // отображаемое имя устройства. Публикуется как
    // .../meta/name в MQTT
    "device_name" : "GPIO DEVICES",

    "channels" : [
    ]
}
```
Описание одного канала соответствует описанию отдельного Gpio. Параметры, которые описывают Gpio, ниже в примерах:
```
{
    "device_name" : "GPIO DEVICES",

    "channels" : [
    // канал, в котором не указан direction, по умолчанию работает на выход
        {
    // name значение которое передается в MQTT в примере ../controls/FET_1
            "name" : "FET_1",

    // номер Gpio, который описывает данный канал
            "gpio" : 22,

    // параметр inverted задает инвертированный канал или нет
            "inverted" : false
        }

    //также gpio могут работать на вход и считывать изменения напряжения с клеммника
        {
            "name" : "D1_IN",
            "gpio" : 36,
            "inverted" : false,

    //для этого указать directoin input
            "direction" : "input"

    // rising прерывания по восходящему фронту, falling по нисходящему,
    //both по обоим фронтам, для GPIO с незаданным type по умолчанию устанавливается both
    //для счетчиков определяется автоматически, если не указан.
            "edge" : "falling"
        }

    //для работы с счетчиком электроэнергии
        {
            "name" : "D3_IN",
            "gpio" : 38,
            "direction" : "input",
            "inverted" : false,

    // type указывает что gpio описывает счетчик электроэнергии
            "type" : "watt_meter",

    //множитель для расчета, количество импульсов на kWh
            "multiplier" : 1000
        },

    //для работы с счетчиком воды
        {
            "name" : "D3_IN",
            "gpio" : 38,
            "direction" : "input",
            "inverted" : false,

    // type указывает что gpio описывает счетчик воды
            "type" : "water_meter",

    //множитель для расчета, количество импульсов на м^3
            "multiplier" : 100
        }
    ]
}

```
wb-mqtt-timestamper 
====================

Служба, которая фиксирует время изменения контрола в MQTT,  
запускается скриптом `/etc/init.d/wb-mqtt-timestamper start`. К каждому контролу публикуется время его изменения 
в топик `/devices/+/controls/CONTROL_NAME/meta/ts` в формате Unix. Параметры запуска службы можно изменить в файле 
`/etc/defaults/wb-mqtt-timestamper` или непосредственно в командной строке.
Описания параметров доступны в help(```wb-mqtt-timestamper -h```).
