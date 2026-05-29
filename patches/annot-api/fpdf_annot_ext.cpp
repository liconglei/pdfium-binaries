
// ============================================================================
// Annotation Dictionary Extension API Implementation
// Supports dot-notation path access to nested dictionaries
// ============================================================================

namespace {

// Parse dot-notation path and resolve to target object.
// Returns the target object, or nullptr if not found.
// If parentDict is provided, returns the parent dictionary.
// If lastKey is provided, returns the final key name.
const CPDF_Object* ResolveKeyPath(const CPDF_Dictionary* dict,
                                   const char* keyPath,
                                   CPDF_Dictionary** parentDict,
                                   ByteString* lastKey) {
  if (!dict || !keyPath || *keyPath == '\0') {
    return nullptr;
  }

  ByteString path(keyPath);
  size_t dotPos = path.Find('.');
  
  if (dotPos == ByteString::kNpos) {
    // No dot - direct key access
    if (parentDict) {
      *parentDict = const_cast<CPDF_Dictionary*>(dict);
    }
    if (lastKey) {
      *lastKey = path;
    }
    return dict->GetObjectFor(path);
  }

  // Split at first dot
  ByteString firstKey = path.Left(dotPos);
  ByteString remainingPath = path.Mid(dotPos + 1, path.GetLength() - dotPos - 1);

  // Get the nested dictionary
  RetainPtr<const CPDF_Dictionary> nestedDict = dict->GetDictFor(firstKey);
  if (!nestedDict) {
    return nullptr;
  }

  // Recursively resolve remaining path
  return ResolveKeyPath(nestedDict.Get(), remainingPath.c_str(), parentDict, lastKey);
}

// Parse dot-notation path and create intermediate dictionaries as needed.
// Returns the parent dictionary where the final key should be set.
// If lastKey is provided, returns the final key name.
CPDF_Dictionary* ResolveOrCreateKeyPath(CPDF_Dictionary* dict,
                                         const char* keyPath,
                                         ByteString* lastKey) {
  if (!dict || !keyPath || *keyPath == '\0') {
    return nullptr;
  }

  ByteString path(keyPath);
  size_t dotPos = path.Find('.');
  
  if (dotPos == ByteString::kNpos) {
    // No dot - this is the final key
    if (lastKey) {
      *lastKey = path;
    }
    return dict;
  }

  // Split at first dot
  ByteString firstKey = path.Left(dotPos);
  ByteString remainingPath = path.Mid(dotPos + 1, path.GetLength() - dotPos - 1);

  // Get or create the nested dictionary
  RetainPtr<CPDF_Dictionary> nestedDict = dict->GetMutableDictFor(firstKey);
  if (!nestedDict) {
    // Create a new dictionary at this key
    nestedDict = dict->SetNewFor<CPDF_Dictionary>(firstKey);
  }

  // Recursively resolve remaining path
  return ResolveOrCreateKeyPath(nestedDict.Get(), remainingPath.c_str(), lastKey);
}

}  // namespace

// ============================================================================
// Section 1: Basic Dictionary Operations (4 APIs)
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_HasKeyEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  CPDF_Dictionary* parentDict = nullptr;
  ByteString lastKey;
  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, &parentDict, &lastKey);
  return obj != nullptr;
}

FPDF_EXPORT FPDF_OBJECT_TYPE FPDF_CALLCONV
FPDFAnnot_GetValueTypeEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return FPDF_OBJECT_UNKNOWN;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj) {
    return FPDF_OBJECT_UNKNOWN;
  }

  return static_cast<FPDF_OBJECT_TYPE>(obj->GetType());
}

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetStringValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           FPDF_WCHAR* buffer,
                           unsigned long buflen) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return 0;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj) {
    return 0;
  }

  WideString wide_value;
  if (obj->IsString()) {
    wide_value = obj->GetUnicodeText();
  } else if (obj->IsName()) {
    ByteString name_str = obj->GetString();
    wide_value = WideString::FromUTF8(name_str.AsStringView());
  } else {
    return 0;
  }

  return Utf16EncodeMaybeCopyAndReturnLength(
      wide_value, UNSAFE_BUFFERS(SpanFromFPDFApiArgs(buffer, buflen)));
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetStringValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           FPDF_WIDESTRING value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !value) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  WideString wide_value = WideStringFromFPDFWideString(value);
  parentDict->SetNewFor<CPDF_String>(lastKey, wide_value.AsStringView());
  return true;
}

