# Fix FreeText character spacing (Tc) support in GenerateFreeTextAP
# perl script: insert Tc parsing code after the BT line that's followed by GenerateColorAP
# This uses slurp mode (-0777) to match across lines
s{(appearance_stream << "BT\\n"\n\s+<< GenerateColorAP)}{  ByteString da_str = annot_dict->GetByteStringFor("DA");
  std::optional<size_t> tc_pos = da_str.Find(" Tc");
  if (tc_pos.has_value()) {
    size_t end = tc_pos.value();
    size_t start = 0;
    ByteStringView da_view = da_str.AsStringView();
    for (size_t i = end; i > 0; --i) {
      if (da_view[i - 1] == ' ') {
        start = i;
        break;
      }
    }
    if (end > start) {
      ByteString tc_num = ByteString(da_view.Substr(start, end - start));
      float tc_val = atof(tc_num.c_str());
      appearance_stream << tc_val << " Tc\\n";
    }
  }
$1}
