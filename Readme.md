# Dynamic Bit Array with Fast Rotate Operation

An implementation of dynamic bit array created to store and handle a time series of neuron spikes.
The existing implementation of [Boost dynamic_bitest](https://www.boost.org/doc/libs/1_82_0/libs/dynamic_bitset/dynamic_bitset.html) 
lacks some important operations needed for the implementation of algorithms that handle the 
[Clustering Coefficients](https://pubmed.ncbi.nlm.nih.gov/25339742/) problem.
The most important operation is an efficient implementation of the 'rotate' or 'circular shift' function. 
The rotate algorithm is very simple if we apply it on each element (bit). 
The [STL library](https://en.cppreference.com/w/cpp/algorithm/rotate)
has implementations of rotate algorithm using iterators and a custom implementation
of the elementwise algorithm for each bit of the set is simple. 
The Boost library implements the left and right sift operations using 
blocks of bits, i.e., the storage integral type for bits, e.g., uint8_t, uint16_t, uint32_t or uint64_t, but 
the 'rotate' operation is missing. In BitArray we implement the rotation using blocks offering an efficient 
alternative. 

Another problem we have to solve is the following. Having two (2) time series, i.e., bit arrays A and B, we want 
to find how many spikes, i.e., 'one bits', of B exists in left of bits of A in a range Dt. I.e., we want to 
know if a spike of A is concurrent or follows a spike in B within a time Dt. Similarly, for the case of right 
direction. To support these functions we defined two operations, the 'CreateLeftNeighbourMask(int Dt)' and
'CreateRightNeighbourMask(int Dt)' which sets the left or right bits in a distance Dt. 
Having these masks The functions then are reduced to a bitwise **and** (&) between two arrays and count of 'one bites'.

There are two programs that test the operations and measure the efficiency of the rotate function.  
The TestBitArray.cpp tests the functionality of 'rotate' and 'CreateMaks' functions. 
The following image shows time periods for elementwise rotate, the rotateRight() on the left of the top row 
and the blockwise efficient implementation on the right. This image produced from the profiler 
of [CLion IDE](https://www.jetbrains.com/clion/). 
![img.png](flamegraph.png)

The second program is `BenchRotate.cpp` which uses the [Google Benchmark](https://github.com/google/benchmark) 
library to measure the time of the two versions of `rotate` function. The results are like the following. The 
`BM_Rotate` is our blockwise implementation of rotate and the `BM_RotateRight` is the elementwise implementation.

~~~
---------------------------------------------------------
Benchmark               Time             CPU   Iterations
---------------------------------------------------------
BM_Rotate           86687 ns        86687 ns         7612
BM_RotateRight   11240405 ns     11240271 ns           63
~~~