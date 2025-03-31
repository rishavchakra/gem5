#include "mem/cache/replacement_policies/splru_rp.hh"
#include "base/logging.h"
#include "params/SplruRP.hh"

namespace gem5 {

namespace replacement_policy {

Splru::SplruReplData::SplruReplData(size_t ind) : leaf_ind(ind) {}

Splru::Splru(const Params &p)
    : Base(p), cold_repl_type(p.cold_repl), hot_repl_type(p.hot_repl),
      probation_type(p.probation_type) {
  fatal_if(p.assoc < 4, "Associativity (assoc) cannot be less than 4");
  tree = new SplruTree(p.assoc);
  count = 0;
  valid = true;
  fatal_if(p.cold_repl < 0 || p.cold_repl > 2,
           "Cold Queue replacement flag invalid");
  fatal_if(p.hot_repl < 0 || p.hot_repl > 2,
           "Hot Queue replacement flag invalid");
  fatal_if(p.probation_type < 0 || p.probation_type > 2,
           "Probation choice flag invalid");
}

void Splru::invalidate(const std::shared_ptr<ReplacementData> &repl_data) {
  std::shared_ptr<SplruReplData> splru_repl =
      std::static_pointer_cast<SplruReplData>(repl_data);
  SplruTree *tree = this->tree;
  size_t leaf_ind = splru_repl->leaf_ind;

  // Actually... maybe I don't need to do anything to invalidate lines
  // TODO: maybe add an invalid bool to each line, first preference for eviction
  splru_repl->valid = false;
}

void Splru::touch(const std::shared_ptr<ReplacementData> &repl_data) const {
  std::shared_ptr<SplruReplData> splru_repl =
      std::static_pointer_cast<SplruReplData>(repl_data);
  splru_repl->valid = true;
  SplruTree *tree = this->tree;
  size_t leaf_ind = splru_repl->leaf_ind;
  size_t cold_assoc = this->tree->assoc / 4;
  size_t hot_ind = leaf_ind;

  if (leaf_ind < cold_assoc) {
    size_t evict_ind = 0;
    if (this->cold_repl_type == 1 || this->cold_repl_type == 2) {
      // LRU or FIFO eviction: trace the tree down
      SplruNode *trace_node = tree->cold;
      while (trace_node->left != nullptr && trace_node->right != nullptr) {
        if (trace_node->direction) {
          trace_node = trace_node->right;
          evict_ind = evict_ind * 2 + 1;
        } else {
          trace_node = trace_node->right;
          evict_ind = evict_ind * 2;
        }
      }
    } else {
      // Random eviction
      evict_ind = rand() % cold_assoc;
    }

    {
      // Swap the leaf nodes and the repl data at evict_ind and leaf_ind
      SplruNode temp_node;
      SplruReplData temp_repl;
      memcpy(&temp_node, tree->leaf_nodes[evict_ind], sizeof(SplruNode));
      memmove(tree->leaf_nodes[evict_ind], tree->leaf_nodes[leaf_ind],
              sizeof(SplruNode));
      memcpy(tree->leaf_nodes[leaf_ind], &temp_node, sizeof(SplruNode));

      memcpy(&temp_repl, tree->repl_data_arr[evict_ind], sizeof(SplruReplData));
      memmove(tree->repl_data_arr[evict_ind], tree->repl_data_arr[leaf_ind],
              sizeof(SplruReplData));
      memcpy(tree->repl_data_arr[leaf_ind], &temp_repl, sizeof(SplruReplData));
    }
    hot_ind = evict_ind;
  }

  if (this->hot_repl_type == 1) {
    // LRU
    SplruNode *trace = tree->leaf_nodes[hot_ind];
    while (trace->parent != NULL) {
      bool is_left_child = trace->parent->left == trace;
      if (is_left_child) {
        trace->parent->direction = true;
      } else {
        trace->parent->direction = false;
      }
      trace = trace->parent;
    }
  } else if (this->hot_repl_type == 2) {
    // FIFO
    SplruNode *trace = tree->leaf_nodes[hot_ind];
    while (trace->parent != NULL) {
      bool is_left_child = trace->parent->left == trace;
      if (is_left_child) {
        trace->parent->direction = true;
        break;
      } else {
        trace->parent->direction = false;
      }
      trace = trace->parent;
    }
  }
}

void Splru::reset(const std::shared_ptr<ReplacementData> &repl_data) const {
  touch(repl_data);
}

ReplaceableEntry *
Splru::getVictim(const ReplacementCandidates &candidates) const {
  // for (const auto &candidate : candidates) {
  //   SplruReplData *cand_repl_data = candidate->replacement_data;
  //   if (!cand_repl_data->valid) {
  //
  //   }
  // }
  // Should seek from the cold queue
  // or, if the probation flag allows it, occasionally from the probation area
  SplruNode *trace_node;
  if (probation_type == 0) {
    // Never
    trace_node = this->tree->cold;
  } else if (probation_type == 1) {
    // Half random
    if (rand() % 2 == 0) {
      trace_node = this->tree->cold;
    } else {
      trace_node = this->tree->probation;
    }
  } else if (probation_type == 2) {
    // Quarter random
    if (rand() % 4 == 0) {
      trace_node = this->tree->cold;
    } else {
      trace_node = this->tree->probation;
    }
  }

  if (cold_repl_type == 1 || cold_repl_type == 2) {
    // Tree-based eviction strategies
    size_t evict_ind = 0;
    while (trace_node->left != NULL && trace_node->right != NULL) {
      if (trace_node->direction) {
        trace_node = trace_node->right;
        evict_ind = evict_ind * 2 + 1;
      } else {
        trace_node = trace_node->left;
        evict_ind = evict_ind * 2;
      }
    }
  } else {
    // Random eviction
    size_t evict_ind = 0;
    while (trace_node->left != NULL && trace_node->right != NULL) {
      if (rand() % 2 == 0) {
        trace_node = trace_node->right;
        evict_ind = evict_ind * 2 + 1;
      } else {
        trace_node = trace_node->left;
        evict_ind = evict_ind * 2;
      }
    }
  }
  return candidates.at(evict_ind);
}

std::shared_ptr<ReplacementData> Splru::instantiateEntry() {
  fatal_if(this->count > this->tree->assoc,
           "How did count get bigger than assoc?");

  SplruReplData *repl = new SplruReplData(count);
  this->tree->repl_data_arr[count] = repl;
  this->count++;
  return std::shared_ptr<ReplacementData>(repl);
}

} // namespace replacement_policy
} // namespace gem5
