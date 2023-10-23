#include "R503Lib.h"

#define GET_PACKET(CMD_SIZE, ...)                                       \
    uint8_t command[] = {__VA_ARGS__};                                  \
    sendPacket(R503Packet(R503_PKT_COMMAND, sizeof(command), command)); \
    uint8_t data[CMD_SIZE];                                             \
    uint16_t dataSize = sizeof(data);                                   \
    int confirmationCode = receiveAck(data, dataSize);

#define SEND_CMD(...)          \
    GET_PACKET(1, __VA_ARGS__) \
    return confirmationCode;

/**
 * @brief Constructor for R503Lib class.
 *
 * @param serial Pointer to HardwareSerial object.
 * @param rxPin RX pin number.
 * @param txPin TX pin number.
 * @param address Device address.
 */
R503Lib::R503Lib(HardwareSerial *serial, uint8_t rxPin, uint8_t txPin, uint32_t address)
{
    fpsSerial = serial;
    fpsRxPin = rxPin;
    fpsTxPin = txPin;
    fpsAddress = address;
}

/**
 * @brief Destructor for the R503Lib class.
 *
 * This function deletes the fpsSerial object created in begin(...)
 */
R503Lib::~R503Lib()
{
    delete fpsSerial;
}

/**
 * @brief Initializes the R503 fingerprint sensor library with the specified baudrate and password.
 *
 * @param baudrate The baudrate to use for serial communication with the sensor.
 * @param passwd The password to use for accessing the sensor.
 *
 * @return uint8_t Returns R503_OK if initialization is successful, otherwise returns an error code.
 */
uint8_t R503Lib::begin(long baudrate, uint32_t passwd)
{
    fpsBaudrate = baudrate;
    fpsPasswd = passwd;

    pinMode(fpsRxPin, INPUT);
    pinMode(fpsTxPin, OUTPUT);

    fpsSerial->begin(fpsBaudrate, SERIAL_8N1, fpsRxPin, fpsTxPin);

    // Check Password
    int retVal = verifyPassword();

    if (retVal != R503_OK)
    {
#if R503_DEBUG
        r503_log_e("error verifying password (code: 0x%02X)\n", retVal);
#endif

        return retVal;
    }

    // Read R503 Parameters
    R503Parameters params;
    retVal = readParameters(params);

    if (retVal != R503_OK)
    {
#if R503_DEBUG
        r503_log_e("error reading parameters from sensor (code: 0x%02X)\n", retVal);
#endif

        return retVal;
    }

    fpsLibrarySize = params.fingerLibrarySize;
    fpsDataPacketSize = params.dataPackageSize;


    R503DeviceInfo info;

    retVal = readDeviceInfo(info);

    if (retVal != R503_OK)
    {
#if R503_DEBUG
        r503_log_e("error reading device info from sensor (code: 0x%02X)\n", retVal);
#endif

        return retVal;
    }

    fpsTemplateSize = info.templateSize;

    return R503_OK;
}

/* --------------------------
    ? R503 Device Related
----------------------------*/

