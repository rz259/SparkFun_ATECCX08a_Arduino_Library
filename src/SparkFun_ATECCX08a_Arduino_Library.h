/*
  This is a library written for the ATECCX08A Criptographic Co-Processor (QWIIC).

  Written by Pete Lewis @ SparkFun Electronics, August 5th, 2019

  The IC uses I2C and 1-wire to communicat. This library only supports I2C.

  https://github.com/sparkfun/SparkFun_ATECCX08A_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.1

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


#include "Wire.h"

#define ATECC508A_ADDRESS_DEFAULT 0x60 //7-bit unshifted default I2C Address
// 0x60 on a fresh chip. note, this is software definable


/* Protocol + Cryptographic defines */
#define RESPONSE_COUNT_SIZE  1
#define RESPONSE_SIGNAL_SIZE 1
#define RESPONSE_SHA_SIZE    32
#define RESPONSE_INFO_SIZE   4
#define RESPONSE_RANDOM_SIZE 32
#define CRC_SIZE             2
#define CONFIG_ZONE_SIZE     128
#define SERIAL_NUMBER_SIZE   9
#define REVISION_NUMBER_SIZE   4

#define RANDOM_BYTES_BLOCK_SIZE 32
#define SHA256_SIZE          32
#define PUBLIC_KEY_SIZE      64
#define SIGNATURE_SIZE       64


// WORD ADDRESS VALUES
// These are sent in any write sequence to the IC.
// They tell the IC what we are going to do: Reset, Sleep, Idle, Command.
#define WORD_ADDRESS_VALUE_COMMAND 	0x03	// This is the "command" word address, 
//this tells the IC we are going to send a command, and is used for most communications to the IC
#define WORD_ADDRESS_VALUE_IDLE 0x02 // used to enter idle mode

// COMMANDS (aka "opcodes" in the datasheet)
#define COMMAND_OPCODE_INFO 	  0x30 // Return device state information.
#define COMMAND_OPCODE_LOCK 	  0x17 // Lock configuration and/or Data and OTP zones
#define COMMAND_OPCODE_RANDOM 	0x1B // Create and return a random number (32 bytes of data)
#define COMMAND_OPCODE_READ 	  0x02 // Return data at a specific zone and address.
#define COMMAND_OPCODE_WRITE 	  0x12 // Return data at a specific zone and address.
#define COMMAND_OPCODE_SHA 		  0x47 // Computes a SHA-256 or HMAC/SHA digest for general purpose use by the system.
#define COMMAND_OPCODE_GENKEY 	0x40 // Creates a key (public and/or private) and stores it in a memory key slot
#define COMMAND_OPCODE_NONCE 	  0x16 // 
#define COMMAND_OPCODE_SIGN 	  0x41 // Create an ECC signature with contents of TempKey and designated key slot
#define COMMAND_OPCODE_VERIFY 	0x45 // takes an ECDSA <R,S> signature and verifies that it is correctly generated from a given message and public key
#define COMMAND_OPCODE_AES      0x51 // AES encryption/decryption
 


// SHA Params
#define SHA_START						0b00000000
#define SHA_UPDATE					0b00000001
#define SHA_END							0b00000010
#define SHA_BLOCK_SIZE			64

// AES paramaters

#define AES_ENCRYPT                   0x00
#define AES_DECRYPT                   0x01
#define AES_BLOCKSIZE                 16      // size in bytes


/* Protocol Sizes */
#define ATRCC508A_PROTOCOL_FIELD_SIZE_COMMAND 1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_LENGTH  1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_OPCODE  1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_PARAM1  1
#define ATRCC508A_PROTOCOL_FIELD_SIZE_PARAM2  2
#define ATRCC508A_PROTOCOL_FIELD_SIZE_CRC     CRC_SIZE
/* Protocol codes */
#define ATRCC508A_SUCCESSFUL_TEMPKEY 0x00
#define ATRCC508A_SUCCESSFUL_VERIFY  0x00
#define ATRCC508A_SUCCESSFUL_WRITE   0x00
#define ATRCC508A_SUCCESSFUL_SHA     0x00
#define ATRCC508A_SUCCESSFUL_LOCK    0x00
#define ATRCC508A_SUCCESSFUL_WAKEUP  0x11
#define ATRCC508A_SUCCESSFUL_GETINFO 0x50 /* Revision number */

