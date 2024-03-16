package main

// #cgo LDFLAGS: -lopenjp2 -lpng
// #include "openjpeg-2.3/openjpeg.h"
//#include "stdio.h"
//#include "string.h"
//#include "png.h"
//#include "stdlib.h"
//#include "converter.h"
import "C"
import (
	"log"
	"unsafe"
)

func ConvertFromJ2kToPng(j2kData []uint8) []uint8 {
	cJ2KData := (*C.uchar)(unsafe.Pointer(&j2kData[0]))

	var pngSize C.size_t

	// Call C function to convert J2K to PNG
	cPNGData := C.convert_j2k_to_png(cJ2KData, C.size_t(len(j2kData)), &pngSize)
	if cPNGData == nil {
		log.Fatalf("conversion failed")
		return nil
	}
	defer C.free(unsafe.Pointer(cPNGData))

	// Convert C char pointer to Go byte slice
	pngData := C.GoBytes(unsafe.Pointer(cPNGData), C.int(pngSize))

	log.Println("Conversion successful")

	return pngData
}
