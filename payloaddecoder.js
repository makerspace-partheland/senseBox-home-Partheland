// ONLY USE THIS IF YOU HAVE ALL THE SENSORS (TEMP, HUMI, PRESSURE, LUX, UV, PM, SOUND) CONNECTED TO YOU SENSEBOX!

function decodeUplink(input) {
    // bytes is of type Buffer.
    'use strict';
    var TEMPERATURE = 'Temperatur',
        HUMIDITY = 'Luftfeuchte',
        PRESSURE = 'Luftdruck',
        LUX = 'Beleuchtungsstaerke',
        UV = 'UV-Intensitaet',
        PM10 = 'PM10',
        PM25 = 'PM25',
        SOUNDLEVELMETER = 'Lautstaerke',
        CASETEMPERATURE = 'Gehaeuse-Temp',
        PM10_MAX = 'PM10_Max',
        PM25_MAX = 'PM25_Max',
        SOUNDLEVELMETER_MAX = 'Lautstaerke_Max',
        PAYSIZE = "PayloadSize";

    var bytesToInt = function (bytes) {
        var i = 0;
        for (var x = 0; x < bytes.length; x++) {
            i |= +(bytes[x] << (x * 8));
        }
        return i;
    };

    var uint8 = function (bytes) {
        if (bytes.length !== uint8.BYTES) {
            throw new Error('int must have exactly 1 byte');
        }
        return bytesToInt(bytes);
    };
    uint8.BYTES = 1;

    var uint16 = function (bytes) {
        if (bytes.length !== uint16.BYTES) {
            throw new Error('int must have exactly 2 bytes');
        }
        return bytesToInt(bytes);
    };
    uint16.BYTES = 2;

    var humidity = function (bytes) {
        if (bytes.length !== humidity.BYTES) {
            throw new Error('Humidity must have exactly 2 bytes');
        }
        var h = bytesToInt(bytes);
        return h / 1e2;
    };
    humidity.BYTES = 2;

    var decode = function (bytes, mask, names) {
        var maskLength = mask.reduce(function (prev, cur) {
            return prev + cur.BYTES;
        }, 0);
        if (bytes.length < maskLength) {
            throw new Error('Mask length is ' + maskLength + ' whereas input is ' + bytes.length);
        }
        names = names || [];
        var offset = 0;
        return mask
            .map(function (decodeFn) {
                var current = bytes.slice(offset, offset += decodeFn.BYTES);
                return decodeFn(current);
            })
            .reduce(function (prev, cur, idx) {
                prev[names[idx] || idx] = cur;
                return prev;
            }, {});
    };

    var json = {
        data: {},
        warnings: [],
        errors: []
    };

    try {
        json.data = decode(input.bytes,
            [
                uint16,
                humidity,
                uint16,
                uint8,
                uint16,
                uint8,
                uint16,
                uint16,
                uint16,
                uint16,
                uint16,
                uint16,
                uint16,
                uint16
            ],
            [
                TEMPERATURE,
                HUMIDITY,
                PRESSURE,
                LUX + '_mod',
                LUX + '_times',
                UV + '_mod',
                UV + '_times',
                PM10,
                PM25,
                SOUNDLEVELMETER,
                CASETEMPERATURE,
                PM10_MAX,
                PM25_MAX,
                SOUNDLEVELMETER_MAX
            ]);

        // Air Temperature
        json.data[TEMPERATURE] = parseFloat(((json.data[TEMPERATURE] / 771) - 18).toFixed(1));

        // Air humidity
        json.data[HUMIDITY] = parseFloat(json.data[HUMIDITY].toFixed(1));

        // Air pressure
        if (json.data[PRESSURE] !== '0') {
            json.data[PRESSURE] = parseFloat(((json.data[PRESSURE] / 81.9187) + 300).toFixed(1));
        } else {
            delete json.data[PRESSURE];
        }

        // Illuminance
        json.data[LUX] = (json.data[LUX + '_times'] * 255) + json.data[LUX + '_mod'];
        delete json.data[LUX + '_times'];
        delete json.data[LUX + '_mod'];

        // UV
        json.data[UV] = (json.data[UV + '_times'] * 255) + json.data[UV + '_mod'];
        delete json.data[UV + '_times'];
        delete json.data[UV + '_mod'];

        // Particulate Matter
        json.data[PM10] = parseFloat(((json.data[PM10] / 10)).toFixed(1));
        json.data[PM25] = parseFloat(((json.data[PM25] / 10)).toFixed(1));
        json.data[PM10_MAX] = parseFloat(((json.data[PM10_MAX] / 10)).toFixed(1));
        json.data[PM25_MAX] = parseFloat(((json.data[PM25_MAX] / 10)).toFixed(1));

        // Sound Level Meter
        json.data[SOUNDLEVELMETER] = parseFloat(((json.data[SOUNDLEVELMETER] / 10)).toFixed(1));
        json.data[SOUNDLEVELMETER_MAX] = parseFloat(((json.data[SOUNDLEVELMETER_MAX] / 10)).toFixed(1));

        // Case Temperature
        json.data[CASETEMPERATURE] = parseFloat(((json.data[CASETEMPERATURE] / 771) - 18).toFixed(1));
        
        //Payload Size
        json.data[PAYSIZE] = input.bytes.length;
        

    } catch (e) {
        json = {
            data: {
                bytes: input.bytes
            },
            warnings: [],
            errors: [e]
        };
    }
    return json;
}