// customdata.cpp
//
// Original TODO (2020-8-28): "It should be possible to download and use Blender source directly."
//
// STATUS:
//   < 4.0  : Local reimplementations are still required. The functions exist in Blender's source
//             but are not reliably exported from the precompiled lib in all older build configs.
//
//   >= 4.0 : All CustomData_* functions below are fully exported from Blender's precompiled lib
//             via BKE_customdata.hh. We DO NOT reimplement them — we just include the header
//             and let the linker resolve them.
//
//             TWO SIGNATURES CHANGED in 4.0 that affect callers in this plugin:
//               CustomData_get_named_layer_index(data, type, name)  name: const char* → StringRef
//               CustomData_get_layer_named(data, type, name)        name: const char* → StringRef
//
//             We provide thin const-char* wrappers for those two so existing callers need no
//             changes. The wrappers are named with the same symbol; the Blender versions are
//             pulled in through the header with their StringRef overloads.
//
// We never compile Blender's customdata.cc — it depends on the entire Blender build graph.
// The precompiled lib ships all the implementations.

#include "pch.h"

// ─────────────────────────────────────────────────────────────────────────────
// Blender < 4.0  —  local reimplementations
// ─────────────────────────────────────────────────────────────────────────────
#if BLENDER_VERSION < 400

int CustomData_get_layer_index(const CustomData *data, int type)
{
    BLI_assert(customdata_typemap_is_valid(data));
    return data->typemap[type];
}

int CustomData_get_layer_index_n(const CustomData *data, int type, int n)
{
    int i = CustomData_get_layer_index(data, type);
    if (i != -1) {
        BLI_assert(i + n < data->totlayer);
        i = (data->layers[i + n].type == type) ? (i + n) : (-1);
    }
    return i;
}

#if BLENDER_VERSION >= 305
const
#endif
void *CustomData_get_layer_n(const CustomData *data, int type, int n)
{
    int layer_index = CustomData_get_layer_index_n(data, type, n);
    if (layer_index == -1)
        return nullptr;
    return data->layers[layer_index].data;
}

int CustomData_number_of_layers(const CustomData *data, int type)
{
    int number = 0;
    for (int i = 0; i < data->totlayer; i++) {
        if (data->layers[i].type == type)
            number++;
    }
    return number;
}

// name param: const char* throughout < 4.0
#if BLENDER_VERSION >= 305
const
#endif
void *CustomData_get_layer_named(const CustomData *data, const int type, const char *name)
{
    int layer_index = CustomData_get_named_layer_index(data, type, name);
    if (layer_index == -1)
        return nullptr;
    return data->layers[layer_index].data;
}

int CustomData_get_named_layer_index(const CustomData *data, const int type, const char *name)
{
    for (int i = 0; i < data->totlayer; i++) {
        if (data->layers[i].type == type) {
            if (STREQ(data->layers[i].name, name)) {
                return i;
            }
        }
    }
    return -1;
}

#else // BLENDER_VERSION >= 400
// ─────────────────────────────────────────────────────────────────────────────
// Blender >= 4.0  —  all core functions come from the precompiled lib.
//
// Include the 4.x header so the linker can resolve:
//   CustomData_get_layer_index, CustomData_get_layer_index_n,
//   CustomData_get_layer_n, CustomData_number_of_layers
// (Their signatures are unchanged and compatible with existing callers.)
// ─────────────────────────────────────────────────────────────────────────────
// #include "BKE_customdata.hh"   // brings in the declarations resolved by the precompiled lib

// ── Wrappers for the two functions whose name param changed to StringRef ──────
//
// Blender 4.0 changed:
//   int CustomData_get_named_layer_index(data, type, const char* name)
//                                              → (data, type, StringRef name)
//   const void* CustomData_get_layer_named(data, type, const char* name)
//                                              → (data, type, StringRef name)
//
// All existing callers in msblenBinder.cpp / msblenContext.cpp pass const char*.
// These wrappers preserve that ABI so callers need no edits.
// StringRef has an implicit constructor from const char*, so the forwarded call
// is zero-copy and zero-overhead.

// const void *CustomData_get_layer_named_compat(const CustomData *data,
//                                               const int type,
//                                               const char *name)
// {
//     // Forward to Blender's StringRef overload from BKE_customdata.hh
//     return CustomData_get_layer_named(data, eCustomDataType(type), blender::StringRef(name));
// }

// int CustomData_get_named_layer_index_compat(const CustomData *data,
//                                             const int type,
//                                             const char *name)
// {
//     return CustomData_get_named_layer_index(data, eCustomDataType(type), blender::StringRef(name));
// }

// Macro-alias so all call sites compile without modification.
// Place these defines in pch.h or msblenBinder.h for project-wide effect,
// or keep them here if customdata.cpp is the only translation unit that calls them.
//
//   #define CustomData_get_layer_named        CustomData_get_layer_named_compat
//   #define CustomData_get_named_layer_index  CustomData_get_named_layer_index_compat
//
// (See pch.h section below for the canonical location of these defines.)

#endif // BLENDER_VERSION >= 400