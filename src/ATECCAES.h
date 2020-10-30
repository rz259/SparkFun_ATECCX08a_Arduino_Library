#pragma once

#include "SparkFun_ATECCX08a_Arduino_Library.h"


#define ATECCAES_SUCCESS                    0
#define ATECCAES_INVALID_INPUT_LENGTH     -10
#define ATECCAES_OUTPUT_LENGTH_TOO_SMALL  -11
#define ATECCAES_INVALID_SLOT             -12
#define ATECCAES_PADDING_ERROR            -13
//#define

typedef enum PaddingType 
{ 
  NoPadding, 
  PKCS7Padding
}	PaddingType;

class ATECCAES
{

  public:
	  ATECCAES(ATECCX08A *atecc, PaddingType padding);
		PaddingType getPadding();
		int         getStatus();
    boolean encrypt(uint8_t *clearText, int sizeClearText, uint8_t *encrypted, int sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
    boolean decrypt(uint8_t *clearText, int sizeClearText, uint8_t *encrypted, int sizeEncrypted, uint8_t slot, uint8_t keyIndex, boolean debug = false);
		
	private:
	  ATECCX08A *atecc;
		PaddingType padding;
		int         status;

    boolean encryptDecrypt(uint8_t *input, int inputSize, uint8_t *output, int outputSize, uint8_t slot, uint8_t keyIndex, uint8_t mode, boolean debug = false);
	  void setStatus(int status);
	
};