/**
 * @brief Reads the parameters from the device and stores them in the provided R503Parameters object.
 *
 * @param params The R503Parameters object to store the read parameters.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::readParameters(R503Parameters &params)
{
    GET_PACKET(17, 0x0F);

    params.statusRegister = data[1] << 8 | data[2];
    params.systemIdentifierCode = data[3] << 8 | data[4];
    params.fingerLibrarySize = data[5] << 8 | data[6];
    params.securityLevel = data[7] << 8 | data[8];
    params.deviceAddress = data[9] << 24 | data[10] << 16 | data[11] << 8 | data[12];
    params.dataPackageSize = 32 << (data[13] << 8 | data[14]);
    params.baudrate = 9600 * (data[15] << 8 | data[16]);

    return confirmationCode;
}

/**
 * @brief Reads device information from the R503 fingerprint module and stores it in the provided R503DeviceInfo struct.
 * 
 * @param info The R503DeviceInfo struct to store the device information in.
 * 
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::readDeviceInfo(R503DeviceInfo &info) {
    GET_PACKET(47, 0x3C);
    
    memcpy(info.moduleType, &data[1], 16);
    memcpy(info.batchNumber, &data[17], 4);
    memcpy(info.serialNumber, &data[21], 8);
    info.hardwareVersion[0] = data[29];
    info.hardwareVersion[1] = data[30];
    memcpy(info.sensorType, &data[31], 8);
    info.sensorWidth = data[39] << 8 | data[40];
    info.sensorHeight = data[41] << 8 | data[42];
    info.templateSize = data[43] << 8 | data[44];
    info.databaseSize = data[45] << 8 | data[46];
    
    return confirmationCode;
}

/**
 * @brief Verifies the password for the fingerprint sensor.
 *
 * @return uint8_t Returns R503_OK if the password is verified successfully, otherwise returns an error code.
 */
uint8_t R503Lib::verifyPassword()
{
    SEND_CMD(0x13, (uint8_t)(fpsPasswd >> 24), (uint8_t)(fpsPasswd >> 16), (uint8_t)(fpsPasswd >> 8), (uint8_t)(fpsPasswd & 0xFF));
}

/**
 * @brief Sets the address of the fingerprint sensor.
 *
 * @param address The new address to set.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::setAddress(uint32_t address)
{
    SEND_CMD(0x15, (uint8_t)(address >> 24), (uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)(address & 0xFF));
}

/**
 * Sets the aura LED of the R503 fingerprint sensor.
 *
 * @param control The control mode of the LED (R503_LED_BREATHING, R503_LED_FLASHING, R503_LED_ON, R503_LED_OFF, )
 * @param color The color of the LED.
 * @param speed The speed of the LED.
 * @param repeat The repeat times of the LED.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::setAuraLED(uint8_t control, uint8_t color, uint8_t speed, uint8_t repeat)
{
    SEND_CMD(0x35, control, speed, color, repeat);
}

/**
 * @brief Sends a handshake command to the R503 fingerprint sensor.
 *
 * @return uint8_t Returns R503_OK if ready to receive commands, otherwise returns an error code.
 */
uint8_t R503Lib::handShake()
{
    SEND_CMD(0x40);
}

/**
 * Sends a command to check the status of the fingerprint sensor.
 *
 * @return uint8_t Returns R503_OK if the sensor "normal" or R503_SENSOR_ABNORMAL if the sensor is abnormal.
 */
uint8_t R503Lib::checkSensor()
{
    SEND_CMD(0x36);
}

/**
 * @brief Sets the security level for the fingerprint sensor.
 *
 * @param level The security level to set. Valid values are between 1 and 5.
 *              1: False Acceptance Rate is lowest, False Rejection Rate is highest.
 *              5: False Acceptance Rate is highest, False Rejection Rate is lowest.
 *
 * @return uint8_t Returns R503_OK if the reset was successful, or an error code otherwise.
 */
uint8_t R503Lib::setSecurityLevel(uint8_t level)
{
    //SEND_CMD(0x0E, 5, level);
    return writeParameter(5, level);
}

/**
 * @brief Sets the baudrate of the fingerprint sensor module.
 * 
 * @param baudrate The desired baudrate. Must be one of 9600, 19200, 38400, 57600, or 115200.
 * @return uint8_t Returns R503_OK if the baudrate was set successfully, or R503_INVALID_BAUDRATE if the provided baudrate is not valid.
 */
uint8_t R503Lib::setBaudrate(long baudrate)
{
    switch (baudrate)
    {
    case 9600:
    case 19200:
    case 38400:
    case 57600:
    case 115200:
    {
        uint8_t confirmationCode = writeParameter(4, static_cast<uint8_t>(baudrate / 9600));

        if (confirmationCode == R503_OK)
        {
            fpsSerial->end();
            fpsSerial->begin(baudrate);
            fpsBaudrate = baudrate;
        }

        return confirmationCode;
    }
    default:
        return R503_INVALID_BAUDRATE;
    }
}

