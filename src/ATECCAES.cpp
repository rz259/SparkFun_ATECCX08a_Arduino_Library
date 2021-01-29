#pragma once

#include "ATECCAES.h"


ATECCAES::ATECCAES(const ATECCX08A *atecc, PaddingType padding)
{
	this->atecc = (ATECCX08A *) atecc;
	this->padding = padding;
}


PaddingType ATECCAES::getPadding()
{
	return padding;
}

int ATECCAES::getStatus()
{
	return status;
}

void  ATECCAES::setStatus(int status)
{
	this->status = status;
}



void ATECCAES::printHexValue(byte value)
{
	if ((value >> 4) == 0) 
		Serial.print("0"); // print preceeding high nibble if it's zero
	Serial.print(value, HEX);
}

void ATECCAES::printHexValue(const uint8_t *value, int length, const char *separator)
{
	for (int index = 0; index < length; index++)
	{
		printHexValue(value[index]);
	  Serial.print(String(separator));
		if ((index + 1) % 16 == 0)
		  Serial.println();
	}
	Serial.println();
}


int ATECCAES::calcSizeNeeded(int length)
{
	PaddingType padding;
	int size = 0;
	
	padding = getPadding();
	if (padding == NoPadding)
	{
		size = length;
	}
	else if (padding = PKCS7Padding)
	{
		int noBlocks = (length / AES_BLOCKSIZE) + 1;
		size = noBlocks * AES_BLOCKSIZE;
	}
	return size;
}

void ATECCAES::appendPadding(uint8_t *data, int sizePlainText, int totalSize)
{
	if (padding == PKCS7Padding)
	{
		int paddingByte = totalSize - sizePlainText;
		memset(&data[sizePlainText], paddingByte, paddingByte);
	}
}

boolean ATECCAES::removePadding(uint8_t *decryptBuffer, int &bytesDecrypted)
{
	PaddingType  padding;
	int          offset;
	
	padding = getPadding();
	if (padding == PKCS7Padding)
	{
		// the last byte of the buffer contains the padding byte value. The value must be in range 1 - 0x10
		uint8_t paddingByte = decryptBuffer[bytesDecrypted-1];
		
		if (paddingByte == 0x00 || paddingByte > 0x10)
		{
			setStatus(ATECCAES_PADDING_ERROR);
			return false;
		}
		// now check, if the correct number of padding bytes present
		offset = bytesDecrypted - paddingByte;
		for (int index = offset; index < bytesDecrypted; index++)
		{
			if (decryptBuffer[index] != paddingByte)
			{
				setStatus(ATECCAES_PADDING_ERROR);
				return false;
			}
		}
    bytesDecrypted = offset;
	}
	return true;
}

boolean ATECCAES::performChecksForEncryption(int sizePlainText, int sizeEncrypted)
{
	PaddingType  padding;
	int          totalSize;
	
	padding = getPadding();
	if (padding == NoPadding)
	{		
    if ((sizePlainText % AES_BLOCKSIZE) != 0)
		{
			setStatus(ATECCAES_INVALID_INPUT_LENGTH);   // size must be a multiple of AES_BLOCKSIZE
			return false;
	  }	
	}
	totalSize = calcSizeNeeded(sizePlainText);
	if (sizeEncrypted < totalSize)                  // size provided for encrypted data must at least the size calculated
	{
		setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
		return false;
	}
	return true;
}

boolean ATECCAES::performChecksForDecryption(int sizeEncrypted, int sizeDecrypted)
{
	if ((sizeEncrypted % AES_BLOCKSIZE) != 0)
	{
		setStatus(ATECCAES_INVALID_INPUT_LENGTH);
		return false;
  }	
	if (sizeDecrypted < sizeEncrypted)
	{
		setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
		return false;
	}
  return true;
}


uint8_t * ATECCAES::initInputBuffer(const uint8_t *plainText, int sizePlainText, int &totalSize)
{
	uint8_t *inputBuffer;
	
	totalSize = calcSizeNeeded(sizePlainText);
	inputBuffer = (uint8_t *) calloc(1, totalSize);
	memcpy(inputBuffer, plainText, sizePlainText);
  appendPadding(inputBuffer, sizePlainText, totalSize);   // if necessary, append padding
	return inputBuffer;
}

ATECCX08A * ATECCAES::getCryptoAdapter()
{
	return atecc;
}


ATECCAES_ECB::ATECCAES_ECB(ATECCX08A *atecc, PaddingType padding) : ATECCAES(atecc, padding)
{
}


boolean ATECCAES_ECB::encrypt(const uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     blocks, offset, totalSize, bytesEncrypted = 0;
  uint8_t *inputBuffer = NULL;
	
	result = performChecksForEncryption(sizePlainText, sizeEncrypted);
	if (result == false)
	{
		return result;
	}

  inputBuffer = initInputBuffer(plainText, sizePlainText, totalSize);
	blocks = totalSize / AES_BLOCKSIZE;
	for (int index = 0; index < blocks; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = getCryptoAdapter()->encryptDecryptBlock(&inputBuffer[offset], AES_BLOCKSIZE, &encrypted[offset], AES_BLOCKSIZE, slot, keyIndex, AES_ENCRYPT, debug);
		if (result == false)
		{
			free(inputBuffer);
			return result;
		}
		bytesEncrypted += AES_BLOCKSIZE;
	}
	setStatus(ATECCAES_SUCCESS);
	sizeEncrypted = bytesEncrypted;
	free(inputBuffer);
	return true;
}