// predefined status codes
#define STATUS_SUCCESS                0x00
#define STATUS_VERIFICATION_ERROR     0x01
#define STATUS_PARSE_ERROR            0x03
#define STATUS_ECC_FAULT              0x05
#define STATUS_SELFTEST_ERROR         0x07
#define STATUS_EXECUTION_ERROR        0x0F
#define STATUS_WAKE_TOKEN_RECEIVED    0x11
#define STATUS_WATCHDOG_EXPIRATION    0xEE
#define STATUS_CRC_ERROR              0xFF


// our own status codes
#define STATUS_TIMEOUT_ERROR          0x1000
#define STATUS_INVALID_PARAMETER      0x1001
#define STATUS_MESSAGE_COUNT_ERROR    0x1002
#define STATUS_MESSAGE_CRC_ERROR      0x1003
#define STATUS_INPUT_BUFFER_TOO_SMALL 0x1004

/* Receive constants */
#define ATRCC508A_MAX_REQUEST_SIZE 32
#define ATRCC508A_MAX_RETRIES 20

/* configZone EEPROM mapping */
#define CONFIG_ZONE_READ_SIZE       32
#define CONFIG_ZONE_SERIAL_PART0     0
#define CONFIG_ZONE_SERIAL_PART1     8
#define CONFIG_ZONE_REVISION_NUMBER  4
#define CONFIG_ZONE_AES_STATUS      13
#define CONFIG_ZONE_SLOT_CONFIG     20
#define CONFIG_ZONE_OTP_LOCK        86
#define CONFIG_ZONE_LOCK_STATUS     87
#define CONFIG_ZONE_SLOTS_LOCK0     88
#define CONFIG_ZONE_SLOTS_LOCK1     89
#define CONFIG_ZONE_KEY_CONFIG	    96


// Lock command PARAM1 zone options (aka Mode). more info at table on datasheet page 75
// 		? _ _ _  _ _ _ _ 	Bits 7 verify zone summary, 1 = ignore summary and write to zone!
// 		_ ? _ _  _ _ _ _ 	Bits 6 Unused, must be zero
// 		_ _ ? ?  ? ? _ _ 	Bits 5-2 Slot number (in this example, we use slot 0, so "0 0 0 0")
// 		_ _ _ _  _ _ ? ? 	Bits 1-0 Zone or locktype. 00=Config, 01=Data/OTP, 10=Single Slot in Data, 11=illegal

#define LOCK_MODE_ZONE_CONFIG 			0b10000000
#define LOCK_MODE_ZONE_DATA_AND_OTP 	0b10000001
#define LOCK_MODE_SLOT			0b10000010
#define LOCK_MODE_SLOT0			0b10000010
#define LOCKMODE_SLOT1      0b10000110
#define LOCKMODE_SLOT2      0b10001010
#define LOCKMODE_SLOT3      0b10001110
#define LOCKMODE_SLOT4      0b10010010
#define LOCKMODE_SLOT5      0b10010110
#define LOCKMODE_SLOT6      0b10011010
#define LOCKMODE_SLOT7      0b10011110
#define LOCKMODE_SLOT8      0b10100010
#define LOCKMODE_SLOT9      0b10100110
#define LOCKMODE_SLOT10     0b10101010
#define LOCKMODE_SLOT11     0b10101110
#define LOCKMODE_SLOT12     0b10110010
#define LOCKMODE_SLOT13     0b10110110
#define LOCKMODE_SLOT14     0b10111010
#define LOCKMODE_SLOT15     0b10111110

