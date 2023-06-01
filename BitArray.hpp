/**
 * @file BitArray.hpp
 *
 * @brief Implementation of Bit Array type with fast rotate rotate operations
 *
 * @ingroup StrictClusteringCoefficient
 *
 * @author Christos Tsalidis
 * Contact: tsalidis@neurolingo.gr
 * Created on 5/4/2023.
 *
 */

#ifndef FAST_ROTATE_BITARRAY_HPP
#define FAST_ROTATE_BITARRAY_HPP

#include <cstdint>
#include <cassert>
#include <memory>
#include <vector>
#include <bit>

#ifdef __has_include
# if __has_include(<version>)
#   include <version>
# endif
#endif

#ifdef __cpp_lib_execution
#include <execution>
#endif  /* __cpp_lib_execution */

#ifdef __cpp_lib_ranges
#include <ranges>
#endif /* __cpp_lib_ranges */

// in order to benchmark the two rotate functions
// we define them as 'no inline'

#if defined(WIN32)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif /* WIN32 */

template <typename Block=std::uint64_t, typename Allocator=std::allocator<Block>>
class BitArray
{
public:
  using block_type       = Block;
  using size_type        = size_t;
  using buffer_type      = std::vector<Block,Allocator>;
  using block_width_type = typename buffer_type::size_type;

#ifdef __cpp_constinit
  static constinit const size_t bits_per_block = std::numeric_limits<Block>::digits;
#else
  static constexpr const size_t bits_per_block = std::numeric_limits<Block>::digits;
#endif /*  __cpp_constinit */

private:
  size_t       m_bitset_capacity;
  size_t       m_num_bits;
  size_t       m_count{0};
  buffer_type  m_bits;

private:
  static size_type        block_index(size_type pos) noexcept { return pos / bits_per_block; }
  static block_width_type bit_index  (size_type pos) noexcept { return static_cast<block_width_type>(pos % bits_per_block); }
  static Block            bit_mask   (size_type pos) noexcept { return Block(1) << bit_index(pos); }

public:

  explicit BitArray(std::size_t num_bits) :
      m_bitset_capacity((num_bits-1) / bits_per_block + 1),
      m_num_bits(num_bits),
      m_bits(m_bitset_capacity)
  {  }

  BitArray() = delete;
  BitArray(BitArray const &) = default;
  BitArray(BitArray &&) noexcept = default;

  BitArray &operator=(BitArray const &) = default;
  BitArray &operator=(BitArray &&) noexcept = default;

  ~BitArray() = default;

  BitArray &set(uint32_t pos)
  {
    assert(pos < m_num_bits);

    m_bits[block_index(pos)] |= bit_mask(pos);
    m_count++;

    return *this;
  }

  BitArray &clear(uint32_t pos)
  {
    assert(pos < m_num_bits);

    m_bits[block_index(pos)] &= ~bit_mask(pos);
    m_count--;

    return *this;
  }

  block_type at(uint32_t pos) const noexcept
  {
    assert(pos < size());

    block_type  blk{m_bits[block_index(pos)]};

    return blk >> bit_index(pos) & static_cast<block_type>(1);
  }

  void reset() noexcept
  {
#ifdef __cpp_lib_ranges
    std::ranges::fill(m_bits, Block(0));
#else
    std::fill(begin(),end(),Block(0));
#endif /* __cpp_lib_ranges */

    m_count = 0;
  }

  [[nodiscard]] size_type num_blocks() const noexcept
  {
    return static_cast<size_type>(m_bits.size());
  }

  [[nodiscard]] size_t count() const
  {
    return m_count;
  }

  // return the number of set bits
  size_t recount()
  {
#ifdef __cpp_lib_execution
      std::atomic<size_t> _count = 0;

      std::for_each(std::execution::par_unseq, begin(), end(),
                    [&](block_type block) { _count += std::popcount(block); });

      return _count;
#else
    size_t _count = 0;

      std::for_each(begin(), end(),
                    [&](block_type block) { _count += std::popcount(block); });

      return _count;
#endif /* __cpp_lib_execution */
  }

  // return the number of common set bits (intersection of the two bitsets)
  [[nodiscard]]  size_t common(BitArray const &other)
  {
    assert(m_num_bits == other.m_num_bits);

    size_t _count = 0;

    for (block_type const *lp=m_bits.data(), *rp=other.m_bits.data(), *lend=m_bits.data()+m_bits.size(); lp < lend; lp++,rp++)
      _count += std::popcount(*lp & *rp);

    return _count;
  }

