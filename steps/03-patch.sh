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
  # Auto-detect patch format: -p0 for patches without a/b prefix, -p1 for patches with a/b prefix
  local PLEVEL=1
  # Check if first "---" line does NOT start with "--- a/" or "--- b/" (i.e., no a/b prefix)
  if head -n 10 "$FILE" 2>/dev/null | grep -qE "^--- [^/ab]"; then
    PLEVEL=0
  fi
  patch --verbose -p"$PLEVEL" -d "$DIR" -i "$FILE"
}

pushd "${SOURCE}"

[ "$BUILD_TYPE" == "shared" ] && [ "$OS" != "emscripten" ] && apply_patch "$PATCHES/shared_library.patch"
apply_patch "$PATCHES/common/implementation_config.patch"
apply_patch "$PATCHES/public_headers.patch"

[ "$ENABLE_V8" == "true" ] && apply_patch "$PATCHES/v8/pdfium.patch"

case "$OS" in
  android)
    apply_patch "$PATCHES/common/fpdfsdk.patch"
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
