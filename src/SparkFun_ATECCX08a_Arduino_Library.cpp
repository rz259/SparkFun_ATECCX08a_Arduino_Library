/*
  This is a library written for the ATECCX08A Criptographic Co-Processor (QWIIC).

  Written by Pete Lewis @ SparkFun Electronics, August 5th, 2019

  The IC uses I2C or 1-wire to communicate. This library only supports I2C.

  https://github.com/sparkfun/SparkFun_ATECCX08A_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.10

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SparkFun_ATECCX08a_Arduino_Library.h"

/** \brief 

	begin(uint8_t i2caddr, TwoWire &wirePort, Stream &serialPort)
	
	returns false if IC does not respond,
	returns true if wake() function is successful
	
	Note, in most SparkFun Arduino Libraries, begin would call a different
	function called isConnected() to check status on the bus, but because 
	this IC will ACK and respond with a status, we are gonna use wakeUp() 
	for the same purpose.
*/

boolean ATECCX08A::begin(uint8_t i2caddr, TwoWire &wirePort, Stream &serialPort)
{
  //Bring in the user's choices
  _i2cPort = &wirePort; //Grab which port the user wants us to use
  
  _debugSerial = &serialPort; //Grab which port the user wants us to use

  _i2caddr = i2caddr;

  return ( wakeUp() ); // see if the IC wakes up properly, return responce.
}

/** \brief 

	wakeUp()
	
	This function wakes up the ATECCX08a IC
	Returns TRUE if the IC responds with correct verification
	Message (0x04, 0x11, 0x33, 0x44) 
	The actual status byte we are looking for is the 0x11.
	The complete message is as follows:
	COUNT, DATA, CRC[0], CRC[1].
	0x11 means that it received the wake condition and is goat to goo.
	
	Note, in most SparkFun Arduino Libraries, we would use a different
	function called isConnected(), but because this IC will ACK and
	respond with a status, we are gonna use wakeUp() for the same purpose.
*/

boolean ATECCX08A::wakeUp()
{
  _i2cPort->beginTransmission(0x00); // set up to write to address "0x00",
  // This creates a "wake condition" where SDA is held low for at least tWLO
  // tWLO means "wake low duration" and must be at least 60 uSeconds (which is acheived by writing 0x00 at 100KHz I2C)
  _i2cPort->endTransmission(); // actually send it

  delayMicroseconds(1500); // required for the IC to actually wake up.
  // 1500 uSeconds is minimum and known as "Wake High Delay to Data Comm." tWHI, and SDA must be high during this time.

  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if (receiveResponseData(4) == false) 
		return false;
  if (checkCount() == false) 
		return false;
  if (checkCrc() == false) 
		return false;
  if (inputBuffer[1] == 0x11) 
		return true;   // If we hear a "0x11", that means it had a successful wake up.
  else 
		return false;
}

/** \brief

	idleMode()
	
	The ATECCX08A goes into the idle mode and ignores all subsequent I/O transitions
	until the next wake flag. The contents of TempKey and RNG Seed registers are retained.
	Idle Power Supply Current: 800uA.
	Note, it will automatically go into sleep mode after watchdog timer has been reached (1.3-1.7sec).
*/

void ATECCX08A::idleMode()
{
  _i2cPort->beginTransmission(_i2caddr); // set up to write to address
  _i2cPort->write(WORD_ADDRESS_VALUE_IDLE); // enter idle command (aka word address - the first part of every communication to the IC)
  _i2cPort->endTransmission(); // actually send it  
}

/** \brief

	getInfo()
	
	This function sends the INFO Command and listens for the correct version (0x50) within the response.
	The Info command has a mode parameter, and in this function we are using the "Revision" mode (0x00)
	At the time of data sheet creation the Info command will return 0x00 0x00 0x50 0x00. For
	all versions of the ECC508A the 3rd byte will always be 0x50. The fourth byte will indicate the
	silicon revision.
*/

boolean ATECCX08A::getInfo()
{
  sendCommand(COMMAND_OPCODE_INFO, 0x00, 0x0000); // param1 - 0x00 (revision mode).

  delay(1); // time for IC to process command and exectute
  
    // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if (receiveResponseData(7, true) == false) return false;
  idleMode();
  if (checkCount() == false) return false;
  if (checkCrc() == false) return false;
  if (inputBuffer[3] == 0x50) 
	{
		
  	setStatus(STATUS_SUCCESS);
		return true;   // If we hear a "0x50", that means it had a successful version response.
	}
  else 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
}

/** \brief

	lockConfiguration()
	
	This function sends the LOCK Command with the configuration zone parameter, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lockConfiguration()
{
  return lock(LOCK_MODE_ZONE_CONFIG);
}

/** \brief

	readConfigZone()
	
	This function reads the entire configuration zone EEPROM memory on the device.
	It stores them for vewieing in a large array called configZone[128].
	In addition to configuration settings, the configuration memory on the IC also
	contains the serial number, revision number, lock statuses, and much more.
	This function also updates global variables for these other things.
*/