/**
 * @brief Writes a parameter to the R503 module.
 * 
 * @param paramNumber The parameter number to write.
 * @param value The value to write for the parameter.
 * 
 * @return uint8_t Returns R503_OK if the reset was successful, or an error code otherwise.
 */
uint8_t R503Lib::writeParameter(uint8_t paramNumber, uint8_t value) {
    SEND_CMD(0x0E, paramNumber, value);
}

/**
 * @brief Sets the packet size for the fingerprint sensor.
 * 
 * @param size The desired packet size (32, 64, 128, or 256 bytes).
 * 
 * @return uint8_t Returns R503_OK if the reset was successful, or an error code otherwise.
 */
uint8_t R503Lib::setPacketSize(uint16_t size) {
    uint8_t value = 0;

    if (size == 32 || size == 64 || size == 128 || size == 256) {
        value =  log2(size) - 5;
    } 
    else {
        value = 2;
        #if R503_DEBUG
        r503_log_e("invalid packet size: %d\n", size);
        #endif
    }

    return writeParameter(6, value);
}

/**
 * @brief Sends a reset command to the R503 fingerprint sensor module.
 *
 * @return uint8_t Returns R503_OK if the reset was successful, or an error code otherwise.
 */
uint8_t R503Lib::softReset()
{
    GET_PACKET(1, 0x3D);
    if (confirmationCode != R503_OK)
        return confirmationCode;

    unsigned long start = millis();

    while (millis() - start < R503_RESET_TIMEOUT)
    {
        int byte = fpsSerial->read();

        if (byte == -1)
        {
            delay(1);
        }
        else if (byte == 0x55)
        {
            return R503_OK;
        }
    }

    return R503_TIMEOUT;
}

/* --------------------------
    ? Fingerprint Related
----------------------------*/

/**
 * @brief Takes an image and stores it in the buffer (R503)
 *
 * @return R503_OK on success
 *         R503_NO_FINGER if no finger is detected
 *         R503_ERROR_TAKING_IMAGE if unable to take image
 *         R503_MESSY_IMAGE if the image is too messy
 */
uint8_t R503Lib::takeImage()
{
    SEND_CMD(0x01);
}

/**
 * @brief Downloads an image from the R503 fingerprint sensor to the MCU.
 *
 * @param image Pointer to the image data to be uploaded.
 * @param size The size of the image data.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::downloadImage(uint8_t *image, uint16_t size)
{
    GET_PACKET(1, 0x0A);
    if (confirmationCode != R503_OK)
        return confirmationCode;

    return receiveData(image, size);
}

/**
 * @brief Uploads an image to the fingerprint sensor.
 *
 * @param image Pointer to the image data to be downloaded.
 * @param size Size of the image data in bytes.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::uploadImage(uint8_t *image, uint16_t &size)
{
    GET_PACKET(1, 0x0B);
    if (confirmationCode != R503_OK)
        return confirmationCode;

    return sendData(image, size);
}

/**
 * @brief Extracts features from the given character buffer.
 *
 * @param charBuffer The character buffer to extract features from (range from 1 to 6)
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::extractFeatures(uint8_t charBuffer)
{
    SEND_CMD(0x02, charBuffer);
}

/**
 * @brief Create a fingerprint template.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::createTemplate()
{
    SEND_CMD(0x05);
}

/**
 * @brief Stores a template in the specified location.
 *
 * @param charBuffer The character buffer to store the template in.
 * @param location The location to store the template in.
 *
 * @return uint8_t Returns 0 on success, or an error code on failure.
 */
uint8_t R503Lib::storeTemplate(uint8_t charBuffer, uint16_t location)
{
    SEND_CMD(0x06, charBuffer, static_cast<uint8_t>(location >> 8), static_cast<uint8_t>(location));
}

