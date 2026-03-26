#include "ffi/cat_factory.hpp"
#include "amoeba.hpp"
#include "utilities/debug_console.hpp"
#include "utilities/sqlite3_conn_wrapper.hpp"

#include "lz4.h"

// Utilities for producing CatData objects.
//
// polymeric 2026

void CatDataDeleter::operator()(CatData *p_cat) const {
    using glaiel__CatData_dtor_FP = void (__cdecl *)(struct CatData* thiss);
    glaiel__CatData_dtor_FP glaiel__CatData_dtor_fp = *reinterpret_cast<glaiel__CatData_dtor_FP>(ADDRESS_glaiel__CatData_dtor + G.host_exec_base_va);
    glaiel__CatData_dtor_fp(p_cat);
    delete p_cat;
}

ManagedCatData new_default_cat() {
    CatData *p_new_cat = reinterpret_cast<CatData *>(new char[sizeof(CatData)]()); // zero-init
    using glaiel__CatData_ctor_FP = CatData *(__cdecl *)(struct CatData* thiss);
    glaiel__CatData_ctor_FP glaiel__CatData_ctor_fp = *reinterpret_cast<glaiel__CatData_ctor_FP>(ADDRESS_glaiel__CatData_ctor + G.host_exec_base_va);
    glaiel__CatData_ctor_fp(p_new_cat);

    return ManagedCatData(p_new_cat);
}

void deserialize_into_cat(CatData *p_cat, ByteStream *p_byte_stream) {
    using glaiel__SerializeCatData_FP = void (__cdecl *)(struct CatData* cat, struct ByteStream *byte_stream, bool assert_version_cutoff_on_load);
    glaiel__SerializeCatData_FP glaiel__SerializeCatData_fp = *reinterpret_cast<glaiel__SerializeCatData_FP>(ADDRESS_glaiel__SerializeCatData + G.host_exec_base_va);
    glaiel__SerializeCatData_fp(p_cat, p_byte_stream, false);
}

