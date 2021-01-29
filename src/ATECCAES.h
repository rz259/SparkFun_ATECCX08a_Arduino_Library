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


class ATECCAES
{
  public:
	  ATECCAES(const ATECCX08A *atecc, PaddingType padding);
		int     getStatus();
    virtual boolean encrypt(const uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false) = 0;
    virtual boolean decrypt(const uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false) = 0;

  protected:
	  void setStatus(int status);
		void appendPadding(uint8_t *data, int sizePlainText, int totalSize);
		boolean removePadding(uint8_t *decryptBuffer, int &bytesDecrypted);
		boolean performChecksForEncryption(int sizePlainText, int sizeEncrypted);
    boolean performChecksForDecryption(int sizeEncrypted, int sizeDecrypted);
		uint8_t *initInputBuffer(const uint8_t *plainText, int sizePlainText, int &totalSize);
		ATECCX08A *getCryptoAdapter();

  protected:
		PaddingType getPadding();
	  void printHexValue(uint8_t value);
		void printHexValue(const uint8_t *value, int length, const char *separator);
		int  calcSizeNeeded(int length);

	
	private:
	  ATECCX08A   *atecc;
		PaddingType padding;
		int         status;
		
};

class ATECCAES_ECB : public ATECCAES
{
	public:
	  ATECCAES_ECB(ATECCX08A *atecc, PaddingType padding);
    virtual boolean encrypt(const uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
    virtual boolean decrypt(const uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
		
  private: 
	
};

class ATECCAES_CBC : public ATECCAES
{
	public:
	  ATECCAES_CBC(ATECCX08A *atecc, PaddingType padding, uint8_t *iv);
    virtual boolean encrypt(const uint8_t *plainText, int sizePlainText, uint8_t *encrypted, int &sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
    virtual boolean decrypt(const uint8_t *encrypted, int sizeEncrypted, uint8_t *decrypted, int &sizeDecrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
		void setIV(const uint8_t *iv);
		
  private: 
	  uint8_t iv[AES_BLOCKSIZE];
		
		void    xorBlock(uint8_t *data, const uint8_t *block, int size);
	
};