/**
 * @brief Gets a template from the specified location and stores it in the specified character buffer.
 *
 * @param charBuffer The character buffer to store the template in.
 * @param location The location of the template to retrieve.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::getTemplate(uint8_t charBuffer, uint16_t location)
{
    SEND_CMD(0x07, charBuffer, static_cast<uint8_t>(location >> 8), static_cast<uint8_t>(location));
}

/**
 * @brief Downloads the template data from the specified character buffer from R503
 *
 * @param charBuffer The character buffer to download the template from.
 * @param templateData Pointer to the buffer where the template data will be stored.
 * @param size Reference to the variable where the size of the downloaded template data will be stored.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::downloadTemplate(uint8_t charBuffer, uint8_t *templateData, uint16_t &size)
{
    GET_PACKET(1, 0x08, charBuffer);
    if (confirmationCode != R503_OK)
        return confirmationCode;

    return receiveData(templateData, size);
}

/**
 * @brief Uploads a template to the specified character buffer on R503
 *
 * @param charBuffer The character buffer to upload the template to.
 * @param templateData The template data to upload.
 * @param size The size of the template data.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::uploadTemplate(uint8_t charBuffer, uint8_t *templateData, uint16_t size)
{
    uint16_t tempBufferSize = fpsTemplateSize + 256;
    uint8_t tempBuffer[tempBufferSize];
    memset(tempBuffer, 0xFF, tempBufferSize); // Fill the buffer with 0xFF
    if(size <= tempBufferSize) {
        memcpy(tempBuffer, templateData, size); // Copy the template data to the buffer, leaving the rest as 0xFF if the template data is smaller than the buffer size
    }
    else {
        memcpy(tempBuffer, templateData, tempBufferSize); 
    }

    GET_PACKET(1, 0x09, charBuffer);
    if (confirmationCode != R503_OK)
        return confirmationCode;

    return sendData(tempBuffer, tempBufferSize); // Send the buffer to the sensor
}

/**
 * @brief Deletes a template from the specified location.
 *
 * @param location The location of the template to be deleted.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::deleteTemplate(uint16_t location, uint16_t count)
{
    SEND_CMD(0x0C, static_cast<uint8_t>(location >> 8), static_cast<uint8_t>(location), static_cast<uint8_t>(count >> 8), static_cast<uint8_t>(count));
}

/**
 * @brief Gets the number of templates stored in the device.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::getTemplateCount(uint16_t &count)
{
    GET_PACKET(3, 0x1D);
    count = data[1] << 8 | data[2];

    return confirmationCode;
}

/**
 * @brief Sends a command to empty the library of fingerprints.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::emptyLibrary()
{
    SEND_CMD(0x0D);
}

/**
 * @brief Matches the fingerprint and returns the confidence level.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::matchFinger(uint16_t &confidence)
{
    GET_PACKET(3, 0x03);
    confidence = data[1] << 8 | data[2];

    return confirmationCode;
}

/**
 * @brief Searches for a finger in the fingerprint library.
 *
 * @param charBuffer The character buffer to search for the finger.
 *
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::searchFinger(uint8_t charBuffer, uint16_t &location, uint16_t &confidence)
{
    uint16_t startPage = 0;
    uint16_t pageCount = fpsLibrarySize;
    GET_PACKET(5, 0x04, charBuffer, static_cast<uint8_t>(startPage >> 8), static_cast<uint8_t>(startPage), static_cast<uint8_t>(pageCount >> 8), static_cast<uint8_t>(pageCount));
    location = data[1] << 8 | data[2];
    confidence = data[3] << 8 | data[4];

    return confirmationCode;
}

/**
 * @brief Reads the index table of a specified page and stores it in the provided buffer.
 * 
 * @param table Pointer to the buffer where the index table will be stored.
 * @param page The page number of the index table to be read.
 * 
 * @return uint8_t Returns R503_OK if successful, otherwise returns an error code.
 */