// GenKey command PARAM1 zone options (aka Mode). more info at table on datasheet page 71
#define GENKEY_MODE_PUBLIC 			0b00000000
#define GENKEY_MODE_NEW_PRIVATE 	0b00000100

#define NONCE_MODE_PASSTHROUGH		 0b00000011 // Operate in pass-through mode and Write TempKey with NumIn. datasheet pg 79
#define SIGN_MODE_TEMPKEY			     0b10000000 // The message to be signed is in TempKey. datasheet pg 85
#define VERIFY_MODE_EXTERNAL		   0b00000010 // Use an external public key for verification, pass to command as data post param2, ds pg 89
#define VERIFY_MODE_STORED			   0b00000000 // Use an internally stored public key for verification, param2 = keyID, ds pg 89
#define VERIFY_MODE_SOURCE_TEMPKEY 0b00000000
#define VERIFY_MODE_SOURCE_MSGDIGBUF 0x20

#define VERIFY_PARAM2_KEYTYPE_ECC 	0x0004 // When verify mode external, param2 should be KeyType, ds pg 89
#define VERIFY_PARAM2_KEYTYPE_NONECC 	0x0007 // When verify mode external, param2 should be KeyType, ds pg 89

#define ZONE_CONFIG 0x00
#define ZONE_OTP 0x01
#define ZONE_DATA 0x02

#define ADDRESS_CONFIG_READ_BLOCK_0 0x0000 // 00000000 00000000 // param2 (byte 0), address block bits: _ _ _ 0  0 _ _ _ 
#define ADDRESS_CONFIG_READ_BLOCK_1 0x0008 // 00000000 00001000 // param2 (byte 0), address block bits: _ _ _ 0  1 _ _ _ 
#define ADDRESS_CONFIG_READ_BLOCK_2 0x0010 // 00000000 00010000 // param2 (byte 0), address block bits: _ _ _ 1  0 _ _ _ 
#define ADDRESS_CONFIG_READ_BLOCK_3 0x0018 // 00000000 00011000 // param2 (byte 0), address block bits: _ _ _ 1  1 _ _ _ 

/* Response signals always come after the first count byte */
#define RESPONSE_COUNT_INDEX 0
#define RESPONSE_SIGNAL_INDEX RESPONSE_COUNT_SIZE
#define RESPONSE_SHA_INDEX RESPONSE_COUNT_SIZE
#define RESPONSE_READ_INDEX RESPONSE_COUNT_SIZE
#define RESPONSE_GETINFO_SIGNAL_INDEX (RESPONSE_COUNT_SIZE + 2)

#define BUFFER_SIZE  256


class ATECCX08A {
  public:
  
    //By default use Wire, standard I2C speed, and the default ADS1015 address
		boolean begin(uint8_t i2caddr = ATECC508A_ADDRESS_DEFAULT, TwoWire &wirePort = Wire, Stream &serialPort = Serial); 
		
		boolean receiveResponseData(uint8_t length = 0, boolean debug = false);
		boolean checkCount(boolean debug = false);
		boolean checkCrc(boolean debug = false);
		void cleanInputBuffer();
		
		boolean wakeUp();
		void idleMode();
		
		boolean getInfo();
		
		// locking methods
		boolean lockConfiguration(); // note, this PERMINANTLY disables changes to config zone - including changing the I2C address!
		boolean lockDataAndOTP();
		boolean lockDataSlot(uint16_t slot);
		boolean lock(uint8_t zone);
		
		
		// Random array and fuctions
		boolean generateRandomBytes(uint8_t *randomValue, int length, boolean debug = false);
		byte getRandomByte(boolean debug = false);
		int getRandomInt(boolean debug = false);
		long getRandomLong(boolean debug = false);
		long random(long max);
		long random(long min, long max);
		
	// SHA256
		boolean sha256(const uint8_t *data, size_t len, uint8_t *hash);
		
		void atca_calculate_crc(uint8_t length, const uint8_t *data);	
		
