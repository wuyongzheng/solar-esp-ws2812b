# Solar powered RGB LED decorative night lights

## What is this?

This is a DIY project using solar pannels to power decorative night lights.

At daytime, the solar pannels charges two 18650 batteries. | In the evening, the batteries power up an ESP8266 IC which controls two WS2812B LED strips.
:-------------------------:|:-------------------------:
![Solar Pannels Daytime](/solar-day.jpg)  |  ![Night Light](/night-light.jpg)

## Circuit

The system consists of 4 parts: solar pannels, battery/BMS, ESP8266 IC, and
WS2812B LED strips.

The LED strips are completely cut off from power using a PMOS controlled by ESP.
However, there is a caveat.
If the power pin (GPIO4) is low (meaning turn off LEDs),
the data pin (GPIO5) cannot be high, otherwise may break the LEDs.
In the code, before setting the power pin low, I have to deconstruct the
`Adafruit_NeoPixel` object and set data pin to float.

![Night Light](/schematic.png)

## My ESP8266 and BMS Perfboards

Front | Back
:-------------------------:|:-------------------------:
![Night Light](/circuit-front.jpg) | ![Night Light](/circuit-back.jpg)

The ESP32 perfboard designed using DIY Layout Creator.
![Night Light](/esp-schematic.png)

## Solar Pannels

I connected six 140x45mm solar pannels in parallel. Each solar pannel is rated
5.5V 150mA by the seller.
Under direct sunlight, my measurement says it gives 5.67V in open circuit,
and 4.2V with 210mA.
![Night Light](/solar-back.jpg)

## BOM

| Item | Price (USD) | Details |
| --- | --- | --- |
| RGB LED Strip | 2.65 x 6 | ![RGB LED Strip](/bom-led.jpeg) WS2812B RGBIC Christmas Lights LED String 5 m 50 leds WS2812 Birthday Party Room Decoration Light Addressable Individually DC5V |
| TP4056 Li-ion charger | < 1.00 | ![RGB LED Strip](/bom-bms.jpeg) Type-c/Micro/Mini USB 5V 1A 18650 TP4056 Lithium Battery Charger Module Charging Board With Protection Dual Functions 1A Li-ion |
| ESP-12F | 1.67 | ![RGB LED Strip](/bom-esp.jpeg) |
| 140x45mm solar pannel | 2.65 x 6 | ![RGB LED Strip](/bom-solar.jpeg) |

# Power Consumption

At 7pm, the ESP turns on the LED strips if the battery voltage is above 3.7V.
The light show continues until the battery voltage is too low or midnight,
whichever is earlier.
Every 30 minutes, the battery voltage is recorded for analysis purpose.
From the recorded data, it seems that the solar power is more than the power
that the LEDs can consume.
![Night Light](/power-plot.png)