boolean ATECCX08A::readConfigZone(boolean debug)
{
  // read block 0, the first 32 bytes of config zone into inputBuffer
  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_0, 32); 
  
  // copy current contents of inputBuffer into configZone[] (for later viewing/comparing)
  memcpy(&configZone[0], &inputBuffer[1], 32);
  
  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_1, 32); 	// read block 1
  memcpy(&configZone[32], &inputBuffer[1], 32); 	// copy block 1
  
  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_2, 32); 	// read block 2
  memcpy(&configZone[64], &inputBuffer[1], 32); 	// copy block 2
  
  read(ZONE_CONFIG, ADDRESS_CONFIG_READ_BLOCK_3, 32); 	// read block 3
  memcpy(&configZone[96], &inputBuffer[1], 32); 	// copy block 3  
  
  // pull out serial number from configZone, and copy to public variable within this instance
  memcpy(&serialNumber[0], &configZone[0], 4); 	// copy SN<0:3> 
  memcpy(&serialNumber[4], &configZone[8], 5); 	// copy SN<4:8> 
  
  // pull out revision number from configZone, and copy to public variable within this instance
  memcpy(&revisionNumber[0], &configZone[4], 4); 	// copy RevNum<0:3>   
  
  // set lock statuses for config, data/otp, and slot 0
  if (configZone[87] == 0x00) configLockStatus = true;
  else configLockStatus = false;
  
  if (configZone[86] == 0x00) dataOTPLockStatus = true;
  else dataOTPLockStatus = false;
  
  if ( (configZone[88] & (1<<0) ) == true) slot0LockStatus = false; // LSB is slot 0. if bit set = UN-locked.
  else slot0LockStatus = true;
  
  if (debug)
  {
    _debugSerial->println("configZone: ");
    for (int i = 0; i < sizeof(configZone) ; i++)
    {
      _debugSerial->print(i);
	  _debugSerial->print(": 0x");
	  if ((configZone[i] >> 4) == 0) _debugSerial->print("0"); // print preceeding high nibble if it's zero
	  _debugSerial->print(configZone[i], HEX); 
	  _debugSerial->print(" \t0b");
	  for(int bit = 7; bit >= 0; bit--) _debugSerial->print(bitRead(configZone[i],bit)); // print binary WITH preceding '0' bits
	  _debugSerial->println();
    }
    _debugSerial->println();
  }
  
  
  return true;
}

/** \brief

	lockDataAndOTP()
	
	This function sends the LOCK Command with the Data and OTP (one-time-programming) zone parameter, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lockDataAndOTP()
{
  return lock(LOCK_MODE_ZONE_DATA_AND_OTP);
}

/** \brief

	lockDataSlot0()
	
	This function sends the LOCK Command with the Slot 0 zone parameter, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lockDataSlot0()
{
  return lock(LOCK_MODE_SLOT0);
}

boolean ATECCX08A::lockDataSlot(uint16_t slot)
{
	uint8_t lockMode;
	
	
	slot = slot << 2; // the slot to be locked is encoded in bits 2 to 5 
	                  // (see data sheet p. 75), so we need to shift the slot 2 bits left
	lockMode = LOCK_MODE_SLOT | slot; // set bits in lockmode parameter
  return lock(lockMode);
}


/** \brief

	lock(byte zone)
	
	This function sends the LOCK Command using the argument zone as parameter 1, 
	and listens for success response (0x00).
*/

boolean ATECCX08A::lock(uint8_t zone)
{
  sendCommand(COMMAND_OPCODE_LOCK, zone, 0x0000);

  delay(32); // time for IC to process command and exectute
  
  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if (receiveResponseData(4, true) == false) return false;
  idleMode();
  if (checkCount() == false) return false;
  if (checkCrc() == false) return false;
	setStatus(inputBuffer[1]);
  if (inputBuffer[1] == 0x00) return true;   // If we hear a "0x00", that means it had a successful lock
  else return false;
}

/** \brief

	generateRandomBytes(boolean debug)
	
    This function pulls a complete random number (all 32 bytes)
    In order to keep compatibility with ATmega328 based arduinos,
    We have offered some other functions that return variables more usable (i.e. byte, int, long)
    They are getRandomByte(), getRandomInt(), and getRandomLong().
*/

