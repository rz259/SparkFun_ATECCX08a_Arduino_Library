SparkFun ATECCX08A Arduino Library
===========================================================

![SparkFun Cryptographic Breakout](https://cdn.sparkfun.com/assets/parts/1/4/1/6/9/15573-SparkFun_Cryptographic_Co-Processor_Breakout_-_ATECC508A__Qwiic_-01.jpg)

The library in this fork has been modified in several aspects

* the API has been tidied up a little bit, especially regarding the debug flag which was not consistently used throughout the library
* the method "writeConfigSparkFun" has been removed, since it should not really be in the library. The configuration should better be done outside the library
* the API for sha256 has been broken up in three methods
  - beginSHA256
  - updateSHA256
  - endSHA256
  sha256() now calls these 3 methods to improve readability
* a new method "signWithSHA256" has been introduced which first calculates the sha256-value of the data and then signs the hash-value  
* a new method "readSlot" for reading a slot has been added
* a new method "writeSlot" for writing a slot has been added
* a new method "encryptDecryptBlock" for encrypting and decrypting a block of 16 bytes has been added

Due to these changes the examples provided don't work any longer since there are breaking changes in the API.

A new class ATECCAES (ATECCAES.cpp and ATECAES.h) have been created to implement AES encryption and decryption. This class prvides the following features:

* encrypting and decryption message of arbitrary size with no padding or PKCS7Padding
* encryption mode ECB (not recommended)
* encryption mode CBC

I decided to implement these features in a new class to separate the additional functionality from the SparkFun basis. I also wanted to avoid that the base 
library gets bigger and bigger.


And now the original readme....


The SparkFun Cryptographic Co-processor Breakout ATECC508A (Qwiic) takes all the great features of the Microchip ATECC508A cryptographic authentication device and adds two Qwiic ports for plug and play functionality. The ATECC508A chip is capable of many cryptographic processes, including, but not limited to:

* An asymmetric key/signature solution based upon Elliptic Curve Cryptography.
* A standard hash-based challenge-response protocol using a SHA-256 algorithm.
* A FIPS random number generator. 

Embedded in the chip is a 10 Kb EEPROM array that can be used for storing keys, certificates, data, consumption logging, and security configurations. Access to the sections of memory can then be restricted and the configuration locked to prevent changes.

Each ATECC508A ships with a guaranteed unique 72-bit serial number and includes several security features to prevent physical attacks on the device itself, or logical attacks on the data transmitted between the device.

**&#x1F512; Note:** Please follow through the hookup guide in its entirety before using this board. The chip can be only configured before it is **PERMANENTLY** locked. It is advisable that novice users purchase multiple boards in order to explore the multiple functions of the ATECC508A.

Repository Contents
-------------------

* **/examples** - Example sketches for the library (.ino). Run these from the Arduino IDE.
* **/reference** - Includes configuration readings from a fresh IC.
* **/src** - Source files for the library (.cpp, .h).
* **keywords.txt** - Keywords from this library that will be highlighted in the Arduino IDE. 
* **library.properties** - General library properties for the Arduino package manager. 

Documentation
--------------

* **[SparkFun Cryptographic Breakout Hookup Guide](https://learn.sparkfun.com/tutorials/cryptographic-co-processor-atecc508a-qwiic-hookup-guide)** - Basic hookup guide for the SparkFun Cryptographic Co-Processor Breakout.
* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.
* **[SparkFun Cryptographic Co-Processor Breakout - ATECC508A Product Repository](https://github.com/sparkfun/SparkFun_Cryptographic_Co-Processor_Breakout_ATECC508A_Qwiic)** - Main repository (including hardware files)

License Information
-------------------

This product is _**open source**_! 

Various bits of the code have different licenses applied. Anything SparkFun wrote is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
