/**
 * @file scctest.cpp
 *
 * @brief test case for NeuronSpikes.hpp
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

#include "BitArray.hpp"

template <typename T>
void print(BitArray<T> const &bitarr)
{
  std::cout << "BitArray Block: " << BitArray<T>::bits_per_block << ", size: " << bitarr.size() << "\n";

  for (T block : bitarr)
  {
    std::bitset<BitArray<T>::bits_per_block>  bset(block);

    std::cout << bset << std::endl;
  }

  std::cout << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]]  char **argv)
{
  int  num_tests = 1000;

  if (argc == 2)
    num_tests = std::stoi(argv[1]);

  std::cout << "Testing BitArray.hpp" << std::endl;

  using block_type = uint64_t;

  std::random_device rd;        // Will be used to obtain a seed for the random number engine
  std::mt19937       gen(rd()); // Standard mersenne_twister_engine seeded with rd()

  std::uniform_int_distribution distribution(100,1500);

  for (uint32_t i=0; i < num_tests; i++)
  {
    int   num_bits{distribution(gen)};

    BitArray<block_type>          bitarr(num_bits);
    std::uniform_int_distribution bitdistribution(0,num_bits-1);
    uint32_t                      num_ones = static_cast<uint32_t>(distribution(gen));

    for (uint32_t j=0; j < num_ones; j++)
    {
      int bit{bitdistribution(gen)};

      if (bit >= num_bits)
        throw std::invalid_argument("Wrong value from random number generator");

      bitarr.set(bit);
    }

    uint32_t  num_rotates = static_cast<uint32_t>(num_bits);

    for (uint32_t j=1; j < num_rotates; j++)
    {
      BitArray<block_type> bitarr_r1(num_bits);
      BitArray<block_type> bitarr_r2(num_bits);

      int num_shifts{bitdistribution(gen)};

      bitarr_r1.rotate(bitarr,num_shifts);
      bitarr_r2.rotateRight(bitarr,num_shifts);

      if (bitarr_r1 != bitarr_r2)
      {
        std::cout << "***** spikes >> " << num_bits << " *****" << std::endl;
        print(bitarr);

        std::cout << std::endl << "***** rotate *****" << std::endl;
        print(bitarr_r1);

        std::cout << std::endl << "***** rotate right *****" << std::endl;
        print(bitarr_r2);

        break;
      }
    }
  }

  return 0;
}