		// Key functions
		boolean createNewKeyPair(uint8_t *publicKey, int size, uint16_t slot = 0x0000);
		boolean generatePublicKey(uint8_t *publicKey, int size, uint16_t slot = 0x0000, boolean debug = false);
		boolean createSignature(uint8_t *signature, int size, const uint8_t *data, uint16_t slot, boolean debug = false); 
    boolean signWithSHA256(uint8_t *signature, int sigSize, const uint8_t *data, int length, int slot, boolean debug = false);
    boolean verifyWithSHA256(const uint8_t *signature, int sigSize, const uint8_t *data, int length, int slot, boolean debug = false);
		
		boolean loadTempKey(const uint8_t *data);  // load 32 bytes of data into tempKey (a temporary memory spot in the IC)
		boolean signTempKey(uint8_t *signature, int size, uint16_t slot = 0x0000, boolean debug = false); // create signature using contents of TempKey and PRIVATE KEY in slot
		boolean verifySignature(const uint8_t *message, const uint8_t *signature, const uint8_t *publicKey); // external ECC publicKey only

		boolean read(uint8_t zone, uint16_t address, uint8_t length, boolean debug = false);
		boolean read(uint8_t zone, uint16_t address, uint8_t *response, uint8_t length, boolean debug = false);
		boolean write(uint8_t zone, uint16_t address, const uint8_t *data, uint8_t length_of_data, boolean debug = false);
		boolean writeSlot(const uint8_t *data, int length, int slot, boolean debug = false);
    boolean readSlot(uint8_t *data, int length, int slot, boolean debug = false);
		boolean encryptDecryptBlock(const uint8_t *input, int inputSize, uint8_t *output, int outputSize, uint8_t slot, uint8_t keyIndex, uint8_t mode, boolean debug=false);
    int     addressForSlotOffset(int slot, int offset);
		
		boolean readConfigZone(boolean debug = false);
		byte    *getConfigZone();

		// get lock states	
		boolean getConfigLockStatus();
		boolean getDataOTPLockStatus();
		boolean getSlotLockStatus(uint16_t slot);
		boolean isSlotLocked(uint16_t slot);
		
	  boolean getSerialNumber(uint8_t *serialNo, int length);
	  boolean getRevisionNumber(uint8_t *revisionNo, int length);
		int     getStatus();
		boolean isAESEnabled();
	
	protected:
		boolean sendCommand(uint8_t command_opcode, uint8_t param1, uint16_t param2, const uint8_t *data = NULL, size_t length_of_data = 0, boolean debug=false);
	  void setStatus(int status);
	
  private:
		TwoWire *_i2cPort;
		uint8_t _i2caddr;
		Stream *_debugSerial; //The generic connection to user's chosen serial hardware
		
		uint8_t crc[2] = {0, 0};
  	byte configZone[128]; // used to store configuration zone bytes read from device EEPROM
  	byte inputBuffer[BUFFER_SIZE]; // used to store messages received from the IC as they come in
    byte status;
  	boolean configLockStatus; // pulled from configZone[87], then set according to status (0x55=UNlocked, 0x00=Locked)
	  boolean dataOTPLockStatus; // pulled from configZone[86], then set according to status (0x55=UNlocked, 0x00=Locked)
		uint8_t countGlobal = 0; // used to add up all the bytes on a long message. Important to reset before each new receiveMessageData();
  	uint8_t revisionNumber[REVISION_NUMBER_SIZE]; // used to store the complete revision number, pulled from configZone[4-7]
	  uint8_t serialNumber[SERIAL_NUMBER_SIZE]; // used to store the complete Serial number, pulled from configZone[0-3] and configZone[8-12]		
		
		boolean beginSHA256();
    boolean updateSHA256(const uint8_t *plainText, int length);
		boolean endSHA256(uint8_t *hash, int size);


	  void printHexValue(byte value);
		void printHexValue(const byte *value, int length, const char *separator);
	
};


