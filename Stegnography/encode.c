#include <stdio.h>
#include "encode.h"
#include "types.h"
#include <string.h>
#define MAGIC_STRING "#*"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    uint size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    return size;
}


/*
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    // validate source image ends with .bmp
    char *src = argv[2];
    int len_src = strlen(src);
    if (len_src < 4 || strcmp(src + len_src - 4, ".bmp") != 0)
    {
        printf("Error: Source image must be a .bmp file\n");
        return e_failure;
    }
    encInfo->src_image_fname = src; // store src name

    // validate secret file has an extension
    char *secret = argv[3];
    char *dot = strrchr(secret, '.'); // last dot
    if (!dot || dot == secret || *(dot + 1) == '\0')
    {
        printf("Error: Secret file must have a valid extension\n");
        return e_failure;
    }
    encInfo->secret_fname = secret; // store secret file

    // store secret file extension (max 4 chars)
    strncpy(encInfo->extn_secret_file, dot, 4);
    encInfo->extn_secret_file[4] = '\0'; // ensure null terminated

    // optional destination file
    if (argv[4] != NULL)
    {
        char *dest = argv[4];
        int len_dest = strlen(dest);

        // validate destination ends with .bmp
        if (len_dest < 4 || strcmp(dest + len_dest - 4, ".bmp") != 0)
        {
            printf("Error: Destination image must be a .bmp file\n");
            return e_failure;
        }

        encInfo->stego_image_fname = dest; // store user-given destination
    }
    else
    {
        encInfo->stego_image_fname = "destination.bmp"; // default name
    }

    return e_success; // all validations passed
}



Status open_files(EncodeInfo *encInfo)
{
    /* Open source image */
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open source image file %s\n",
                encInfo->src_image_fname);
        return e_failure;
    }

    /* Open secret file */
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open secret file %s\n",
                encInfo->secret_fname);
        return e_failure;
    }

    /* Open stego image file */
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open stego image file %s\n",
                encInfo->stego_image_fname);
        return e_failure;
    }

    printf("INFO: All files opened successfully.\n");
    return e_success;
}


Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    if (encInfo->image_capacity > encInfo->size_secret_file * 8)
    {
        printf("Capacity check passed\n");
        return e_success;
    }
    else
    {
        printf("Error: Image does not have enough capacity\n");
        return e_failure;
    }
}


Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char image_buffer[54];

    rewind(fptr_src_image);

    if (fread(image_buffer, 1, 54, fptr_src_image) != 54)
    {
        printf("Error: Could not read BMP header\n");
        return e_failure;
    }

    if (fwrite(image_buffer, 1, 54, fptr_dest_image) != 54)
    {
        printf("Error: Could not write BMP header\n");
        return e_failure;
    }

    long src_offset = ftell(fptr_src_image);
    long dest_offset = ftell(fptr_dest_image);

    if (src_offset == 54 && dest_offset == 54)
    {
        printf("BMP header copied successfully\n");
        return e_success;
    }
    else
    {
        printf("Error: BMP header copy failed\n");
        return e_failure;
    }
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    char image_buffer[8]; // buffer to hold 8 bytes of image
    long src_offset_before = ftell(encInfo->fptr_src_image);
    long dest_offset_before = ftell(encInfo->fptr_stego_image);

    for (int i = 0; magic_string[i] != '\0'; i++)
    {
        // Read 8 bytes from source image
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
        {
            printf("Error: Unable to read image data for magic string\n");
            return e_failure;
        }

        // Encode one byte of magic string into LSB
        if (encode_byte_to_lsb(magic_string[i], image_buffer) != e_success)
        {
            printf("Error: Unable to encode magic string byte\n");
            return e_failure;
        }

        // Write modified buffer to stego image
        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("Error: Unable to write image data for magic string\n");
            return e_failure;
        }
    }

    // Check offsets **relative to start of encoding**
    long src_offset_after = ftell(encInfo->fptr_src_image);
    long dest_offset_after = ftell(encInfo->fptr_stego_image);

    if ((src_offset_after - src_offset_before) != (8 * strlen(magic_string)) ||
        (dest_offset_after - dest_offset_before) != (8 * strlen(magic_string)))
    {
        printf("Error: Offset mismatch after encoding magic string\n");
        return e_failure;
    }

    printf("Magic string encoded successfully\n");
    return e_success;
}


Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    char image_buffer[32];
    long src_offset_before = ftell(encInfo->fptr_src_image);
    long dest_offset_before = ftell(encInfo->fptr_stego_image);

    if (fread(image_buffer, 1, 32, encInfo->fptr_src_image) != 32)
    {
        printf("Error: Unable to read image data for extension size\n");
        return e_failure;
    }

    if (encode_size_to_lsb(size, image_buffer) != e_success)
    {
        printf("Error: Unable to encode extension size\n");
        return e_failure;
    }

    if (fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        printf("Error: Unable to write image data for extension size\n");
        return e_failure;
    }

    long src_offset_after = ftell(encInfo->fptr_src_image);
    long dest_offset_after = ftell(encInfo->fptr_stego_image);

    if ((src_offset_after - src_offset_before) != 32 ||
        (dest_offset_after - dest_offset_before) != 32)
    {
        printf("Error: Source and destination file offsets mismatch after extension size\n");
        return e_failure;
    }

    printf("Secret file extension size encoded successfully\n");
    return e_success;
}



Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    char image_buffer[8];

    for (int i = 0; file_extn[i] != '\0'; i++)
    {
        // Save current offsets
        long src_offset_before = ftell(encInfo->fptr_src_image);
        long dest_offset_before = ftell(encInfo->fptr_stego_image);

        // Read 8 bytes from source image
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
        {
            printf("Error: Unable to read image data for file extension\n");
            return e_failure;
        }

        // Encode one byte from file extension into LSBs
        if (encode_byte_to_lsb(file_extn[i], image_buffer) != e_success)
        {
            printf("Error: Unable to encode file extension character\n");
            return e_failure;
        }

        // Write modified buffer to destination
        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("Error: Unable to write image data for file extension\n");
            return e_failure;
        }

        // Check offsets to ensure correct read/write
        long src_offset_after = ftell(encInfo->fptr_src_image);
        long dest_offset_after = ftell(encInfo->fptr_stego_image);

        if ((src_offset_after - src_offset_before) != 8 ||
            (dest_offset_after - dest_offset_before) != 8)
        {
            printf("Error: Source and destination file offsets mismatch\n");
            return e_failure;
        }
    }

    printf("Secret file extension encoded successfully\n");
    return e_success;
}


Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char image_buffer[32];  // buffer to read 32 bytes from source image

    // Save offsets
    long src_offset_before = ftell(encInfo->fptr_src_image);
    long dest_offset_before = ftell(encInfo->fptr_stego_image);

    // Read 32 bytes from source image into buffer
    if (fread(image_buffer, 1, 32, encInfo->fptr_src_image) != 32)
    {
        printf("Error: Unable to read image data for secret file size\n");
        return e_failure;
    }

    // Encode the file size (32-bit int) into the buffer using LSB
    if (encode_size_to_lsb((int)file_size, image_buffer) != e_success)
    {
        printf("Error: Unable to encode secret file size\n");
        return e_failure;
    }

    // Write modified buffer to destination file
    if (fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image) != 32)
    {
        printf("Error: Unable to write image data for secret file size\n");
        return e_failure;
    }

    // Check offsets
    long src_offset_after = ftell(encInfo->fptr_src_image);
    long dest_offset_after = ftell(encInfo->fptr_stego_image);

    if ((src_offset_after - src_offset_before) != 32 ||
        (dest_offset_after - dest_offset_before) != 32)
    {
        printf("Error: Source and destination file offsets mismatch\n");
        return e_failure;
    }

    printf("Secret file size encoded successfully\n");
    return e_success;
}


