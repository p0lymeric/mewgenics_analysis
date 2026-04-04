#include "types/glaiel.hpp"

#include <functional>

// Various experimental functions that will modify the state of a save.
//
// polymeric 2026

void spawn_stray_at_house(std::function<void(CatData *)> customize_your_cat);
void despawn_housecat(int64_t sql_id);
