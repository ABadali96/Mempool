#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <thread>
#include <mutex>

// Define a structure for each sub-pool
struct SubPool {
  uint32_t chunk_size;
  uint32_t chunk_count;
  std::vector<char> memory;
  std::vector<bool> used_chunks;

  // Statistics
  uint32_t peak_usage = 0;
  uint32_t unfreed_chunks = 0;

  // Constructor to initialize sub-pool with specified chunk size and count
  SubPool(uint32_t chunk_size, uint32_t chunk_count)
      : chunk_size(chunk_size), chunk_count(chunk_count), memory(chunk_size * chunk_count), used_chunks(chunk_count, false) {}

  // Allocate a chunk from the sub-pool
  void* Allocate()
  {
    for (uint32_t i = 0; i < chunk_count; i++)
    {
      // Avoid accessing out of range
      if (i < used_chunks.size() && !used_chunks[i])
      {
        // A chunk is found
        used_chunks[i] = true;
        // Peak usage is updated
        peak_usage = std::max(peak_usage, i+1);
        // Number of unfree chunks ıs updated
        unfreed_chunks++;
        // Return a pointer to the start of the chunk
        return &memory[i * chunk_size];
      }
    }
    // If no unused chunk is found, return null
    return nullptr;
  }

  // Deallocate a chunk to the sub-pool
  void Deallocate(void* ptr)
  {
    // If the given pointer is null, return
    if (ptr == nullptr)
    {
        return;
    }
    // Calculate the index of the corresponding chunk in subpool's memory
    uint32_t index = (reinterpret_cast<char*>(ptr) - &memory[0]) / chunk_size;
    // Check if the calculated index is within the range
    assert(index < chunk_count);
    // Check if the chunk was already allocated
    assert(used_chunks[index]);
    // Number of unfree chunks ıs updated
    unfreed_chunks--;
    // Deallocate the chunk
    used_chunks[index] = false;
  }
};

// Define the mempool class
class MemPool {
 public:
  MemPool(uint32_t max_memory, uint32_t alignment,
          const std::vector<uint32_t>& chunk_sizes)
      : max_memory_(max_memory),
        alignment_(alignment),
        chunk_sizes_(chunk_sizes) {
    // Sort the chunk sizes in ascending order
    std::sort(chunk_sizes_.begin(), chunk_sizes_.end());

    // Calculate the total memory required for the sub-pools
    uint32_t total_memory = 0;
    for (uint32_t chunk_size : chunk_sizes_)
    {
      total_memory += chunk_size * alignment;
    }

    // Ensure that the total memory does not exceed the maximum limit
    assert(total_memory <= max_memory);

    // Allocate memory for the sub-pools
    memory = std::unique_ptr<char[]>(new char[max_memory]);
    uint32_t offset = 0;
    // Calculating and allocating the memory of each sub-pool out of whole memory
    for (uint32_t chunk_size : chunk_sizes_)
    {
      uint32_t chunk_count = (max_memory - offset) / chunk_size;
      sub_pools_.emplace_back(chunk_size, chunk_count);
      offset += chunk_count * chunk_size;
    }
  }

  ~MemPool()
  {
    for (const auto& sub_pool : sub_pools_)
    {
      std::cout << "Sub-pool chunk size: " << sub_pool.chunk_size << "\n";
      std::cout << "Peak usage: " << sub_pool.peak_usage << "\n";
      std::cout << "Unfreed chunks: " << sub_pool.unfreed_chunks << "\n";
    }
    memory.reset(); // Deallocate the memory used by the sub-pools
  }

 private:
  uint32_t max_memory_;
  uint32_t alignment_;
  std::vector<uint32_t> chunk_sizes_;
  std::vector<SubPool> sub_pools_;
  std::unique_ptr<char[]> memory;
};