Status encode_secret_file_data(EncodeInfo *encInfo)
{
    // Rewind source secret file to start
    rewind(encInfo->fptr_secret);

    // Allocate buffer to hold secret file data
    char secret_data[encInfo->size_secret_file];

    // Read entire secret file into buffer
    if (fread(secret_data, 1, encInfo->size_secret_file, encInfo->fptr_secret) != encInfo->size_secret_file)
    {
        printf("Error: Unable to read secret file data\n");
        return e_failure;
    }

    char image_buffer[8]; // buffer to hold 8 bytes from source image
    long src_offset_before, dest_offset_before, src_offset_after, dest_offset_after;

    for (long i = 0; i < encInfo->size_secret_file; i++)
    {
        // Save offsets
        src_offset_before = ftell(encInfo->fptr_src_image);
        dest_offset_before = ftell(encInfo->fptr_stego_image);

        // Read 8 bytes from source image
        if (fread(image_buffer, 1, 8, encInfo->fptr_src_image) != 8)
        {
            printf("Error: Unable to read image data for secret file encoding\n");
            return e_failure;
        }

        // Encode one byte of secret file into image buffer
        if (encode_byte_to_lsb(secret_data[i], image_buffer) != e_success)
        {
            printf("Error: Unable to encode byte to LSB\n");
            return e_failure;
        }

        // Write modified buffer to destination
        if (fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image) != 8)
        {
            printf("Error: Unable to write image data for secret file encoding\n");
            return e_failure;
        }

        // Check offsets
        src_offset_after = ftell(encInfo->fptr_src_image);
        dest_offset_after = ftell(encInfo->fptr_stego_image);

        if ((src_offset_after - src_offset_before) != 8 ||
            (dest_offset_after - dest_offset_before) != 8)
        {
            printf("Error: Source and destination file offsets mismatch\n");
            return e_failure;
        }
    }

    printf("Secret file data encoded successfully\n");
    return e_success;
}


Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char buffer[1024]; // buffer to read chunks
    size_t bytesRead;

    // Copy until end of file
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        if (fwrite(buffer, 1, bytesRead, fptr_dest) != bytesRead)
        {
            printf("Error: Unable to write remaining image data\n");
            return e_failure;
        }
    }

    // Check for fread error
    if (ferror(fptr_src))
    {
        printf("Error: Reading source file failed\n");
        return e_failure;
    }

    printf("Remaining image data copied successfully\n");
    return e_success;
}


Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        // Clear LSB of image byte
        image_buffer[i] &= 0xFE; // 11111110

        // Get the i-th bit of data (from MSB to LSB)
        char bit = (data >> (7 - i)) & 1;

        // Set the bit into LSB of image byte
        image_buffer[i] |= bit;
    }

    return e_success;
}