uint8_t R503Lib::readIndexTable(uint8_t *table, uint8_t page) {
    GET_PACKET(33, 0x1F, page);
    memcpy(table, &data[1], 32);

    return confirmationCode;
}

/*uint8_t R503Lib::autoEnroll(uint16_t location)
{   
    uint8_t config[4] = {0x01, 0x01, 0x00, 0x01};
    GET_PACKET(5, 0x31, location, config[0], config[1], config[2], config[3]);

    return confirmationCode;
}*/

/* --------------------------
    ? Communication Related
----------------------------*/

void R503Lib::sendPacket(const R503Packet &packet)
{
    uint16_t length = packet.length + 2;
    uint8_t header[] = {
        highByte(R503_PKT_START_CODE), lowByte(R503_PKT_START_CODE),
        static_cast<uint8_t>(fpsAddress >> 24), static_cast<uint8_t>(fpsAddress >> 16), static_cast<uint8_t>(fpsAddress >> 8), static_cast<uint8_t>(fpsAddress),
        packet.type,
        highByte(length), lowByte(length)};

    fpsSerial->write(header, sizeof(header));
    fpsSerial->write(packet.payload, packet.length);;
    fpsSerial->write(highByte(packet.checksum));
    fpsSerial->write(lowByte(packet.checksum));

#if R503_DEBUG
    Serial.println("\n>> Sent packet: ");
    Serial.printf("- startCode: %02X %02X\n", highByte(R503_PKT_START_CODE), lowByte(R503_PKT_START_CODE));
    Serial.printf("- address: %02X %02X %02X %02X\n", static_cast<uint8_t>(fpsAddress >> 24), static_cast<uint8_t>(fpsAddress >> 16), static_cast<uint8_t>(fpsAddress >> 8), static_cast<uint8_t>(fpsAddress));
    Serial.printf("- type: %02X\n", packet.type);
    Serial.printf("- length: %02X %02X (%d bytes inc. checksum)\n", highByte(length), lowByte(length), length);
    Serial.println("- payload: ");
    for (int i = 0; i < packet.length; i++)
    {
        Serial.printf("%02X ", packet.payload[i]);
    }

    Serial.printf("\n- checksum: %02X %02X\n", highByte(packet.checksum), lowByte(packet.checksum));
    Serial.println("-------------------------");
#endif
}

/**
 * @brief Receives a packet from the R503 fingerprint sensor module.
 * 
 * @param packet The packet to be populated with the received data.
 * @return uint8_t Returns R503_OK if the packet is received successfully, otherwise returns an error code.
 *         Possible error codes are R503_TIMEOUT and R503_CHECKSUM_MISMATCH.
 */
