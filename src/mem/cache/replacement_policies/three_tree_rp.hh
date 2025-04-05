#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_3TREE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_3TREE_RP_HH__

#include "mem/cache/replacement_policies/base.hh"

#include <cmath>
#include <vector>

#include "params/ThreeTreeRP.hh"

namespace gem5 {

struct ThreeTreeRPParams;

namespace replacement_policy {

class ThreeTree : public Base {
private:
  struct ThreeTreeNode {
    ThreeTreeNode *left;
    ThreeTreeNode *right;
    ThreeTreeNode *parent;
    bool direction;

    ThreeTreeNode(ThreeTreeNode *parent, int depth, int ind,
                  ThreeTreeNode **leaf_arr)
        : direction(false), parent(parent) {
      // Double check this indexing
      if (depth >= 0) {
        this->left = new ThreeTreeNode(this, depth - 1, ind * 2, leaf_arr);
        this->right = new ThreeTreeNode(this, depth - 1, ind * 2 + 1, leaf_arr);
      } else {
        this->left = nullptr;
        this->right = nullptr;
        leaf_arr[ind] = this;
      }
    }

    ~ThreeTreeNode() {
      if (this->left != nullptr && this->right != nullptr) {
        delete left;
        delete right;
      }
    }
  };

protected:
  struct ThreeTreeReplData : ReplacementData {
    size_t leaf_ind;

    ThreeTreeReplData(size_t ind);
  };

private:
  struct ThreeTreeTree {
    ThreeTreeNode *cold;
    ThreeTreeNode *hot;
    ThreeTreeNode *probation;
    ThreeTreeNode **leaf_nodes;
    ThreeTreeReplData **repl_data_arr;
    ThreeTreeNode *trash_node;
    size_t assoc;

    ThreeTreeTree(int assoc) {
      int tree_depth = int(log(tree_depth));
      ThreeTreeNode **leaf_nodes = new ThreeTreeNode *[assoc];
      ThreeTreeReplData **repl_data_arr = new ThreeTreeReplData *[assoc];
      ThreeTreeNode *tree =
          new ThreeTreeNode(nullptr, tree_depth, 0, leaf_nodes);
      ThreeTreeNode *first_left = tree->left;
      tree->left = first_left->right;
      this->probation = first_left->right;
      this->hot = tree;
      this->cold = first_left->left;
      this->cold->parent = nullptr;
      this->probation->parent = this->hot;
      this->leaf_nodes = leaf_nodes;
      first_left->left = nullptr;
      first_left->right = nullptr;
      this->trash_node = first_left;
      this->assoc = assoc;
      this->repl_data_arr = repl_data_arr;
    }

    ~ThreeTreeTree() {
      delete leaf_nodes;
      delete repl_data_arr;
      delete hot;
      delete cold;
      delete trash_node;
    }

    void swap_leaves(size_t ind1, size_t ind2) {
      // ThreeTreeNode temp_node = *this->leaf_nodes[ind1];
      // *this->leaf_nodes[ind1] = *this->leaf_nodes[ind2];
      // *this->leaf_nodes[ind2] = temp_node;
      ThreeTreeReplData temp_repl = *this->repl_data_arr[ind1];
      *this->repl_data_arr[ind1] = *this->repl_data_arr[ind2];
      *this->repl_data_arr[ind2] = temp_repl;
    }

    // Get the index of the cold queue element closest to eviction
    size_t get_victim(ThreeTreeNode *root, int repl_type) {
      if (repl_type == 1 || repl_type == 2) {
        // LRU and FIFO selection
        ThreeTreeNode *trace_node = root;
        size_t evict_ind = 0;
        while (trace_node->left != nullptr && trace_node->right != nullptr) {
          if (trace_node->direction) {
            trace_node = trace_node->right;
            evict_ind = evict_ind * 2 + 1;
          } else {
            trace_node = trace_node->right;
            evict_ind = evict_ind * 2;
          }
        }
        if (root == this->cold) {
          return evict_ind;
        } else {
          if (evict_ind < this->assoc / 4) {
            // Selecting from the probation queue, whether tracing from hot or
            // prob One less level of recursion means different offset
            return evict_ind + (this->assoc / 4);
          } else {
            // Hot queue correctly indexes using recursion
            return evict_ind;
          }
        }
      } else {
        // Random selection
        // Range of random values depends on the root node used
        if (root == this->cold) {
          return rand() % (this->assoc / 4);
        } else if (root == this->hot) {
          size_t hot_assoc = this->assoc - (this->assoc / 4);
          return (rand() % hot_assoc) + (this->assoc / 4);
        } else if (root == this->probation) {
          return (rand() % (this->assoc / 4)) + (this->assoc / 4);
        } else {
          // What did you pass in??
          return rand() % this->assoc;
        }
      }
    }

    // Get the index of the hot queue element furthest from eviction
    size_t get_safe(ThreeTreeNode *root, int repl_type) {
      if (repl_type == 1 || repl_type == 2) {
        // LRU and FIFO selection
        ThreeTreeNode *trace_node = root;
        size_t evict_ind = 0;
        while (trace_node->left != nullptr && trace_node->right != nullptr) {
          if (trace_node->direction) {
            trace_node = trace_node->right;
            evict_ind = evict_ind * 2 + 1;
          } else {
            trace_node = trace_node->right;
            evict_ind = evict_ind * 2;
          }
        }
        return evict_ind + (this->assoc / 4);
      } else {
        // Random selection
        return (rand() % (this->assoc / 4)) + (this->assoc / 4);
      }
    }

    // Touch an element within its queue. Does not alter the other queue
    // is_first_placement: differentiating between placement and promotion
    // touching
    void touch(size_t ind, int repl_type, bool is_first_placement) {
      // Random (0) has no touching logic
      if (repl_type == 1 || repl_type == 2) {
        // LRU or FIFO differ only in re-touching logic, not first insertion
        if (repl_type == 2 && !is_first_placement) {
          return;
        }
        ThreeTreeNode *trace = this->leaf_nodes[ind];
        while (trace->parent != nullptr) {
          bool is_left_child = trace->parent->left == trace;
          if (is_left_child) {
            trace->parent->direction = true;
          } else {
            trace->parent->direction = false;
          }
          trace = trace->parent;
        }
      }
    }
  };

  size_t count;
  ThreeTreeTree *tree;

  // 3Tree variant parameters
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
  // 2: 1/8 place new items in probation
  int probation_type;

public:
  typedef ThreeTreeRPParams Params;
  ThreeTree(const Params &p);
  ~ThreeTree() = default;

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
