#pragma once

#include "ATECCAES.h"


ATECCAES::ATECCAES(ATECCX08A *atecc, PaddingType padding)
{
	this->atecc = atecc;
	this->padding = padding;
}

boolean ATECCAES::encrypt(uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int *sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	boolean result;
	int     iterations, offset;
	PaddingType  padding;
	uint8_t *inputBuffer = NULL;
	
	padding = getPadding();
	if (padding == NoPadding)
	{		
    if ((sizePlainText % AES_BLOCKSIZE) != 0)
		{
			setStatus(ATECCAES_INVALID_INPUT_LENGTH);
			return false;
	  }	
		if (*sizeEncrypted < sizePlainText)
		{
			setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
			return false;
		}
	}
	else if (padding == PKCS7Padding)
	{
    if ((sizePlainText % AES_BLOCKSIZE) != 0)		// last block is not fully used
		{
			if (*sizeEncrypted < sizePlainText)  // outputSize must be at least as long als inputSize
			{
				setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
				return false;
			}
			int sizeNeeded = ((sizePlainText / AES_BLOCKSIZE) + 1) * AES_BLOCKSIZE;
			inputBuffer = (uint8_t *) calloc(1, sizeNeeded);
			memcpy(inputBuffer, plainText, sizePlainText);
			// now append padding bytes
			int paddingBytes = sizeNeeded - sizePlainText;
			Serial.println("paddingBytes : " + String(paddingBytes));
			memset(&inputBuffer[sizePlainText], paddingBytes, paddingBytes);
			printHexValue(inputBuffer, sizeNeeded, " ");
		}
		else 
		{
			Serial.println("Mode : AES_ENCYPT, inputSize :" + String(sizePlainText) + ", outputSize : " + String(*sizeEncrypted));
			// we need to append an additional block for padding
			if (*sizeEncrypted < (sizePlainText + AES_BLOCKSIZE))
			{
				setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
				return false;
			}
			inputBuffer = plainText;
		}
	}
	
	int bytesEncrypted = 0;
	iterations = sizePlainText / AES_BLOCKSIZE;
	if ((sizePlainText % AES_BLOCKSIZE) != 0)
	{
	  iterations++;	
	}
	
	for (int index = 0; index < iterations; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = atecc->encryptDecryptBlock(&inputBuffer[offset], AES_BLOCKSIZE, &encrypted[offset], AES_BLOCKSIZE, slot, keyIndex, AES_ENCRYPT, debug);
		if (result == false)
		{
			if (inputBuffer != NULL && inputBuffer != plainText)
			{
				free(inputBuffer);
			}	
			return result;
		}
		bytesEncrypted += AES_BLOCKSIZE;
	}
	// handle special case we need to append a block
	if (padding == PKCS7Padding && (sizePlainText % AES_BLOCKSIZE) == 0)
	{
		uint8_t padding[AES_BLOCKSIZE];
		
		Serial.println("Appending block for padding");
		memset(padding, 0x10, 16);
		offset += AES_BLOCKSIZE;
		result = atecc->encryptDecryptBlock(padding, AES_BLOCKSIZE, &encrypted[offset], AES_BLOCKSIZE, slot, keyIndex, AES_ENCRYPT, debug);
		if (result == false)
		{
			if (inputBuffer != NULL && inputBuffer != plainText)
			{
				free(inputBuffer);
			}	
			return result;
		}

 		bytesEncrypted += AES_BLOCKSIZE;
	}
	setStatus(ATECCAES_SUCCESS);
	*sizeEncrypted = bytesEncrypted;
	if (inputBuffer != NULL && inputBuffer != plainText)
	{
		free(inputBuffer);
	}	
	return true;
}

boolean ATECCAES::decrypt(uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int *sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
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
		if (*sizeDecrypted < sizeEncrypted)
		{
			setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
			return false;
		}
	}
	else if (padding == PKCS7Padding)
	{
    if ((sizeEncrypted % AES_BLOCKSIZE) != 0)		// last block is not fully used
		{
			if (*sizeDecrypted < sizeEncrypted)  // outputSize must be at least as long als inputSize
			{
				setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
				return false;
			}
		}
		else 
		{
			// we need to append an additional block for padding
			if ((sizeEncrypted - AES_BLOCKSIZE) > *sizeDecrypted)
			{
				setStatus(ATECCAES_INPUT_LENGTH_TOO_SMALL);
				return false;
			}
		}
	}
	
	int bytesDecrypted = 0;
	iterations = (sizeEncrypted / AES_BLOCKSIZE) - 1;
  if ((sizeEncrypted % AES_BLOCKSIZE) != 0)
	{
	  iterations++;	
	}

	for (int index = 0; index < iterations; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = atecc->encryptDecryptBlock(&encrypted[offset], AES_BLOCKSIZE, &decrypted[offset], AES_BLOCKSIZE, slot, keyIndex, AES_DECRYPT, debug);
		if (result == false)
			return result;
		bytesDecrypted += AES_BLOCKSIZE;
	}
	
	// handle padding
	if (padding == PKCS7Padding)
	{
		if ((sizeEncrypted % AES_BLOCKSIZE) == 0)
		{
			// we need to remove the last block completely
			uint8_t padding[AES_BLOCKSIZE];
			
			// First decrypt last block
			offset += AES_BLOCKSIZE;
			result = atecc->encryptDecryptBlock(&encrypted[offset], AES_BLOCKSIZE, padding, AES_BLOCKSIZE, slot, keyIndex, AES_DECRYPT, debug);
			Serial.println("Result endryptDecryptBlock padding: " + result);
			
			// verify if last input block only contains 0x10
			for (int index = 0; index < AES_BLOCKSIZE; index++)
			{
				Serial.println("Pos: " + String(index+offset) + ", Value : " + String(padding[index]));
				if (padding[index] != 0x10)
				{
					setStatus(ATECCAES_PADDING_ERROR);
					return false;
				}
			}
		}
		else
		{
			// delete padding bytes from last block
			uint8_t paddingValue;
			
			paddingValue = decrypted[bytesDecrypted-1];
			bytesDecrypted -= paddingValue;
			offset = sizeEncrypted - paddingValue;
			memset(&decrypted[offset], 0, paddingValue);
		}
	}
  // printHexValue(output, outputSize, " ");	
	setStatus(ATECCAES_SUCCESS);
	*sizeDecrypted = bytesDecrypted;
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
