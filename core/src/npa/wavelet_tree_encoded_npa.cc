#include "npa/wavelet_tree_encoded_npa.h"

WaveletTreeEncodedNPA::WaveletTreeEncodedNPA(uint64_t npa_size,
                                             uint64_t sigma_size,
                                             uint32_t context_len,
                                             uint32_t sampling_rate,
                                             Bitmap *data_bitmap,
                                             Bitmap *compactSA,
                                             Bitmap *compactISA,
                                             SuccinctAllocator &s_allocator)
    : NPA(npa_size, sigma_size, context_len, sampling_rate,
          NPAEncodingScheme::WAVELET_TREE_ENCODED, s_allocator) {
  this->wavelet_tree_ = NULL;
  this->column_sizes_ = NULL;
  Encode(data_bitmap, compactSA, compactISA);
}

WaveletTreeEncodedNPA::WaveletTreeEncodedNPA(uint32_t context_len,
                                             uint32_t sampling_rate,
                                             SuccinctAllocator &s_allocator)
    : NPA(0, 0, context_len, sampling_rate,
          NPAEncodingScheme::WAVELET_TREE_ENCODED, s_allocator) {
  this->wavelet_tree_ = NULL;
  this->column_sizes_ = NULL;
}

void WaveletTreeEncodedNPA::CreateWaveletTree(
    WaveletNode **root, uint32_t start, uint32_t end,
    std::vector<uint64_t> &values, std::vector<uint64_t> &value_columns) {
  if (start == end) {
    (*root) = NULL;
    return;
  }

  uint32_t mid = value_columns.at((values.size() - 1) / 2);
  mid = SuccinctUtils::Min(mid, end - 1);

  uint64_t r;
  std::vector<uint64_t> right_values, right_value_columns, left_values,
      left_value_columns;

  (*root) = new WaveletNode;
  Bitmap *B = new Bitmap;
  SuccinctBase::InitBitmap(&(B), values.size(), s_allocator_);
  (*root)->id = mid;

  for (uint64_t i = 0; i < values.size(); i++) {
    if (value_columns.at(i) > mid && value_columns.at(i) <= end) {
      SETBITVAL((B), values.at(i));
    }
  }

  SuccinctBase::CreateDictionary(B, &((*root)->D), s_allocator_);
  SuccinctBase::DestroyBitmap(&B, s_allocator_);

  for (uint64_t i = 0; i < values.size(); i++) {
    if (value_columns.at(i) > mid && value_columns.at(i) <= end) {
      r = SuccinctBase::GetRank1(&((*root)->D), values.at(i)) - 1;
      assert(r >= 0);
      right_values.push_back(r);
      right_value_columns.push_back(value_columns.at(i));
    } else {
      r = SuccinctBase::GetRank0(&((*root)->D), values.at(i)) - 1;
      assert(r >= 0);
      left_values.push_back(r);
      left_value_columns.push_back(value_columns.at(i));
    }
  }

  CreateWaveletTree(&((*root)->lt), start, mid, left_values,
                    left_value_columns);
  left_values.clear();
  left_value_columns.clear();

  CreateWaveletTree(&((*root)->rt), mid + 1, end, right_values,
                    right_value_columns);
  right_values.clear();
  right_value_columns.clear();
}

uint64_t WaveletTreeEncodedNPA::LookupWaveletTree(WaveletNode *tree,
                                                  uint64_t context_off,
                                                  uint64_t cell_off, uint64_t s,
                                                  uint64_t e) {

  if (tree == NULL)
    return cell_off;

  unsigned char m = tree->id;
  uint64_t p, v;
  if (context_off > m && context_off <= e) {
    p = LookupWaveletTree(tree->rt, context_off, cell_off, m + 1, e);
    v = SuccinctBase::GetSelect1(&(tree->D), p);
  } else {
    p = LookupWaveletTree(tree->lt, context_off, cell_off, s, m);
    v = SuccinctBase::GetSelect0(&(tree->D), p);
  }

  return v;

}

