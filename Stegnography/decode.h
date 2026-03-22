#ifndef DECODE_H
#define DECODE_H

#include "types.h"
#include "encode.h"   // To use EncodeInfo structure

#define MAGIC_STRING "#*"
Status read_and_validate_decode_args(char *argv[], EncodeInfo *decInfo);


Status decode_byte_from_lsb(char *image_buffer, char *data);
Status decode_size_from_lsb(char *image_buffer, int *size);

Status decode_magic_string(EncodeInfo *encInfo);
Status decode_secret_file_extn_size(int *size, EncodeInfo *encInfo);
Status decode_secret_file_extn(char *file_extn, int size, EncodeInfo *encInfo);
Status decode_secret_file_size(long *file_size, EncodeInfo *encInfo);
Status decode_secret_file_data(EncodeInfo *encInfo);
Status copy_remaining_img_data_decode(FILE *fptr_src, FILE *fptr_dest);

Status do_decoding(EncodeInfo *decInfo);

#endif