boolean ATECCAES_ECB::decrypt(const uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     blocks, offset;
	int bytesDecrypted = 0;
	uint8_t   *decryptBuffer;
		
	result = performChecksForDecryption(sizeEncrypted, sizeDecrypted);
  if (result == false)	
	{
		return result;
	}
	
	blocks = (sizeEncrypted / AES_BLOCKSIZE);
	decryptBuffer = (uint8_t *) calloc(1, sizeEncrypted);
	for (int index = 0; index < blocks; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = getCryptoAdapter()->encryptDecryptBlock(&encrypted[offset], AES_BLOCKSIZE, &decryptBuffer[offset], AES_BLOCKSIZE, slot, keyIndex, AES_DECRYPT, debug);
		if (result == false)
		{
			free(decryptBuffer);
			return result;
		}
		bytesDecrypted += AES_BLOCKSIZE;
	}
	result = removePadding(decryptBuffer, bytesDecrypted);
	if (result == false)
	{
		return result;
	}
	
  // now copy the decryptBuffer to the parameter decrypted in the correct length
  memcpy(decrypted, decryptBuffer, bytesDecrypted);
  sizeDecrypted = bytesDecrypted;
	free(decryptBuffer);
	setStatus(ATECCAES_SUCCESS);
	return true;
}


ATECCAES_CBC::ATECCAES_CBC(ATECCX08A *atecc, PaddingType padding, uint8_t *iv) : ATECCAES(atecc, padding)
{
	setIV(iv);
}


void ATECCAES_CBC::setIV(const uint8_t *iv)
{
	memcpy(this->iv, iv, AES_BLOCKSIZE);
}


boolean ATECCAES_CBC::encrypt(const uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     blocks, offset, totalSize, bytesEncrypted = 0;
  uint8_t *inputBuffer = NULL;
	uint8_t ivBlock[AES_BLOCKSIZE];
	
	result = performChecksForEncryption(sizePlainText, sizeEncrypted);
	if (result == false)
	{
		return result;
	}

  inputBuffer = initInputBuffer(plainText, sizePlainText, totalSize);
	blocks = totalSize / AES_BLOCKSIZE;
	memcpy(ivBlock, iv, AES_BLOCKSIZE);
	
	for (int index = 0; index < blocks; index++)
	{
		offset = index * AES_BLOCKSIZE;
		xorBlock(&inputBuffer[offset], ivBlock, AES_BLOCKSIZE);
	  result = getCryptoAdapter()->encryptDecryptBlock(&inputBuffer[offset], AES_BLOCKSIZE, &encrypted[offset], AES_BLOCKSIZE, slot, keyIndex, AES_ENCRYPT, debug);
		if (debug == true)
		{
		  printHexValue(encrypted, totalSize, " ");
		  Serial.println();
		}
		if (result == false)
		{
			free(inputBuffer);
			return result;
		}
		bytesEncrypted += AES_BLOCKSIZE;
    memcpy(ivBlock, &encrypted[offset], AES_BLOCKSIZE);
  }
	setStatus(ATECCAES_SUCCESS);
	sizeEncrypted = bytesEncrypted;
	free(inputBuffer);
	return true;
}



boolean ATECCAES_CBC::decrypt(const uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     blocks, offset;
	int bytesDecrypted = 0;
	uint8_t   *decryptBuffer;
	uint8_t   ivBlock[AES_BLOCKSIZE];
		
	result = performChecksForDecryption(sizeEncrypted, sizeDecrypted);
  if (result == false)	
	{
		return result;
	}
	
	blocks = (sizeEncrypted / AES_BLOCKSIZE);
	decryptBuffer = (uint8_t *) calloc(1, sizeEncrypted);
	memcpy(ivBlock, iv, AES_BLOCKSIZE);
	for (int index = 0; index < blocks; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = getCryptoAdapter()->encryptDecryptBlock(&encrypted[offset], AES_BLOCKSIZE, &decryptBuffer[offset], AES_BLOCKSIZE, slot, keyIndex, AES_DECRYPT, debug);
		if (result == false)
		{
			free(decryptBuffer);
			return result;
		}
		bytesDecrypted += AES_BLOCKSIZE;
		xorBlock(&decryptBuffer[offset], ivBlock, AES_BLOCKSIZE);
  	memcpy(ivBlock, &encrypted[offset], AES_BLOCKSIZE);
	}
	result = removePadding(decryptBuffer, bytesDecrypted);
	if (result == false)
	{
		return result;
	}
	
  // now copy the decryptBuffer to the parameter decrypted in the correct length
  memcpy(decrypted, decryptBuffer, bytesDecrypted);
  sizeDecrypted = bytesDecrypted;
	free(decryptBuffer);
	setStatus(ATECCAES_SUCCESS);
	return true;
}

void  ATECCAES_CBC::xorBlock(uint8_t *data, const uint8_t *ivBlock, int size)
{
	for (int index = 0; index < size; index++)
	{
		data[index] ^= ivBlock[index];
	}
}

