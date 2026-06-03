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
# 23 Ex APIs with dot-notation path support + AP stream generation

# Create the extension header file
cp "$PATCHES/annot-api/fpdf_annot_ext.h" "public/fpdf_annot_ext.h"

# Insert include right before the closing brace of extern "C" block
# Match "}  // extern "C"" pattern
perl -i -pe 'print "#include \"fpdf_annot_ext.h\"\n" if /^\}\s*\/\/\s*extern\s+"C"/' "public/fpdf_annot.h"

# Append implementation code to fpdf_annot.cpp
cat "$PATCHES/annot-api/fpdf_annot_ext.cpp" >> "fpdfsdk/fpdf_annot.cpp"

# Add required includes to fpdf_annot.cpp
sed -i '/#include "core\/fpdfapi\/parser\/cpdf_dictionary.h"/a #include "core/fpdfapi/parser/cpdf_null.h"' "fpdfsdk/fpdf_annot.cpp"

# Add includes for AP stream generation
sed -i '/#include "core\/fpdfapi\/page\/cpdf_annotcontext.h"/a #include "core/fpdfdoc/cpdf_generateap.h"' "fpdfsdk/fpdf_annot.cpp"
sed -i '/#include "core\/fpdfapi\/page\/cpdf_annotcontext.h"/a #include "core/fpdfdoc/cpdf_interactiveform.h"' "fpdfsdk/fpdf_annot.cpp"

# Add includes for Font API
sed -i '/#include "core\/fpdfapi\/page\/cpdf_annotcontext.h"/a #include "core/fpdfapi/font/cpdf_font.h"' "fpdfsdk/fpdf_annot.cpp"

# Fix FreeText multi-line support: add SetMultiLine and SetAutoReturn before vt.Initialize()
perl -i -pe 'print "  vt.SetMultiLine(true);\n  vt.SetAutoReturn(true);\n" if /vt\.SetAlignment.*GetIntegerFor.*"Q"/' "core/fpdfdoc/cpdf_generateap.cpp"

# Fix FreeText character spacing (Tc) support: parse Tc from DA and write to content stream
perl -i -0777 -pe "$(cat $PATCHES/annot-api/freetext-tc.pl)" "core/fpdfdoc/cpdf_generateap.cpp"

# Modify FPDFAnnot_IsSupportedSubtype to support LINE, POLYGON, POLYLINE
perl -i -pe 'print "    case FPDF_ANNOT_LINE:\n    case FPDF_ANNOT_POLYGON:\n    case FPDF_ANNOT_POLYLINE:\n" if /case FPDF_ANNOT_UNDERLINE:/' "fpdfsdk/fpdf_annot.cpp"

# Modify FPDFAnnot_IsObjectSupportedSubtype to support LINE, POLYGON, POLYLINE
perl -i -pe 's/^  return subtype == FPDF_ANNOT_INK \|\| subtype == FPDF_ANNOT_STAMP;$/  return subtype == FPDF_ANNOT_INK || subtype == FPDF_ANNOT_STAMP ||\n      subtype == FPDF_ANNOT_LINE || subtype == FPDF_ANNOT_POLYGON ||\n      subtype == FPDF_ANNOT_POLYLINE;/' "fpdfsdk/fpdf_annot.cpp"

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
