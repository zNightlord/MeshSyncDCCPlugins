#pragma once

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #define NOMINMAX
    #include <windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <thread>
#include <future>

#include "pybind11/pybind11.h"
#include "pybind11/operators.h"
#include "pybind11/eval.h"
#include "pybind11/stl.h"
namespace py = pybind11;

#ifndef NDEBUG
    #define NDEBUG
#endif
#pragma warning( push )
#pragma warning( disable : 4200 ) // zero length array

// ─────────────────────────────────────────────────────────────────────────────
// Core Blender headers — version-gated where the .h → .hh rename occurred
// ─────────────────────────────────────────────────────────────────────────────

#include "BKE_blender_version.h"
#include "BKE_main.h"
#include "BKE_context.h"
#include "BKE_material.h"

// BKE_customdata: .h in < 4.0, .hh in >= 4.0
// The .hh version uses StringRef for name params — see compat macros below.
#if BLENDER_VERSION >= 400
#  include "BKE_customdata.hh"
#  include "BLI_string_ref.hh"
#else
#  include "BKE_customdata.h"
#endif

// BKE_node: .h in < 4.0, .hh in >= 4.0
#if BLENDER_VERSION >= 400
#  include "BKE_node.hh"
#else
#  include "BKE_node.h"
#endif

// BKE_fcurve: .h in < 4.2, .hh in >= 4.2
#if BLENDER_VERSION >= 420
#  include "BKE_fcurve.hh"
#else
#  include "BKE_fcurve.h"
#endif

// BKE_editmesh: .h in < 4.3, .hh in >= 4.3
#if BLENDER_VERSION >= 430
#  include "BKE_editmesh.hh"
#else
#  include "BKE_editmesh.h"
#endif

#include "RNA_define.h"
#include "RNA_types.h"

// ─────────────────────────────────────────────────────────────────────────────
// DNA headers
// ─────────────────────────────────────────────────────────────────────────────

#include "DNA_anim_types.h"
#include "DNA_armature_types.h"
#include "DNA_camera_types.h"
#include "DNA_collection_types.h"

// Grease Pencil DNA:
//   < 4.3  : DNA_gpencil_types.h  (legacy GP v2, struct bGPdata)
//   >= 4.3 : GP v2 was renamed to "gpencil legacy"; DNA_gpencil_legacy_types.h
//   >= 5.0 : Legacy GP types fully removed. MeshSync does not export GP objects
//             so we skip the include entirely on 5.0+.
#if BLENDER_VERSION < 430
#  include "DNA_gpencil_types.h"
#elif BLENDER_VERSION < 500
#  include "DNA_gpencil_legacy_types.h"
#endif
// >= 5.0: no GP include needed — MeshSync never exported GP geometry.

#if BLENDER_VERSION < 302
#  include "DNA_hair_types.h"
#endif

#include "DNA_key_types.h"
#include "DNA_light_types.h"
#include "DNA_material_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_modifier_types.h"
#include "DNA_object_types.h"
#include "DNA_pointcloud_types.h"
#include "DNA_scene_types.h"
#include "DNA_volume_types.h"
#include "DNA_packedFile_types.h"

// ─────────────────────────────────────────────────────────────────────────────
// BLI / BMesh internals
// ─────────────────────────────────────────────────────────────────────────────

#include "BLI_utildefines.h"
#include "BLI_math_base.h"
#include "BLI_math_vector.h"
#include "bmesh_class.h"
#include "intern/rna_internal_types.h"
#include "intern/bpy_rna.h"
#include "intern/bmesh_structure.h"

// ─────────────────────────────────────────────────────────────────────────────
// Mesh API — versioned accessor headers
// ─────────────────────────────────────────────────────────────────────────────

#if BLENDER_VERSION >= 362 && BLENDER_VERSION < 400
// 3.6.x: new-style span accessors available but MPoly/MLoop still exist.
#  include "BKE_mesh.h"
#endif

#if BLENDER_VERSION >= 400
// 4.0+: fully attribute-based mesh API.
//   vert_positions(), corner_verts(), corner_edges(), faces()
//   vert_normals(), corner_normals()
// MVert.no removed; MPoly/MLoop removed in 4.1; MEdge deprecated in 4.2.
// CD_MLOOPUV → CD_PROP_FLOAT2; CD_MLOOPCOL → CD_PROP_BYTE_COLOR.
#  include "BKE_mesh.hh"
#  include "BKE_attribute.hh"
#  include "BLI_offset_indices.hh"
#endif

// ─────────────────────────────────────────────────────────────────────────────
// CustomData name-param compatibility shims  (Blender >= 4.0)
//
// Two functions changed their `name` parameter from const char* to StringRef:
//   CustomData_get_layer_named(data, type, name)
//   CustomData_get_named_layer_index(data, type, name)
//
// The wrappers accepting const char* are defined in customdata.cpp.
// These macros redirect all existing call sites transparently.
// ─────────────────────────────────────────────────────────────────────────────

#if BLENDER_VERSION >= 400

// const void *CustomData_get_layer_named_compat(const CustomData *data,
//                                               int type,
//                                               const char *name);
// int CustomData_get_named_layer_index_compat(const CustomData *data,
//                                             int type,
//                                             const char *name);

#  define CustomData_get_layer_named(data, type, name) \
       CustomData_get_layer_named_compat((data), (int)(type), (name))
#  define CustomData_get_named_layer_index(data, type, name) \
       CustomData_get_named_layer_index_compat((data), (int)(type), (name))

#endif // BLENDER_VERSION >= 400

#pragma warning( pop )