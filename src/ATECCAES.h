#pragma once

#include "SparkFun_ATECCX08a_Arduino_Library.h"


#define ATECCAES_SUCCESS                    0
#define ATECCAES_INVALID_INPUT_LENGTH     -10
#define ATECCAES_OUTPUT_LENGTH_TOO_SMALL  -11
#define ATECCAES_INPUT_LENGTH_TOO_SMALL   -12
#define ATECCAES_INVALID_SLOT             -13
#define ATECCAES_PADDING_ERROR            -14
#define ATECCAES_IV_MISSING               -15

typedef enum PaddingType 
{ 
  NoPadding, 
  PKCS7Padding
}	PaddingType;

typedef enum AESMode
{
	ECB,
	CBC
} AESMode;

class ATECCAES
{

  public:
	  ATECCAES(ATECCX08A *atecc, PaddingType padding, AESMode mode, byte *iv = NULL);
		int         getStatus();
    boolean encrypt(uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
    boolean decrypt(uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
		
	private:
	  ATECCX08A *atecc;
		PaddingType padding;
		AESMode     mode;
		byte        *iv;
		int         status;
		
		boolean encryptECB(uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
		boolean encryptCBC(uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
    boolean decryptECB(uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
    boolean decryptCBC(uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
		PaddingType getPadding();
	  void setStatus(int status);
		AESMode getMode();
	  void printHexValue(uint8_t value);
		void printHexValue(const uint8_t *value, int length, const char *separator);
		int  calcSizeNeeded(int length);
		void appendPadding(uint8_t *data, int sizePlainText, int totalSize);
		boolean removePadding(uint8_t *decryptBuffer, int &bytesDecrypted);
		boolean performChecksForEncryption(int sizePlainText, int sizeEncrypted);
    boolean performChecksForDecryption(int sizeEncrypted, int sizeDecrypted);
		uint8_t *initInputBuffer(uint8_t *plainText, int sizePlainText, int &totalSize);
		void    xorBlock(uint8_t *data, uint8_t *block, int size);
};