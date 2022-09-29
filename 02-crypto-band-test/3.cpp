#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/aes.h>

// g++ -g -o -Wall -m64 AesTest AesTest.cpp -lssl -lcrypto
// g++ -g -o -Wall AesTest AesTest.cpp -lssl -lcrypto

int main(int argc, char **argv)
{ //由于与直接对接用的char，那么加解密要强制转换
    char Source[1024];
    char *InputData = NULL;
    char *EncryptData = NULL;
    char *DecryptData = NULL;

    unsigned char Key[AES_BLOCK_SIZE + 1]; //建议用unsigned char
    unsigned char ivec[AES_BLOCK_SIZE];    //建议用unsigned char
    AES_KEY AesKey;

    int DataLen = 0, SetDataLen = 0, i;
    FILE * file = fopen("sendtext.txt", "r");

    memset(Source, 0x00, sizeof(Source));
    // strcpy(Source, "1234567890abcde"); //要加密的数据
    fread(Source, 1, 1024, file);
    DataLen = sizeof(Source);
    // DataLen = strlen(Source);
    printf("Datalen: %d\n", DataLen);

    memset(Key, 0x00, sizeof(Key));
    memcpy(Key, "0123456789abcdef", AES_BLOCK_SIZE);

    // set the encryption length
    SetDataLen = 0;
    if ((DataLen % AES_BLOCK_SIZE) == 0)
    {
        SetDataLen = DataLen;
    }
    else
    {
        SetDataLen = ((DataLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
    }
    printf("SetDataLen:%d\n", SetDataLen); //取16的倍数

    InputData = (char *)calloc(SetDataLen + 1, sizeof(char));
    if (InputData == NULL) //注意要SetDataLen+1
    {
        fprintf(stderr, "Unable to allocate memory for InputData ");
        exit(-1);
    }
    memcpy(InputData, Source, DataLen);
    printf("InputData:%p; %s..\n", InputData, InputData);
    printf("Source:%p; %s..\n", Source, Source);
    printf("Inputdata[16]:%p; hex:%x..\n", InputData + 16, InputData[16]);

    EncryptData = (char *)calloc(SetDataLen + 1, sizeof(char));
    if (EncryptData == NULL) //注意要SetDataLen+1
    {
        fprintf(stderr, "Unable to allocate memory for EncryptData ");
        exit(-1);
    }

    DecryptData = (char *)calloc(SetDataLen + 1, sizeof(char));
    if (DecryptData == NULL) //注意要SetDataLen+1
    {
        fprintf(stderr, "Unable to allocate memory for DecryptData ");
        exit(-1);
    }

    memset(&AesKey, 0x00, sizeof(AES_KEY));
    printf("size of AES_KEY: %ld\n", sizeof(AES_KEY));
    if (AES_set_encrypt_key(Key, 128, &AesKey) < 0)
    { //设置加密密钥
        fprintf(stderr, "Unable to set encryption key in AES... ");
        exit(-1);
    }

    // for (i = 0; i < AES_BLOCK_SIZE; i++)
    // { //必须要有
    //     ivec[i] = 0;
    // }
    memset(ivec, 0x00, sizeof(ivec));
    //加密
    AES_cbc_encrypt((unsigned char *)InputData, (unsigned char *)EncryptData,
                    SetDataLen, &AesKey, ivec, AES_ENCRYPT);

    memset(&AesKey, 0x00, sizeof(AES_KEY));
    if (AES_set_decrypt_key(Key, 128, &AesKey) < 0)
    { //设置解密密钥
        fprintf(stderr, "Unable to set encryption key in AES... ");
        exit(-1);
    }

    for (i = 0; i < AES_BLOCK_SIZE; i++)
    { //必须要有
        printf("ivec[%d]: %x\n", i, ivec[i]);
    }
    memset(ivec, 0x00, sizeof(ivec));
    for (i = 0; i < AES_BLOCK_SIZE; i++)
    { //必须要有
        printf("ivec[%d]: %x\n", i, ivec[i]);
    }
    
    printf("\nencryptodata:%s\n", EncryptData);
    printf("EncryptoData length: %ld\n", sizeof(EncryptData));
    printf("EncryptoData length: %ld\n", strlen(EncryptData));
    printf("EncryptoData[1023]:%p; hex:%x..\n", EncryptData + 1023, EncryptData[1023]);
    printf("EncryptoData[1024]:%p; hex:%x..\n", EncryptData + 1024, EncryptData[1024]);
    printf("EncryptoData[1025]:%p; hex:%x..\n", EncryptData + 1025, EncryptData[1025]);
    //解密
    AES_cbc_encrypt((unsigned char *)EncryptData, (unsigned char *)DecryptData,
                    SetDataLen, &AesKey, ivec, AES_DECRYPT);

    printf("DecryptData:%s... ", (char *)DecryptData);
    printf("DecryptData length: %ld\n", sizeof(DecryptData));
    printf("DecryptData length: %ld\n", strlen(DecryptData));

    if (InputData != NULL)
    {
        free(InputData);
        InputData = NULL;
    }

    if (EncryptData != NULL)
    {
        free(EncryptData);
        EncryptData = NULL;
    }

    if (DecryptData != NULL)
    {
        free(DecryptData);
        DecryptData = NULL;
    }

    exit(0);
}