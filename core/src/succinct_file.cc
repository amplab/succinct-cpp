#include "succinct_file.h"

SuccinctFile::SuccinctFile(std::string filename, SuccinctMode s_mode)
    : SuccinctCore(filename.c_str(), s_mode) {
  this->input_filename_ = filename;
  this->succinct_filename_ = filename + ".succinct";
}

uint64_t SuccinctFile::ComputeContextValue(const char *p, uint64_t i) {
  uint64_t val = 0;
  uint64_t max = SuccinctUtils::Min((i + npa_->GetContextLength()), input_size_);
  for (uint64_t t = i; t < max; t++) {
    val = val * alphabet_size_ + alphabet_map_[p[t]].second;
  }

  if (max < i + npa_->GetContextLength()) {
    for (uint64_t t = 0; t < (i + npa_->GetContextLength()) % input_size_;
        t++) {
      val = val * alphabet_size_ + alphabet_map_[p[t]].second;
    }
  }

  return val;
}

std::pair<int64_t, int64_t> SuccinctFile::GetRangeSlow(const char *p,
                                                       uint64_t len) {
  std::pair<int64_t, int64_t> range(0, -1);
  uint64_t m = strlen(p);
  int64_t left, right, c1, c2;

  fprintf(stderr, "\nLooking at sigma = %c\n", p[m - 1]);
  if (alphabet_map_.find(p[m - 1]) != alphabet_map_.end()) {
    left = (alphabet_map_[p[m - 1]]).first;
    right = alphabet_map_[alphabet_[alphabet_map_[p[m - 1]].second + 1]].first
        - 1;
    fprintf(stderr, "Left end = %lld\n", left);
    fprintf(stderr, "Right end = %lld\n", right);
  } else {
    fprintf(stderr, "Could not find %c in alphabet_map.\n", p[m - 1]);
    return range;
  }

  for (int64_t i = m - 2; i >= 0; i--) {
    fprintf(stderr, "\nLooking at sigma = %c\n", p[i]);
    if (alphabet_map_.find(p[m - 1]) != alphabet_map_.end()) {
      c1 = alphabet_map_[p[i]].first;
      c2 = alphabet_map_[alphabet_[alphabet_map_[p[i]].second + 1]].first - 1;
      fprintf(stderr, "Left end = %lld\n", c1);
      fprintf(stderr, "Right end = %lld\n", c2);
    } else {
      fprintf(stderr, "Could not find %c in alphabet_map.\n", p[i]);
      return range;
    }

    if (c2 < c1) {
      fprintf(stderr, "Left end was greater than right end.\n");
      return range;
    }

    left = npa_->BinarySearch(left, c1, c2, false);
    right = npa_->BinarySearch(right, left, c2, true);

    fprintf(stderr, "Binary search left end = %lld\n", left);
    fprintf(stderr, "Binary search right end = %lld\n", right);

    if (left > right) {
      fprintf(stderr, "Left end was greater than right end.\n");
      return range;
    }
  }

  range.first = left;
  range.second = right;

  fprintf(stderr, "\nFinal range: (%lld, %lld)", left, right);

  return range;
}