// ============================================================================
// Section 2: Number Value Operations (2 APIs)
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_GetNumberValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           float* value) {
  if (!value) {
    return false;
  }

  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsNumber()) {
    return false;
  }

  *value = obj->GetNumber();
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNumberValueEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           float value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  parentDict->SetNewFor<CPDF_Number>(lastKey, value);
  return true;
}

// ============================================================================
// Section 3: Boolean Value Operations (2 APIs)
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_GetBooleanValueEx(FPDF_ANNOTATION annot,
                            FPDF_BYTESTRING key,
                            FPDF_BOOL* value) {
  if (!value) {
    return false;
  }

  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsBoolean()) {
    return false;
  }

  *value = obj->GetInteger() != 0;
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetBooleanValueEx(FPDF_ANNOTATION annot,
                            FPDF_BYTESTRING key,
                            FPDF_BOOL value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  parentDict->SetNewFor<CPDF_Boolean>(lastKey, value != 0);
  return true;
}

// ============================================================================
// Section 4: Name Value Operations (2 APIs)
// ============================================================================

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNameValueEx(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         FPDF_WCHAR* buffer,
                         unsigned long buflen) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return 0;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsName()) {
    return 0;
  }

  ByteString name_str = obj->GetString();
  WideString wide_value = WideString::FromUTF8(name_str.AsStringView());

  return Utf16EncodeMaybeCopyAndReturnLength(
      wide_value, UNSAFE_BUFFERS(SpanFromFPDFApiArgs(buffer, buflen)));
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameValueEx(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         FPDF_BYTESTRING value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !value) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  parentDict->SetNewFor<CPDF_Name>(lastKey, value);
  return true;
}

// ============================================================================
// Section 5: Null and Remove Operations (2 APIs)
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNullValueEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  parentDict->SetNewFor<CPDF_Null>(lastKey);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_RemoveKeyEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  parentDict->RemoveFor(lastKey);
  return true;
}

// ============================================================================
// Section 6: Number Array Operations (3 APIs)
// ============================================================================

FPDF_EXPORT int FPDF_CALLCONV
FPDFAnnot_GetNumberArrayCountEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return -1;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsArray()) {
    return -1;
  }

  RetainPtr<const CPDF_Array> array = obj->AsArray();
  return static_cast<int>(array->size());
}

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNumberArrayEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           float* buffer,
                           unsigned long buflen) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return 0;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsArray()) {
    return 0;
  }

  RetainPtr<const CPDF_Array> array = obj->AsArray();
  size_t count = array->size();

  if (!buffer || buflen == 0) {
    return static_cast<unsigned long>(count * sizeof(float));
  }

  unsigned long copy_count = (buflen < static_cast<unsigned long>(count * sizeof(float)))
      ? buflen / sizeof(float) : static_cast<unsigned long>(count);

  for (unsigned long i = 0; i < copy_count; i++) {
    buffer[i] = array->GetFloatAt(static_cast<size_t>(i));
  }

  return static_cast<unsigned long>(copy_count * sizeof(float));
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetFloatArrayEx(FPDF_ANNOTATION annot,
                          FPDF_BYTESTRING key,
                          const float* values,
                          size_t count) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !values || count == 0) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  RetainPtr<CPDF_Array> array = parentDict->SetNewFor<CPDF_Array>(lastKey);
  for (size_t i = 0; i < count; i++) {
    array->AppendNew<CPDF_Number>(values[i]);
  }

  return true;
}

// ============================================================================
// Section 7: Name Array Operations (3 APIs)
// ============================================================================

