
#include <random>
#include <benchmark/benchmark.h>

#include "BitArray.hpp"

using block_type = uint64_t;

std::random_device rd;             // Will be used to obtain a seed for the random number engine
std::mt19937       gen(rd());  // Standard mersenne_twister_engine seeded with rd()

std::uniform_int_distribution distribution(1000,1500);

template <typename T>
BitArray<T> make_bitarray(int num_bits)
{
  BitArray<block_type>          bitarr(num_bits);
  std::uniform_int_distribution bitdistribution(0,num_bits-1);
  int                           num_ones = distribution(gen);

  for (int j=0; j < num_ones; j++)
  {
    int bit{bitdistribution(gen)};

    bitarr.set(bit);
  }

  return bitarr;
}

int   num_bits{distribution(gen)};

BitArray<block_type>   bitarr{make_bitarray<block_type>(num_bits)};


// Benchmark the rotate
static void BM_Rotate(benchmark::State& state)
{
  for (auto _: state)
  {
    for (int j = 1; j < num_bits; j++)
    {
      BitArray<block_type> bitarr_r(num_bits);
      bitarr_r.rotate(bitarr, j);
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_Rotate);

// Benchmark the rotateRight
static void BM_RotateRight(benchmark::State& state)
{
  for (auto _ : state)
  {
    for (int j=1; j < num_bits; j++)
    {
      BitArray<block_type> bitarr_r(num_bits);

      bitarr_r.rotateRight(bitarr,j);
    }
  }
}
BENCHMARK(BM_RotateRight);

BENCHMARK_MAIN();
