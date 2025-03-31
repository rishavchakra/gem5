#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_SPLRU_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_SPLRU_RP_HH__

#include "mem/cache/replacement_policies/base.hh"

#include <cmath>
#include <vector>

#include "params/SplruRP.hh"

namespace gem5 {

struct SplruRPParams;

namespace replacement_policy {

class Splru : public Base {
private:
  struct SplruNode {
    SplruNode *left;
    SplruNode *right;
    SplruNode *parent;
    bool direction;

    SplruNode(SplruNode *parent, int depth, int ind, SplruNode **leaf_arr)
        : direction(false), parent(parent) {
      // Double check this indexing
      if (depth >= 0) {
        this->left = new SplruNode(this, depth - 1, ind * 2, leaf_arr);
        this->right = new SplruNode(this, depth - 1, ind * 2 + 1, leaf_arr);
      } else {
        this->left = nullptr;
        this->right = nullptr;
        leaf_arr[ind] = this;
      }
    }

    ~SplruNode() {
      if (this->left != nullptr && this->right != nullptr) {
        delete left;
        delete right;
      }
    }
  };

protected:
  struct SplruReplData : ReplacementData {
    size_t leaf_ind;
    bool valid;

    SplruReplData(size_t ind);
  };

private:
  struct SplruTree {
    SplruNode *cold;
    SplruNode *hot;
    SplruNode *probation;
    SplruNode **leaf_nodes;
    SplruReplData **repl_data_arr;
    SplruNode *trash_node;
    size_t assoc;

    SplruTree(int assoc) {
      int tree_depth = int(log(tree_depth));
      SplruNode **leaf_nodes = new SplruNode *[assoc];
      SplruReplData **repl_data_arr = new SplruReplData *[assoc];
      SplruNode *tree = new SplruNode(nullptr, tree_depth, 0, leaf_nodes);
      SplruNode *first_left = tree->left;
      tree->left = first_left->right;
      this->probation = first_left->right;
      this->hot = tree;
      this->cold = first_left->left;
      this->cold->parent = nullptr;
      first_left->left = nullptr;
      first_left->right = nullptr;
      this->trash_node = first_left;
      this->assoc = assoc;
    }

    ~SplruTree() {
      delete leaf_nodes;
      delete repl_data_arr;
      delete hot;
      delete cold;
      delete trash_node;
    }
  };

  size_t count;
  SplruTree *tree;

  // SPLRU variant parameters
  // Cold tree type
  // 0: Random
  // 1: LRU
  // 2: FIFO
  int cold_repl_type;
  // Hot tree type
  // 0: Random
  // 1: LRU
  // 2: FIFO
  int hot_repl_type;
  // Probation type
  // 0: never place new elements in probation
  // 1: 50% place new items in probation
  // 2: 25% place new items in probation
  int probation_type;

public:
  typedef SplruRPParams Params;
  Splru(const Params &p);
  ~Splru() = default;

  // Invalidate an entry
  void
  invalidate(const std::shared_ptr<ReplacementData> &replacement_data) override;

  // Touch the entry
  void touch(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  // Reset replacement data - provides the same functionality as touch?
  void reset(
      const std::shared_ptr<ReplacementData> &replacement_data) const override;

  // Find an eviction candidate
  ReplaceableEntry *
  getVictim(const ReplacementCandidates &candidates) const override;

  // Instantiate a replacement data entry
  std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif
