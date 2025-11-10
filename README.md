# WaveshareESP32S3_EPD1_54_BattPwrTst_NoFreeRTOS
Converted Waveshare's Example for the power button from LVGL to GxEPD2 - FreeRTOS has been removed for simplicity

Volos Projects YouTube video made me aware of this affordable, full feature ESP32S3 EPD product with many sensors and switches - https://www.waveshare.com/esp32-s3-epaper-1.54.htm?&aff_id=VolosProjects

Waveshare provide a number of examples:  https://www.waveshare.com/wiki/ESP32-S3-ePaper-1.54

Here is Waveshare's github repository for their examples:  https://github.com/waveshareteam/ESP32-S3-ePaper-1.54

One of them shows how to use the power button to turn the product on and off.  When turned on, "ON" is written to the EPD and the onboard LED flashes.  When turne off, "OFF" is written to the EPD and the onboard LED is turned off.

All this code does is remove the LVGL EPD code and replace it with GXEPD2 then further simplifies by removing FreeRTOS tasks.  The bulk of the modifications were done by ChatGPT.
