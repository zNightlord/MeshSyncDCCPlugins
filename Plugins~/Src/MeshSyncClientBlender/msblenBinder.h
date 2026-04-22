#pragma once

#include "msblenMacros.h"
#include "MeshUtils/muMath.h"             //mu::float3
#include "BlenderPyObjects/BlenderPyID.h" //BlenderPyID

struct Depsgraph;

// ─────────────────────────────────────────────────────────────────────────────
// Compatibility shims for removed / changed Blender DNA structs.
//
// The goal is to keep all call-sites above this layer unchanged across versions.
// The actual data is fetched in the .cpp files using the appropriate versioned
// Blender API; these shims just give the compiler something to name the type.
// ─────────────────────────────────────────────────────────────────────────────

// ── Blender 3.5: MLoopUV / MVert layouts changed ─────────────────────────────
// MLoopUV.flag / MLoopUV.toCo removed; MVert.no / MVert.flag removed.
// We define trimmed-down local structs so callers only access what still exists.
#if BLENDER_VERSION >= 305 && BLENDER_VERSION < 400
typedef struct MLoopUV
{
    float uv[2];
} MLoopUV;

typedef struct MVert
{
    float co[3];
} MVert;
#endif

// ── Blender 4.0: MVert removed from DNA entirely ─────────────────────────────
// Vertex positions are now Span<float3> via mesh->vert_positions().
// We keep the same shim shape so vertices() callers compile unchanged;
// the .cpp implementation switches to vert_positions() internally.
#if BLENDER_VERSION >= 400
typedef struct MVert
{
    float co[3]; // populated from vert_positions()[i] in .cpp
} MVert;

// MLoopUV is also gone (CD_MLOOPUV → CD_PROP_FLOAT2).
// Keep shim so uv() callers compile unchanged.
typedef struct MLoopUV
{
    float uv[2];
} MLoopUV;
#endif

// ── Blender 4.1: MPoly and MLoop removed from DNA ────────────────────────────
// Replaced by OffsetIndices<int> faces() and Span<int> corner_verts().
// Shims let polygons() / indices() callers remain source-compatible.
#if BLENDER_VERSION >= 410
typedef struct MPoly
{
    int loopstart;
    int totloop;
    short mat_nr;
    char flag;
    char _pad;
} MPoly;

typedef struct MLoop
{
    unsigned int v; // vertex index
    unsigned int e; // edge index
} MLoop;
#endif

// ── Blender 4.2: MEdge deprecated ────────────────────────────────────────────
// mesh->edges() now returns Span<int2>; BKE_mesh_edges() is removed.
// Shim preserves the edges() return type for callers.
#if BLENDER_VERSION >= 420
typedef struct MEdge
{
    unsigned int v1, v2;
    char crease, bweight;
    short flag;
} MEdge;
#endif

namespace blender
{
    bool ready();
    void setup(py::object bpy_context);
    const void *CustomData_get(const CustomData &data, int type);
    int CustomData_get_offset(const CustomData &data, int type);
    mu::float3 BM_loop_calc_face_normal(const BMLoop &l);
    std::string abspath(const std::string &path, const std::string &libName = "");
    void callPythonMethod(const char *name);
    std::string getBlenderVersion();

    struct ListHeader
    {
        ListHeader *next, *prev;
    };

    template <typename T>
    inline T rna_sdata(py::object p)
    {
        return reinterpret_cast<T>(reinterpret_cast<BPy_StructRNA *>(p.ptr())->ptr.data);
    }
    template <typename T>
    inline void rna_sdata(py::object p, T &v)
    {
        v = reinterpret_cast<T>(reinterpret_cast<BPy_StructRNA *>(p.ptr())->ptr.data);
    }

    template <typename T>
    struct blist_iterator
    {
        T *m_ptr;
        blist_iterator(T *p) : m_ptr(p) {}
        T *operator*() const { return m_ptr; }
        T *operator++()
        {
            m_ptr = (T *)(((ListHeader *)m_ptr)->next);
            return m_ptr;
        }
        bool operator==(const blist_iterator &v) const { return m_ptr == v.m_ptr; }
        bool operator!=(const blist_iterator &v) const { return m_ptr != v.m_ptr; }
    };

