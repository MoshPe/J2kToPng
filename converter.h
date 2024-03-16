#ifndef CONVERTER_H
#define CONVERTER_H

unsigned char* convert_j2k_to_png(const unsigned char *j2k_data, size_t j2k_size, size_t* png_data_size);

#endif
