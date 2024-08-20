/**
 * @file
 * Declaration of a Sieve replacement policy.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_SIEVE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_SIEVE_RP_HH__

#include "mem/cache/replacement_policies/base.hh"

namespace gem5 {

struct SIEVERPParams;

namespace replacement_policy {

class SIEVE : public Base {
protected:
  // SIEVE-specific implementation of replacement data
  // Parameter per cache line
  struct SIEVEReplData : ReplacementData {
    // Flag informing whether the cache line has been visited recently.
    // Set when touched and unset when the hand passes over it.
    bool visited;

    // Flag informing whether the cache line is in the safe set.
    // Set when touched and visited when the hand passes over it.
    // Unset when all lines are in the safe set.
    bool isSafe;

    // Cache lines start out unvisited in the unsafe set
    SIEVEReplData() : visited(false), isSafe(false) {}
  };

public:
  typedef SIEVERPParams Params;
  SIEVE(const Params &p);
  ~SIEVE() = default;

  void
  invalidate(const std::shared_ptr<ReplacementData> &replacement_data) override;

  void touch(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  // Called to initialize a cache line on entry insertion
  void reset(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  /**
   * Find replacement victim
   */
  ReplaceableEntry *
  getVictim(const ReplacementCandidates &candidates) const override;

  std::shared_ptr<ReplacementData> instantiateEntry() override;
}; // class SIEVE

} // namespace replacement_policy
} // namespace gem5

#endif // !__MEM_CACHE_REPLACEMENT_POLICIES_SIEVE_RP_HH__