    template <typename T>
    struct blist_range
    {
        using iterator = blist_iterator<T>;
        using const_iterator = blist_iterator<const T>;
        T *ptr;

        blist_range(T *p) : ptr(p) {}
        iterator begin() { return iterator(ptr); }
        iterator end() { return iterator(nullptr); }
        const_iterator begin() const { return const_iterator(ptr); }
        const_iterator end() const { return const_iterator(nullptr); }
    };
    template <typename T>
    blist_range<T> list_range(T *t) { return blist_range<T>(t); }

    template <typename T>
    struct barray_range
    {
        using iterator = T *;
        using const_iterator = const T *;
        using reference = T &;
        using const_reference = const T &;

        T *m_ptr;
        size_t m_size;

        barray_range(T *p, size_t s) : m_ptr(p), m_size(s) {}
        iterator begin() { return m_ptr; }
        iterator end() { return m_ptr + m_size; }
        const_iterator begin() const { return m_ptr; }
        const_iterator end() const { return m_ptr + m_size; }
        reference operator[](size_t i) { return m_ptr[i]; }
        const_reference operator[](size_t i) const { return m_ptr[i]; }
        size_t size() const { return m_size; }
        bool empty() const { return !m_ptr || m_size == 0; }
    };
    template <typename T>
    barray_range<T> array_range(T *t, size_t s) { return barray_range<T>(t, s); }

    // ─────────────────────────────────────────────────────────────────────────────
    class BObject
    {
    public:
        MSBLEN_BOILERPLATE(Object)
        MSBLEN_COMPATIBLE(BlenderPyID)

        blist_range<ModifierData> modifiers();
        blist_range<bDeformGroup> deform_groups();

        const char *name() const;
        void *data();
        mu::float4x4 matrix_local() const;
        mu::float4x4 matrix_world() const;
        bool is_selected() const;
        bool hide_viewport() const;
        bool hide_render() const;
        Mesh *to_mesh() const;
        void to_mesh_clear();
        void modifiers_clear();
    };

    // ─────────────────────────────────────────────────────────────────────────────
    class BMesh
    {
    public:
        MSBLEN_BOILERPLATE(Mesh)
        MSBLEN_COMPATIBLE(BlenderPyID)

        // ── Core topology ─────────────────────────────────────────────────────
        // Return types use the shim structs above so all callers are unchanged.
        // .cpp implementations switch on BLENDER_VERSION to fetch the real data.

        barray_range<MLoop> indices();      // 4.1+: populated from corner_verts()
        barray_range<MEdge> edges();        // 4.2+: populated from mesh->edges() Span<int2>
        barray_range<MPoly> polygons();     // 4.1+: populated from faces() OffsetIndices
        barray_range<MVert> vertices();     // 4.0+: populated from vert_positions()
        barray_range<mu::float3> normals(); // split/corner normals (loop-space)

#if BLENDER_VERSION >= 400
        // 4.0+: expose raw Span accessors for code that wants to skip the shim layer.
        // These are zero-copy views into the mesh attribute arrays.
        barray_range<mu::float3> vert_normals();   // per-vertex normals
        barray_range<mu::float3> corner_normals(); // per-loop (corner) split normals
#endif

        barray_range<MLoopUV> uv();
        barray_range<MLoopCol> colors();

#if BLENDER_VERSION >= 304
        barray_range<int> material_indices();
#endif

        MLoopUV *GetUV(const int index) const;

        inline uint32_t GetNumUVs() const
        {
#if BLENDER_VERSION >= 400
            // CD_MLOOPUV was renamed CD_PROP_FLOAT2 in Blender 4.0
            return CustomData_number_of_layers(&m_ptr->ldata, CD_PROP_FLOAT2);
#elif BLENDER_VERSION >= 305
            return CustomData_number_of_layers(&m_ptr->ldata, CD_PROP_FLOAT2);
#else
            return CustomData_number_of_layers(&m_ptr->ldata, CD_MLOOPUV);
#endif
        }

