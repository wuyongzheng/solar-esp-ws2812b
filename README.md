# Solar powered RGB LED decorative night lights

## What is this?

This is a DIY project using solar pannels to power decorative night lights.

At daytime, the solar pannels charges two 18650 batteries. | In the evening, the batteries power up an ESP8266 IC which controls two WS2812B LED strips.
:-------------------------:|:-------------------------:
![Solar Pannels Daytime](/solar-day.jpg)  |  ![Night Light](/night-light.jpg)

## Hardware

The system consists of 4 parts: solar pannels, battery/BMS, ESP8266 IC, and
WS2812B LED strips.

The LED strips are completely cut off from power using a PMOS controlled by ESP.
However, there is a caveat.
If the power pin (GPIO4) is low (meaning turn off LEDs),
the data pin (GPIO5) cannot be high, otherwise may break the LEDs.
In the code, before setting the power pin low, I have to deconstruct the
`Adafruit_NeoPixel` object and set data pin to float.

![Night Light](/schematic.png)

### My ESP8266 and BMS Perfboards

Front | Back
:-------------------------:|:-------------------------:
![Night Light](/circuit-front.jpg) | ![Night Light](/circuit-back.jpg)

The ESP32 perfboard designed using DIY Layout Creator.
![Night Light](/esp-schematic.png)

### Solar Pannels

I connected six 140x45mm solar pannels in parallel. Each solar pannel is rated
5.5V 150mA by the seller.
Under direct sunlight, my measurement says it gives 5.67V in open circuit,
and 4.2V with 210mA.
![Night Light](/solar-back.jpg)

### BOM

I don't give Aliexpress or Amazon links because they expire quickly.
The items can be easily searched with the description and pictures.
Resistors, capactors, wires, connectors, perfboards, buttons and switchs are
ignored.

| Item | Price (USD) | Details | Picture |
| --- | --- | --- | --- |
| RGB LED Strip | 5.00 x 2 | WS2812B RGBIC Christmas Lights LED String 5 m 50 leds WS2812 Birthday Party Room Decoration Light Addressable Individually DC5V | ![RGB LED Strip](/bom-led.jpeg) |
| TP4056 Li-ion charger | ~ 0.50 | Type-c/Micro/Mini USB 5V 1A 18650 TP4056 Lithium Battery Charger Module Charging Board With Protection Dual Functions 1A Li-ion | ![RGB LED Strip](/bom-bms.jpeg) |
| 140x45mm solar pannel | 2.5 x 6 | 4V 5.5V 5V 6V 7V 10V 12V Mono/polycrystalline solar panel battery module Epoxy board PET power generation board model | ![RGB LED Strip](/bom-solar.jpeg) |
| ESP-12F | 2.00 | I'm using the bare module instead of development board for compactness | ![RGB LED Strip](/bom-esp.jpeg) |
| 18650 Battery case | ~1.50 | Can hold two batteries. Can be configured both serial and parallel.
| 18650 Battery | 10.00 x 2 |
| HT7333 | < 1.00 | Holtek Low Power Consumption LDO |
| NDP6020P | ~ 1.50 |
| 2N3904 | < 0.10 |
| BAT85 | < 0.10 x 6 | Small Signal Schottky Diode |

## Software

TODO

## Power Consumption

At 7pm, the ESP turns on the LED strips if the battery voltage is above 3.7V.
The light show continues until the battery voltage is too low or midnight,
whichever is earlier.
Every 30 minutes, the battery voltage is recorded for analysis purpose.
From the recorded data, it seems that the solar power is more than the power
that the LEDs can consume.
![Night Light](/power-plot.png)
