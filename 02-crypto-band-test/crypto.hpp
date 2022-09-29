#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/aes.h>



class Crypto{
public:
    unsigned char *Key;
    AES_KEY AesKey;
    unsigned char ivec[AES_BLOCK_SIZE];
    Crypto(){
        Key = (unsigned char *)calloc(AES_BLOCK_SIZE + 1, sizeof(char));
        if(Key == NULL){
            fprintf(stderr, "Unable to allocate memory for Key ");
            exit(-1);
        }
        memcpy(Key, "0123456789abcdef", AES_BLOCK_SIZE);
        memset(&AesKey, 0x00, sizeof(AES_KEY));
        if (AES_set_encrypt_key(Key, 128, &AesKey) < 0)
        { //设置加密密钥
            fprintf(stderr, "Unable to set encryption key in AES... ");
            exit(-1);
        }
    }
    ~Crypto(){
        free(Key);
    }
    int encrypto(char * plain_data, int data_len, char * encrypt_data){
        int SetDataLen = 0;
        if((data_len % AES_BLOCK_SIZE) == 0){
            SetDataLen = data_len;
        }else{
            SetDataLen = ((data_len / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
        }

        AES_set_encrypt_key((unsigned char *)Key, 128, &AesKey);
        char * InputData = (char *)calloc(SetDataLen + 1, sizeof(char));
        memcpy(InputData, plain_data, data_len);
        memset(ivec, 0x00, sizeof(ivec));
        AES_cbc_encrypt((unsigned char *)plain_data, (unsigned char *)encrypt_data, SetDataLen, &AesKey, ivec, AES_ENCRYPT);
        if (InputData != NULL)
        {
            free(InputData);
            InputData = NULL;
        }

        return SetDataLen;
    }
    int decrypto(char * encrypt_data, int data_len, char * plain_data){
        memset(&AesKey, 0x00, sizeof(AES_KEY));
        AES_set_decrypt_key((unsigned char *)Key, 128, &AesKey);
        memset(ivec, 0x00, sizeof(ivec));
        AES_cbc_encrypt((unsigned char *)encrypt_data, (unsigned char *)plain_data, data_len, &AesKey, ivec, AES_DECRYPT);
        return data_len;
    }
};
