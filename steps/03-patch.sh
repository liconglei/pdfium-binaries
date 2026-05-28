#!/bin/bash -eux

PATCHES="$PWD/patches"
SOURCE="${PDFium_SOURCE_DIR:-pdfium}"
OS="${PDFium_TARGET_OS:?}"
TARGET_CPU="${PDFium_TARGET_CPU:?}"
TARGET_ENVIRONMENT="${PDFium_TARGET_ENVIRONMENT:-}"
ENABLE_V8=${PDFium_ENABLE_V8:-false}
BUILD_TYPE=${PDFium_BUILD_TYPE:-shared}

apply_patch() {
  local FILE="$1"
  local DIR="${2:-.}"
  patch --verbose -p1 -d "$DIR" -i "$FILE"
}

pushd "${SOURCE}"

case "$BUILD_TYPE" in
  shared)
    [ "$OS" != "emscripten" ] && apply_patch "$PATCHES/shared_library.patch"
    ;;
  static)
    apply_patch "$PATCHES/static_library.patch"
    ;;
esac

apply_patch "$PATCHES/public_headers.patch"

# Apply annotation dictionary API extension
# Insert API declarations into fpdf_annot.h inside the extern "C" block
# Find the last function (FPDFAnnot_AddFileAttachment) and insert after its declaration

# Create the extension header file with new APIs (wrapped in extern "C")
cat > "$SOURCE/public/fpdf_annot_ext.h" << 'EOF'
// ============================================================================
// Annotation Dictionary Operations API Extension
// Provides 11 APIs for complete annotation dictionary manipulation
// ============================================================================

// Set boolean value in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetBooleanValue(FPDF_ANNOTATION annot,
                          FPDF_BYTESTRING key,
                          FPDF_BOOL value);

// Get boolean value from annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_GetBooleanValue(FPDF_ANNOTATION annot,
                          FPDF_BYTESTRING key,
                          FPDF_BOOL* value);

// Set number value in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNumberValue(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         float value);

// Set name value in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameValue(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key,
                       FPDF_BYTESTRING value);

// Get name value from annotation dictionary.
FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNameValue(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key,
                       FPDF_WCHAR* buffer,
                       unsigned long buflen);

// Set null value in annotation dictionary (key remains, value is null).
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNullValue(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key);

// Remove key from annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_RemoveKey(FPDF_ANNOTATION annot,
                    FPDF_BYTESTRING key);

// Set float array in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetFloatArray(FPDF_ANNOTATION annot,
                        FPDF_BYTESTRING key,
                        const float* values,
                        size_t count);

// Set name array in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameArray(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key,
                       const FPDF_BYTESTRING* values,
                       size_t count);

// Set annotation reference in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefValue(FPDF_ANNOTATION annot,
                      FPDF_BYTESTRING key,
                      FPDF_ANNOTATION targetAnnot);

// Set object reference by object number in annotation dictionary.
FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefByObjNum(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         int objNum,
                         int genNum);
EOF

# Insert include right before the closing brace of extern "C" block
# Use Perl for more reliable multiline matching (sed has issues with line endings)
perl -i -pe 'print "#include \"fpdf_annot_ext.h\"\n" if /^}\s*\/\/\s*extern\s+"C"/' "$SOURCE/public/fpdf_annot.h"

# Append implementation code to fpdf_annot.cpp
cat "$PATCHES/annot-api/fpdf_annot_append.cpp" >> "$SOURCE/fpdfsdk/fpdf_annot.cpp"

# Add cpdf_null.h include to fpdf_annot.cpp
sed -i '/#include "core\/fpdfapi\/parser\/cpdf_dictionary.h"/a #include "core/fpdfapi/parser/cpdf_null.h"' "$SOURCE/fpdfsdk/fpdf_annot.cpp"

# Modify FPDFAnnot_IsSupportedSubtype to support LINE, POLYGON, POLYLINE
# Insert after FPDF_ANNOT_UNDERLINE case
perl -i -pe 'print "    case FPDF_ANNOT_LINE:\n    case FPDF_ANNOT_POLYGON:\n    case FPDF_ANNOT_POLYLINE:\n" if /case FPDF_ANNOT_UNDERLINE:/' "$SOURCE/fpdfsdk/fpdf_annot.cpp"

# Modify FPDFAnnot_IsObjectSupportedSubtype to support LINE, POLYGON, POLYLINE
# Change the return statement to include these types
perl -i -pe 's/return subtype == FPDF_ANNOT_INK || subtype == FPDF_ANNOT_STAMP;/return subtype == FPDF_ANNOT_INK || subtype == FPDF_ANNOT_STAMP || subtype == FPDF_ANNOT_LINE || subtype == FPDF_ANNOT_POLYGON || subtype == FPDF_ANNOT_POLYLINE;/' "$SOURCE/fpdfsdk/fpdf_annot.cpp"

[ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/v8/pdfium.patch"

case "$OS" in
  android)
    apply_patch "$PATCHES/android/build.patch" build
    ;;

  ios)
    apply_patch "$PATCHES/ios/pdfium.patch"
    [ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/ios/v8.patch" v8
    ;;

  mac)
    apply_patch "$PATCHES/mac/build.patch" build
    ;;

  linux)
    [ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/linux/v8.patch" v8
    ;;

  emscripten)
    apply_patch "$PATCHES/wasm/pdfium.patch"
    apply_patch "$PATCHES/wasm/build.patch" build
    if [ "$ENABLE_V8" == "true" ]; then
      apply_patch "$PATCHES/wasm/v8.patch" v8
    fi
    mkdir -p "build/config/wasm"
    cp "$PATCHES/wasm/config.gn" "build/config/wasm/BUILD.gn"
    ;;

  win)
    apply_patch "$PATCHES/win/build.patch" build

    VERSION=${PDFium_VERSION:-0.0.0.0}
    YEAR=$(date +%Y)
    VERSION_CSV=${VERSION//./,}
    export YEAR VERSION VERSION_CSV
    envsubst < "$PATCHES/win/resources.rc" > "resources.rc"
    ;;
esac

case "$TARGET_ENVIRONMENT" in
  musl)
    apply_patch "$PATCHES/musl/pdfium.patch"
    apply_patch "$PATCHES/musl/build.patch" build
    mkdir -p "build/toolchain/linux/musl"
    cp "$PATCHES/musl/toolchain.gn" "build/toolchain/linux/musl/BUILD.gn"
    ;;
esac

case "$TARGET_CPU" in
  ppc64)
    apply_patch "$PATCHES/ppc64/pdfium.patch"
    apply_patch "$PATCHES/ppc64/build.patch" build
    ;;
esac

popd
