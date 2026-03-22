#include <stdio.h>
#include "encode.h"
#include "types.h"
#include<string.h>
#include "decode.h"
OperationType check_operation_type(char *);

void print_help()
{
    printf("Usage:\n");
    printf("  Encode: ./a.out -e <source.bmp> <secret_file> [destination.bmp]\n");
    printf("  Decode: ./a.out -d <stego_image.bmp> [output_file]\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) // no args → show help
    {
        print_help();
        return e_failure;
    }

    OperationType op = check_operation_type(argv[1]);

    if (op == e_encode)
{
    if (argc < 4)   // -e source.bmp secretfile required
    {
        printf("Invalid number of arguments for encoding\n");
        print_help();
        return e_failure;
    }

    EncodeInfo encodeInfo;

    if (read_and_validate_encode_args(argv, &encodeInfo) != e_success)
    {
        printf("Argument validation failed\n");
        return e_failure;
    }

    do_encoding(&encodeInfo);
}

    else if (op == e_decode)
{
    if (argc < 3) // need at least stego_image.bmp
    {
        printf("Invalid number of arguments for decoding\n");
        print_help();
        return e_failure;
    }

    EncodeInfo decodeInfo;  // reuse the same struct

    if (read_and_validate_decode_args(argv, &decodeInfo) != e_success)
    {
        printf("Argument validation failed\n");
        return e_failure;
    }

    do_decoding(&decodeInfo);
}

}

OperationType check_operation_type(char *symbol)
{
    if (strcmp(symbol, "-e") == 0)
        return e_encode;
    if (strcmp(symbol, "-d") == 0)
        return e_decode;
    return e_unsupported;
}
