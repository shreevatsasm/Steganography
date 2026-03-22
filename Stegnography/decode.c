#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include<stdlib.h>
#define MAGIC_STRING "#*"

/* Function to decode one byte from 8 bytes of image buffer LSBs */
/*Status do_decoding(EncodeInfo *decInfo)
{
    // Open stego image
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");
    if (!decInfo->fptr_stego_image)
        return e_failure;

    // Open output secret file
    decInfo->fptr_secret = fopen(decInfo->decoded_secret_fname, "wb");
    if (!decInfo->fptr_secret)
        return e_failure;

    // Here: Implement decoding steps (reverse of encoding)
    // 1. Skip BMP header
    // 2. Decode magic string
    // 3. Decode secret file extension
    // 4. Decode secret file size
    // 5. Decode secret data

    printf("Secret file data decoded successfully into %s\n", decInfo->decoded_secret_fname);

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_secret);

    return e_success;
}*/
Status read_and_validate_decode_args(char *argv[], EncodeInfo *decInfo)
{
    // argv[2] → stego image file
    decInfo->stego_image_fname = argv[2];

    // argv[3] → optional output filename
    if (argv[3] != NULL)
        snprintf(decInfo->decoded_secret_fname, sizeof(decInfo->decoded_secret_fname), "%s", argv[3]);
    else
        strcpy(decInfo->decoded_secret_fname, "decoded_secret.txt");

    return e_success;
}
Status decode_byte_from_lsb(char *image_buffer, char *data)
{
    *data = 0;
    for (int i = 0; i < 8; i++)
    {
        char bit = image_buffer[i] & 1;   // Extract LSB
        *data |= (bit << (7 - i));        // Set corresponding bit
    }
    return e_success;
}

/* Function to decode a 32-bit size from 32 bytes of image buffer LSBs */
Status decode_size_from_lsb(char *image_buffer, int *size)
{
    *size = 0;
    for (int i = 0; i < 32; i++)
    {
        char bit = image_buffer[i] & 1;
        *size |= (bit << (31 - i));
    }
    return e_success;
}

/* Decode magic string to verify */
Status decode_magic_string(EncodeInfo *encInfo)
{
    char image_buffer[8];
    char magic[3]; // "#*"
    for (int i = 0; i < 2; i++)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("Error: Unable to read image data for magic string\n");
            return e_failure;
        }

        if (decode_byte_from_lsb(image_buffer, &magic[i]) != e_success)
        {
            printf("Error: Unable to decode magic string byte\n");
            return e_failure;
        }
    }
    magic[2] = '\0';

    if (strcmp(magic, MAGIC_STRING) != 0)
    {
        printf("Error: Magic string mismatch, no hidden data found\n");
        return e_failure;
    }

    printf("Magic string decoded successfully: %s\n", magic);
    return e_success;
}

/* Decode secret file extension size */
Status decode_secret_file_extn_size(int *size, EncodeInfo *encInfo)
{
    char image_buffer[32];
    if (fread(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        printf("Error: Unable to read image data for extension size\n");
        return e_failure;
    }

    if (decode_size_from_lsb(image_buffer, size) != e_success)
    {
        printf("Error: Unable to decode extension size\n");
        return e_failure;
    }

    printf("Secret file extension size decoded: %d\n", *size);
    return e_success;
}

/* Decode secret file extension */
Status decode_secret_file_extn(char *file_extn, int size, EncodeInfo *encInfo)
{
    char image_buffer[8];
    for (int i = 0; i < size; i++)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("Error: Unable to read image data for file extension\n");
            return e_failure;
        }

        if (decode_byte_from_lsb(image_buffer, &file_extn[i]) != e_success)
        {
            printf("Error: Unable to decode file extension byte\n");
            return e_failure;
        }
    }
    file_extn[size] = '\0';
    printf("Secret file extension decoded: %s\n", file_extn);
    return e_success;
}

/* Decode secret file size */
Status decode_secret_file_size(long *file_size, EncodeInfo *encInfo)
{
    char image_buffer[32];
    if (fread(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        printf("Error: Unable to read image data for secret file size\n");
        return e_failure;
    }

    int size;
    if (decode_size_from_lsb(image_buffer, &size) != e_success)
    {
        printf("Error: Unable to decode secret file size\n");
        return e_failure;
    }

    *file_size = size;
    printf("Secret file size decoded: %ld bytes\n", *file_size);
    return e_success;
}

/* Decode secret file data */
Status decode_secret_file_data(EncodeInfo *encInfo)
{
    char image_buffer[8];
    char *secret_data = malloc(encInfo->size_secret_file);
    if (!secret_data)
    {
        printf("Error: Memory allocation failed\n");
        return e_failure;
    }

    for (long i = 0; i < encInfo->size_secret_file; i++)
    {
        if (fread(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("Error: Unable to read image data for secret file\n");
            free(secret_data);
            return e_failure;
        }

        if (decode_byte_from_lsb(image_buffer, &secret_data[i]) != e_success)
        {
            printf("Error: Unable to decode byte from LSB\n");
            free(secret_data);
            return e_failure;
        }
    }

    // Write decoded secret file
    FILE *fptr_secret = fopen(encInfo->decoded_secret_fname, "w");
    if (!fptr_secret)
    {
        printf("Error: Unable to create decoded secret file\n");
        free(secret_data);
        return e_failure;
    }

    if (fwrite(secret_data, 1, encInfo->size_secret_file, fptr_secret) != encInfo->size_secret_file)
    {
        printf("Error: Unable to write decoded secret data\n");
        free(secret_data);
        fclose(fptr_secret);
        return e_failure;
    }

    fclose(fptr_secret);
    free(secret_data);

    printf("Secret file data decoded successfully into %s\n", encInfo->decoded_secret_fname);
    return e_success;
}

/* Decode remaining image data (if needed) */
Status copy_remaining_img_data_decode(FILE *fptr_src, FILE *fptr_dest)
{
    char buffer[1024];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        if (fwrite(buffer, 1, bytesRead, fptr_dest) != bytesRead)
        {
            printf("Error: Unable to write remaining image data\n");
            return e_failure;
        }
    }

    if (ferror(fptr_src))
    {
        printf("Error: Reading source file failed\n");
        return e_failure;
    }

    printf("Remaining image data copied successfully\n");
    return e_success;
}

/* Main decode function */
Status do_decoding(EncodeInfo *decInfo)
{
    // Open stego image for reading
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (!decInfo->fptr_stego_image)
    {
        printf("Error: Unable to open stego image %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    // Skip BMP header
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    // Decode magic string
    if (decode_magic_string(decInfo) != e_success)
        return e_failure;

    // Decode secret file extension size
    int extn_size;
    if (decode_secret_file_extn_size(&extn_size, decInfo) != e_success)
        return e_failure;

    // Decode secret file extension
    if (decode_secret_file_extn(decInfo->extn_secret_file, extn_size, decInfo) != e_success)
        return e_failure;

    // Prepare decoded secret file name
    snprintf(decInfo->decoded_secret_fname, sizeof(decInfo->decoded_secret_fname),
             "decoded_secret%s", decInfo->extn_secret_file);

    // Decode secret file size
    if (decode_secret_file_size(&decInfo->size_secret_file, decInfo) != e_success)
        return e_failure;

    // Decode secret file data
    if (decode_secret_file_data(decInfo) != e_success)
        return e_failure;

    printf("Decoding completed successfully!\n");
    return e_success;
}