void WaveletTreeEncodedNPA::Encode(Bitmap *data_bitmap, Bitmap *compact_sa,
                                   Bitmap *compact_isa) {
  uint32_t logn, q;
  uint64_t k1, k2, k = 0, l_off = 0, c_id, c_val, npa_val, p = 0;

  std::map<uint64_t, uint64_t> context_size;
  std::vector<uint64_t> **table;
  std::vector<std::pair<std::vector<uint64_t>, unsigned char> > context;
  std::vector<uint64_t> cell;
  std::vector<uint64_t> values, value_columns;

  bool flag;
  uint64_t *sizes, *starts;
  uint64_t last_i = 0;

  logn = SuccinctUtils::IntegerLog2(npa_size_ + 1);

  for (uint64_t i = 0; i < npa_size_; i++) {
    uint64_t c_val = ComputeContextValue(data_bitmap, i);
    contexts_.insert(std::pair<uint64_t, uint64_t>(c_val, 0));
    context_size[c_val]++;
  }

  sizes = new uint64_t[contexts_.size()];
  starts = new uint64_t[contexts_.size()];

  for (std::map<uint64_t, uint64_t>::iterator it = contexts_.begin();
      it != contexts_.end(); it++) {
    sizes[k] = context_size[it->first];
    starts[k] = npa_size_;
    contexts_[it->first] = k++;
  }

  assert(k == contexts_.size());
  context_size.clear();

  Bitmap *E = new Bitmap;
  table = new std::vector<uint64_t>*[k];
  cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
  col_nec_ = new std::vector<uint64_t>[sigma_size_];
  row_nec_ = new std::vector<uint64_t>[k];
  wavelet_tree_ = new WaveletNode*[k];
  column_sizes_ = new uint64_t[k]();

  for (uint64_t i = 0; i < k; i++) {
    table[i] = new std::vector<uint64_t>[sigma_size_];
  }

  SuccinctBase::InitBitmap(&(E), k * sigma_size_, s_allocator_);

  k1 = SuccinctBase::LookupBitmapArray(compact_sa, 0, logn);
  c_id = ComputeContextValue(data_bitmap, (k1 + 1) % npa_size_);
  c_val = contexts_[c_id];
  npa_val = SuccinctBase::LookupBitmapArray(
      compact_isa,
      (SuccinctBase::LookupBitmapArray(compact_sa, 0, logn) + 1) % npa_size_,
      logn);
  table[c_val][l_off / k].push_back(npa_val);
  starts[c_val] = SuccinctUtils::Min(starts[c_val], npa_val);

  SETBITVAL(E, c_val);
  col_nec_[0].push_back(c_val);
  col_offsets_.push_back(0);
  cell_offsets_[0].push_back(0);

  for (uint64_t i = 1; i < npa_size_; i++) {
    k1 = SuccinctBase::LookupBitmapArray(compact_sa, i, logn);
    k2 = SuccinctBase::LookupBitmapArray(compact_sa, i - 1, logn);

    c_id = ComputeContextValue(data_bitmap, (k1 + 1) % npa_size_);
    c_val = contexts_[c_id];

    assert(c_val < k);

    if (!CompareDataBitmap(data_bitmap, k1, k2, 1)) {
      col_offsets_.push_back(i);
      l_off += k;
      last_i = i;
      cell_offsets_[l_off / k].push_back(i - last_i);
    } else if (!CompareDataBitmap(data_bitmap, (k1 + 1) % npa_size_,
                                  (k2 + 1) % npa_size_, context_len_)) {
      // Context has changed; mark in L
      cell_offsets_[l_off / k].push_back(i - last_i);
    }

    // If we haven't marked it already, mark current cell as non empty
    if (!ACCESSBIT(E, (l_off + c_val))) {
      SETBITVAL(E, (l_off + c_val));
      col_nec_[l_off / k].push_back(c_val);
    }

    // Obtain actual npa value
    npa_val = SuccinctBase::LookupBitmapArray(
        compact_isa,
        (SuccinctBase::LookupBitmapArray(compact_sa, i, logn) + 1) % npa_size_,
        logn);

    assert(l_off / k < sigma_size_);
    // Push npa value to npa table: note indices. indexed by context value, and l_offs / k
    table[c_val][l_off / k].push_back(npa_val);
    assert(table[c_val][l_off / k].back() == npa_val);
    starts[c_val] = SuccinctUtils::Min(starts[c_val], npa_val);
  }

  for (uint64_t i = 0; i < k; i++) {
    q = 0;
    flag = false;
    for (uint64_t j = 0; j < sigma_size_; j++) {
      if (!flag && ACCESSBIT(E, (i + j * k))) {
        row_offsets_.push_back(p);
        p += table[i][j].size();
        flag = true;
      } else if (ACCESSBIT(E, (i + j * k))) {
        p += table[i][j].size();
      }

      if (ACCESSBIT(E, (i + j * k))) {
        for (uint64_t t = 0; t < table[i][j].size(); t++) {
          cell.push_back(table[i][j][t] - starts[i]);
        }
        context.push_back(
            std::pair<std::vector<uint64_t>, unsigned char>(cell, j));
        cell.clear();
        row_nec_[i].push_back(j);
        column_sizes_[i]++;
        q++;
      }
    }

    if (i > 0) {
      column_sizes_[i] += column_sizes_[i - 1];
    }

    for (size_t j = 0; j < context.size(); j++) {
      for (size_t t = 0; t < context[j].first.size(); t++) {
        values.push_back(context[j].first[t]);
        value_columns.push_back(j);
      }
    }
    CreateWaveletTree(&wavelet_tree_[i], 0, context.size() - 1, values,
                      value_columns);
    values.clear();
    value_columns.clear();
    context.clear();
  }

  // Clean up
  for (uint64_t i = 0; i < k; i++) {
    for (uint64_t j = 0; j < sigma_size_; j++) {
      if (table[i][j].size()) {
        table[i][j].clear();
      }
    }
    delete[] table[i];
  }
  delete[] table;
  delete[] starts;
  delete[] sizes;
  delete[] E->bitmap;
  delete E;

  context_size.clear();
}

