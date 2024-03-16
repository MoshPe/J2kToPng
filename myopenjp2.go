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

func main() {}

//func main() {
//	jp2Path := "/home/mosheper/GolandProjects/J2kToJpeg-Wasm/test/out_j2k.j2k"
//	pngPath := "/home/mosheper/GolandProjects/J2kToJpeg-Wasm/test/out_png.png"
//	// Read JP2 file as bytes
//	j2kData, err := os.ReadFile(jp2Path)
//	if err != nil {
//		log.Fatalf("Error reading j2k file: %v", err)
//	}
//
//	cJ2KData := (*C.uchar)(unsafe.Pointer(&j2kData[0]))
//
//	var pngSize C.size_t
//
//	// Call C function to convert J2K to PNG
//	cPNGData := C.convert_j2k_to_png(cJ2KData, C.size_t(len(j2kData)), &pngSize)
//	if cPNGData == nil {
//		fmt.Errorf("conversion failed")
//		return
//	}
//	defer C.free(unsafe.Pointer(cPNGData))
//
//	// Convert C char pointer to Go byte slice
//	pngData := C.GoBytes(unsafe.Pointer(cPNGData), C.int(pngSize))
//
//	err = os.WriteFile(pngPath, pngData, 0644)
//	if err != nil {
//		log.Fatalf("error writing PNG file: %v", err)
//		return
//	}
//
//	fmt.Println("Conversion successful")
//}
