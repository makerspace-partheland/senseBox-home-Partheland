// ONLY USE THIS IF YOU HAVE ALL THE SENSORS (TEMP, HUMI, PRESSURE, LUX, UV, PM, AKKU) CONNECTED TO YOU SENSEBOX!


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
        AKKU_SOC = 'Akku',
        AKKU_Voltage = 'Akku-Voltage',
        AKKU_Temp = 'Akku-Temp',
        TX = 'TxInterval';
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
    
      // Based on https://stackoverflow.com/a/37471538 by Ilya Bursov
    var rawfloat = function (bytes) {
      if (bytes.length !== rawfloat.BYTES) {
        throw new Error('Humidity must have exactly 2 bytes');
      }
      // JavaScript bitwise operators yield a 32 bits integer, not a float.
      // Assume LSB (least significant byte first).
      var bits = bytes[3]<<24 | bytes[2]<<16 | bytes[1]<<8 | bytes[0];
      var sign = (bits>>>31 === 0) ? 1.0 : -1.0;
      var e = bits>>>23 & 0xff;
      var m = (e === 0) ? (bits & 0x7fffff)<<1 : (bits & 0x7fffff) | 0x800000;
      var f = sign * m * Math.pow(2, e - 150);
      return f.toFixed(3);
    };
    rawfloat.BYTES = 4;
    
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
          uint16, //TEMPERATURE
          humidity, //HUMIDITY
          uint16, //PRESSURE
          uint8, //LUX
          uint16, //UV
          uint8, //PM10
          uint16, //PM25
          uint16, //SOUNDLEVELMETER
          uint16, //CASETEMPERATURE
          humidity, //AKKU_SOC
          uint16, //AKKU_Voltage    
          uint16, //AKKU_Temp
          uint16, //TX
          uint16, //PM10_MAX
          uint16, //PM25_MAX
          uint16 //SOUNDLEVELMETER_MAX
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
          AKKU_SOC,
          AKKU_Voltage,
          AKKU_Temp,
          TX,
          PM10_MAX,
          PM25_MAX,
          SOUNDLEVELMETER_MAX
        ]);
  
      // Temperatur
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
      
      
       json.data[AKKU_SOC] = parseFloat(json.data[AKKU_SOC]);
       json.data[AKKU_Voltage] = parseFloat(((json.data[AKKU_Voltage] / 771) - 18).toFixed(2));   
        json.data[AKKU_Temp] = parseFloat(((json.data[AKKU_Temp] / 771) - 18).toFixed(1));
   
        // Case Temperature
        json.data[CASETEMPERATURE] = parseFloat(((json.data[CASETEMPERATURE] / 771) - 18).toFixed(1));
        
        //Transmission Intervall
        json.data[TX] = parseFloat(json.data[TX]);
        
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