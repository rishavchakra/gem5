#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_THREE_TREE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_THREE_TREE_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include <memory>
#include <stddef.h>

namespace gem5 {

struct ThreeTreeRPParams;

namespace replacement_policy {

class ThreeTree : public Base {
  typedef std::vector<bool> TTTree;

protected:
  struct ThreeTreeReplData : ReplacementData {
    size_t tree_index;
    size_t cache_index;
    // std::shared_ptr<Tree> tree;

    // The tree group that this cache line belongs to
    // This cache line must be a member of only one of these trees
    std::shared_ptr<TTTree> cold_tree;
    std::shared_ptr<TTTree> prob_tree;
    std::shared_ptr<TTTree> hot_tree;

    // Pointers to the group of arrays that this line belongs to
    std::shared_ptr<std::vector<ThreeTreeReplData *>> cold_repl_arr;
    std::shared_ptr<std::vector<ThreeTreeReplData *>> prob_repl_arr;
    std::shared_ptr<std::vector<ThreeTreeReplData *>> hot_repl_arr;

    // ThreeTreeReplData(int cache_index);
    //
    ThreeTreeReplData(int cache_index, std::shared_ptr<TTTree> cold_tree,
                      std::shared_ptr<TTTree> prob_tree,
                      std::shared_ptr<TTTree> hot_tree);
  };

private:
  // Pointers to the most recent instances of these structures
  std::shared_ptr<TTTree> cold_tree;
  std::shared_ptr<TTTree> prob_tree;
  std::shared_ptr<TTTree> hot_tree;

  std::shared_ptr<std::vector<ThreeTreeReplData *>> cold_repl_arr;
  std::shared_ptr<std::vector<ThreeTreeReplData *>> prob_repl_arr;
  std::shared_ptr<std::vector<ThreeTreeReplData *>> hot_repl_arr;

  size_t cold_depth;
  size_t prob_depth;
  size_t hot_depth;

  size_t assoc;
  size_t obj_count;

public:
  typedef ThreeTreeRPParams Params;
  ThreeTree(const Params &p);
  ~ThreeTree();

  void
  invalidate(const std::shared_ptr<ReplacementData> &replacement_data) override;

  void touch(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  void reset(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  ReplaceableEntry *
  getVictim(const ReplacementCandidates &candidates) const override;

  std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_THREE_TREE_RP_HH__
