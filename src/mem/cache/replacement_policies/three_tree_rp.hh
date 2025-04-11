#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_THREE_TREE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_THREE_TREE_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include <stddef.h>

namespace gem5 {

struct ThreeTreeRPParams;

namespace replacement_policy {

class ThreeTree : public Base {
protected:
  struct ThreeTreeReplData : ReplacementData {
    size_t tree_index;
    size_t cache_index;
    std::vector<bool> *tree;
    ThreeTreeReplData(int cache_index);
  };

private:
  std::vector<bool> cold_tree;
  std::vector<bool> prob_tree;
  std::vector<bool> hot_tree;

  ThreeTreeReplData **cold_repl_arr;
  ThreeTreeReplData **prob_repl_arr;
  ThreeTreeReplData **hot_repl_arr;

  size_t cold_depth;
  size_t prob_depth;
  size_t hot_depth;

  size_t assoc;
  size_t obj_count;

public:
  typedef ThreeTreeRPParams Params;
  ThreeTree(const Params &p);
  ~ThreeTree() = default;

  void
  invalidate(const std::shared_ptr<ReplacementData> &replacement_data) override;

  void touch(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  ReplaceableEntry *
  getVictim(const ReplacementCandidates &candidates) const override;

  std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_THREE_TREE_RP_HH__
