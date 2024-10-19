// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Konstanten für maximale und minimale Sendeintervalle
const unsigned long MAX_INTERVAL_S = 60; // 1 Minute ist höchstes Sendeintervall
const unsigned long MEDIUM_INTERVAL_S = 900; // 15 Minuten in Sekunden (5 Messungen)
const unsigned long MIN_INTERVAL_S = 1800; // 30 Minuten in Sekunden (5 Messungen)
const unsigned long VERY_LOW_INTERVAL_S = 3600; // 60 Minuten in Sekunden (3 Messungen)


// Konstanten für Spannungsschwellen und zugehörige Sendeintervalle
//1S LiPo Akku
const float VOLTAGE_HIGH = 4.2;         //Sensoren dauerhaft an; Senden jedes MAX_INTERVAL_S
const float VOLTAGE_MEDIUM = 4.05;      //>= Sensoren dauerhaft an; Senden jedes MAX_INTERVAL_S
const float VOLTAGE_MEDIUM_LOW = 3.91;  //>= Sensoren aus, zum Messen im MEDIUM_INTERVAL_S anschalten
const float VOLTAGE_LOW = 3.84;         //>= Sensoren aus, zum Messen im MIN_INTERVAL_S anschalten
const float VOLTAGE_VERY_LOW = 3.73;    //>= Sensoren aus, zum Messen im VERY_LOW_INTERVAL_S anschalten
                                        //darunter, zum Messen der Akkuspannung nur kurz I2C anschalten

// Sendeintervall-Variable
unsigned long TxInterval = MAX_INTERVAL_S;

// Anzahl der Messungen für bestimmte Spannungsschwellen
const int HIGH_MEASUREMENTS = 5;
const int MEDIUM_MEASUREMENTS = 5;
const int MIN_MEASUREMENTS = 4;
const int VERY_LOW_MEASUREMENTS = 3;
const int CRITICAL_MEASUREMENTS = 1;

// Akku - LC709203F
#define LC709203F_CONNECTED