std::pair<int64_t, int64_t> SuccinctFile::GetRange(const char *p,
                                                   uint64_t len) {
  uint64_t m = strlen(p);

  if (m <= npa_->GetContextLength()) {
    return GetRangeSlow(p, len);
  }

  typedef std::pair<int64_t, int64_t> Range;

  Range range(0, -1);
  uint32_t sigma_id;
  int64_t left, right, c1, c2;
  uint64_t start_off;
  uint64_t context_val, context_id;

  // Get the sigma_id and context_id corresponding to the last
  // context and sigma

  fprintf(stderr, "\nLooking at sigma = %c\n", p[m - npa_->GetContextLength() - 1]);

  // Get sigma_id:
  if (alphabet_map_.find(p[m - npa_->GetContextLength() - 1])
      == alphabet_map_.end()) {
    fprintf(stderr, "Could not find %c in alphabet_map.\n", p[m - npa_->GetContextLength() - 1]);
    return range;
  }
  sigma_id = alphabet_map_[p[m - npa_->GetContextLength() - 1]].second;
  fprintf(stderr, "Sigma ID = %u\n", sigma_id);

  // Get context_id:
  context_val = ComputeContextValue(p, m - npa_->GetContextLength());
  fprintf(stderr, "Context value = %llu\n", context_val);
  if (npa_->contexts_.find(context_val) == npa_->contexts_.end()) {
    fprintf(stderr, "Could not find %llu in contexts.\n", context_val);
    return range;
  }
  context_id = npa_->contexts_[context_val];
  fprintf(stderr, "Context ID = %llu\n", context_id);

  // Get start offset:
  start_off = GetRank1(&(npa_->col_nec_[sigma_id]), context_id) - 1;
  fprintf(stderr, "Start offset = %llu\n", start_off);

  // Get left boundary:
  left = npa_->col_offsets_[sigma_id]
      + npa_->cell_offsets_[sigma_id][start_off];
  fprintf(stderr, "Left end = %lld\n", left);

  // Get right boundary:
  if (start_off + 1 < npa_->cell_offsets_[sigma_id].size()) {
    right = npa_->col_offsets_[sigma_id]
        + npa_->cell_offsets_[sigma_id][start_off + 1] - 1;
  } else if ((sigma_id + 1) < alphabet_size_) {
    right = npa_->col_offsets_[sigma_id + 1] - 1;
  } else {
    right = input_size_ - 1;
  }

  fprintf(stderr, "Right end = %lld\n", right);

  if (left > right) {
    fprintf(stderr, "Left end was greater than right end.\n");
    return range;
  }

  for (int64_t i = m - npa_->GetContextLength() - 2; i >= 0; i--) {
    // Get sigma_id:
    fprintf(stderr, "\nLooking at sigma = %c\n", p[i]);
    if (alphabet_map_.find(p[i]) == alphabet_map_.end()) {
      fprintf(stderr, "Could not find %c in alphabet_map.\n", p[i]);
      return range;
    }

    sigma_id = alphabet_map_[p[i]].second;
    fprintf(stderr, "Sigma ID = %u\n", sigma_id);

    // Get context_id:
    context_val = ComputeContextValue(p, i + 1);
    fprintf(stderr, "Context value = %llu\n", context_val);
    if (npa_->contexts_.find(context_val) == npa_->contexts_.end()) {
      fprintf(stderr, "Could not find %llu in contexts.\n", context_val);
      return range;
    }
    context_id = npa_->contexts_[context_val];
    fprintf(stderr, "Context ID = %llu\n", context_id);

    // Get start offset:
    start_off = GetRank1(&(npa_->col_nec_[sigma_id]), context_id) - 1;
    fprintf(stderr, "Start offset = %llu\n", start_off);

    // Get left boundary:
    c1 = npa_->col_offsets_[sigma_id]
        + npa_->cell_offsets_[sigma_id][start_off];
    fprintf(stderr, "Left end = %lld\n", c1);

    // Get right boundary:
    if (start_off + 1 < npa_->cell_offsets_[sigma_id].size()) {
      c2 = npa_->col_offsets_[sigma_id]
          + npa_->cell_offsets_[sigma_id][start_off + 1] - 1;
    } else if ((sigma_id + 1) < alphabet_size_) {
      c2 = npa_->col_offsets_[sigma_id + 1] - 1;
    } else {
      c2 = input_size_ - 1;
    }
    fprintf(stderr, "Right end = %lld\n", c2);
    if (c2 < c1) {
      fprintf(stderr, "Left end was greater than right end.\n");
      return range;
    }

    left = npa_->BinarySearch(left, c1, c2, false);
    right = npa_->BinarySearch(right, c1, c2, true);
    fprintf(stderr, "Binary search left end = %lld\n", left);
    fprintf(stderr, "Binary search right end = %lld\n", right);

    if (left > right) {
      fprintf(stderr, "Left end was greater than right end.\n");
      return range;
    }
  }

  range.first = left;
  range.second = right;

  fprintf(stderr, "\nFinal range: (%lld, %lld)", left, right);

  return range;
}

std::string SuccinctFile::Name() {
  return input_filename_;
}

void SuccinctFile::Extract(std::string& result, uint64_t offset, uint64_t len) {
  result.resize(len);
  uint64_t idx = LookupISA(offset);
  for (uint64_t k = 0; k < len; k++) {
    result[k] = alphabet_[LookupC(idx)];
    idx = LookupNPA(idx);
  }
}

void SuccinctFile::Extract2(std::string& result, uint64_t offset, uint64_t len) {
  result.resize(len);
  uint64_t idx = LookupISA(offset);
  uint64_t *npa_vals = new uint64_t[len];
  for (uint64_t k = 0; k < len; k++) {
    npa_vals[k] = idx;
    idx = LookupNPA(idx);
  }
  for (uint64_t k = 0; k < len; k++) {
    result[k] = alphabet_[LookupC(npa_vals[k])];
  }
}

void SuccinctFile::Extract3(std::string& result, uint64_t offset, uint64_t len) {
  result.resize(len);
  uint64_t idx = LookupISA(offset);
  std::map<uint64_t, uint64_t> npa_vals;
  for (uint64_t k = 0; k < len; k++) {
    npa_vals[idx] = k;
    idx = LookupNPA(idx);
  }
  for(auto npa_entry : npa_vals) {
    result[npa_entry.second] = alphabet_[LookupC(npa_entry.first)];
  }
}

uint64_t SuccinctFile::Count(std::string& str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  return range.second - range.first + 1;
}

uint64_t SuccinctFile::Count2(std::string& str) {
  std::pair<int64_t, int64_t> range = GetRangeSlow(str.c_str(), str.length());
  return range.second - range.first + 1;
}

void SuccinctFile::Search(std::vector<int64_t>& result, std::string str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  if (range.first > range.second)
    return;
  result.resize((uint64_t) (range.second - range.first + 1));
  for (int64_t i = range.first; i <= range.second; i++) {
    result[i - range.first] = ((int64_t) LookupSA(i));
  }
}

void SuccinctFile::WildCardSearch(std::vector<int64_t>& result,
                                  std::string pattern, uint64_t max_sep) {
  // TODO: Implement wild card search
}