Status encode_size_to_lsb(int size, char *image_buffer)
{
    for (int i = 0; i < 32; i++)
    {
        // Clear LSB of current byte
        image_buffer[i] &= 0xFE; // 11111110

        // Get the i-th bit of size (MSB first)
        char bit = (size >> (31 - i)) & 1;

        // Set the bit into LSB of image byte
        image_buffer[i] |= bit;
    }

    return e_success;
}


Status do_encoding(EncodeInfo *encInfo)
{
    // 1. Open all files
    if (open_files(encInfo) != e_success)
        return e_failure;
    printf("Files opened successfully\n");

    // 2. Get image capacity
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);

    // 3. Get secret file size
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    // 4. Check if secret file can fit in image
    if (encInfo->image_capacity < encInfo->size_secret_file * 8)
    {
        printf("Error: Image capacity is not enough to hold secret file\n");
        return e_failure;
    }

    // 5. Copy BMP header
    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) != e_success)
        return e_failure;

    // 6. Encode magic string
    if (encode_magic_string(MAGIC_STRING, encInfo) != e_success)
        return e_failure;

    // 7. Encode secret file extension size
    if (encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) != e_success)
        return e_failure;

    // 8. Encode secret file extension
    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) != e_success)
        return e_failure;

    // 9. Encode secret file size
    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) != e_success)
        return e_failure;

    // 10. Encode secret file data
    if (encode_secret_file_data(encInfo) != e_success)
        return e_failure;

    // 11. Copy remaining image data
    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) != e_success)
        return e_failure;

    printf("Encoding completed successfully!\n");
    return e_success;
}

