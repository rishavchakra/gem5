/**
 * @file
 * Declaration of a Sieve replacement policy.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_TREE_SIEVE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_TREE_SIEVE_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include <cstdint>
#include <memory>
#include <vector>

namespace gem5 {

struct TreeSIEVERPParams;

namespace replacement_policy {
class TreeSIEVE : public Base {
private:
  typedef std::vector<bool> SIEVETree;

  uint64_t count;
  SIEVETree *tree;

  const uint64_t assoc;

  const uint64_t tree_depth;

  const uint64_t num_nodes;

protected:
  struct TreeSIEVEReplData : ReplacementData {

    bool stale;

    // TreeSIEVEReplData(const uint64_t index, std::shared_ptr<SIEVETree> tree);
    TreeSIEVEReplData() : stale(true) {}
  };

public:
  typedef TreeSIEVERPParams Params;
  TreeSIEVE(const Params &p);
  ~TreeSIEVE() = default;

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
}; // class TreeSIEVE
} // namespace replacement_policy
} // namespace gem5

#endif