uint8_t R503Lib::receivePacket(R503Packet &packet)
{
    unsigned long startTime = millis();
    
    uint8_t buffer[9];  // startCode(2) + address(4) + type(1) + length(2)

    // Wait for the start code with timeout
    while (millis() - startTime < R503_RECEIVE_TIMEOUT) {
        uint8_t byte = fpsSerial->read();

        if (byte == 0xEF) {
            buffer[0] = byte;
            break;
        }
    }

    // Check if timeout occurred
    if (millis() - startTime >= R503_RECEIVE_TIMEOUT) {
        Serial.println("[X] Timeout waiting for start code");
        return R503_TIMEOUT;
    }

    // Read the rest of the header
    fpsSerial->readBytes(buffer + 1, 8);

#if R503_DEBUG
    // Print packet details
    Serial.println("\n>> Received packet: ");
    Serial.printf("- startCode: %02X %02X\n", buffer[0], buffer[1]);
    Serial.printf("- address: %02X %02X %02X %02X\n", buffer[2], buffer[3], buffer[4], buffer[5]);
    Serial.printf("- type: %02X\n", buffer[6]);
    Serial.printf("- length: %02X %02X (%d bytes inc. checksum)\n", buffer[7], buffer[8], (buffer[7] << 8) | buffer[8]);
#endif
    
    // Verify start code
    uint16_t startCode = (buffer[0] << 8) | buffer[1];
    if (startCode != R503_PKT_START_CODE) {
        #if R503_DEBUG
        r503_log_e("invalid start code: %02X %02X\n", buffer[0], buffer[1]);
        #endif
        return 1;  // Error code: invalid start code
    }
    
    // Populate packet fields
    packet.address = (buffer[2] << 24) | (buffer[3] << 16) | (buffer[4] << 8) | buffer[5];
    packet.type = buffer[6];
    packet.length = ((buffer[7] << 8) | buffer[8]) - 2;
    fpsSerial->readBytes(packet.payload, packet.length);

    #if R503_DEBUG
    // Print payload
    Serial.println("- payload: ");
    for (int i = 0; i < packet.length; i++) {
        Serial.printf("%02X ", packet.payload[i]);
    }

    #endif
    
    // Read checksum
    uint8_t checksumBuffer[2];
    fpsSerial->readBytes(checksumBuffer, 2);
    packet.checksum = (checksumBuffer[0] << 8) | checksumBuffer[1];

    #if R503_DEBUG
    Serial.println();
    Serial.printf("- checksum: %02X %02X\n", checksumBuffer[0], checksumBuffer[1]);
    #endif
    
    // Verify checksum
    if (!packet.isChecksumValid()) {
        #if R503_DEBUG
        r503_log_e("checksum mismatch: %02X %02X\n", checksumBuffer[0], checksumBuffer[1]);
        #endif
        return R503_CHECKSUM_MISMATCH;  // Error code: checksum mismatch
    }

    return R503_OK;  // Successful reception
}

/**
 * @brief Sends data to the fingerprint sensor in packets.
 * 
 * @param data Pointer to the data to be sent.
 * @param length Length of the data to be sent.
 * 
 * @return Returns R503_OK if the data was sent successfully.
 */
uint8_t R503Lib::sendData(const uint8_t *data, uint16_t length)
{
    uint16_t offset = 0;
    uint8_t buffer[fpsDataPacketSize];
    R503Packet packet(sizeof(buffer), buffer);

    do
    {
        // Check if it's the last packet
        if (length - offset <= packet.length)
        {
            packet.type = R503_PKT_DATA_END;
            packet.length = length - offset;
        }
        else
        {
            packet.type = R503_PKT_DATA_START;
        }

        memcpy(packet.payload, data + offset, packet.length);
        packet.calculateChecksum();
        sendPacket(packet);
        offset += packet.length;
    } 
    while (offset < length);

    return R503_OK;
}

/**
 * @brief Receives data from the fingerprint sensor module.
 * 
 * @param data Pointer to the buffer where the received data will be stored.
 * @param length Reference to a variable that will store the length of the received data.
 * 
 * @return uint8_t Returns R503_OK if the data is received successfully.
 */
