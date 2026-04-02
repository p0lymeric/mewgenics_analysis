#include "types/glaiel.hpp"

#include <unordered_map>
#include <memory>

// Utilities for producing CatData objects.
//
// These functions directly load CatData instances from a save using the game's deserialization routine
// (glaiel::SerializeCatData), bypassing the normal engine's process that registers loaded cats into a map.
// As a result, this gives us the ability to load any saved cat into our analyzer, without disturbing the game's state.
//
// polymeric 2026

struct CatDataDeleter {
    void operator()(CatData *p_cat) const;
};

using ManagedCatData = std::unique_ptr<CatData, CatDataDeleter>;

ManagedCatData load_cat(int64_t sql_id);
void overwrite_cat(CatData *target_cat, int64_t source_sql_id);
std::unordered_map<int64_t, ManagedCatData> load_all_cats();
ManagedCatData make_stray(Xoshiro256pContext *rng_override = nullptr);
double calculate_coi(int64_t parent_a_key, int64_t parent_b_key);
ManagedCatData make_kitten(CatData *p_parent_a, CatData *p_parent_b, double coi, Xoshiro256pContext *rng_override = nullptr);
