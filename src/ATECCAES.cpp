#pragma once

#include "ATECCAES.h"


ATECCAES::ATECCAES(ATECCX08A *atecc, PaddingType padding)
{
	this->atecc = atecc;
	this->padding = padding;
}

boolean ATECCAES::encrypt(uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     iterations, offset, totalSize, bytesEncrypted = 0;
  uint8_t *inputBuffer = NULL;
	
	result = performChecksForEncryption(sizePlainText, sizeEncrypted);
	if (result == false)
	{
		return result;
	}

  inputBuffer = initInputBuffer(plainText, sizePlainText, totalSize);
	iterations = totalSize / AES_BLOCKSIZE;
	for (int index = 0; index < iterations; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = atecc->encryptDecryptBlock(&inputBuffer[offset], AES_BLOCKSIZE, &encrypted[offset], AES_BLOCKSIZE, slot, keyIndex, AES_ENCRYPT, debug);
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

boolean ATECCAES::decrypt(uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     iterations, offset;
	PaddingType  padding;
	
	padding = getPadding();
	if (padding == NoPadding)
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
	}
	else if (padding == PKCS7Padding)
	{
    if ((sizeEncrypted % AES_BLOCKSIZE) != 0)		// last block is not fully used
		{
			if (sizeDecrypted < sizeEncrypted)  // outputSize must be at least as long als inputSize
			{
				setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
				return false;
			}
		}
		else 
		{
			// we need to append an additional block for padding
			if ((sizeEncrypted - AES_BLOCKSIZE) > sizeDecrypted)
			{
				setStatus(ATECCAES_INPUT_LENGTH_TOO_SMALL);
				return false;
			}
		}
	}
	
	int bytesDecrypted = 0;
	iterations = (sizeEncrypted / AES_BLOCKSIZE);

	uint8_t decryptBuffer[sizeEncrypted];
	for (int index = 0; index < iterations; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = atecc->encryptDecryptBlock(&encrypted[offset], AES_BLOCKSIZE, &decryptBuffer[offset], AES_BLOCKSIZE, slot, keyIndex, AES_DECRYPT, debug);
		if (result == false)
			return result;
		bytesDecrypted += AES_BLOCKSIZE;
	}

//  Serial.print("decryptBuffer: ");
//  printHexValue(decryptBuffer, bytesDecrypted, " ");	

	// handle padding
	if (padding == PKCS7Padding)
	{
		// the last byte of the buffer contains the padding byte value. The value must be in range 1 - 0x10
		uint8_t paddingByte = decryptBuffer[sizeEncrypted-1];
		
		if (paddingByte == 0x00 || paddingByte > 0x10)
		{
			setStatus(ATECCAES_PADDING_ERROR);
			return false;
		}
		// now check, if there are the correct number of padding bytes present
		offset = sizeEncrypted - paddingByte;
		for (int index = offset; index < sizeEncrypted; index++)
		{
//			Serial.println("Pos: " + String(index) + ", Value : " + String(decryptBuffer[index]));
			if (decryptBuffer[index] != paddingByte)
			{
				setStatus(ATECCAES_PADDING_ERROR);
				return false;
			}
		}

    // now copy the decryptBuffer to the parameter decrypted in the correct length
		memcpy(decrypted, decryptBuffer, offset);
  	sizeDecrypted = offset;
	}
	else
	{
		memcpy(decrypted, decryptBuffer, bytesDecrypted);
  	sizeDecrypted = bytesDecrypted;
	}
	setStatus(ATECCAES_SUCCESS);
	return true;
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

void ATECCAES::printHexValue(byte value[], int length, char *separator)
{
	for (int index = 0; index < length; index++)
	{
		printHexValue(value[index]);
	 Serial.print(String(separator));
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

uint8_t * ATECCAES::initInputBuffer(uint8_t *plainText, int sizePlainText, int &totalSize)
{
	uint8_t *inputBuffer;
	
	totalSize = calcSizeNeeded(sizePlainText);
	inputBuffer = (uint8_t *) calloc(1, totalSize);
	memcpy(inputBuffer, plainText, sizePlainText);
  appendPadding(inputBuffer, sizePlainText, totalSize);   // if necessary, append padding
	return inputBuffer;
}
