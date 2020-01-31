BeepBike
========

Test implementation of CEPAS deduction with ESP8266 board and PN532 NFC chip using EZ-Link Online SAM API

Was originally planned to be used on bike-sharing systems which are unable to accomodate traditional SAM-based terminals

You are recommended to use our modified Adafruit_PN532 library to support ISO1443B (required by CEPAS), https://github.com/zhongfu/Adafruit-PN532

For this implementation, the CEPAS encrypted purse is sent to our middleware which delays to EZ-Link; EZ-Link then replies us with the debit cryptogram which we relay to the card.

After deduction is completed, the card returns a debit receipt cryptogram which should be subsequently relayed back to EZ-Link (Debit receipt cryptogram upload was not implemented in this code due to commercial constraints) 


How do I use it for my own projects?
========

0. Purchase the CEPAS specifications from SPRING Singapore, www.singaporestandardseshop.sg, sorry no sharing too :-(
1. Find EZ-Link’s business/partnerships team and present a business case (number of touchpoints, transaction volume, etc) to request for the online SAM API 
2. Perform certification with EZ-Link (will cost you some money)
3. Profit(??)

P.S: In Singapore, CEPAS cards are also issued by NETS and TransitLink under NETS FlashPay and Concession respectively, not sure if they have a similar API (you can check with them)
