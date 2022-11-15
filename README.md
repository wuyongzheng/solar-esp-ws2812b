# Solar powered RGB LED decorative night lights

## What is this?

This is a DIY project using solar pannels to power decorative night lights.

![Solar Pannels Daytime](/solar-day.jpg)
At daytime, the solar pannels charges two 18650 batteries.

![Night Light](/night-light.jpg)
In the evening, the batteries power up an ESP8266 IC which controls two WS2812B LED strips.

## Circuit

![Night Light](/schematic.png)

The system consists of 4 parts: solar pannels, battery/BMS, ESP8266 IC, and WS2812B LED strips.

## Solar Pannels

I connected six 68x37mm solar pannels in parallel. Each solar pannel is rated 5.5V 150mA.
Under direct sunlight, my measurement says it can give 5.67V in open circuit, and 4.2V with 210mA.
![Night Light](/solar-back.jpg)

## Battery and My ESP8266 Perfboard

![Night Light](/circuit-back.jpg)
![Night Light](/circuit-front.jpg)
![Night Light](/esp-schematic.png)

## BOM

TODO

## Power Consumption