boolean ATECCX08A::generateRandomBytes(uint8_t *randomValue, int length, boolean debug)
{
	if (length < 0 || length > 32)
		return false;
	
  sendCommand(COMMAND_OPCODE_RANDOM, 0x00, 0x0000); 
  // param1 = 0. - Automatically update EEPROM seed only if necessary prior to random number generation. Recommended for highest security.
  // param2 = 0x0000. - must be 0x0000.

  delay(23); // time for IC to process command and exectute

  // Now let's read back from the IC. This will be 35 bytes of data (count + 32_data_bytes + crc[0] + crc[1])

  if (receiveResponseData(35, debug) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  idleMode();
  if (checkCount(debug) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
	
  if (checkCrc(debug) == false) 
	{	
		setStatus(STATUS_CRC_ERROR);
		return false;
  }
  
  // update random32Bytes[] array
  // we don't need the count value (which is currently the first byte of the inputBuffer)
  for (int i = 0 ; i < length ; i++) // for loop through to grab all but the first position (which is "count" of the message)
  {
    randomValue[i] = inputBuffer[i + 1];
  }

  if (debug)
  {
    _debugSerial->print("randomValue: ");
    for (int i = 0; i < length; i++)
    {
      _debugSerial->print(randomValue[i], HEX);
      _debugSerial->print(",");
    }
    _debugSerial->println();
  }
  
  setStatus(STATUS_SUCCESS);
  return true;
}

/** \brief

	getRandomByte(boolean debug)
	
    This function returns a random byte.
	It calls generateRandomBytes(), then uses the first byte in that array for a return value.
*/

byte ATECCX08A::getRandomByte(boolean debug)
{
	uint8_t randomValue[32];
	
  generateRandomBytes(randomValue, sizeof(randomValue), debug);
  return randomValue[0];
}

/** \brief

	getRandomInt(boolean debug)
	
    This function returns a random Int.
	It calls generateRandomBytes(), then uses the first 2 bytes in that array for a return value.
	It bitwize ORS the first two bytes of the array into the return value.
*/

int ATECCX08A::getRandomInt(boolean debug)
{
	uint8_t randomValue[32];
	
  generateRandomBytes(randomValue, sizeof(randomValue), debug);
  int return_val;
  return_val = randomValue[0]; // store first randome byte into return_val
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= randomValue[1]; // "or in" the next byte in the array
  return return_val;
}

/** \brief

	getRandomLong(boolean debug)
	
    This function returns a random Long.
	It calls generateRandomBytes(), then uses the first 4 bytes in that array for a return value.
	It bitwize ORS the first 4 bytes of the array into the return value.
*/

long ATECCX08A::getRandomLong(boolean debug)
{
	uint8_t randomValue[32];

  generateRandomBytes(randomValue, sizeof(randomValue), debug);
  long return_val;
  return_val = randomValue[0]; // store first randome byte into return_val
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= randomValue[1]; // "or in" the next byte in the array
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= randomValue[2]; // "or in" the next byte in the array
  return_val <<= 8; // shift it over, to make room for the next byte
  return_val |= randomValue[3]; // "or in" the next byte in the array
  return return_val;
}

/** \brief

	random(long max)
	
    This function returns a positive random Long between 0 and max
	max can be up to the larges positive value of a long: 2147483647
*/

long ATECCX08A::random(long max)
{
  return random(0, max);
}

/** \brief

	random(long min, long max)
	
    This function returns a random Long with set boundaries of min and max.
	If you flip min and max, it still works!
	Also, it can handle negative numbers. Wahoo!
*/

long ATECCX08A::random(long min, long max)
{
  long randomLong = getRandomLong();
  long halfFSR = (max - min) / 2; // half of desired full scale range
  long midPoint = (max + min) / 2; // where we "start" out output value, then add in a fraction of halfFSR
  float fraction = float(randomLong) / 2147483647;
  return (midPoint + (halfFSR * fraction) );
}

/** \brief

	receiveResponseData(uint8_t length, boolean debug)
	
	This function receives messages from the ATECCX08a IC (up to 128 Bytes)
	It will return true if it receives the correct amount of data and good CRCs.
	What we hear back from the IC is always formatted with the following series of bytes:
	COUNT, DATA, CRC[0], CRC[1]
	Note, the count number includes itself, the num of data bytes, and the two CRC bytes in the total, 
	so a simple response message from the IC that indicates that it heard the wake 
	condition properly is like so:
	EXAMPLE Wake success response: 0x04, 0x11, 0x33, 0x44
	It needs length argument:
	length: length of data to receive (includes count + DATA + 2 crc bytes)
*/

boolean ATECCX08A::receiveResponseData(uint8_t length, boolean debug)
{	

  // pull in data 32 bytes at at time. (necessary to avoid overflow on atmega328)
  // if length is less than or equal to 32, then just pull it in.
  // if length is greater than 32, then we must first pull in 32, then pull in remainder.
  // lets use length as our tracker and we will subtract from it as we pull in data.
  const int maxRequests = 20;
	
  countGlobal = 0; // reset for each new message (most important, like wensleydale at a cheese party)
  cleanInputBuffer();
  byte requestAttempts = 0; // keep track of how many times we've attempted to request, to break out if necessary
  
  while(length)
  {
    byte requestAmount; // amount of bytes to request, needed to pull in data 32 bytes at a time
	  if (length > 32) 
			requestAmount = 32; // as we have more than 32 to pull in, keep pulling in 32 byte chunks
	  else 
			requestAmount = length; // now we're ready to pull in the last chunk.
	  _i2cPort->requestFrom(_i2caddr, requestAmount);    // request bytes from slave
	  requestAttempts++;

		while (_i2cPort->available())   // slave may send less than requested
		{
			inputBuffer[countGlobal] = _i2cPort->read();    // receive a byte as character
			length--; // keep this while loop active until we've pulled in everything
			countGlobal++; // keep track of the count of the entire message.
		}  
		if (requestAttempts == maxRequests) 
			 break; // this probably means that the device is not responding.
		 /*
		 else
			 delay(1);
		 */
	}

	if (debug)
	{
//		_debugSerial->println("requestAttempts: " + String(requestAttempts));
		_debugSerial->println("countGlobal    : " + String(countGlobal));
		_debugSerial->print("inputBuffer: ");
		for (int i = 0; i < countGlobal ; i++)
		{
			_debugSerial->print(inputBuffer[i], HEX);
			_debugSerial->print(",");
		}
	_debugSerial->println();	  
  }
	if (requestAttempts < maxRequests)
    return true;
	else
		return false;
}

/** \brief

	checkCount(boolean debug)
	
	This function checks that the count byte received in the most recent message equals countGlobal
	Call receiveResponseData, and then imeeditately call this to check the count of the complete message.
	Returns true if inputBuffer[0] == countGlobal.
*/

boolean ATECCX08A::checkCount(boolean debug)
{
  if (debug)
  {
    _debugSerial->print("countGlobal: 0x");
	  _debugSerial->println(countGlobal, HEX);
	  _debugSerial->print("count heard from IC (inputBuffer[0]): 0x");
    _debugSerial->println(inputBuffer[0], HEX);
  }
  // Check count; the first byte sent from IC is count, and it should be equal to the actual message count
  if (inputBuffer[0] != countGlobal) 
  {
	  if (debug) 
			_debugSerial->println("Message Count Error");
	  return false;
  }  
  return true;
}

/** \brief

	checkCrc(boolean debug)
	
	This function checks that the CRC bytes received in the most recent message equals a calculated CRCs
	Call receiveResponseData, then call immediately call this to check the CRCs of the complete message.
*/

boolean ATECCX08A::checkCrc(boolean debug)
{
  // Check CRC[0] and CRC[1] are good to go.
  
  atca_calculate_crc(countGlobal-2, inputBuffer);   // first calculate it
  
  if (debug)
  {
    _debugSerial->print("CRC[0] Calc: 0x");
	  _debugSerial->println(crc[0], HEX);
	  _debugSerial->print("CRC[1] Calc: 0x");
    _debugSerial->println(crc[1], HEX);
  }
  
  if ( (inputBuffer[countGlobal-1] != crc[1]) || (inputBuffer[countGlobal-2] != crc[0]) )   // then check the CRCs.
  {
	  if (debug) 
			_debugSerial->println("Message CRC Error");
	  return false;
  }
  if (debug)
	{
		_debugSerial->println("CRC verification ok");
	}
  return true;
}

/** \brief

	atca_calculate_crc(uint8_t length, uint8_t *data)
	
    This function calculates CRC.
    It was copied directly from the App Note provided from Microchip.
    Note, it seems to be their own unique type of CRC cacluation.
    View the entire app note here:
    http://ww1.microchip.com/downloads/en/AppNotes/Atmel-8936-CryptoAuth-Data-Zone-CRC-Calculation-ApplicationNote.pdf
    \param[in] length number of bytes in buffer
    \param[in] data pointer to data for which CRC should be calculated
*/

void ATECCX08A::atca_calculate_crc(uint8_t length, uint8_t *data)
{
  uint8_t counter;
  uint16_t crc_register = 0;
  uint16_t polynom = 0x8005;
  uint8_t shift_register;
  uint8_t data_bit, crc_bit;
  for (counter = 0; counter < length; counter++) {
    for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
      data_bit = (data[counter] & shift_register) ? 1 : 0;
      crc_bit = crc_register >> 15;
      crc_register <<= 1;
      if (data_bit != crc_bit)
        crc_register ^= polynom;
    }
  }
  crc[0] = (uint8_t) (crc_register & 0x00FF);
  crc[1] = (uint8_t) (crc_register >> 8);
}


/** \brief

	cleanInputBuffer()
	
    This function sets the entire inputBuffer to 0xFFs.
	It is helpful for debugging message/count/CRCs errors.
*/

void ATECCX08A::cleanInputBuffer()
{
  for (int i = 0; i < sizeof(inputBuffer) ; i++)
  {
    inputBuffer[i] = 0xFF;
  }
}

/** \brief

	createNewKeyPair(uint16_t slot)
	
    This function sends the command to create a new key pair (private AND public)
	in the slot designated by argument slot (default slot 0).
	Sparkfun Default Configuration Sketch calls this, and then locks the data/otp zones and slot 0.
*/

boolean ATECCX08A::createNewKeyPair(uint16_t slot)
{  
  sendCommand(COMMAND_OPCODE_GENKEY, GENKEY_MODE_NEW_PRIVATE, slot);

  delay(115); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if (receiveResponseData(64 + 2 + 1) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false; // public key (64), plus crc (2), plus count (1)
	}
  idleMode();
  boolean checkCountResult = checkCount();
  boolean checkCrcResult = checkCrc();
  
  
  // update publicKey64Bytes[] array
  if (checkCountResult && checkCrcResult) // check that it was a good message
  {
  	// we don't need the count value (which is currently the first byte of the inputBuffer)
	  for (int i = 0 ; i < 64 ; i++) // for loop through to grab all but the first position (which is "count" of the message)
	  {
	    publicKey64Bytes[i] = inputBuffer[i + 1];
  	}
		setStatus(STATUS_SUCCESS);
	  return true;
  }
  else 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
}

/** \brief

	generatePublicKey(uint16_t slot, boolean debug)

    This function uses the GENKEY command in "Public Key Computation" mode.
	
    Generates an ECC public key based upon the private key stored in the slot defined by the KeyID
	parameter (aka slot). Defaults to slot 0. 
	
	Note, if you haven't created a private key in the slot already, then this will fail.
	
	The generated public key is read back from the device, and then copied from inputBuffer to 
	a global variable named publicKey64Bytes for later use.
*/

boolean ATECCX08A::generatePublicKey(uint16_t slot, boolean debug)
{
  sendCommand(COMMAND_OPCODE_GENKEY, GENKEY_MODE_PUBLIC, slot);

  delay(115); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if (receiveResponseData(64 + 2 + 1) == false)
	{ 
		setStatus(STATUS_EXECUTION_ERROR);
    return false; // public key (64), plus crc (2), plus count (1)
	}
  idleMode();
  boolean checkCountResult = checkCount();
  boolean checkCrcResult = checkCrc();
  
  // update publicKey64Bytes[] array
  if (checkCountResult && checkCrcResult) // check that it was a good message
  {  
    // we don't need the count value (which is currently the first byte of the inputBuffer)
    for (int i = 0 ; i < 64 ; i++) // for loop through to grab all but the first position (which is "count" of the message)
    {
      publicKey64Bytes[i] = inputBuffer[i + 1];
    }
	
		if (debug)
		{
			_debugSerial->println("This device's Public Key:");
			_debugSerial->println();
			_debugSerial->println("uint8_t publicKey[64] = {");
			for (int i = 0; i < sizeof(publicKey64Bytes) ; i++)
			{
				_debugSerial->print("0x");
				if ((publicKey64Bytes[i] >> 4) == 0) _debugSerial->print("0"); // print preceeding high nibble if it's zero
				_debugSerial->print(publicKey64Bytes[i], HEX);
				if (i != 63) _debugSerial->print(", ");
				if ((63-i) % 16 == 0) _debugSerial->println();
			}
			_debugSerial->println("};");
			_debugSerial->println();
		}
  	setStatus(STATUS_SUCCESS);
  	return true;
  }
  else 
	{
  	setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
}

/** \brief

	read(uint8_t zone, uint16_t address, uint8_t length, boolean debug)

    Reads data from the IC at a specific zone and address.
	Your data response will be available at inputBuffer[].
	
	For more info on address encoding, see datasheet pg 58.
*/

boolean ATECCX08A::read(uint8_t zone, uint16_t address, uint8_t *response, uint8_t length, boolean debug)
{
  // adjust zone as needed for whether it's 4 or 32 bytes length read
  // bit 7 of zone needs to be set correctly 
  // (0 = 4 Bytes are read) 
  // (1 = 32 Bytes are read)
  if (length == 32) 
  {
	  zone |= 0b10000000; // set bit 7
  }
  else if (length == 4)
  {
	  zone &= ~0b10000000; // clear bit 7
  }
  else
  {
	  return false; // invalid length, abort.
  }

  sendCommand(COMMAND_OPCODE_READ, zone, address);
  
  delay(1); // time for IC to process command and exectute

  // Now let's read back from the IC. 
  
  if (receiveResponseData(length + 3, debug) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  idleMode();
  if (checkCount(debug) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  if (checkCrc(debug) == false) 
	{
		setStatus(STATUS_CRC_ERROR);
  	return false;
	}
  memcpy(response, &inputBuffer[1], length);
	setStatus(STATUS_SUCCESS);
  return true;
}

boolean ATECCX08A::read(uint8_t zone, uint16_t address, uint8_t length, boolean debug)
{
  // adjust zone as needed for whether it's 4 or 32 bytes length read
  // bit 7 of zone needs to be set correctly 
  // (0 = 4 Bytes are read) 
  // (1 = 32 Bytes are read)
  if (length == 32) 
  {
	  zone |= 0b10000000; // set bit 7
  }
  else if (length == 4)
  {
	  zone &= ~0b10000000; // clear bit 7
  }
  else
  {
	  return 0; // invalid length, abort.
  }

  sendCommand(COMMAND_OPCODE_READ, zone, address);
  
  delay(1); // time for IC to process command and exectute

  // Now let's read back from the IC. 
  
  if (receiveResponseData(length + 3, debug) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  idleMode();
  if (checkCount(debug) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  if (checkCrc(debug) == false) 
	{
		setStatus(STATUS_CRC_ERROR);
  	return false;
	}
  
	setStatus(STATUS_SUCCESS);
  return true;
}


/** \brief

	write(uint8_t zone, uint16_t address, uint8_t *data, uint8_t length_of_data)

    Writes data to a specific zone and address on the IC.
	
	For more info on zone and address encoding, see datasheet pg 58.
*/

boolean ATECCX08A::write(uint8_t zone, uint16_t address, const uint8_t *data, uint8_t length_of_data)
{
  // adjust zone as needed for whether it's 4 or 32 bytes length write
  // bit 7 of param1 needs to be set correctly 
  // (0 = 4 Bytes are written) 
  // (1 = 32 Bytes are written)
  if (length_of_data == 32) 
  {
	  zone |= 0b10000000; // set bit 7
  }
  else if (length_of_data == 4)
  {
	  zone &= ~0b10000000; // clear bit 7
  }
  else
  {
	  return 0; // invalid length, abort.
  }
 
  sendCommand(COMMAND_OPCODE_WRITE, zone, address, data, length_of_data);

  delay(26); // time for IC to process command and exectute
  
  // Now let's read back from the IC and see if it reports back good things.
  countGlobal = 0; 
  if (receiveResponseData(4) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  idleMode();
  if (checkCount() == false)
  {		
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  if (checkCrc() == false)
	{		
		setStatus(STATUS_CRC_ERROR);
		return false;
	}
	setStatus(inputBuffer[1]);
  if (inputBuffer[1] == 0x00) 
		return true;   // If we hear a "0x00", that means it had a successful write
  else 
		return false;
}


boolean ATECCX08A::readSlot(int slot, uint8_t *data, int length)
{
  if (slot < 0 || slot > 15) 
	{
    return false;
  }

  if (length % 4 != 0) 
	{
    return false;
  }

  int chunkSize = 32;

  for (int i = 0; i < length; i += chunkSize) 
	{
    if ((length - i) < 32) 
		{
      chunkSize = 4;
    }

    if (!read(ZONE_DATA, addressForSlotOffset(slot, i), &data[i], chunkSize)) 
		{
      return false;
    }
  }

  return true;
}

boolean ATECCX08A::writeSlot(int slot, const uint8_t *data, int length)
{
  if (slot < 0 || slot > 15) 
	{
    return false;
  }

  if (length % 4 != 0) 
	{
    return false;
  }

  int chunkSize = 32;

  for (int i = 0; i < length; i += chunkSize) 
	{
    if ((length - i) < 32) 
		{
      chunkSize = 4;
    }

    if (!write(ZONE_DATA, addressForSlotOffset(slot, i), &data[i], chunkSize)) 
		{
      return false;
    }
  }

  return true;
}

int ATECCX08A::addressForSlotOffset(int slot, int offset)
{
  int block = offset / 32;
  offset = (offset % 32) / 4;  

  return (slot << 3) | (block << 8) | (offset);
}


/*
boolean ATECCX08A::writeKey(uint16_t slot, uint8_t *data, uint8_t length)
{
	if (slot < 0 || slot > 15)
		return false;
	if (length != 32)
		return false;
	return write(ZONE_DATA, slot, data, length);
}
*/

/** \brief

	createSignature(uint8_t *data, uint16_t slot)

    Creates a 64-byte ECC signature on 32 bytes of data.
	Defautes to use private key located in slot 0.
	Your signature will be available at global variable signature[].
	
	Note, the IC actually needs you to store your data in a temporary memory location
	called TempKey. This function first loads TempKey, and then signs TempKey. Then it 
	receives the signature and copies it to signature[].
*/

boolean ATECCX08A::createSignature(uint8_t *data, uint16_t slot)
{
  boolean loadTempKeyResult = loadTempKey(data);
  boolean signTempKeyResult = signTempKey(slot);
  if (loadTempKeyResult && signTempKeyResult) 
		return true;
  else 
		return false;
}

/** \brief

	loadTempKey(uint8_t *data)

	Writes 32 bytes of data to memory location "TempKey" on the IC.
	Note, the data is provided externally by you, the user, and is included in the
	command NONCE.

    We will use the NONCE command in passthrough mode to load tempKey with our data (aka message).
    Note, the datasheet warns that this does not provide protection agains replay attacks,
    but we will protect again this because our server (Bob) is going to send us it's own unique random TOKEN,
    when it requests data, and this will allow us to create a unique data + signature for every communication.
*/

boolean ATECCX08A::loadTempKey(uint8_t *data)
{
  sendCommand(COMMAND_OPCODE_NONCE, NONCE_MODE_PASSTHROUGH, 0x0000, data, 32);
  
  // note, param2 is 0x0000 (and param1 is PASSTHROUGH), so OutData will be just a single byte of zero upon completion.
  // see ds pg 77 for more info

  delay(7); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if (receiveResponseData(4) == false) 
	{
  	setStatus(STATUS_EXECUTION_ERROR);
		return false; // responds with "0x00" if NONCE executed properly
	}
  idleMode();
  boolean checkCountResult = checkCount();
  boolean checkCrcResult = checkCrc();
  
  if ( (checkCountResult == false) || (checkCrcResult == false) ) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}

	setStatus(inputBuffer[1]);
   if (inputBuffer[1] == 0x00) 
		 return true;   // If we hear a "0x00", that means it had a successful nonce
  else 
		return false;
}

/** \brief

	signTempKey(uint16_t slot)

	Create a 64 byte ECC signature for the contents of TempKey using the private key in Slot.
	Default slot is 0.
	
	The response from this command (the signature) is stored in global varaible signature[].
*/

boolean ATECCX08A::signTempKey(uint16_t slot)
{
  sendCommand(COMMAND_OPCODE_SIGN, SIGN_MODE_TEMPKEY, slot);

  delay(60); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if (receiveResponseData(64 + 2 + 1) == false) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
  	return false; // signature (64), plus crc (2), plus count (1)
	}
  idleMode();
  boolean checkCountResult = checkCount();
  boolean checkCrcResult = checkCrc();
  
  // update signature[] array and print it to serial terminal nicely formatted for easy copy/pasting between sketches
  if (checkCountResult && checkCrcResult) // check that it was a good message
  {  
    // we don't need the count value (which is currently the first byte of the inputBuffer)
    for (int i = 0 ; i < 64 ; i++) // for loop through to grab all but the first position (which is "count" of the message)
    {
      signature[i] = inputBuffer[i + 1];
    }
  
	  _debugSerial->println();
    _debugSerial->println("uint8_t signature[64] = {");
    for (int i = 0; i < sizeof(signature) ; i++)
    {
	    _debugSerial->print("0x");
	    if ((signature[i] >> 4) == 0) _debugSerial->print("0"); // print preceeding high nibble if it's zero
        _debugSerial->print(signature[i], HEX);
      if (i != 63) 
				_debugSerial->print(", ");
	    if ((63-i) % 16 == 0) 
				_debugSerial->println();
    }
	  _debugSerial->println("};");
		setStatus(STATUS_SUCCESS);
	  return true;
  }
  else 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
}

/** \brief

	verifySignature(uint8_t *message, uint8_t *signature, uint8_t *publicKey)
	
	Verifies a ECC signature using the message, signature and external public key.
	Returns true if successful.
	
	Note, it acutally uses loadTempKey, then uses the verify command in "external public key" mode.
*/

boolean ATECCX08A::verifySignature(uint8_t *message, uint8_t *signature, uint8_t *publicKey)
{
  // first, let's load the message into TempKey on the device, this uses NONCE command in passthrough mode.
  boolean loadTempKeyResult = loadTempKey(message);
  if (loadTempKeyResult == false) 
  {
    _debugSerial->println("Load TempKey Failure");
    return false;
  }

  // We can only send one *single* data array to sendCommand as Param2, so we need to combine signature and public key.
  uint8_t data_sigAndPub[128]; 
  memcpy(&data_sigAndPub[0], &signature[0], 64);	// append signature
  memcpy(&data_sigAndPub[64], &publicKey[0], 64);	// append external public key
  
  sendCommand(COMMAND_OPCODE_VERIFY, VERIFY_MODE_EXTERNAL, VERIFY_PARAM2_KEYTYPE_ECC, data_sigAndPub, sizeof(data_sigAndPub));
	

  delay(58); // time for IC to process command and exectute

  // Now let's read back from the IC.
  
  if (receiveResponseData(4, false) == false) 
	{
    _debugSerial->println("receiveResponseData(4) Failure");
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  idleMode();
  boolean checkCountResult = checkCount(false);
  boolean checkCrcResult = checkCrc(false);
  
  if ((checkCountResult == false) || (checkCrcResult == false)) 
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
  
	setStatus(inputBuffer[1]);
  if (inputBuffer[1] == 0x00) 
		return true;   // If we hear a "0x00", that means it had a successful verify
  else 
		return false;
}

/** \brief

	writeConfigSparkFun()
	
	Writes the necessary configuration settings to the IC in order to work with the SparkFun Arduino Library examples.
	For key slots 0 and 1, this enables ECC private key pairs,public key generation, and external signature verifications.
	
	Returns true if write commands were successful.
*/

boolean ATECCX08A::writeConfigSparkFun()
{
  // keep track of our write command results.
  boolean result1;
  boolean result2;
  boolean result3;
  boolean result4;

  // set keytype on slots 0,1, 2 and 3 to 0x3300
  // Lockable, ECC, PuInfo set (public key always allowed to be generated), contains a private Key
  uint8_t data1[] = {0x33, 0x00, 0x33, 0x00}; // 0x3300 sets the keyconfig.keyType, see datasheet pg 20
  result1 = write(ZONE_CONFIG, (96 / 4), data1, 4);  // slot 0 and slot 1
  result2 = write(ZONE_CONFIG, (100 / 4), data1, 4); // slot 2 and slot 3
	
  // set slot config on slots 0, 1, 2 and 3 to 0x8320
  // EXT signatures, INT signatures, IsSecret, Write config never
  uint8_t data2[] = {0x83, 0x20, 0x83, 0x20}; // for slot config bit definitions see datasheet pg 20
  result3 = write(ZONE_CONFIG, (20 / 4), data2, 4);  // slot 0 and slot 1
  result4 = write(ZONE_CONFIG, (24 / 4), data2, 4);  // slot 2 and slot 3

  return (result1 && result2 && result3 && result4);
}

/** \brief

	sendCommand(uint8_t command_opcode, uint8_t param1, uint16_t param2, uint8_t *data, size_t length_of_data)
	
	Generic function for sending commands to the IC. 
	
	This function handles creating the "total transmission" to the IC.
	This contains WORD_ADDRESS_VALUE, COUNT, OPCODE, PARAM1, PARAM2, DATA (optional), and CRCs.
	
	Note, it always calls the "wake()" function, assuming that you have let the IC fall asleep (default 1.7 sec)
	
	Note, for anything other than a command (reset, sleep and idle), you need a different "Word Address Value",
	So those specific transmissions are handled in unique functions.
*/

boolean ATECCX08A::sendCommand(uint8_t command_opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t length_of_data, boolean debug)
{
  // build packet array (total_transmission) to send a communication to IC, with opcode COMMAND
  // It expects to see: word address, count, command opcode, param1, param2, data (optional), CRC[0], CRC[1]
  
  uint8_t total_transmission_length;
  total_transmission_length = (1 +1 +1 +1 +2 +length_of_data +2); 
  // word address val (1) + count (1) + command opcode (1) param1 (1) + param2 (2) data (0-?) + crc (2)

  uint8_t total_transmission[total_transmission_length];
  total_transmission[0] = WORD_ADDRESS_VALUE_COMMAND; 		// word address value (type command)
  total_transmission[1] = total_transmission_length-1; 		// count, does not include itself, so "-1"
  total_transmission[2] = command_opcode; 			// command
  total_transmission[3] = param1;							// param1
  memcpy(&total_transmission[4], &param2, sizeof(param2));	// append param2 
  memcpy(&total_transmission[6], &data[0], length_of_data);	// append data
  
  // update CRCs
  uint8_t packet_to_CRC[total_transmission_length-3]; // minus word address (1) and crc (2).
  memcpy(&packet_to_CRC[0], &total_transmission[1], (total_transmission_length-3)); // copy over just what we need to CRC starting at index 1
  
	if (debug == true)
	{
    _debugSerial->println("packet_to_CRC: ");
    for (int i = 0; i < sizeof(packet_to_CRC) ; i++)
    {
      _debugSerial->print(packet_to_CRC[i], HEX);
      _debugSerial->print(",");
    }
    _debugSerial->println();
	}
  
  atca_calculate_crc((total_transmission_length-3), packet_to_CRC); // count includes crc[0] and crc[1], so we must subtract 2 before creating crc
	if (debug == true)
	{
    _debugSerial->println(crc[0], HEX);
    _debugSerial->println(crc[1], HEX);
	}

  memcpy(&total_transmission[total_transmission_length-2], &crc[0], 2);  // append crcs

  wakeUp();
  
  _i2cPort->beginTransmission(_i2caddr);
  _i2cPort->write(total_transmission, total_transmission_length); 
  _i2cPort->endTransmission();
  
  return true;
}



boolean ATECCX08A::sha256(uint8_t * plain, size_t len, uint8_t * hash)
{
	int i;
	size_t chunks = len / SHA_BLOCK_SIZE + !!(len % SHA_BLOCK_SIZE);
  if ((len % SHA_BLOCK_SIZE) == 0) chunks += 1; // END command can only accept up to 63 bytes, so we must add a "blank chunk" for the end command

  // Serial.print("chunks:");
  // Serial.println(chunks);

	if (!sendCommand(COMMAND_OPCODE_SHA, SHA_START, 0, NULL, 0, true))
		return false;

	/* Divide into blocks of 64 bytes per chunk */
	for (i = 0; i < chunks; ++i)
	{
		size_t data_size = SHA_BLOCK_SIZE;
		uint8_t chunk[SHA_BLOCK_SIZE];

		delay(9);

		if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SIGNAL_SIZE + CRC_SIZE))
		{
			setStatus(STATUS_EXECUTION_ERROR);
			return false;
		}

		idleMode();

		if (!checkCount() || !checkCrc())
		{
			setStatus(STATUS_EXECUTION_ERROR);
			return false;
		}

		// If we hear a "0x00", that means it had a successful load
		if (inputBuffer[RESPONSE_SIGNAL_INDEX] != ATRCC508A_SUCCESSFUL_SHA)
		{
			setStatus(inputBuffer[RESPONSE_SIGNAL_INDEX]);
			return false;
		}

		if (i + 1 == chunks) // if we're on the last chunk, there will be a remainder or 0 (and 0 is okay for an end command)
			data_size = len % SHA_BLOCK_SIZE;

    // Serial.print("chunk:");
    // Serial.println(i);
    // Serial.print("data_size:");
    // Serial.println(data_size);
    // Serial.print("update vs end:");
    // Serial.println((i + 1 != chunks) ? SHA_UPDATE : SHA_END);

		/* Send next */
		if (!sendCommand(COMMAND_OPCODE_SHA, (i + 1 != chunks) ? SHA_UPDATE : SHA_END, data_size, plain + i * SHA_BLOCK_SIZE, data_size, true))
			return false;
	}

	/* Read digest */
	delay(9);

	if (!receiveResponseData(RESPONSE_COUNT_SIZE + RESPONSE_SHA_SIZE + CRC_SIZE))
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}

	idleMode();

	if (!checkCount() || !checkCrc())
	{
		setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}

	/* Copy digest */
	for (i = 0; i < SHA256_SIZE; ++i)
	{
		hash[i] = inputBuffer[RESPONSE_SHA_INDEX + i];
	}

	setStatus(STATUS_SUCCESS);
	return true;
}


boolean ATECCX08A::encryptDecryptBlock(uint8_t *input, int inputSize, uint8_t *output, int outputSize, uint8_t slot, uint8_t keyIndex, uint8_t mode, boolean debug)
{
	int size;

	if (input == NULL || inputSize != AES_BLOCKSIZE || output == NULL || outputSize != AES_BLOCKSIZE)
	{
		setStatus(STATUS_INVALID_PARAMETER);
		return false;
	}
	
	if (slot < 0 || slot > 15)
	{
		setStatus(STATUS_INVALID_PARAMETER);
		return false;
	}
	
	if (keyIndex < 0 || keyIndex > 3)
	{
		setStatus(STATUS_INVALID_PARAMETER);
		return false;
	}

	mode |= (keyIndex << 6);
	if (debug)
	{
		if ((mode & AES_ENCRYPT) == AES_ENCRYPT)
		  _debugSerial->println("Encryption:");
		else
		  _debugSerial->println("Decryption:");
/*		
		_debugSerial->println("size cleartext : " + String(sizeClearText));
		_debugSerial->println("Slot           : " + String(slot));
		_debugSerial->println("KeyIndex       : " + String(keyIndex));
		_debugSerial->println("Mode           : " + String(mode));
		_debugSerial->print("Input data     : " );
		for (int index = 0; index < AES_BLOCKSIZE; index++)
		{
			printHexValue(clearText[index]);
		}
		_debugSerial->println();
*/		
	}
	
  sendCommand(COMMAND_OPCODE_AES, mode, slot, input, inputSize, false);

  delay(10); // time for IC to process command and execute

  // Now let's read the response 
	size = 1 + AES_BLOCKSIZE + 2;  // length byte, encrypted data (16 bytes), crc (2 bytes)
  if (receiveResponseData(size, false) == false)
	{ 
    _debugSerial->println("receiveResponseData return false");
		setStatus(STATUS_EXECUTION_ERROR);
    return false; // encrypted data (16), plus crc (2), plus count (1)
	}
  idleMode();
  boolean checkCountResult = checkCount(false);
  boolean checkCrcResult = checkCrc(false);
  
  if (checkCountResult && checkCrcResult) // check that it was a good message
  {  
    // ignore first byte (length byte), so we start from offset 1
		memcpy(output, &inputBuffer[1], AES_BLOCKSIZE);
		if (debug)
		{
			_debugSerial->println("output data:");
			_debugSerial->println();
			printHexValue(output, AES_BLOCKSIZE, ", ");
		}
  	setStatus(STATUS_SUCCESS);
  	return true;
  }
  else 
	{
     _debugSerial->println("checkCount() or checkCrc() returned false");
  	setStatus(STATUS_EXECUTION_ERROR);
		return false;
	}
}




byte * ATECCX08A::getPublicKey()
{
	return publicKey64Bytes;
}


boolean ATECCX08A::getSlotLockStatus(uint16_t slot)
{
	byte configByte;
	int  bitPosition;
	bool result;
	
	if (slot > 16)
	{
		return false;
	}
	
	if (slot < 8)
	{
	  configByte = configZone[CONFIG_ZONE_SLOTS_LOCK0];
		bitPosition = slot;
	}
	else
	{
	  configByte = configZone[CONFIG_ZONE_SLOTS_LOCK1];
		bitPosition = slot - 8;
	}
	
	if ((configByte & (1 << bitPosition)) == 0)
	{
		result = true;
	}
	else
	{
		result = false;
	}
	return result;
}


boolean ATECCX08A::getConfigLockStatus()
{
  return configLockStatus;	
}

boolean ATECCX08A::getDataOTPLockStatus()
{
  return dataOTPLockStatus;	
}

byte   * ATECCX08A::getConfigZone()
{
	return configZone;
}

boolean ATECCX08A::isAESEnabled()
{
	return configZone[CONFIG_ZONE_AES_STATUS];
}


boolean ATECCX08A::getSerialNumber(uint8_t *serialNo, int length)
{
	if (length < sizeof(serialNumber))
		return false;
	else
	{
		memcpy(serialNo, serialNumber, SERIAL_NUMBER_SIZE);
		return true;
	}
}

boolean ATECCX08A::getRevisionNumber(uint8_t *revisionNo, int length)
{
	if (length < sizeof(revisionNumber))
		return false;
	else
	{
		memcpy(revisionNo, revisionNumber, REVISION_NUMBER_SIZE);
		return true;
	}
}


boolean ATECCX08A::getSignature(uint8_t *signature, int length)
{
	if (length < SIGNATURE_SIZE)
		return false;
	else
	{
		memcpy(signature, this->signature, SIGNATURE_SIZE);
		return true;
	}
}

int ATECCX08A::getStatus()
{
	return status;
}
		
	
void ATECCX08A::setStatus(int status)
{
	this->status = status;
}


void ATECCX08A::printHexValue(byte value)
{
	if ((value >> 4) == 0) 
		_debugSerial->print("0"); // print preceeding high nibble if it's zero
	_debugSerial->print(value, HEX);
}

void ATECCX08A::printHexValue(byte value[], int length, char *separator)
{
	for (int index = 0; index < length; index++)
	{
		printHexValue(value[index]);
	 _debugSerial->print(String(separator));
	}
	_debugSerial->println();
}

