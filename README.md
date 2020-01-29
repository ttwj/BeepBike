BeepBike
========

Test implementation of CEPAS deduction with ESP8266 board and PN532 NFC chip using EZ-Link Online SAM API

Was originally planned to be used on bike-sharing systems which are unable to accomodate traditional SAM-based terminals

You are recommended to use our modified Adafruit_PN532 library to support IS01443B (required by CEPAS), https://github.com/zhongfu/Adafruit-PN532


How do I use it for my own projects?
========

For this implementation, the CEPAS debit cryptogram is generated on our server which then relays the message to EZ-Link's online SAM API (which we cannot share, sorry)

0. Purchase the CEPAS specifications from SPRING Singapore, www.singaporestandardseshop.sg, sorry no sharing too :-(
1. Find EZ-Link and request for the online SAM API 
2. Perform certification with EZ-Link (will cost you some money)
3. Profit(??)
