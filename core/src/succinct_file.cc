#include "succinct_file.h"

SuccinctFile::SuccinctFile(std::string filename, SuccinctMode s_mode)
    : SuccinctCore(filename.c_str(), s_mode) {
  this->input_filename_ = filename;
  this->succinct_filename_ = filename + ".succinct";
}

uint64_t SuccinctFile::ComputeContextValue(const char *p, uint64_t i) {
  uint64_t val = 0;
  uint64_t max = MIN((i + npa_->get_context_len()), input_size_);
  for (uint64_t t = i; t < max; t++) {
    val = val * alphabet_size_ + alphabet_map_[p[t]].second;
  }

  if (max < i + npa_->get_context_len()) {
    for (uint64_t t = 0; t < (i + npa_->get_context_len()) % input_size_; t++) {
      val = val * alphabet_size_ + alphabet_map_[p[t]].second;
    }
  }

  return val;
}

std::pair<int64_t, int64_t> SuccinctFile::GetRange(const char *p,
                                                         uint64_t len) {
  std::pair<int64_t, int64_t> range(0, -1);
  uint64_t m = strlen(p);
  int64_t sp, ep, c1, c2;

  if (alphabet_map_.find(p[m - 1]) != alphabet_map_.end()) {
    sp = (alphabet_map_[p[m - 1]]).first;
    ep = alphabet_map_[alphabet_[alphabet_map_[p[m - 1]].second + 1]].first - 1;
  } else
    return range;

  for (int64_t i = m - 2; i >= 0; i--) {
    if (alphabet_map_.find(p[m - 1]) != alphabet_map_.end()) {
      c1 = alphabet_map_[p[i]].first;
      c2 = alphabet_map_[alphabet_[alphabet_map_[p[i]].second + 1]].first - 1;
    } else
      return range;

    if (c2 < c1)
      return range;

    sp = npa_->BinarySearch(sp, c1, c2, false);
    ep = npa_->BinarySearch(ep, c1, c2, true);

    if (sp > ep)
      return range;
  }

  range.first = sp;
  range.second = ep;

  return range;
}

/*
std::pair<int64_t, int64_t> SuccinctFile::GetRange(const char *p,
                                                    uint64_t len) {
  uint64_t m = strlen(p);
  if (m <= npa_->get_context_len()) {
    return GetRangeSlow(p, len);
  }
  std::pair<int64_t, int64_t> range(0, -1);
  uint32_t sigma_id;
  int64_t sp, ep, c1, c2;
  uint64_t start_off;
  uint64_t context_val, context_id;

  sigma_id = alphabet_map_[p[m - npa_->get_context_len() - 1]].second;
  context_val = ComputeContextValue(p, m - npa_->get_context_len());
  context_id = npa_->contexts_[context_val];
  start_off = GetRank1(&(npa_->col_nec_[sigma_id]), context_id) - 1;
  sp = npa_->col_offsets_[sigma_id] + npa_->cell_offsets_[sigma_id][start_off];
  if (start_off + 1 < npa_->cell_offsets_[sigma_id].size()) {
    ep = npa_->col_offsets_[sigma_id] + npa_->cell_offsets_[sigma_id][start_off + 1]
        - 1;
  } else if ((sigma_id + 1) < alphabet_size_) {
    ep = npa_->col_offsets_[sigma_id + 1] - 1;
  } else {
    ep = input_size_ - 1;
  }

  if (sp > ep)
    return range;

  for (int64_t i = m - npa_->get_context_len() - 2; i >= 0; i--) {
    if (alphabet_map_.find(p[i]) != alphabet_map_.end()) {
      sigma_id = alphabet_map_[p[i]].second;
      context_val = ComputeContextValue(p, i + 1);
      if (npa_->contexts_.find(context_val) == npa_->contexts_.end()) {
        return range;
      }
      context_id = npa_->contexts_[context_val];
      start_off = GetRank1(&(npa_->col_nec_[sigma_id]), context_id) - 1;
      c1 = npa_->col_offsets_[sigma_id] + npa_->cell_offsets_[sigma_id][start_off];
      if (start_off + 1 < npa_->cell_offsets_[sigma_id].size()) {
        c2 = npa_->col_offsets_[sigma_id]
            + npa_->cell_offsets_[sigma_id][start_off + 1] - 1;
      } else if ((sigma_id + 1) < alphabet_size_) {
        c2 = npa_->col_offsets_[sigma_id + 1] - 1;
      } else {
        c2 = input_size_ - 1;
      }
      if (c2 < c1)
        return range;
    } else
      return range;

    sp = npa_->BinarySearch(sp, c1, c2, false);
    ep = npa_->BinarySearch(ep, c1, c2, true);

    if (sp > ep)
      return range;
  }

  range.first = sp;
  range.second = ep;

  return range;
}
*/

std::string SuccinctFile::Name() {
  return input_filename_;
}

void SuccinctFile::Extract(std::string& result, uint64_t offset, uint64_t len) {
  result = "";
  uint64_t idx = LookupISA(offset);
  for (uint64_t k = 0; k < len; k++) {
    result += alphabet_[LookupC(idx)];
    idx = LookupNPA(idx);
  }
}

char SuccinctFile::CharAt(uint64_t pos) {
  uint64_t idx = LookupISA(pos);
  return alphabet_[LookupC(idx)];
}

uint64_t SuccinctFile::Count(std::string str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  return range.second - range.first + 1;
}

void SuccinctFile::Search(std::vector<int64_t>& result, std::string str) {
  std::pair<int64_t, int64_t> range = GetRange(str.c_str(), str.length());
  if (range.first > range.second)
    return;
  result.resize((uint64_t) (range.second - range.first + 1));
#pragma omp parallel for
  for (int64_t i = range.first; i <= range.second; i++) {
    result[i - range.first] = ((int64_t) LookupSA(i));
  }
}

void SuccinctFile::WildCardSearch(std::vector<int64_t>& result,
                                   std::string pattern, uint64_t max_sep) {
  // TODO: Implement wild card search
}