uint64_t WaveletTreeEncodedNPA::operator[](uint64_t i) {

  // Get column id
  uint64_t column_id = SuccinctBase::GetRank1(&(col_offsets_), i) - 1;

  // Get column offset
  uint64_t column_off = col_offsets_[column_id];

  // Get pseudo row id
  uint64_t pseudo_row_id = SuccinctBase::GetRank1(&(cell_offsets_[column_id]),
                                                  i - column_off) - 1;

  // Get cell offset
  uint64_t cell_off = i - column_off - cell_offsets_[column_id][pseudo_row_id];

  // Get row id
  uint64_t row_id = col_nec_[column_id][pseudo_row_id];

  // Get row offset
  uint64_t row_off = row_offsets_[row_id];

  // Get cell size
  uint64_t num_contexts =
      (row_id > 0) ?
          column_sizes_[row_id] - column_sizes_[row_id - 1] : column_sizes_[0];

  // Get context offset
  uint64_t context_off = SuccinctBase::GetRank1(&(row_nec_[row_id]), column_id)
      - 1;

  // Get row value
  uint64_t row_val = LookupWaveletTree(wavelet_tree_[row_id], context_off,
                                       cell_off, 0, num_contexts - 1);

  // Get npa value
  uint64_t npa_val = row_off + row_val;
  return npa_val;
}

size_t WaveletTreeEncodedNPA::SerializeWaveletNode(WaveletNode* node,
                                                   std::ostream& out) {
  size_t out_size = 0;

  if (node == NULL) {
    // Output -1
    char null_node = -1;
    out.write(reinterpret_cast<const char *>(&(null_node)), sizeof(char));
    out_size += sizeof(char);

    return out_size;
  }

  out.write(reinterpret_cast<const char *>(&(node->id)), sizeof(char));
  out_size += sizeof(char);

  out_size += SuccinctBase::SerializeDictionary(&(node->D), out);

  return out_size;
}

size_t WaveletTreeEncodedNPA::SerializeWaveletTree(WaveletNode* root,
                                                   std::ostream& out) {
  size_t out_size = 0;

  out_size += SerializeWaveletNode(root, out);
  if (root != NULL) {
    out_size += SerializeWaveletTree(root->lt, out);
    out_size += SerializeWaveletTree(root->rt, out);
  }

  return out_size;
}

