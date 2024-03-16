#include <stdio.h>
#include <string.h>
#include <openjpeg-2.3/openjpeg.h>
#include <png.h>
#include <stdlib.h>
#include "converter.h"

static void opj_close_from_file(void* p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    fclose(p_file);
}

static OPJ_SIZE_T opj_read_from_file(void * p_buffer, OPJ_SIZE_T p_nb_bytes,
                                     void * p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    OPJ_SIZE_T l_nb_read = fread(p_buffer, 1, p_nb_bytes, (FILE*)p_file);
    return l_nb_read ? l_nb_read : (OPJ_SIZE_T) - 1;
}

static OPJ_SIZE_T opj_write_from_file(void * p_buffer, OPJ_SIZE_T p_nb_bytes,
                                      void * p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    return fwrite(p_buffer, 1, p_nb_bytes, p_file);
}

static OPJ_OFF_T opj_skip_from_file(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    if (fseek(p_file, p_nb_bytes, SEEK_CUR)) {
        return -1;
    }

    return p_nb_bytes;
}

static OPJ_BOOL opj_seek_from_file(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    if (fseek(p_file, p_nb_bytes, SEEK_SET)) {
        return OPJ_FALSE;
    }

    return OPJ_TRUE;
}

static OPJ_UINT64 opj_get_data_length_from_file(void * p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    OPJ_OFF_T file_length = 0;

    fseek(p_file, 0, SEEK_END);
    file_length = (OPJ_OFF_T)ftell(p_file);
    fseek(p_file, 0, SEEK_SET);

    return (OPJ_UINT64)file_length;
}

unsigned char* convert_j2k_to_png(const unsigned char *j2k_data, size_t j2k_size, size_t* png_data_size) {
    unsigned char* png_data = NULL;
    *png_data_size = 0;
    // Open the JPEG 2000 file

    FILE *infile = fmemopen((void*)j2k_data, j2k_size, "rb");
    if (!infile) {
        fprintf(stderr, "Error: Cannot create File from J2K data\n");
        return NULL;
    }

    OPJ_SIZE_T p_size = OPJ_J2K_STREAM_CHUNK_SIZE;
    opj_stream_t* stream = 00;

    stream = opj_stream_create(p_size, 1);
    if (!stream) {
        fclose(infile);
        return NULL;
    }

    opj_stream_set_user_data(stream, infile, opj_close_from_file);
    opj_stream_set_user_data_length(stream,
                                    opj_get_data_length_from_file(infile));
    opj_stream_set_read_function(stream, opj_read_from_file);
    opj_stream_set_write_function(stream,
                                  (opj_stream_write_fn) opj_write_from_file);
    opj_stream_set_skip_function(stream, opj_skip_from_file);
    opj_stream_set_seek_function(stream, opj_seek_from_file);

    // Create decompressor
    opj_codec_t *codec = opj_create_decompress(OPJ_CODEC_J2K);
    if (!codec) {
        opj_stream_destroy(stream);
        fprintf(stderr, "Error: Cannot create codec\n");
        return NULL;
    }

    // Setup decompression parameters
    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);
    opj_image_t *image = NULL;

    if (!opj_setup_decoder(codec, &parameters)) {
        fprintf(stderr, "ERROR -> opj_decompress: failed to setup the decoder\n");
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        return NULL;
    }

    /* Read the main header of the code stream and if necessary the JP2 boxes*/
    if (!opj_read_header(stream, codec, &image)) {
        fprintf(stderr, "ERROR -> opj_decompress: failed to read the header\n");
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return NULL;
    }

    // Decode the image
    if (!(opj_decode(codec, stream, image) &&
          opj_end_decompress(codec, stream))) {
        fprintf(stderr, "ERROR -> j2k_to_image: failed to decode image!\n");
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return NULL;
    }

    // Cleanup stream and file
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);


    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Error creating PNG write struct\n");
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "Error creating PNG info struct\n");
        png_destroy_write_struct(&png_ptr, NULL);
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return NULL;
    }

    FILE *output_png = open_memstream((char**)&png_data, png_data_size);
    if (!output_png) {
        fprintf(stderr, "Error opening memory stream\n");
        png_destroy_write_struct(&png_ptr, &info_ptr);
        opj_stream_destroy(stream);
        opj_destroy_codec(codec);
        opj_image_destroy(image);
        return NULL;
    }

    png_init_io(png_ptr, output_png);

    png_set_IHDR(png_ptr, info_ptr, image->x1, image->y1, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    png_bytep row = (png_bytep)malloc(3 * image->x1 * sizeof(png_byte));
    for (int y = 0; y < image->y1; y++) {
        for (int x = 0; x < image->x1; x++) {
            row[x * 3] = image->comps[0].data[y * image->x1 + x];
            row[x * 3 + 1] = image->comps[1].data[y * image->x1 + x];
            row[x * 3 + 2] = image->comps[2].data[y * image->x1 + x];
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, info_ptr);

    // Cleanup
    free(row);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(output_png);
    opj_image_destroy(image);

    return png_data;
}
