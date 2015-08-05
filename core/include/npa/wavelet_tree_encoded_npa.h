#ifndef WAVELET_TREE_ENCODED_NPA_H
#define WAVELET_TREE_ENCODED_NPA_H

#include "npa.h"

class WaveletTreeEncodedNPA : public NPA {

 public:
  typedef SuccinctBase::Dictionary Dictionary;

  typedef struct _wletnode {
    Dictionary D;
    char id;
    struct _wletnode *lt, *rt;
  } WaveletNode;

  // Constructor
  WaveletTreeEncodedNPA(uint64_t npa_size, uint64_t sigma_size,
                        uint32_t context_len, uint32_t sampling_rate,
                        Bitmap *data_bitmap, Bitmap *compactSA,
                        Bitmap *compactISA, SuccinctAllocator &s_allocator);

  WaveletTreeEncodedNPA(uint32_t context_len, uint32_t sampling_rate,
                        SuccinctAllocator &s_allocator);

  // Virtual destructor
  ~WaveletTreeEncodedNPA() {
  }

  // Encode EliasDeltaEncodedNPA
  virtual void Encode(Bitmap *data_bitmap, Bitmap *compact_sa,
                      Bitmap *compact_isa);

  // Access element at index i
  virtual uint64_t operator[](uint64_t i);

  // Serialize the wavelet tree enoded NPA
  virtual size_t Serialize(std::ostream& out);

  // Deserialize the wavelet tree encoded NPA
  virtual size_t Deserialize(std::istream& in);

  // Memory map the wavelet tree encoded NPA
  virtual size_t MemoryMap(std::string filename);

  virtual size_t StorageSize();

 protected:
  // Create wavelet tree
  void CreateWaveletTree(WaveletNode **w, uint32_t start, uint32_t end,
                         std::vector<uint64_t> &values,
                         std::vector<uint64_t> &value_contexts);

  // Lookup wavelet tree
  uint64_t LookupWaveletTree(WaveletNode *tree, uint64_t context_pos, uint64_t cell_pos,
                             uint64_t start, uint64_t end);

  size_t SerializeWaveletTree(WaveletNode *root, std::ostream& out);
  size_t DeserializeWaveletTree(WaveletNode **root, std::istream& in);

  size_t SerializeWaveletNode(WaveletNode *node, std::ostream& out);
  size_t DeserializeWaveletNode(WaveletNode **node, std::istream& in);

 private:
  uint64_t *column_sizes_;
  WaveletNode **wavelet_tree_;

};

#endif