        inline uint32_t GetNumColorLayers() const
        {
#if BLENDER_VERSION >= 400
            // CD_MLOOPCOL was renamed CD_PROP_BYTE_COLOR in Blender 4.0
            return CustomData_number_of_layers(&m_ptr->ldata, CD_PROP_BYTE_COLOR);
#else
            return CustomData_number_of_layers(&m_ptr->ldata, CD_MLOOPCOL);
#endif
        }

        // ── Normals ───────────────────────────────────────────────────────────
        void calc_normals_split();
        // Note: in 4.1+ calc_normals_split() is a no-op — corner_normals() is
        // always up to date. The .cpp implementation guards this internally.

        // ── Mutation helpers ──────────────────────────────────────────────────
        void update();
        void clear_geometry();
        void add_vertices(int count);
        void add_polygons(int count);
        void add_loops(int count);
        void add_edges(int count);
        void add_normals(int count);
    };

    // ─────────────────────────────────────────────────────────────────────────────
    using BMTriangle = BMLoop *[3];
    class BEditMesh
    {
    public:
        MSBLEN_BOILERPLATE2(BEditMesh, BMEditMesh)

        barray_range<BMFace *> polygons();
        barray_range<BMVert *> vertices();
        barray_range<BMTriangle> triangles();
        int uv_data_offset(int index) const;

        inline uint32_t GetNumUVs() const
        {
#if BLENDER_VERSION >= 400
            // BMesh CustomData also uses CD_PROP_FLOAT2 from 4.0+
            return CustomData_number_of_layers(&m_ptr->bm->ldata, CD_PROP_FLOAT2);
#else
            return CustomData_number_of_layers(&m_ptr->bm->ldata, CD_MLOOPUV);
#endif
        }
    };

    // ─────────────────────────────────────────────────────────────────────────────
    class BNurb
    {
    public:
        MSBLEN_BOILERPLATE(Nurb)
        MSBLEN_COMPATIBLE(BlenderPyID)

        barray_range<MLoop> indices();
        barray_range<MEdge> edges();
        barray_range<MPoly> polygons();
        barray_range<MVert> bezier_points();

        void add_bezier_points(int count, Object *obj);
    };

    class BCurve
    {
    public:
        MSBLEN_BOILERPLATE(Curve)
        MSBLEN_COMPATIBLE(BlenderPyID)

        void clear_splines();
        Nurb *new_spline();
    };

    // ─────────────────────────────────────────────────────────────────────────────
    class BMaterial
    {
    public:
        MSBLEN_BOILERPLATE(Material)
        MSBLEN_COMPATIBLE(BlenderPyID)

        const char *name() const;
        const mu::float3 &color() const;
        bool use_nodes() const;
        Material *active_node_material() const;
    };

    class BCamera
    {
    public:
        MSBLEN_BOILERPLATE(Camera)
        MSBLEN_COMPATIBLE(BlenderPyID)

        float clip_start() const;
        float clip_end() const;
        float angle_x() const;
        float angle_y() const;
        float lens() const;
        /*
            CAMERA_SENSOR_FIT_AUTO,
            CAMERA_SENSOR_FIT_HOR,
            CAMERA_SENSOR_FIT_VERT,
        */
        int sensor_fit() const;
        float sensor_width() const;  // mm
        float sensor_height() const; // mm
        float shift_x() const;       // percent
        float shift_y() const;       // percent
    };

    class BData
    {
    public:
        MSBLEN_BOILERPLATE2(BData, Main)

        blist_range<Object> objects();
        blist_range<Mesh> meshes();
        blist_range<Material> materials();
        blist_range<Collection> collections();

        bool objects_is_updated();
        void remove(Mesh *v);
    };
} // namespace blender