size_t WaveletTreeEncodedNPA::Serialize(std::ostream& out) {
  size_t out_size = 0;
  typedef std::map<uint64_t, uint64_t>::iterator iterator_t;

  // Output NPA scheme
  out.write(reinterpret_cast<const char *>(&(encoding_scheme_)),
            sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  // Output NPA size
  out.write(reinterpret_cast<const char *>(&(npa_size_)), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  // Output sigma_size
  out.write(reinterpret_cast<const char *>(&(sigma_size_)), sizeof(uint64_t));
  out_size += sizeof(uint64_t);

  // Output context_len
  out.write(reinterpret_cast<const char *>(&(context_len_)), sizeof(uint32_t));
  out_size += sizeof(uint32_t);

  // Output sampling rate
  out.write(reinterpret_cast<const char *>(&(sampling_rate_)),
            sizeof(uint32_t));
  out_size += sizeof(uint32_t);

  // Output contexts
  uint64_t context_size = contexts_.size();
  out.write(reinterpret_cast<const char *>(&(context_size)), sizeof(uint64_t));
  for (iterator_t it = contexts_.begin(); it != contexts_.end(); ++it) {
    out.write(reinterpret_cast<const char *>(&(it->first)), sizeof(uint64_t));
    out.write(reinterpret_cast<const char *>(&(it->second)), sizeof(uint64_t));
  }

  // Output row_offsets
  out_size += SuccinctBase::SerializeVector(row_offsets_, out);

  // Output col_offsets
  out_size += SuccinctBase::SerializeVector(col_offsets_, out);

  // Output neccol
  for (uint64_t i = 0; i < sigma_size_; i++) {
    out_size += SuccinctBase::SerializeVector(col_nec_[i], out);
  }

  // Output necrow
  for (uint64_t i = 0; i < contexts_.size(); i++) {
    out_size += SuccinctBase::SerializeVector(row_nec_[i], out);
  }

  // Output cell offsets
  for (uint64_t i = 0; i < sigma_size_; i++) {
    out_size += SuccinctBase::SerializeVector(cell_offsets_[i], out);
  }

  // Output column sizes
  for (uint64_t i = 0; i < contexts_.size(); i++) {
    out.write(reinterpret_cast<const char *>(&(column_sizes_[i])),
              sizeof(uint64_t));
    out_size += sizeof(uint64_t);
  }

  for (uint64_t i = 0; i < contexts_.size(); i++) {
    out_size += SerializeWaveletTree(wavelet_tree_[i], out);
  }

  return out_size;
}

size_t WaveletTreeEncodedNPA::DeserializeWaveletNode(WaveletNode** node,
                                                     std::istream& in) {
  size_t in_size = 0;

  char id;
  in.read(reinterpret_cast<char *>(&(id)), sizeof(char));
  in_size += sizeof(char);

  if (id == -1) {
    *node = NULL;
    return in_size;
  }

  *node = new WaveletNode;
  (*node)->id = id;

  in_size += SuccinctBase::DeserializeDictionary(&(*node)->D, in);

  return in_size;
}

size_t WaveletTreeEncodedNPA::DeserializeWaveletTree(WaveletNode** root,
                                                     std::istream& in) {
  size_t in_size = 0;
  in_size += DeserializeWaveletNode(root, in);
  if (*root != NULL) {
    in_size += DeserializeWaveletTree(&((*root)->lt), in);
    in_size += DeserializeWaveletTree(&((*root)->rt), in);
  }
  return in_size;
}

size_t WaveletTreeEncodedNPA::Deserialize(std::istream& in) {
  size_t in_size = 0;

  // Read NPA scheme
  in.read(reinterpret_cast<char *>(&(encoding_scheme_)), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  // Read NPA size
  in.read(reinterpret_cast<char *>(&(npa_size_)), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  // Read sigma_size
  in.read(reinterpret_cast<char *>(&(sigma_size_)), sizeof(uint64_t));
  in_size += sizeof(uint64_t);

  // Read context_len
  in.read(reinterpret_cast<char *>(&(context_len_)), sizeof(uint32_t));
  in_size += sizeof(uint32_t);

  // Read sampling rate
  in.read(reinterpret_cast<char *>(&(sampling_rate_)), sizeof(uint32_t));
  in_size += sizeof(uint32_t);

  // Read contexts
  uint64_t context_size;
  in.read(reinterpret_cast<char *>(&(context_size)), sizeof(uint64_t));
  for (uint64_t i = 0; i < context_size; i++) {
    uint64_t first, second;
    in.read(reinterpret_cast<char *>(&(first)), sizeof(uint64_t));
    in.read(reinterpret_cast<char *>(&(second)), sizeof(uint64_t));
    contexts_[first] = second;
  }

  // Read rowoffsets
  in_size += SuccinctBase::DeserializeVector(row_offsets_, in);

  // Read coloffsets
  in_size += SuccinctBase::DeserializeVector(col_offsets_, in);

  // Read neccol
  col_nec_ = new std::vector<uint64_t>[sigma_size_];
  for (uint64_t i = 0; i < sigma_size_; i++) {
    in_size += SuccinctBase::DeserializeVector(col_nec_[i], in);
  }

  // Read necrow
  row_nec_ = new std::vector<uint64_t>[contexts_.size()];
  for (uint64_t i = 0; i < contexts_.size(); i++) {
    in_size += SuccinctBase::DeserializeVector(row_nec_[i], in);
  }

  // Read cell offsets
  cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
  for (uint64_t i = 0; i < sigma_size_; i++) {
    in_size += SuccinctBase::DeserializeVector(cell_offsets_[i], in);
  }

  column_sizes_ = new uint64_t[contexts_.size()];
  for (uint64_t i = 0; i < contexts_.size(); i++) {
    in.read(reinterpret_cast<char *>(&(column_sizes_[i])), sizeof(uint64_t));
    in_size += sizeof(uint64_t);
  }

  wavelet_tree_ = new WaveletNode*[contexts_.size()];
  for (uint64_t i = 0; i < contexts_.size(); i++) {
    DeserializeWaveletTree(&(wavelet_tree_[i]), in);
  }

  return in_size;
}

size_t WaveletTreeEncodedNPA::MemoryMap(std::string filename) {
  uint8_t *data, *data_beg;
  data = data_beg = (uint8_t *) SuccinctUtils::MemoryMap(filename);

  encoding_scheme_ = (NPAEncodingScheme) (*((uint64_t *) data));
  data += sizeof(uint64_t);
  npa_size_ = *((uint64_t *) data);
  data += sizeof(uint64_t);
  sigma_size_ = *((uint64_t *) data);
  data += sizeof(uint64_t);
  context_len_ = *((uint32_t *) data);
  data += sizeof(uint32_t);
  sampling_rate_ = *((uint32_t *) data);
  data += sizeof(uint32_t);

  // Read contexts
  uint64_t context_size = *((uint64_t *) data);
  data += sizeof(uint64_t);
  for (uint64_t i = 0; i < context_size; i++) {
    uint64_t first = *((uint64_t *) data);
    data += sizeof(uint64_t);
    uint64_t second = *((uint64_t *) data);
    data += sizeof(uint64_t);
    contexts_[first] = second;
  }

  // Read rowoffsets
  data += SuccinctBase::MemoryMapVector(row_offsets_, data);

  // Read coloffsets
  data += SuccinctBase::MemoryMapVector(col_offsets_, data);

  // Read neccol
  col_nec_ = new std::vector<uint64_t>[sigma_size_];
  for (uint64_t i = 0; i < sigma_size_; i++) {
    data += SuccinctBase::MemoryMapVector(col_nec_[i], data);
  }

  // Read necrow
  row_nec_ = new std::vector<uint64_t>[contexts_.size()];
  for (uint64_t i = 0; i < contexts_.size(); i++) {
    data += SuccinctBase::MemoryMapVector(row_nec_[i], data);
  }

  // Read cell offsets
  cell_offsets_ = new std::vector<uint64_t>[sigma_size_];
  for (uint64_t i = 0; i < sigma_size_; i++) {
    data += SuccinctBase::MemoryMapVector(cell_offsets_[i], data);
  }

  // TODO: Memory map WaveletTrees
  fprintf(stderr, "Memory mapping for Wavelet Trees not supported yet.\n");
  assert(0);

  return data - data_beg;
}

size_t WaveletTreeEncodedNPA::StorageSize() {
  // TODO: fix
  return 0;
}