uint8_t R503Lib::receiveData(uint8_t *data, uint16_t &length)
{
    unsigned long startTime = millis();
    uint8_t tempBuffer[1792];
    memset(tempBuffer, 0xFF, sizeof(tempBuffer));

    uint32_t tempBufferIndex = 0;

    uint32_t totalBytesReceived = 0;

    bool isEndPacketDetected = false;
    uint32_t expectedTotalBytes = 0;

    #if R503_DEBUG
    r503_log_d("receiving data...\n");
    #endif

    while(millis() - startTime < 4000) {
        if(fpsSerial->available() > 0) {
            tempBuffer[tempBufferIndex] = fpsSerial->read();

            tempBufferIndex++;
            totalBytesReceived++;

            if(tempBufferIndex >= 8) {
                if(tempBuffer[tempBufferIndex - 8] == 0xEF && tempBuffer[tempBufferIndex - 7] == 0x01) {
                    if(tempBuffer[tempBufferIndex - 2] == 0x08) {
                        break;
                    }
                }
            }
        }
    }

    #if R503_DEBUG
    Serial.println("\n- Received data: ");
    for(uint32_t i = 0; i < tempBufferIndex; i++) {
        Serial.printf("%02X ", tempBuffer[i]);
    }

    Serial.println();
    Serial.printf("- Total Bytes Received: %d\n", totalBytesReceived);
    #endif

    uint32_t startCopyPosition = UINT32_MAX;
    uint32_t endCopyPosition = UINT32_MAX;
    uint16_t outputIndex = 0; // Index to append data to the output data buffer

    for(uint32_t i = 0; i < tempBufferIndex - 1; i++) {
        if(tempBuffer[i] == 0xEF && tempBuffer[i + 1] == 0x01) {
            if(startCopyPosition == UINT32_MAX) {
                startCopyPosition = i + 9; // +9 to exclude the header
            } 
            else {
                endCopyPosition = i;

                // Calculate the size of the data to copy
                uint32_t dataSize = endCopyPosition - startCopyPosition - 2; // -2 to exclude the checksum bytes

                memcpy(&data[outputIndex], &tempBuffer[startCopyPosition], dataSize);
                outputIndex += dataSize;

                // Reset positions for the next block
                startCopyPosition = endCopyPosition + 9;
                endCopyPosition = UINT32_MAX;
            }
        }
    }

    length = outputIndex; // Set the length to the number of bytes added to the output data buffer

    return R503_OK;
}

/**
 * @brief Receives an acknowledgement packet from the R503 fingerprint sensor module.
 * 
 * @param data Pointer to the data buffer to store the received packet.
 * @param length Reference to the length of the received packet.
 * 
 * @return uint8_t Returns R503_OK if the packet is received successfully, R503_PACKET_MISMATCH if the received packet type is not an acknowledgement, and the first byte of the received packet if the packet is an acknowledgement.
 */
uint8_t R503Lib::receiveAck(uint8_t *data, uint16_t &length)
{
    R503Packet ack(length, data);
    uint8_t ret = receivePacket(ack);
    length = ack.length;

    if (ret != R503_OK)
        return ret;
    if (ack.type != R503_PKT_ACK)
        return R503_PACKET_MISMATCH;

    return data[0];
}

uint8_t R503Lib::printDeviceInfo() {
    R503DeviceInfo info;
    int retVal = readDeviceInfo(info);
    if(retVal == R503_OK) {
        Serial.printf("Module Type: %s\n"
        "Module Batch Number: %s\n"
        "Module Serial Number: %s\n"
        "Hardware Version: %d.%d\n"
        "Sensor Type: %s\n"
        "Sensor Dimension: %dx%d\n"
        "Sensor Template Size: %d\n"
        "Sensor Database Size: %d\n",
            info.moduleType, 
            info.batchNumber, 
            info.serialNumber,
            info.hardwareVersion[0], info.hardwareVersion[1], 
            info.sensorType,
            info.sensorWidth, 
            info.sensorHeight, 
            info.templateSize, 
            info.databaseSize);
    } else {
        r503_log_e("error retreiving device info (code: 0x%02X)\n", retVal);
    }
    return retVal;
}

uint8_t R503Lib::printParameters() {
    R503Parameters params;
    int retVal = readParameters(params);
    if(retVal == R503_OK) {
        Serial.printf("Status Register: 0x%02X\n"
        "System Identifier Code: 0x%04X\n"
        "Finger Library Capacity: %d\n"
        "Security Level: %d\n"
        "Device Address: 0x%08X\n"
        "Data Package Size: %d bytes\n"
        "Baudrate: %d\n",
            params.statusRegister, 
            params.systemIdentifierCode, 
            params.fingerLibrarySize,
            params.securityLevel, 
            params.deviceAddress, 
            params.dataPackageSize, 
            params.baudrate);
    } else {
        r503_log_e("error retreiving parameters (code: 0x%02X)\n", retVal);
    }
    return retVal;
}