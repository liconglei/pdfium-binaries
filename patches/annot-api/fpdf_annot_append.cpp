

// ============================================================================
// Annotation Dictionary Operations API Implementation
// ============================================================================

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetBooleanValue(FPDF_ANNOTATION annot,
                          FPDF_BYTESTRING key,
                          FPDF_BOOL value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  annot_dict->SetNewFor<CPDF_Boolean>(key, value != 0);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_GetBooleanValue(FPDF_ANNOTATION annot,
                          FPDF_BYTESTRING key,
                          FPDF_BOOL* value) {
  if (!value) {
    return false;
  }

  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  RetainPtr<const CPDF_Object> obj = annot_dict->GetObjectFor(key);
  if (!obj || obj->GetType() != CPDF_Object::Type::kBoolean) {
    return false;
  }

  *value = obj->GetBoolean() ? 1 : 0;
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNumberValue(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         float value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  annot_dict->SetNewFor<CPDF_Number>(key, value);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameValue(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key,
                       FPDF_BYTESTRING value) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !value) {
    return false;
  }

  annot_dict->SetNewFor<CPDF_Name>(key, value);
  return true;
}

FPDF_EXPORT unsigned long FPDF_CALLCONV
FPDFAnnot_GetNameValue(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key,
                       FPDF_WCHAR* buffer,
                       unsigned long buflen) {
  const CPDF_Dictionary* annot_dict = GetAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return 0;
  }

  RetainPtr<const CPDF_Object> obj = annot_dict->GetObjectFor(key);
  if (!obj) {
    return 0;
  }

  CPDF_Object::Type type = obj->GetType();
  if (type != CPDF_Object::Type::kName && type != CPDF_Object::Type::kString) {
    return 0;
  }

  ByteString name_value = obj->GetString();
  WideString wide_value = WideString::FromUTF8(name_value.AsStringView());
  return Utf16EncodeMaybeCopyAndReturnLength(
      wide_value, UNSAFE_BUFFERS(SpanFromFPDFApiArgs(buffer, buflen)));
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNullValue(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  annot_dict->SetNewFor<CPDF_Null>(key);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_RemoveKey(FPDF_ANNOTATION annot,
                    FPDF_BYTESTRING key) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  annot_dict->RemoveFor(key);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetFloatArray(FPDF_ANNOTATION annot,
                        FPDF_BYTESTRING key,
                        const float* values,
                        size_t count) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !values || count == 0) {
    return false;
  }

  RetainPtr<CPDF_Array> array = annot_dict->SetNewFor<CPDF_Array>(key);
  for (size_t i = 0; i < count; i++) {
    array->AppendNew<CPDF_Number>(values[i]);
  }

  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetNameArray(FPDF_ANNOTATION annot,
                       FPDF_BYTESTRING key,
                       const FPDF_BYTESTRING* values,
                       size_t count) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key || !values || count == 0) {
    return false;
  }

  RetainPtr<CPDF_Array> array = annot_dict->SetNewFor<CPDF_Array>(key);
  for (size_t i = 0; i < count; i++) {
    if (!values[i]) {
      return false;
    }
    array->AppendNew<CPDF_Name>(values[i]);
  }

  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefValue(FPDF_ANNOTATION annot,
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
  CPDF_Document* doc = pAnnotContext->GetPage()->GetDocument();
  if (!doc) {
    return false;
  }

  uint32_t obj_num = target_dict->GetObjNum();
  if (obj_num == 0) {
    obj_num = doc->AddIndirectObject(target_dict);
    if (obj_num == 0) {
      return false;
    }
  }

  annot_dict->SetNewFor<CPDF_Reference>(key, doc, obj_num);
  return true;
}

FPDF_EXPORT FPDF_BOOL FPDF_CALLCONV
FPDFAnnot_SetRefByObjNum(FPDF_ANNOTATION annot,
                         FPDF_BYTESTRING key,
                         int objNum,
                         int genNum) {
  RetainPtr<CPDF_Dictionary> annot_dict =
      GetMutableAnnotDictFromFPDFAnnotation(annot);
  if (!annot_dict || !key) {
    return false;
  }

  CPDF_AnnotContext* pAnnotContext = CPDFAnnotContextFromFPDFAnnotation(annot);
  CPDF_Document* doc = pAnnotContext->GetPage()->GetDocument();
  if (!doc) {
    return false;
  }

  RetainPtr<const CPDF_Object> target_obj =
      doc->GetIndirectObject(static_cast<uint32_t>(objNum));
  if (!target_obj) {
    return false;
  }

  annot_dict->SetNewFor<CPDF_Reference>(key, doc,
                                        static_cast<uint32_t>(objNum));
  return true;
}