ManagedCatData load_cat(int64_t sql_id) {
    // TITLE: Dr. Beanies' guide to (safe?) cat resurrection
    // LECTURER: Beanies, Thomas A., Dr.
    // RIGHTSHOLDER: Beanies Teaching Films, Ltd.
    // YEAR OF PRODUCTION: 1958 (MCMLVIII)

    MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);

    if(p_md == nullptr) {
        return nullptr;
    }

    std::vector<char> decompressed_cat;
    // Here's an important tip for successful cat resurrection!
    // Remember to obtain a cryopreserved cat in the first place!
    // You may often find prepared specimen at your local morgue, or the cats table of your save file!
    G.sqlite3.exec_path(
        std::filesystem::path(p_md->sqlsavefile.file_path.as_native_string_view()),
        "SELECT data from cats WHERE key = :key;",
        {std::make_pair(":key", sql_id)},
        [&decompressed_cat](sqlite3_stmt * stmt) -> bool {
            // This corpse may currently look like a nondescript blob, but rest assured,
            // it shall be alive and kicking again in no time!
            const char *blob = reinterpret_cast<const char *>(sqlite3_column_blob(stmt, 0));
            int size = sqlite3_column_bytes(stmt, 0);

            // Sometimes the morgue may not have cats available, or you selected the wrong compartment numbers in your search!
            // Always have an escape plan for when you hear the mortician coming!
            if(size == 0) {
                D::warn("A cat blob in the cats table has 0 size!");
                return false;
            }

            // Cryopreserved cats are often embalmed with an agent known as "LZ4", invented by the world-renowned Mr. Collet!
            // While effective at preserving the dead, it is decidely not for the living!
            // Fortunately, "LZ4" can be removed with these simple steps, known as the "LZ4 decompression process"!
            int decompressed_size = reinterpret_cast<const int *>(blob)[0];
            decompressed_cat.resize(decompressed_size);
            if(LZ4_decompress_safe(blob + 4, decompressed_cat.data(), size - 4, decompressed_size) != decompressed_size) {
                D::error("An LZ4-compressed cat failed to decompress!");
                // Gentle reminder! It's a good habit to clean your equipment!
                // Helps to alleviate those messy cases!
                std::vector<char>().swap(decompressed_cat);
                return false;
            }

            return false;
        }
    );

    // As I mentioned earlier, be ready to bail at any time!
    // $1,000 and a change of clothes in a trusted confidant's home provides sufficient insurance, from my experience!
    if(decompressed_cat.size() == 0) {
        return nullptr;
    }

    // Now we need to prepare a glaiel::ByteStream!
    // The namesake of this apparatus comes from the venerated Professor Glaiel, an unparalleled genius of his era!
    // Even I, Thomas A. Beanies, with my 2 PhDs, barely understand enough to use a fraction of its full potential!
    ByteStream byte_stream = {};
    byte_stream.direction_0_des_buffer_1_ser_buffer_2_ser_ostream = 0;
    byte_stream.either_platform_or_stream_endianness = 0;
    byte_stream.either_stream_or_platform_endianness = 0;
    byte_stream.maximum_auto_endian_swap_size = 8;
    byte_stream.string_intern_table = nullptr;
    byte_stream.des_buffer = decompressed_cat.data();
    byte_stream.des_buffer_size = static_cast<int>(decompressed_cat.size());
    byte_stream.des_buffer_needs_free = false;
    byte_stream.des_buffer_read_cursor = 0;

    // (Notably, the glaiel::ByteStream can handle "LZ4 decompression" itself, but allowing it to do so
    // creates internal allocations, which are best avoided when we cannot easily see how it works!)

    // I've nearly forgotten to mention! You need a donor cat to complete the operation!
    // *I* need a donor cat to complete this demonstration!
    // ... What to do? ...
    // We'll synthesize one from scratch!
    ManagedCatData new_cat = new_default_cat(); // And while we're at it, don't forget to implant a remote detonator!

    // (The astute among you will note that proper cat synthesis requires several more procedures,
    // some of which involve defying the very designs of God [1]!
    // We omit them here, as this shall be a mere laboratory cat, not destined for the great outdoors!)
    // ---
    // [1]: It is scarce worth noting that the doctor is speaking of the process of genetic recombination
    //      in an allegorical sense, and is not offering theological or scientific advice.

    // (Question from the audience?
    // [...]
    // Well well well, somebody clearly slept through their Catology 101 lectures!
    // Recall the conventional synthesis process:
    //     1) capture the state of the universe,
    //     2) allow Mother Nature to do her delicate work on the cat's genes,
    //     3) reshape the genes with the ingenuity of Man,
    //     4) wind the universe back to the previously captured state.
    // [...]
    // Yes, omitting steps 1 and 4 is a sure cause for timeline fragmentation!
    // [...]
    // What? Of course that's not going to happen here! Do you think we're amateurs? [2])
    // ---
    // [2]: To provide some insight into the good doctor's commentary, the game loads cats from the save file by:
    //          A) saving the current RNG context
    //          B) constructing a CatData structure
    //          C) initializing the CatData with some field randomization (this advances RNG context)
    //          D) deserializing the saved cat into the CatData structure, overwriting any relevant fields
    //          E) initializing the body parts compartment of the CatData
    //          F) restoring RNG state
    //      Dr. Beanies recognized a shortcut and decided to omit steps C and E to save some work,
    //      since he didn't intend for this cat to be rendered in the world.
    //      By omitting those steps, he notices he isn't disturbing RNG, so he further omits steps A and F.

    // Aww, poor li'l guy! It's barely a cat at this stage!
    // Let's fix that!
    deserialize_into_cat(new_cat.get(), &byte_stream);

    // (And that is the technique of cat resurrection!)

    // [...]
    // Hmm? Yes, we're done.
    // [...]
    // The cat? Take it with you!
    return new_cat;
}

std::unordered_map<int64_t, ManagedCatData> load_all_cats() {
    MewDirector *p_md = *reinterpret_cast<MewDirector **>(ADDRESS_glaiel__MewDirector__p_singleton + G.host_exec_base_va);

    ByteStream byte_stream = {};
    byte_stream.direction_0_des_buffer_1_ser_buffer_2_ser_ostream = 0;
    byte_stream.either_platform_or_stream_endianness = 0;
    byte_stream.either_stream_or_platform_endianness = 0;
    byte_stream.maximum_auto_endian_swap_size = 8;
    byte_stream.string_intern_table = nullptr;
    byte_stream.des_buffer = nullptr;
    byte_stream.des_buffer_size = 0;
    byte_stream.des_buffer_needs_free = false;
    byte_stream.des_buffer_read_cursor = 0;

    std::vector<char> decompressed_cat;
    std::unordered_map<int64_t, ManagedCatData> tracked_cats;

    if(p_md == nullptr) {
        return tracked_cats;
    }

    G.sqlite3.exec_path(
        std::filesystem::path(p_md->sqlsavefile.file_path.as_native_string_view()),
        "SELECT key, data from cats;",
        {},
        [&byte_stream, &decompressed_cat, &tracked_cats](sqlite3_stmt * stmt) -> bool {
            int64_t sql_key = sqlite3_column_int64(stmt, 0);
            const char *blob = reinterpret_cast<const char *>(sqlite3_column_blob(stmt, 1));
            int size = sqlite3_column_bytes(stmt, 1);

            if(size == 0) {
                D::warn("A cat blob in the cats table has 0 size!");
                return true;
            }

            int decompressed_size = reinterpret_cast<const int *>(blob)[0];
            decompressed_cat.resize(decompressed_size);
            if(LZ4_decompress_safe(blob + 4, decompressed_cat.data(), size - 4, decompressed_size) != decompressed_size) {
                D::error("An LZ4-compressed cat failed to decompress!");
                return true;
            }

            ManagedCatData new_cat = new_default_cat();

            byte_stream.des_buffer = decompressed_cat.data();
            byte_stream.des_buffer_size = static_cast<int>(decompressed_cat.size());
            byte_stream.des_buffer_read_cursor = 0;

            deserialize_into_cat(new_cat.get(), &byte_stream);

            tracked_cats.emplace(sql_key, std::move(new_cat));

            return true;
        }
    );

    return tracked_cats;
}