  // The fast blockwise implementation of (right) rotate
  NOINLINE void rotate(BitArray const &other, size_t n)
  {
    assert(size() == other.size());
    assert(m_bits.size() == other.m_bits.size());

    if (n >= m_num_bits)
      n %= m_num_bits;

    if (n == 0)
    {
      *this = other;

      return;
    }

    size_type  start_blk_pos{block_index(m_num_bits - n)};
    size_type  start_bit_pos{bit_index(m_num_bits - n)};

    size_type  opos{0};
    size_type  iblk_pos{start_blk_pos};
    size_type  ibit_pos{start_bit_pos};
    size_type  last_bits = m_num_bits % bits_per_block;

    while (opos < num_blocks())
    {
      block_type block{other.m_bits[iblk_pos]};

      // make block for output
      block = (ibit_pos >= bits_per_block) ?  0 :  block >> ibit_pos;


      if ( (start_bit_pos == ibit_pos) && (iblk_pos == num_blocks()-1) )
        ibit_pos = last_bits == 0 ? ibit_pos : (bits_per_block - (last_bits - ibit_pos));

      iblk_pos++;
      if (iblk_pos == num_blocks())
        iblk_pos = 0;

      if ( (iblk_pos == num_blocks()-1) && (iblk_pos != start_blk_pos) )
      {
        block_type rblock{other.m_bits[iblk_pos]};

        rblock = (ibit_pos == 0) ? static_cast<block_type>(0) : rblock << (bits_per_block-ibit_pos);
        block |= rblock;

        if ( (last_bits == 0) || (last_bits >= ibit_pos) )
        {
          m_bits.at(opos) = block;
          opos++;

          rblock = other.m_bits[iblk_pos];
          block = (rblock >> ibit_pos) & ~(~static_cast<block_type>(0) << (last_bits-ibit_pos));
          ibit_pos = (last_bits == 0) ? ibit_pos : (bits_per_block-(last_bits - ibit_pos));

          if ( (ibit_pos==0) && (last_bits == 0) )
            continue;
        }
        else
        {
          ibit_pos -= last_bits;
          // ibit_pos = last_bits + (bits_per_block - ibit_pos);
        }
        iblk_pos = 0;
      }

      if (iblk_pos == start_blk_pos)
      {
        block_type rblock{other.m_bits[iblk_pos]};

        rblock = (ibit_pos == 0) ? 0 : rblock << (bits_per_block-ibit_pos);

        if (ibit_pos >= start_bit_pos)
        {
          if (last_bits == 0)
          {
            block |= rblock;
          }
          else
          {
            rblock &= (~static_cast<block_type>(0) >> (bits_per_block - last_bits));
            block |= rblock;
            block &= ~(~static_cast<block_type>(0) << last_bits);
          }
        }
        else
        {
          block |= rblock;

          m_bits.at(opos) = block;
          opos++;

          assert(opos == num_blocks()-1);

          block = other.m_bits[start_blk_pos];

          block >>= ibit_pos;
          block &= ~(~static_cast<block_type>(0) << last_bits);
        }
      }
      else
      {
        block_type rblock{other.m_bits[iblk_pos]};

        rblock = (ibit_pos == 0) ? 0 : rblock << (bits_per_block-ibit_pos);
        block |= rblock;
      }

      m_bits.at(opos) = block;
      opos++;
    }
  }

  // the slow classical elementwise implementation of rotate
  NOINLINE void rotateRight(BitArray const &other, size_t n)
  {
    assert(size() == other.size());
    assert(m_bits.size() == other.m_bits.size());

    if (n >= m_num_bits)
      n %= m_num_bits;

    if (n == 0)
    {
      *this = other;

      return;
    }

    reset();
    for (uint32_t i=0; i < size(); i++)
      if (other.at((i+size()-n) % size()))
        set(i);
  }

  // create the left neighbour bits, i.e., for each '1' bit
  //  of the operand we set also the dt left bits in result
  void createLeftNeighbourMask(BitArray const other, int dt)
  {
    m_bits            = other.m_bits;
    m_num_bits        = other.m_num_bits;
    m_bitset_capacity = other.m_bitset_capacity;

    if ( (dt > 0) && (dt < int(bits_per_block)) )
    {
      buffer_type  lbits = other.m_bits;

      for (int ir = 0; ir < dt; ir++)
      {
        size_type   const last = num_blocks() - 1;     // num_blocks() is >= 1
        block_type *const b    = &lbits[0];
        block_type  prev       = static_cast<block_type>(0);
        block_type  newv;

        for (int i = last; i >= 0; --i)
        {
          newv = (b[i] << 1) | (prev >> (bits_per_block-1));
          prev = b[i];
          b[i] = newv;
        }

        for (size_type i = 0; i <= last; ++i)
        {
          m_bits[i] |= b[i];
        }
      }

      m_count = recount();
    }
  }

  // create the right neighbour bits, i.e., for each '1' bit
  //  of the operand we set also the dt right bits in result
  void createRightNeighbourMask(BitArray const other, int dt)
  {
    m_bits            = other.m_bits;
    m_num_bits        = other.m_num_bits;
    m_bitset_capacity = other.m_bitset_capacity;

    // distance must be less than the half of the size
    if ( (dt > 0) && (dt < int(bits_per_block)) )
    {
      buffer_type rbits = other.m_bits;

      for (int ir = 0; ir < dt; ir++)
      {
        size_type   const last = num_blocks() - 1;     // num_blocks() is >= 1
        block_type *const b    = &rbits[0];
        block_type  prev       = static_cast<block_type>(0);
        block_type  newv;

        for (size_type i = 0; i <= last; ++i)
        {
          newv = (b[i] >> 1) | (prev << (bits_per_block-1));
          prev = b[i];
          b[i] = newv;
        }

        for (size_type i = 0; i <= last; ++i)
        {
          m_bits[i] |= b[i];
        }
      }

      m_count = recount();
    }
  }

  [[nodiscard]] size_t size() const
  {
    return m_num_bits;
  }

  bool operator==(BitArray const &other) const noexcept
  {
    assert(size() == other.size());
    assert(m_bits.size() == other.m_bits.size());

    for (uint32_t i=0; i < m_bits.size(); i++)
      if (m_bits[i] != other.m_bits[i])
        return false;

    return true;
  }

  auto begin()
  {
    return m_bits.begin();
  }

  auto end()
  {
    return m_bits.end();
  }

  auto begin() const
  {
    return m_bits.begin();
  }

  auto end() const
  {
    return m_bits.end();
  }
};

#endif //FAST_ROTATE_BITARRAY_HPP
