#pragma once

#include "ATECCAES.h"


ATECCAES::ATECCAES(ATECCX08A *atecc, PaddingType padding)
{
	this->atecc = atecc;
	this->padding = padding;
}


boolean ATECCAES::encryptDecrypt(uint8_t *input, int inputSize, uint8_t *output, int outputSize, uint8_t slot, uint8_t keyIndex, uint8_t mode, boolean debug)
{
	boolean result;
	int     iterations, offset;
	PaddingType  padding;
	
	padding = getPadding();
	if (padding == NoPadding)
	{		
    if (inputSize % AES_BLOCKSIZE != 0)
		{
			setStatus(ATECCAES_INVALID_INPUT_LENGTH);
			return false;
	  }	
		if (outputSize < inputSize)
		{
			setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
			return false;
		}
	}
	else if (padding == PKCS7Padding)
	{
    if (inputSize % AES_BLOCKSIZE != 0)		// outputSize must be at least as long als inputSize
		{
			if (outputSize < inputSize)
			{
				setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
				return false;
			}
		}
		else 
		{
			// we need to append an additional block for padding
			if (outputSize < (inputSize + AES_BLOCKSIZE))
			{
				setStatus(ATECCAES_OUTPUT_LENGTH_TOO_SMALL);
				return false;
			}
		}
	}
	
	// TODO: consider padding case!
	
	iterations = inputSize / AES_BLOCKSIZE;
	for (int index = 0; index < iterations; index++)
	{
		offset = index * AES_BLOCKSIZE;
	  result = atecc->encryptDecryptBlock(&input[offset], AES_BLOCKSIZE, &output[offset], AES_BLOCKSIZE, slot, keyIndex, mode, debug);
		if (result == false)
			return result;
	}
	// handle special case we need to append a block
	if (padding == PKCS7Padding && (inputSize % AES_BLOCKSIZE) == 0)
	{
		if ((mode & AES_ENCRYPT) == AES_ENCRYPT)
		{
			uint8_t padding[AES_BLOCKSIZE];
			
			memset(padding, 0x10, 16);
			offset += AES_BLOCKSIZE;
			result = atecc->encryptDecryptBlock(padding, AES_BLOCKSIZE, &output[offset], AES_BLOCKSIZE, slot, keyIndex, mode, debug);
			if (result == false)
				return result;
		}
		else  // decryption
		{
			// verify if last input block only contains 0x10
			offset += AES_BLOCKSIZE;
			for (int index = 0; index < AES_BLOCKSIZE; index++)
			{
				if (input[offset+index] != 0x10)
				{
					setStatus(ATECCAES_PADDING_ERROR);
					return false;
				}
			}
			// throw away last block and init output with zeros
			memset(&output[offset], 0, AES_BLOCKSIZE);
		}
	}
  // printHexValue(output, outputSize, " ");	
	setStatus(ATECCAES_SUCCESS);
	return true;
}

boolean ATECCAES::encrypt(uint8_t *clearText, int sizeClearText, uint8_t *encrypted, int sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	return encryptDecrypt(clearText, sizeClearText, encrypted, sizeEncrypted, slot, keyIndex, AES_ENCRYPT, debug);
}

boolean ATECCAES::decrypt(uint8_t *encrypted, int sizeEncrypted, uint8_t *clearText, int sizeClearText, uint8_t slot, uint8_t keyIndex, boolean debug)
{
	return encryptDecrypt(encrypted, sizeEncrypted, clearText, sizeClearText, slot, keyIndex, AES_DECRYPT, debug);
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