void unk_init(CatData *p_cat, int32_t sex, bool register_in_name_history) {
    using ADDRESS_glaiel__CatParts_unk_init_FP = void (__cdecl *)(CatData *p_cat, void *ofstream_eliminated_by_opt, int32_t sex, bool register_in_name_history);
    ADDRESS_glaiel__CatParts_unk_init_FP ADDRESS_glaiel__CatParts_unk_init_fp = *reinterpret_cast<ADDRESS_glaiel__CatParts_unk_init_FP>(ADDRESS_glaiel__CatData_unk_init + G.host_exec_base_va);
    // 0 - male, 1 - female, 2 - neutral, 3 - randomize
    // guessing that appeal modifier lookup is baked in this fn
    ADDRESS_glaiel__CatParts_unk_init_fp(p_cat, nullptr, sex, register_in_name_history);
}

void unk_init_bodyparts(BodyParts *p_bodyparts) {
    using ADDRESS_glaiel__CatData_unk_init_bodyparts_FP = void (__cdecl *)(BodyParts *p_bodyparts);
    ADDRESS_glaiel__CatData_unk_init_bodyparts_FP ADDRESS_glaiel__CatData_unk_init_bodyparts_fp = *reinterpret_cast<ADDRESS_glaiel__CatData_unk_init_bodyparts_FP>(ADDRESS_glaiel__CatData_unk_init_bodyparts + G.host_exec_base_va);
    ADDRESS_glaiel__CatData_unk_init_bodyparts_fp(p_bodyparts);
}

void breed(CatData *p_kitten, CatData *p_parent_a, CatData *p_parent_b, double coi) {
    using ADDRESS_glaiel__CatData_breed_FP = void (__cdecl *)(CatData *p_kitten, CatData *p_parent_a, CatData *p_parent_b, double coi, void *vector_of_furniture_effects);
    ADDRESS_glaiel__CatData_breed_FP ADDRESS_glaiel__CatData_breed_fp = *reinterpret_cast<ADDRESS_glaiel__CatData_breed_FP>(ADDRESS_glaiel__CatData_breed + G.host_exec_base_va);
    // TODO furniture effects, guessing the house interstitial function retrieves vector_of_furniture_effects based on the room where the deed was done
    ADDRESS_glaiel__CatData_breed_fp(p_kitten, p_parent_a, p_parent_b, coi, nullptr);
}

ManagedCatData make_stray(Xoshiro256pContext *rng_override) {
    auto *p_tls = get_tls0_base<char>();
    Xoshiro256pContext *p_rng = reinterpret_cast<Xoshiro256pContext *>(p_tls + TLS0OFF_xoshiro256p_rng_context);
    Xoshiro256pContext rng_backup = *p_rng;
    if(rng_override != nullptr) {
        *p_rng = *rng_override;
    }
    ManagedCatData cat = new_default_cat();
    // "Now I am become Cat, destroyer of worlds, small animals, and upholstery"
    unk_init(cat.get(), 3, false);
    unk_init_bodyparts(&cat->body_parts);
    if(rng_override != nullptr) {
        *rng_override = *p_rng;
    }
    *p_rng = rng_backup;
    return cat;
}

ManagedCatData make_kitten(CatData *p_parent_a, CatData *p_parent_b, double coi, Xoshiro256pContext *rng_override) {
    auto *p_tls = get_tls0_base<char>();
    Xoshiro256pContext *p_rng = reinterpret_cast<Xoshiro256pContext *>(p_tls + TLS0OFF_xoshiro256p_rng_context);
    Xoshiro256pContext rng_backup = *p_rng;
    if(rng_override != nullptr) {
        *p_rng = *rng_override;
    }
    ManagedCatData kitten = new_default_cat();
    // Test tube kitty!
    breed(kitten.get(), p_parent_a, p_parent_b, coi); // FIXME: this call will mutate the name history table and we do not restore it
    if(rng_override != nullptr) {
        *rng_override = *p_rng;
    }
    *p_rng = rng_backup;
    return kitten;
}
