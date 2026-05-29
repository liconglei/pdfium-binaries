// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PUBLIC_FPDF_ANNOT_EXT_H_
#define PUBLIC_FPDF_ANNOT_EXT_H_

#include "fpdf_annot.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Annotation Dictionary Extension API
// 
// This header provides 25 extended APIs for complete annotation dictionary
// manipulation, supporting dot-notation path access to nested dictionaries.
//
// Key Features:
//   - Dot-notation path support: "BS.W", "AP.N", "A.URI"
//   - Complete PDF 32000 standard annotation property coverage
//   - No modification to standard PDFium APIs
//
// Path Examples:
//   "BS"         -> Direct key access (Border Style dictionary)
//   "BS.W"       -> Nested key access (Border Width)
//   "BS.S"       -> Nested key access (Border Style: Solid/Dashed/Dotted)
//   "BS.D"       -> Nested key access (Dash pattern array)
//   "AP.N"       -> Appearance dictionary -> Normal appearance
//   "A.URI"      -> Action dictionary -> URI
// ============================================================================

// ----------------------------------------------------------------------------
// Section 1: Basic Dictionary Operations (4 APIs)
// ----------------------------------------------------------------------------

// Check if the key exists in the annotation's dictionary.
// Supports dot-notation path for nested dictionary access.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_HasKeyEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// Get the type of the value corresponding to the key.
// Returns FPDF_OBJECT_* type or FPDF_OBJECT_UNKNOWN on error.
FPDF_EXPORT FPDF_OBJECT_TYPE FPDF_CALLCONV
FPDFAnnot_GetValueTypeEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// Get the string value corresponding to the key.
// Returns the length of the string value in bytes including null terminator.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetStringValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           FPDF_WCHAR* buffer,
                           unsigned long buflen);

// Set the string value corresponding to the key.
// Automatically creates intermediate dictionaries if they don't exist.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetStringValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           FPDF_WIDESTRING value);

// ----------------------------------------------------------------------------
// Section 2: Number Value Operations (2 APIs)
// ----------------------------------------------------------------------------

// Get the float value corresponding to the key.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_GetNumberValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           float* value);

// Set the float value corresponding to the key.
// Automatically creates intermediate dictionaries if they don't exist.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNumberValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           float value);

// ----------------------------------------------------------------------------
// Section 3: Boolean Value Operations (2 APIs)
// ----------------------------------------------------------------------------

// Get the boolean value corresponding to the key.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_GetBooleanValueEx(FPDF_ANNOTATION annot,
                            FPDF_BYTESTRING key,
                            FPDF_BOOL* value);

// Set the boolean value corresponding to the key.
// Automatically creates intermediate dictionaries if they don't exist.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetBooleanValueEx(FPDF_ANNOTATION annot,
                            FPDF_BYTESTRING key,
                            FPDF_BOOL value);

// ----------------------------------------------------------------------------
// Section 4: Name Value Operations (2 APIs)
// ----------------------------------------------------------------------------

// Get the name value corresponding to the key.
// Returns the length of the name string in bytes including null terminator.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNameValueEx(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         FPDF_WCHAR* buffer,
                         unsigned long buflen);

// Set the name value corresponding to the key.
// Automatically creates intermediate dictionaries if they don't exist.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameValueEx(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         FPDF_BYTESTRING value);

// ----------------------------------------------------------------------------
// Section 5: Null and Remove Operations (2 APIs)
// ----------------------------------------------------------------------------

// Set the value corresponding to the key to null.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNullValueEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// Remove the key from the annotation's dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_RemoveKeyEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// ----------------------------------------------------------------------------
// Section 6: Number Array Operations (3 APIs)
// ----------------------------------------------------------------------------

// Get the count of elements in a number array.
// Returns -1 on error.
FPDF_EXPORT int FPDF_CALLCONV
FPDFAnnot_GetNumberArrayCountEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// Get the number array values.
// Returns the number of bytes written or required buffer size.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNumberArrayEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           float* buffer,
                           unsigned long buflen);

// Set the number array corresponding to the key.
// Automatically creates intermediate dictionaries if they don't exist.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetFloatArrayEx(FPDF_ANNOTATION annot,
                          FPDF_BYTESTRING key,
                          const float* values,
                          size_t count);

// ----------------------------------------------------------------------------
// Section 7: Name Array Operations (3 APIs)
// ----------------------------------------------------------------------------

// Get the count of elements in a name array.
// Returns -1 on error.
FPDF_EXPORT int FPDF_CALLCONV
FPDFAnnot_GetNameArrayCountEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// Get a name array element at the specified index.
// Returns the length of the name string in bytes including null terminator.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNameArrayElementEx(FPDF_ANNOTATION annot,
                                FPDF_BYTESTRING key,
                                int index,
                                FPDF_WCHAR* buffer,
                                unsigned long buflen);

// Set the name array corresponding to the key.
// Automatically creates intermediate dictionaries if they don't exist.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameArrayEx(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         const FPDF_BYTESTRING* values,
                         size_t count);

// ----------------------------------------------------------------------------
// Section 8: Reference Operations (2 APIs)
// ----------------------------------------------------------------------------

// Set an indirect reference to another annotation.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefValueEx(FPDF_ANNOTATION annot,
                        FPDF_BYTESTRING key,
                        FPDF_ANNOTATION targetAnnot);

// Set an indirect reference by object number.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefByObjNumEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           int objNum,
                           int genNum);

// ----------------------------------------------------------------------------
// Section 9: Dictionary Creation Operations (1 API)
// ----------------------------------------------------------------------------

// Create an empty nested dictionary at the specified key path.
// Automatically creates all intermediate dictionaries.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetDictValueEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key);

// ----------------------------------------------------------------------------
// Section 10: Debug/Introspection Operations (1 API)
// ----------------------------------------------------------------------------

// Get a list of all keys in the annotation's dictionary.
// Returns the length of the keys string in bytes including null terminator.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetDictKeys(FPDF_ANNOTATION annot,
                      FPDF_WCHAR* buffer,
                      unsigned long buflen);

#ifdef __cplusplus
}
#endif

#endif  // PUBLIC_FPDF_ANNOT_EXT_H_
