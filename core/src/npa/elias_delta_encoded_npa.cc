#include "../../include/npa/elias_delta_encoded_npa.h"

EliasDeltaEncodedNPA::EliasDeltaEncodedNPA(uint64_t npa_size,
                                           uint64_t sigma_size,
                                           uint32_t context_len,
                                           uint32_t sampling_rate,
                                           bitmap_t *data_bitmap,
                                           bitmap_t *compactSA,
                                           bitmap_t *compactISA,
                                           SuccinctAllocator &s_allocator)
    : DeltaEncodedNPA(npa_size, sigma_size, context_len, sampling_rate,
                      NPAEncodingScheme::ELIAS_DELTA_ENCODED, s_allocator) {
  Encode(data_bitmap, compactSA, compactISA);
}

EliasDeltaEncodedNPA::EliasDeltaEncodedNPA(uint32_t context_len,
                                           uint32_t sampling_rates,
                                           SuccinctAllocator &s_allocator)
    : DeltaEncodedNPA(0, 0, context_len, sampling_rate_,
                      NPAEncodingScheme::ELIAS_DELTA_ENCODED, s_allocator) {
}

EliasDeltaEncodedNPA::~EliasDeltaEncodedNPA() {
}

uint32_t EliasDeltaEncodedNPA::EliasDeltaEncodingSize(uint64_t n) {
  uint32_t N = LowerLog2(n);
  uint32_t NN = LowerLog2(N + 1);
  return N + 2 * NN + 1;
}

void EliasDeltaEncodedNPA::EliasDeltaEncode(SuccinctBase::BitMap **B,
                                              std::vector<uint64_t> &deltas,
                                              uint64_t size) {
  if (size == 0) {
    delete *B;
    *B = NULL;
    return;
  }

  uint64_t pos = 0;
  SuccinctBase::init_bitmap(B, size, s_allocator_);
  for (size_t i = 0; i < deltas.size(); i++) {
    uint32_t N_prime = LowerLog2(deltas[i]);
    uint32_t N = N_prime + 1;
    uint32_t N_bits = EliasGammaEncodedNPA::EliasGammaEncodingSize(N);
    SuccinctBase::set_bitmap_pos(B, pos, N, N_bits);
    pos += N_bits;
    uint64_t val = deltas[i] - (1 << N_prime);
    SuccinctBase::set_bitmap_pos(B, pos, val, N_prime);
    pos += N_prime;
  }
}

// Decode a particular elias-delta encoded delta value at a provided offset
// in the deltas bitmaps
uint64_t EliasDeltaEncodedNPA::EliasDeltaDecode(SuccinctBase::BitMap *B,
                                                  uint64_t *offset) {
  uint32_t L = 0;
  while (!ACCESSBIT(B, (*offset))) {
    L++;
    (*offset)++;
  }
  uint32_t N = SuccinctBase::lookup_bitmap_pos(B, *offset, L + 1);
  assert(N > 0);
  *offset += (L + 1);
  uint64_t val = SuccinctBase::lookup_bitmap_pos(B, *offset, N - 1);
  *offset += (N - 1);
  val |= (1 << (N - 1));
  return val;
}

// Compute the prefix sum for the elias delta encoded deltas
uint64_t EliasDeltaEncodedNPA::EliasDeltaPrefixSum(SuccinctBase::BitMap *B,
                                                      uint64_t offset,
                                                      uint64_t i) {
  uint64_t sum = 0;

  for (uint64_t j = 0; j < i; j++) {
    uint32_t L = 0;
    while (!ACCESSBIT(B, offset)) {
      L++;
      offset++;
    }
    uint32_t N = SuccinctBase::lookup_bitmap_pos(B, offset, L + 1);
    assert(N > 0);
    offset += (L + 1);
    uint64_t val = SuccinctBase::lookup_bitmap_pos(B, offset, N - 1);
    offset += (N - 1);
    val |= (1 << (N - 1));
    sum += val;
  }

  return sum;
}

// Create delta encoded vector
void EliasDeltaEncodedNPA::CreateDeltaEncodedVector(DeltaEncodedVector *dv,
                                     std::vector<uint64_t> &data) {
  if (data.size() == 0) {
    return;
  }

  assert(dv != NULL);

  uint64_t max_sample = 0;

  std::vector<uint64_t> _samples, _deltas, _delta_offsets;
  uint64_t tot_delta_count = 0, delta_count = 0;
  uint64_t delta_enc_size = 0;
  uint64_t cum_delta_size = 0, max_offset = 0;
  uint64_t last_val = 0;

  for (size_t i = 0; i < data.size(); i++) {
    if (i % sampling_rate_ == 0) {
      _samples.push_back(data[i]);
      if (data[i] > max_sample) {
        max_sample = data[i];
      }
      if (cum_delta_size > max_offset)
        max_offset = cum_delta_size;
      _delta_offsets.push_back(cum_delta_size);
      if (i != 0) {
        assert(delta_count == sampling_rate_ - 1);
        tot_delta_count += delta_count;
        delta_count = 0;
      }
    } else {
      long delta = data[i] - last_val;
      assert(delta > 0);
      _deltas.push_back(delta);

      delta_enc_size = EliasDeltaEncodingSize(delta);
      cum_delta_size += delta_enc_size;
      delta_count++;
    }
    last_val = data[i];
  }
  tot_delta_count += delta_count;

  assert(tot_delta_count == _deltas.size());
  assert(_samples.size() + _deltas.size() == data.size());
  assert(_delta_offsets.size() == _samples.size());

  // Can occur at most once per context;
  // only occurs if 0 is the only value in the cell
  if (max_sample == 0)
    dv->sample_bits = 1;
  else
    dv->sample_bits = (uint8_t) SuccinctUtils::int_log_2(max_sample + 1);

  if (max_offset == 0)
    dv->delta_offset_bits = 1;
  else
    dv->delta_offset_bits = (uint8_t) SuccinctUtils::int_log_2(max_offset + 1);

  dv->samples = new bitmap_t;
  dv->deltas = new bitmap_t;
  SuccinctBase::create_bitmap_array(
      &(dv->samples), (_samples.size() == 0) ? NULL : &_samples[0],
      _samples.size(), dv->sample_bits, s_allocator_);

  EliasDeltaEncode(&(dv->deltas), _deltas, cum_delta_size);

  dv->delta_offsets = new bitmap_t;
  SuccinctBase::create_bitmap_array(
      &(dv->delta_offsets),
      (_delta_offsets.size() == 0) ? NULL : &_delta_offsets[0],
      _delta_offsets.size(), dv->delta_offset_bits, s_allocator_);
}

// Lookup delta encoded vector at index i
uint64_t EliasDeltaEncodedNPA::LookupDeltaEncodedVector(DeltaEncodedVector *dv, uint64_t i) {
  uint64_t sample_offset = i / sampling_rate_;
  uint64_t delta_offset_idx = i % sampling_rate_;
  uint64_t val = SuccinctBase::lookup_bitmap_array(dv->samples, sample_offset,
                                                   dv->sample_bits);
  if (delta_offset_idx == 0)
    return val;
  uint64_t delta_offset = SuccinctBase::lookup_bitmap_array(
      dv->delta_offsets, sample_offset, dv->delta_offset_bits);
  val += EliasDeltaPrefixSum(dv->deltas, delta_offset, delta_offset_idx);
  return val;
}