FPDF_EXPORT int FPDF_CALLCONV
FPDFAnnot_GetNameArrayCountEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return -1;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsArray()) {
    return -1;
  }

  RetainPtr<const CPDF_Array> array = obj->AsArray();
  return static_cast<int>(array->size());
}

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNameArrayElementEx(FPDF_ANNOTATION annot,
                                FPDF_BYTESTRING key,
                                int index,
                                FPDF_WCHAR* buffer,
                                unsigned long buflen) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return 0;
  }

  const CPDF_Object* obj = ResolveKeyPath(annot_dict, key, nullptr, nullptr);
  if (!obj || !obj->IsArray()) {
    return 0;
  }

  RetainPtr<const CPDF_Array> array = obj->AsArray();
  if (index < 0 || static_cast<size_t>(index) >= array->size()) {
    return 0;
  }

  ByteString name_str = array->GetByteStringAt(static_cast<size_t>(index));
  WideString wide_value = WideString::FromUTF8(name_str.AsStringView());

  return Utf16EncodeMaybeCopyAndReturnLength(
      wide_value, UNSAFE_BUFFERS(SpanFromFPDFApiArgs(buffer, buflen)));
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameArrayEx(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         const FPDF_BYTESTRING* values,
                         size_t count) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !values || count == 0) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  RetainPtr<CPDF_Array> array = parentDict->SetNewFor<CPDF_Array>(lastKey);
  for (size_t i = 0; i < count; i++) {
    if (!values[i]) {
      return false;
    }
    array->AppendNew<CPDF_Name>(values[i]);
  }

  return true;
}

// ============================================================================
// Section 8: Reference Operations (2 APIs)
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefValueEx(FPDF_ANNOTATION annot,
                        FPDF_BYTESTRING key,
                        FPDF_ANNOTATION targetAnnot) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  RetainPtr<CPDF_Dictionary> target_dict =
      GetMutableAnnotDictFromFPDFAnnotation(targetAnnot);

  if (!annot_dict || !key || !target_dict) {
    return false;
  }

  CPDF_AnnotContext* pAnnotContext = CPDFAnnotContextFromFPDFAnnotation(annot);
  if (!pAnnotContext) {
    return false;
  }

  CPDF_Document* doc = pAnnotContext->GetPage()->GetDocument();
  if (!doc) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  uint32_t obj_num = target_dict->GetObjNum();
  if (obj_num == 0) {
    obj_num = doc->AddIndirectObject(target_dict);
  }

  parentDict->SetNewFor<CPDF_Reference>(lastKey, doc, obj_num);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefByObjNumEx(FPDF_ANNOTATION annot,
                           FPDF_BYTESTRING key,
                           int objNum,
                           int genNum) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  CPDF_AnnotContext* pAnnotContext = CPDFAnnotContextFromFPDFAnnotation(annot);
  if (!pAnnotContext) {
    return false;
  }

  CPDF_Document* doc = pAnnotContext->GetPage()->GetDocument();
  if (!doc) {
    return false;
  }

  RetainPtr<const CPDF_Object> target_obj =
      doc->GetIndirectObject(static_cast<uint32_t>(objNum));
  if (!target_obj) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  parentDict->SetNewFor<CPDF_Reference>(lastKey, doc,
                                        static_cast<uint32_t>(objNum));
  return true;
}

// ============================================================================
// Section 9: Dictionary Creation Operations (1 API)
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetDictValueEx(FPDF_ANNOTATION annot, FPDF_BYTESTRING key) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  ByteString lastKey;
  CPDF_Dictionary* parentDict = ResolveOrCreateKeyPath(annot_dict.Get(), key, &lastKey);
  if (!parentDict || lastKey.IsEmpty()) {
    return false;
  }

  // If lastKey is not empty, we need to create a dictionary at lastKey
  // But if there's no dot in the path, ResolveOrCreateKeyPath returns the parent
  // So we need to create the dictionary at lastKey
  if (!lastKey.IsEmpty()) {
    parentDict->SetNewFor<CPDF_Dictionary>(lastKey);
  }
  return true;
}

// ============================================================================
// Section 10: Debug/Introspection Operations (1 API)
// ============================================================================

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetDictKeys(FPDF_ANNOTATION annot,
                      FPDF_WCHAR* buffer,
                      unsigned long buflen) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict) {
    return 0;
  }

  std::vector<ByteString> keys = annot_dict->GetKeys();
  if (keys.empty()) {
    return 2;  // Just null terminator
  }

  // Build comma-separated string
  WideString result;
  for (size_t i = 0; i < keys.size(); i++) {
    if (i > 0) {
      result += L", ";
    }
    result += WideString::FromUTF8(keys[i].AsStringView());
  }

  return Utf16EncodeMaybeCopyAndReturnLength(
      result, UNSAFE_BUFFERS(SpanFromFPDFApiArgs(buffer, buflen)));
}
