# R503-Fingerprint-Library
An ESP32 compatible library (Arduino SDK) for the **R503** capacitive fingerprint sensor from Hangzhou Grow Technology.

⚠ <kbd><strong>NOTE:</strong> This is a work in progress, some things may not work as expected.</kbd>

![image](https://github.com/mpagnoulle/R503-Fingerprint-Library/blob/main/docs/R503_sensor.jpg?raw=true)

## Wiring

|Sensor Side|ESP32 Side|
|---|---|
|Red (Power Supply)|3.3V|
|White (Touch Induction PS)|3.3V|
|Black (Ground)|Ground (GND)|
|Maroon or Green or Brown (RXD)|TXD|
|Yellow  (TXD)|RXD|
|Blue (Wake UP)|GPIO pin of your choice|

ℹ *Information:*

<kbd>This library is ESP32's hardware UARTs, you can choose which pins to be assigned to the UART, consider reserved pins while doing so.</kbd>
󠀠󠀠󠀠󠀠󠀠<kbd>If you do not intend to use touch induction to provide a wake-up signal to the ESP32 when the sensor is touched, the white wire can be left unconnected.</kbd>

## Sensor Documentation
* [R503 User Manual v1.4.1](https://github.com/mpagnoulle/R503-Fingerprint-Library/blob/main/docs/R503%20R503-M22%20Fingerprint%20User%20Manual%20V1.4.1.pdf)
* [R503 User Manual v1.2.1](https://github.com/mpagnoulle/R503-Fingerprint-Library/blob/main/docs/R503%20Fingerprint%20Module%20User%20Manual%20V1.2.1.pdf)

## Methods
...
## Examples
...
