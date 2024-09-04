#include "mem/cache/replacement_policies/sieve_rp.hh"
#include "params/SIEVERP.hh"

namespace gem5 {

namespace replacement_policy {

SIEVE::SIEVE(const Params &p) : Base(p) {}

// Marks a cache line as invalid
void SIEVE::invalidate(
    const std::shared_ptr<ReplacementData> &replacement_data) {
  std::shared_ptr<SIEVEReplData> repl_data =
      std::static_pointer_cast<SIEVEReplData>(replacement_data);
  repl_data->visited = false;
  repl_data->isSafe = false;
}

// Called on access to a cache entry
// Updates the cache line's replacement data
void SIEVE::touch(
    const std::shared_ptr<ReplacementData> &replacement_data) const {

  // When the entry is touched, mark it as visited,
  // whether it is in the safe set or not
  std::static_pointer_cast<SIEVEReplData>(replacement_data)->visited = true;
}

void SIEVE::reset(
    const std::shared_ptr<ReplacementData> &replacement_data) const {
  std::shared_ptr<SIEVEReplData> repl_data =
      std::static_pointer_cast<SIEVEReplData>(replacement_data);
  repl_data->visited = false;
  repl_data->isSafe = false;
}

// Called for a cache miss/eviction
// Search the cache for a replacement candidate and evict it
ReplaceableEntry *
SIEVE::getVictim(const ReplacementCandidates &candidates) const {
  // There should be at least one eviction candidate
  assert(candidates.size() > 0);

  bool allSafe = true;
  bool allUnsafeAreVisited = true;
  for (const auto &candidate : candidates) {
    std::shared_ptr<SIEVEReplData> candidate_replacement_data =
        std::static_pointer_cast<SIEVEReplData>(candidate->replacementData);

    // Check if all the lines are marked safe
    if (!candidate_replacement_data->isSafe) {
      allSafe = false;
    }
    // Check if all the lines marked unsafe are visited
    if (!candidate_replacement_data->isSafe &&
        !candidate_replacement_data->visited) {
      allUnsafeAreVisited = false;
    }
  }
  // If all lines are marked safe, mark them all unsafe
  if (allSafe || allUnsafeAreVisited) {
    for (const auto &candidate : candidates) {
      auto replData =
          std::static_pointer_cast<SIEVEReplData>(candidate->replacementData);
      if (replData->isSafe) {
        // Move the lines in the safe set to the unsafe set
        replData->isSafe = false;
      } else {
        // Mark the lines in the unsafe set as unvisited
        replData->visited = false;
      }
    }
  }

  // Now we search for a victim
  ReplaceableEntry *victim = NULL;
  bool searchingVictim = true;
  unsigned int victimIndex = 0;
  do {
    // Select a new victim in cache order to test
    // In a circuit, this will be a priority encoder (in order)
    victim = candidates[victimIndex];
    // victim = candidates[random_mt.ramdom<unsigned>(0, candidates.size() -
    // 1)];
    auto victimData =
        std::static_pointer_cast<SIEVEReplData>(victim->replacementData);
    if (!victimData->isSafe && !victimData->visited) {
      // We randomly found a suitable candidate:
      // In the unsafe set and unvisited
      searchingVictim = false;
    } else if (!victimData->isSafe && victimData->visited) {
      victimData->isSafe = true;
      victimData->visited = false;
    }
    ++victimIndex;
  } while (searchingVictim && victimIndex < candidates.size());

  // If the algorithm never found a proper candidate, we have a problem
  assert(!searchingVictim);

  return victim;
}

std::shared_ptr<ReplacementData> SIEVE::instantiateEntry() {
  return std::shared_ptr<ReplacementData>(new SIEVEReplData());
}

} // namespace replacement_policy
} // namespace gem5
