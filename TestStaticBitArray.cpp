/**
 * @file TestStaticBitArray.cpp
 *
 * @brief test case for StaticBitArray.hpp
 *
 * @ingroup StrictClusteringCoefficient
 *
 * @author Christos Tsalidis
 * Contact: tsalidis@neurolingo.gr
 * Created on 13/4/2023.
 *
 */

#include <iostream>
#include <bitset>
#include <random>
#include <cstring>

#include "StaticBitArray.hpp"

template <size_t N, typename T>
void print(StaticBitArray<N,T> const &bitarr, std::string_view msg)
{
  std::cout << "StaticBitArray (" << msg << ") "
            << "Block Size: " << StaticBitArray<N,T>::bits_per_block
            << ", Array Size: " << bitarr.size()
            << ", Count(Num of ones): " << bitarr.count() << "\n";

  for (T block : bitarr)
  {
    std::bitset<StaticBitArray<N,T>::bits_per_block>  bset(block);

    std::cout << bset << std::endl;
  }

  std::cout << std::endl;
}

template <size_t N>
void TestBitArrayFastRotate(int num_tests)
{
  std::cout << "Testing StaticBitArray<" << N << ">" << std::endl;

  using block_type = uint64_t;
  constexpr size_t  num_bits = N;

  std::random_device rd;        // Will be used to obtain a seed for the random number engine
  std::mt19937       gen(rd()); // Standard mersenne_twister_engine seeded with rd()

  std::uniform_int_distribution distribution(0,int(num_bits-1));

  for (int i=0; i < num_tests; i++)
  {
    StaticBitArray<num_bits,block_type>  bitarr;
    uint32_t                      num_ones = static_cast<uint32_t>(distribution(gen));

    for (uint32_t j=0; j < num_ones; j++)
    {
      int bit{distribution(gen)};

      if (bit >= num_bits)
        throw std::invalid_argument("Wrong value from random number generator");

      bitarr.set(bit);
    }

    uint32_t  num_rotates = static_cast<uint32_t>(num_bits);

    for (uint32_t j=1; j < num_rotates; j++)
    {
      StaticBitArray<num_bits,block_type> bitarr_r1;
      StaticBitArray<num_bits,block_type> bitarr_r2;

      int num_shifts{distribution(gen)};

      bitarr_r1.rotate(bitarr,num_shifts);
      bitarr_r2.rotateRight(bitarr,num_shifts);

      if (bitarr_r1 != bitarr_r2)
      {
        print(bitarr, "input");

        print(bitarr_r1,"block rotate");

        print(bitarr_r2,"element rotate");

        break;
      }
    }
  }
}

template <size_t N>
void TestMaskCreation()
{
  std::cout << "Testing StaticBitArray<" << N << "> Left & Right Masks" << std::endl;
  using block_type = uint64_t;

  std::size_t constexpr num_bits = N;

  StaticBitArray<num_bits,block_type> spikes_1;
  StaticBitArray<num_bits,block_type> spikes_2;

  char const *bits_1 = "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000001"
                       "0000000000000000000000000000000000000000000000000000000000000000";
  char const *bits_2 = "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000"
                       "1000000000000000000000000000000000000000000000000000000000000000";
  size_t len = std::strlen(bits_1);

  for (size_t i=0; i < len; i++)
  {
    if (bits_1[i] == '1')
      spikes_1.set(i);

    if (bits_2[i] == '1')
      spikes_2.set(i);
  }

  StaticBitArray<num_bits,block_type> shift_spikes;

  print(spikes_1, "input");
  shift_spikes.createLeftNeighbourMask(spikes_1, 2);

  print(shift_spikes,"left neighbours -2-");

  shift_spikes.createRightNeighbourMask(spikes_1, 2);
  print(shift_spikes,"right neighbours -2-");

  // second array
  print(spikes_2, "input");
  shift_spikes.createLeftNeighbourMask(spikes_2, 2);

  print(shift_spikes,"left neighbours -2-");

  shift_spikes.createRightNeighbourMask(spikes_2, 2);
  print(shift_spikes,"right neighbours -2-");
}

int main([[maybe_unused]] int argc, [[maybe_unused]]  char **argv)
{
  int  num_tests = 1000;

  if (argc == 2)
    num_tests = std::stoi(argv[1]);

  TestBitArrayFastRotate<357>(num_tests);
  TestMaskCreation<631>();

  return 0;
}
