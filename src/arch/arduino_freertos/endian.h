/**
 * @file endian.h
 * @brief Endianness conversion functions for Arduino FreeRTOS port.
 * 
 * endian.h is not a standard C header, but is provided by the compiler includes.
 * However on the arduino platform this directory also has other include files
 * such as strings.h that are not needed.
 * The compiler directory is on the include path with the  arduino_freertos tool chain,
 * so using <include/sys/endian.h> will just load the one include file we need.
 * The file path ./contrib/arduino_freertos is then added to the include path when
 * building libcsp.
 */
#include <include/sys/endian.h>