/**
 * @file R503Lib.h
 * @brief Library for interfacing with the R503 fingerprint sensor module.
 * 
 * This library provides an interface for communicating with the R503 fingerprint sensor module using an ESP32.
 * It includes functions for device-related operations such as reading parameters and device information, as well as fingerprint-related operations such as taking and storing fingerprints.
 * 
 * @author Maxime Pagnoulle (MXPG)
 */

#ifndef R503LIB_H
#define R503LIB_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "R503Packet.h"

// Defaults
#define R503_PASSWORD 0x0
#define R503_RECEIVE_TIMEOUT 3000
#define R503_RESET_TIMEOUT 3000

// Confirmation Codes
#define R503_OK 0x00
#define R503_ERROR_RECEIVING_PACKET 0x01
#define R503_NO_FINGER 0x02
#define R503_ERROR_TAKING_IMAGE 0x03
#define R503_IMAGE_MESSY 0x06
#define R503_FEATURE_FAIL 0x07
#define R503_NO_MATCH 0x08
#define R503_NO_MATCH_IN_LIBRARY 0x09
#define R503_WRONG_PASSWORD 0x13
#define R503_NO_IMAGE 0x15
#define R503_BAD_LOCATION 0x0B
#define R503_ERROR_WRITING_FLASH 0x18
#define R503_SENSOR_ABNORMAL 0x29
#define R503_ERROR_TRANSFER_DATA = 0x0E

// Error Codes
#define R503_ADDRESS_MISMATCH 0xE1
#define R503_NOT_ENOUGH_MEMORY 0xE2
#define R503_CHECKSUM_MISMATCH 0xE3
#define R503_PACKET_MISMATCH 0xE5
#define R503_INVALID_START_CODE 0xE6
#define R503_INVALID_BAUDRATE 0xE8
#define R503_TIMEOUT 0xE9

struct R503Parameters
{
    uint16_t statusRegister;
    uint16_t systemIdentifierCode;
    uint16_t fingerLibrarySize;
    uint16_t securityLevel;
    uint32_t deviceAddress;
    uint16_t dataPackageSize;
    uint32_t baudrate;
};

struct R503DeviceInfo
{
    char moduleType[16];
    char batchNumber[4];
    char serialNumber[8];
    uint8_t hardwareVersion[2];
    char sensorType[8];
    uint16_t sensorWidth;
    uint16_t sensorHeight;
    uint16_t templateSize;
    uint16_t databaseSize;
};

typedef enum
{
    aLEDBreathing = 1, // Breathing
    aLEDFlash,         // Quick blink
    aLEDON,            // On
    aLEDOFF,           // Off
    aLEDFadeIn,        // Fade in
    aLEDFadeOut,       // Fade out
} aLEDMode_t;

typedef enum
{
    aLEDRed = 1,
    aLEDBlue,
    aLEDPurple,
    aLEDGreen,
    aLEDYellow,
    aLEDCyan,
    aLEDWhite
} aLEDColor_t;

class R503Lib
{
public:
    R503Lib(HardwareSerial *serial, uint8_t rxPin, uint8_t txPin, uint32_t address);
    virtual ~R503Lib();

    uint8_t begin(long baudrate, uint32_t password = R503_PASSWORD);

    // R503 Device Related
    uint8_t readParameters(R503Parameters &params);
    uint8_t readDeviceInfo(R503DeviceInfo &info);
    uint8_t verifyPassword();
    uint8_t setAddress(uint32_t address);
    uint8_t setAuraLED(uint8_t control, uint8_t color, uint8_t speed, uint8_t repeat);
    uint8_t handShake();
    uint8_t checkSensor();
    uint8_t setSecurityLevel(uint8_t level);
    uint8_t setBaudrate(long baudrate);
    uint8_t setPacketSize(uint16_t size);
    uint8_t writeParameter(uint8_t paramNumber, uint8_t value);
    uint8_t getValidTemplateCount(uint16_t &count);
    uint8_t cancelInstruction();
    uint8_t getRandomNumber(uint32_t &number);
    uint8_t softReset();

    // Fingerprint Related
    uint8_t takeImage();
    uint8_t downloadImage(uint8_t *image, uint16_t size);
    uint8_t uploadImage(uint8_t *image, uint16_t &size);
    uint8_t extractFeatures(uint8_t charBuffer);
    uint8_t createTemplate();
    uint8_t storeTemplate(uint8_t charBuffer, uint16_t location);
    uint8_t getTemplate(uint8_t charBuffer, uint16_t location);
    uint8_t deleteTemplate(uint16_t location, uint16_t count = 1);
    uint8_t downloadTemplate(uint8_t charBuffer, uint8_t *templateData, uint16_t &size);
    uint8_t uploadTemplate(uint8_t charBuffer, uint8_t *templateData, uint16_t size);
    uint8_t getTemplateCount(uint16_t &count);
    uint8_t emptyLibrary();
    uint8_t matchFinger(uint16_t &confidence);
    uint8_t searchFinger(uint8_t charBuffer, uint16_t &location, uint16_t &confidence);
    uint8_t readIndexTable(uint8_t *table, uint8_t page = 0);

    // Debug
    uint8_t printDeviceInfo();
    uint8_t printParameters();

private:
    // Serial communication
    HardwareSerial *fpsSerial;
    uint8_t fpsRxPin, fpsTxPin;
    long fpsBaudrate;

    // R503 parameters
    uint32_t fpsAddress;
    uint32_t fpsPasswd;
    uint16_t fpsLibrarySize;
    uint16_t fpsDataPacketSize;
    uint16_t fpsTemplateSize;

    // Packet handling
    void sendPacket(R503Packet const &packet);
    uint8_t receivePacket(R503Packet &packet);
    uint8_t sendData(const uint8_t *data, uint16_t length);
    uint8_t receiveData(uint8_t *data, uint16_t &length);
    uint8_t receiveAck(uint8_t *data, uint16_t &length);
};

#endif

#if defined ESP32
#define HEAP_AVAILABLE() ESP.getFreeHeap()

#ifdef ESP32
#define R503_LOG_FORMAT(letter, format) "[" #letter "][%s:%u][H:%u] %s(): " format "\r\n", __FILE__, __LINE__, HEAP_AVAILABLE(), __FUNCTION__

#if defined DEBUG_ESP_PORT
#define r503_log_d(format, ...) DEBUG_ESP_PORT.printf(R503_LOG_FORMAT(N, format), ##__VA_ARGS__);
#define r503_log_e(format, ...) DEBUG_ESP_PORT.printf(R503_LOG_FORMAT(E, format), ##__VA_ARGS__);

#else
#define r503_log_d(format, ...) Serial.printf(R503_LOG_FORMAT(N, format), ##__VA_ARGS__);
#define r503_log_e(format, ...) Serial.printf(R503_LOG_FORMAT(E, format), ##__VA_ARGS__);
#endif
#endif
#endif