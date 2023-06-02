/**
 * @file BitArray.hpp
 *
 * @brief Implementation of Bit Array type with fast rotate rotate operations
 *
 * @ingroup StrictClusteringCoefficient
 *
 * @author Christos Tsalidis
 * Contact: tsalidis@neurolingo.gr
 * Created on 01/06/2023.
 *
 */

#ifndef BITARRAYFASTROTATE_STATICBITARRAY_HPP
#define BITARRAYFASTROTATE_STATICBITARRAY_HPP

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

template <size_t N, typename Block=std::uint64_t>
class StaticBitArray
{
public:

#ifdef __cpp_constinit
    static constinit const size_t num_of_bits    = N;
    static constinit const size_t bits_per_block = std::numeric_limits<Block>::digits;
    static constinit const size_t num_of_blocks  = (num_of_bits-1) / bits_per_block + 1;
#else
    static constexpr const size_t bits_per_block = std::numeric_limits<Block>::digits;
    static constexpr const size_t bits_per_block = std::numeric_limits<Block>::digits;
    static constexpr const size_t num_of_blocks  = (num_of_bits-1) / bits_per_block + 1;
#endif /*  __cpp_constinit */


    using block_type       = Block;
    using size_type        = size_t;
    using buffer_type      = Block[num_of_blocks];
    using block_width_type = unsigned short;

private:
    size_t       m_count{0};
    buffer_type  m_bits;

private:
    static size_type        block_index(size_type pos) noexcept { return pos / bits_per_block; }
    static block_width_type bit_index  (size_type pos) noexcept { return static_cast<block_width_type>(pos % bits_per_block); }
    static Block            bit_mask   (size_type pos) noexcept { return Block(1) << bit_index(pos); }

public:

    StaticBitArray()
    {
      reset();
    }

    StaticBitArray(StaticBitArray const &) = default;
    StaticBitArray(StaticBitArray &&) noexcept = default;

    StaticBitArray &operator=(StaticBitArray const &) = default;
    StaticBitArray &operator=(StaticBitArray &&) noexcept = default;

    ~StaticBitArray() = default;


#ifdef __cpp_consteval
  [[nodiscard]] static consteval size_t size() noexcept
  {
    return num_of_bits;
  }

  [[nodiscard]] static consteval size_type num_blocks() noexcept
  {
    return num_of_blocks;
  }
#else
  [[nodiscard]] static  constexpr size_t size() noexcept
  {
    return num_of_bits;
  }

  [[nodiscard]] static constexpr size_type num_blocks() noexcept
  {
    return num_of_blocks;
  }
#endif  /* __cpp_consteval */


    StaticBitArray  &set(uint32_t pos)
    {
      assert(pos < num_of_bits);

      m_bits[block_index(pos)] |= bit_mask(pos);
      m_count++;

      return *this;
    }

    StaticBitArray &clear(uint32_t pos)
    {
      assert(pos < size());

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

    [[nodiscard]] size_t count() const noexcept
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
    [[nodiscard]]  size_t common(StaticBitArray const &other)
    {
      size_t _count = 0;

      for (block_type const *lp=begin(), *rp=other.begin(), *lend=m_bits.end(); lp < lend; lp++,rp++)
        _count += std::popcount(*lp & *rp);

      return _count;
    }

    // The fast blockwise implementation of (right) rotate
    NOINLINE void rotate(StaticBitArray const &other, size_t n)
    {
      if (n >= num_of_bits)
        n %= num_of_bits;

      if (n == 0)
      {
        *this = other;

        return;
      }

      size_type  start_blk_pos{block_index(num_of_bits - n)};
      size_type  start_bit_pos{bit_index(num_of_bits - n)};

      size_type  opos{0};
      size_type  iblk_pos{start_blk_pos};
      size_type  ibit_pos{start_bit_pos};
      size_type  last_bits = num_of_bits % bits_per_block;

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
            m_bits[opos] = block;
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

            m_bits[opos] = block;
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

        m_bits[opos] = block;
        opos++;
      }
    }

    // the slow classical elementwise implementation of rotate
    NOINLINE void rotateRight(StaticBitArray const &other, size_t n)
    {
      if (n >= num_of_bits)
        n %= num_of_bits;

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
    void createLeftNeighbourMask(StaticBitArray const other, int dt)
    {
#ifdef __cpp_lib_ranges
      std::ranges::copy(other.begin(),other.end(),begin());
#else
      std::copy(other.begin(),other.end(),begin());
#endif /* __cpp_lib_ranges */
      if ( (dt > 0) && (dt < int(bits_per_block)) )
      {
        buffer_type  lbits;

#ifdef __cpp_lib_ranges
        std::ranges::copy(other.begin(),other.end(),std::begin(lbits));
#else
        std::copy(other.begin(),other.end(),std::begin(lbits));
#endif /* __cpp_lib_ranges */

        for (int ir = 0; ir < dt; ir++)
        {
          size_type   const last = num_blocks() - 1;     // num_blocks() is >= 1
          block_type *const b    = &lbits[0];
          auto              prev = static_cast<block_type>(0);
          block_type        newv;

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
    void createRightNeighbourMask(StaticBitArray const other, int dt)
    {
#ifdef __cpp_lib_ranges
      std::ranges::copy(other.begin(),other.end(),begin());
#else
      std::copy(other.begin(),other.end(),begin());
#endif /* __cpp_lib_ranges */

      // distance must be less than the half of the size
      if ( (dt > 0) && (dt < int(bits_per_block)) )
      {
        buffer_type rbits;

#ifdef __cpp_lib_ranges
        std::ranges::copy(other.begin(),other.end(),std::begin(rbits));
#else
        std::copy(other.begin(),other.end(),std::begin(rbits));
#endif /* __cpp_lib_ranges */

        for (int ir = 0; ir < dt; ir++)
        {
          size_type   const last = num_blocks() - 1;     // num_blocks() is >= 1
          block_type *const b    = &rbits[0];
          auto              prev = static_cast<block_type>(0);
          block_type        newv;

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

    bool operator==(StaticBitArray const &other) const noexcept
    {
      for (uint32_t i=0; i < num_blocks(); i++)
        if (m_bits[i] != other.m_bits[i])
          return false;

      return true;
    }

    auto begin()
    {
      return std::begin(m_bits);
    }

    auto end()
    {
      return std::end(m_bits);
    }

    auto begin() const
    {
      return std::begin(m_bits);
    }

    auto end() const
    {
      return std::end(m_bits);
    }
};

#endif //BITARRAYFASTROTATE_STATICBITARRAY_HPP
