# PDFium Source Modifications - chromium/7865 Branch

## Overview

This document describes all modifications made to PDFium source code in the `feature/annot-dict-api` branch, targeting the stable branch `chromium/7865` (version 150.0.7865.0).

## Build Verification Status

| Platform | Architecture | Status | Build Run |
|----------|--------------|--------|-----------|
| Android | x64 | **PASS** | [Run #26859519694](https://github.com/liconglei/pdfium-binaries/actions/runs/26859519694) |
| Android | arm64 | Pending | - |

**Verified**: 2026-06-03

## Summary of Changes

### 1. New Public Header: `fpdf_annot_ext.h`

Location: `public/fpdf_annot_ext.h`

Added 24 extended APIs for complete annotation dictionary manipulation:

| Section | APIs | Purpose |
|---------|------|---------|
| 1 | 4 | Basic dictionary operations (HasKey, ValueType, StringValue get/set) |
| 2 | 2 | Number value operations |
| 3 | 2 | Boolean value operations |
| 4 | 2 | Name value operations |
| 5 | 2 | Null and remove operations |
| 6 | 3 | Number array operations |
| 7 | 3 | Name array operations |
| 8 | 2 | Reference operations |
| 9 | 1 | Dictionary creation |
| 10 | 1 | Debug/introspection (GetDictKeys) |
| 11 | 1 | Appearance stream generation (GenerateAPEx) |
| 12 | 1 | Font object number access (GetObjNumEx) |

**Key Feature**: Dot-notation path support for nested dictionaries:
- `"BS"` → Direct key (Border Style)
- `"BS.W"` → Nested (Border Width)
- `"AP.N"` → Appearance → Normal
- `"A.URI"` → Action → URI

### 2. Implementation: `fpdf_annot_ext.cpp`

Location: `fpdfsdk/fpdf_annot.cpp` (appended)

Implementation details:
- Helper function `ResolveKeyPath()` - parses dot-path and resolves target object
- Helper function `ResolveOrCreateKeyPath()` - creates intermediate dictionaries
- All 24 Ex APIs implemented with proper null-checking and error handling

### 3. Core Bug Fixes: `cpdf_generateap.cpp`

Location: `core/fpdfdoc/cpdf_generateap.cpp`

#### 3.1 FreeText Multi-line Support Fix

**Problem**: Original `GenerateFreeTextAP` does not set `SetMultiLine(true)` and `SetAutoReturn(true)` on `CPVT_VariableText`, causing multi-line text in `/Contents` to render as single line.

**Fix**: Insert before `vt.Initialize()`:
```cpp
vt.SetMultiLine(true);
vt.SetAutoReturn(true);
```

**Patch Script**: `perl -i -pe 'print "  vt.SetMultiLine(true);\n  vt.SetAutoReturn(true);\n" if /vt\.SetAlignment.*GetIntegerFor.*"Q"/'`

**Location**: Line matching `vt.SetAlignment(...GetIntegerFor..."Q"...)`

#### 3.2 FreeText Character Spacing (Tc) Fix

**Problem**: `CPDF_DefaultAppearance` only parses font name, size, and color from `/DA` string. Character spacing (`Tc` operator) in DA is ignored, causing incorrect text rendering.

**Fix**: Parse `Tc` from DA string and write to content stream after `BT`:
```cpp
ByteString da_str = annot_dict->GetByteStringFor("DA");
std::optional<size_t> tc_pos = da_str.Find(" Tc");
if (tc_pos.has_value()) {
  // Parse Tc value and write to stream
  appearance_stream << tc_val << " Tc\n";
}
```

**Patch Script**: `patches/annot-api/freetext-tc.pl` (perl -0777 slurp mode)

**Location**: After `appearance_stream << "BT\n"` line followed by `GenerateColorAP`

**Note**: Uses `GetByteStringFor()` instead of `GetStringFor()` because chromium/7865's `GetStringFor()` returns `RetainPtr<const CPDF_String>`.

### 4. Annotation Subtype Support Extension

Location: `fpdfsdk/fpdf_annot.cpp`

#### 4.1 `FPDFAnnot_IsSupportedSubtype`

Added support for LINE, POLYGON, POLYLINE:
```cpp
case FPDF_ANNOT_LINE:
case FPDF_ANNOT_POLYGON:
case FPDF_ANNOT_POLYLINE:
```

**Patch**: Insert before `case FPDF_ANNOT_UNDERLINE:`

#### 4.2 `FPDFAnnot_IsObjectSupportedSubtype`

Extended return statement:
```cpp
return subtype == FPDF_ANNOT_INK || subtype == FPDF_ANNOT_STAMP ||
    subtype == FPDF_ANNOT_LINE || subtype == FPDF_ANNOT_POLYGON ||
    subtype == FPDF_ANNOT_POLYLINE;
```

### 5. Include Additions

Location: `fpdfsdk/fpdf_annot.cpp`

Added includes:
```cpp
#include "core/fpdfapi/parser/cpdf_null.h"
#include "core/fpdfdoc/cpdf_generateap.h"
#include "core/fpdfdoc/cpdf_interactiveform.h"
#include "core/fpdfapi/font/cpdf_font.h"
```

Location: `public/fpdf_annot.h`

Added include before closing `extern "C"`:
```cpp
#include "fpdf_annot_ext.h"
```

## chromium/7865 API Compatibility Notes

The following API differences from newer branches were identified and handled:

| API | chromium/7865 Behavior | Fix Applied |
|-----|------------------------|-------------|
| `ByteString::Find()` | Returns `std::optional<size_t>` | Use `.has_value()` and `.value()` |
| `CPDF_Dictionary::SetNewFor()` | Key requires `const ByteString&` | Use `ByteString(key)` conversion |
| `CPDF_Dictionary::GetDictFor()` | Requires `ByteStringView` | Use `ByteStringView(key)` |
| `CPDF_Dictionary::GetStringFor()` | Returns `RetainPtr<const CPDF_String>` | Use `GetByteStringFor()` instead |
| `CPDF_Dictionary::GetObjectFor()` | Takes `ByteStringView` | Direct use OK |

## Build Configuration

```bash
gh workflow run build-one.yml \
  -r feature/annot-dict-api \
  -f target_os=android \
  -f target_cpu=x64 \
  -f version=150.0.7865.0 \
  -f branch=chromium/7865 \
  -f is_debug=false \
  -f enable_v8=false
```

## Output Artifacts

| File | Location | Size |
|------|----------|------|
| `libpdfium.so` | `pdfium-android-x64/lib/` | ~6.5 MB |
| `fpdf_annot_ext.h` | `pdfium-android-x64/include/` | 11 KB |

## Verification Checklist

- [x] Version: 150.0.7865.0 confirmed in VERSION file
- [x] Header: `fpdf_annot_ext.h` contains 24 API declarations
- [x] Symbols: `FPDFAnnot_HasKeyEx`, `FPDFAnnot_GenerateAPEx`, `FPDFFont_GetObjNumEx` present in SO
- [x] Build: No compilation errors
- [x] Target: Android x64 shared library

## Files Modified

| File | Change Type |
|------|-------------|
| `public/fpdf_annot.h` | Include added |
| `public/fpdf_annot_ext.h` | New file |
| `fpdfsdk/fpdf_annot.cpp` | Implementation appended, includes added |
| `core/fpdfdoc/cpdf_generateap.cpp` | Multi-line fix, Tc fix |

## Related Files (Patch Scripts)

| File | Purpose |
|------|---------|
| `steps/03-patch.sh` | Main patch script (lines 30-63) |
| `patches/annot-api/fpdf_annot_ext.h` | Header template |
| `patches/annot-api/fpdf_annot_ext.cpp` | Implementation template |
| `patches/annot-api/freetext-tc.pl` | Tc fix perl script |

## Future Work

1. Build arm64 architecture for Android
2. Consider upstreaming multi-line and Tc fixes to PDFium
3. Add unit tests for Ex APIs
4. Evaluate upgrade to newer stable branch (7871+) if needed

## References

- PDFium Source: https://github.com/lukas-w/pdfium (branch chromium/7865)
- PDF 32000-1:2008 Standard (PDF Reference)
- PDFium Public API: `public/fpdf_annot.h`