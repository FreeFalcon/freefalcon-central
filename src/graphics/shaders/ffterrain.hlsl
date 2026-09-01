//-----------------------------------------------------------------------------
// ffterrain.hlsl -- Artscout - 2026: #78 mesh-shader terrain (D3D12 + Vulkan).
//
// ONE source for both backends: DXC compiles it to DXIL (SM 6.5) for D3D12 and
// to SPIR-V (VK_EXT_mesh_shader) for Vulkan. See tools/build_shaders.
//
// The legacy GPU terrain builds every vertex on the CPU each frame (GetPost per
// post, geomorph via a LOD+1 fetch, then a fat dynamic VB). Here the CPU uploads
// DATA -- a toroidal clipmap of posts, one array slice per LOD -- and the mesh
// shader generates the grid straight out of it. One DispatchMesh draws the whole
// ground.
//
// Layout: each LOD owns an axis-aligned post RING (outer box minus the finer
// LOD's inner box), exactly as the legacy path tiles them, so the two agree on
// where a LOD ends. A chunk is CHUNK_QUADS^2 quads -> one mesh-shader group.
//
// Geomorph: near the ring's outer edge a post's HEIGHT is blended toward the
// LOD+1 surface, which lives in the very next clipmap slice -- so the coarse
// height is a lookup, not the CPU path's reconstruct-with-fallback. At the edge
// (alpha=1) the fine grid lands exactly on the coarse one: no crack, no pop.
//-----------------------------------------------------------------------------

//============================== Configuration ================================

#define MAX_CLIP_LODS   8       // clipmap array slices (theater LODs drawn on GPU)
#define CHUNK_QUADS     8       // quads per chunk edge
#define CHUNK_POSTS     (CHUNK_QUADS + 1)               // 9
#define CHUNK_CELLS     (CHUNK_QUADS * CHUNK_QUADS)     // 64 quads
// Vertices are PER QUAD, not shared: a quad takes its tile and its uv origin
// from its own corner post (legacy behaviour), so neighbours disagree on uv.
#define CHUNK_VERTS     (CHUNK_CELLS * 4)               // 256 (== the cap)
#define CHUNK_TRIS      (CHUNK_CELLS * 2)               // 128 (<= 256)
#define BORDER_POSTS    (CHUNK_POSTS + 2)               // 11: +1 ring for normals
#define MS_GROUP        64      // threads per mesh-shader group (1 quad each)
#define AS_GROUP        32      // chunks tested per amplification group

// Terrain feature flags (gTerrFlags.x).
#define TF_TEXTURED     (1u << 0)   // sample the tile texture (else flat colour)
#define TF_LIGHTING     (1u << 1)   // shade by the normal built from the posts
#define TF_FOG          (1u << 2)
#define TF_WIREOVERLAY  (1u << 3)   // debug: tint by LOD instead of terrain colour
#define TF_IRGREY       (1u << 4)   // #A5 sensor pass (TGP/MAV/FLIR): luma only
#define TF_NVG          (1u << 5)   // #97 night vision: green phosphor + gain

// Per-post info bits (gClipInfo).
#define PI_VALID        (1u << 16)  // the post is resident (else the quad is skipped)
#define PI_HASTILE      (1u << 17)  // the tile slot is a real bindless index
#define PI_SLOT_MASK    0xFFFFu

//=========================== Constant buffer =================================

// Artscout - 2026: one buffer for the whole terrain draw. Per-LOD rows are
// indexed by clipmap slice, NOT by theater LOD -- slice 0 is the finest LOD the
// GPU path draws (gTerrLod.x holds that base so world math can recover it).
struct ClipLevel
{
    int4 originPost;    // xy = clipmap window origin (row,col) in level posts; zw = pad
    int4 ringOuter;     // rLo, rHi, cLo, cHi -- this LOD's outer box (level posts)
    int4 ringInner;     // rLo, rHi, cLo, cHi -- the finer LOD's box (w<0 = none)
    int4 chunkSpan;     // x = first chunk id, y = chunks per row, z = chunk count,
                        // w = chunks per column
};

[[vk::binding(0, 0)]]
cbuffer cbTerrain : register(b0)
{
    // Per VIEW, indexed by SV_ViewID: the scene is drawn once and the rasterizer
    // replicates to each eye's RT slice (view instancing / multiview). The flat
    // path fills every slot with the same matrix.
    row_major float4x4 gTerrView[4];
    row_major float4x4 gTerrProj[4];
    float4      gTerrCamPos;        // world camera position, feet (xyz)
    float4      gTerrParams;        // x = FeetPerPost, y = day/night level,
                                    // z = morph ring width in posts, w = clip texel edge
    uint4       gTerrFlags;         // x = TF_* flags, y = clip LOD count, z = base LOD,
                                    // w = total chunk count
    float4      gTerrFog;           // x = fog start, y = fog end, zw = pad
    float4      gTerrFogColor;
    float4      gSunDir;            // xyz = ray direction (away from the sun)
    float4      gSunColor;          // rgb = sun diffuse
    float4      gSunAmbient;        // rgb = scene ambient
    float4      gTerrMisc;          // xy = screen size (px), z = time (sec)
    float4      gTerrFrustum[6];    // world-space planes (xyz = n, w = d), camera-relative
    ClipLevel   gClip[MAX_CLIP_LODS];
};

//============================== Resources ====================================

// The clipmap. One array slice per clip LOD, addressed toroidally: a post at
// (row,col) lives at ((row % N) , (col % N)). The window origin per LOD is in
// gClip[].originPost, so a slice never needs a full re-upload -- only the rows
// and columns the camera just crossed.
// x = elevation in feet (z DOWN), y,z = tile u,v, w = tile uv step per post.
[[vk::binding(1, 0)]]
Texture2DArray<float4> gClipPost : register(t0);
[[vk::binding(2, 0)]]
Texture2DArray<uint>   gClipInfo : register(t1);  // PI_* bits + tile slot

// Per-chunk elevation bounds, filled when the clipmap uploads. Indexed by the
// GLOBAL chunk id (gClip[].chunkSpan.x + local). Used by the amplification
// shader to build a world AABB without touching the post texture.
[[vk::binding(3, 0)]]
StructuredBuffer<float2> gChunkBounds : register(t2); // x = minZ, y = maxZ (feet)

// Tile textures. D3D12 (SM 6.6) indexes the resident descriptor heap directly;
// Vulkan indexes an unbounded descriptor-indexing array in set 1. Same slot
// numbers either way -- the CPU hands out one index per tile texture.
#if defined(FF_BINDLESS_HEAP)
    // ResourceDescriptorHeap -- no declaration needed.
#elif defined(FF_BINDLESS_ARRAY)
    [[vk::binding(0, 1)]] Texture2D gTiles[] : register(t0, space1);
#endif

[[vk::binding(4, 0)]]
SamplerState gTileSampler : register(s0);

//============================ Stage plumbing =================================

struct TerrVertex
{
    float4 pos    : SV_Position;
    float3 wpos   : TEXCOORD0;  // camera-relative world position (feet)
    float2 uv     : TEXCOORD1;
    float3 normal : TEXCOORD2;
    nointerpolation uint slot : TEXCOORD3;   // bindless tile slot (~0 = none)
    nointerpolation uint lod  : TEXCOORD4;   // clip slice, for the debug tint
};

// One chunk of one LOD, chosen by the amplification shader.
struct ChunkPayload
{
    uint chunkId[AS_GROUP];
};

//============================ Clipmap access =================================

// Toroidal wrap of an absolute level post onto the clipmap slice.
int2 ClipTexel(int2 post)
{
    const int n = (int)gTerrParams.w;
    int2 t = post % n;
    t.x = (t.x < 0) ? t.x + n : t.x;
    t.y = (t.y < 0) ? t.y + n : t.y;
    return t;
}

float4 LoadPost(int2 post, uint slice)
{
    const int2 t = ClipTexel(post);
    return gClipPost.Load(int4(t.y, t.x, (int)slice, 0));
}

float LoadHeight(int2 post, uint slice)
{
    return LoadPost(post, slice).x;
}

uint LoadInfo(int2 post, uint slice)
{
    const int2 t = ClipTexel(post);
    return gClipInfo.Load(int4(t.y, t.x, (int)slice, 0));
}

// Artscout - 2026: the geomorph target -- the LOD+1 surface at the world spot of
// a fine post. A fine post with EVEN indices coincides with a coarse post; odd
// ones sit between two or four of them. Sampled by hand (four Loads, each
// wrapped on its own) because a hardware bilinear tap would interpolate ACROSS
// the toroidal seam, which cuts right through the live window.
// Returns false when any coarse post it needs is not resident: that slice may
// still be streaming (a refill does one level per frame), and morphing toward
// an unwritten texel drags the vertex down to zero -- the "sinking" patches.
bool CoarseHeight(int2 post, uint coarseSlice, out float outZ)
{
    const int2 c0 = int2(post.x >> 1, post.y >> 1);
    const bool oddR = (post.x & 1) != 0;
    const bool oddC = (post.y & 1) != 0;
    outZ = 0.0f;

    const int2 o1 = oddR ? int2(1, 0) : int2(0, 0);
    const int2 o2 = oddC ? int2(0, 1) : int2(0, 0);
    const int2 o3 = o1 + o2;

    const uint valid = LoadInfo(c0, coarseSlice)
                     & LoadInfo(c0 + o1, coarseSlice)
                     & LoadInfo(c0 + o2, coarseSlice)
                     & LoadInfo(c0 + o3, coarseSlice);
    if ((valid & PI_VALID) == 0)
        return false;

    const float z00 = LoadHeight(c0, coarseSlice);
    if (!oddR && !oddC)
    {
        outZ = z00;
        return true;
    }
    if (oddR && !oddC)
    {
        outZ = 0.5f * (z00 + LoadHeight(c0 + int2(1, 0), coarseSlice));
        return true;
    }
    if (!oddR && oddC)
    {
        outZ = 0.5f * (z00 + LoadHeight(c0 + int2(0, 1), coarseSlice));
        return true;
    }

    outZ = 0.25f * (z00 + LoadHeight(c0 + int2(1, 0), coarseSlice)
                        + LoadHeight(c0 + int2(0, 1), coarseSlice)
                        + LoadHeight(c0 + int2(1, 1), coarseSlice));
    return true;
}

// Morph weight for a post: 1 ON the ring's outer edge, 0 at morphPosts inward.
float MorphAlpha(int2 post, int4 outer)
{
    const int dOut = min(min(outer.y - post.x, post.x - outer.x),
                         min(outer.w - post.y, post.y - outer.z));
    const float w = max(gTerrParams.z, 1.0f);
    return saturate((w - (float)dOut) / w);
}

// Absolute level post -> camera-relative world feet. Rotation-only view, so the
// vertices are pre-translated by the camera (the legacy path does the same).
float3 PostToWorld(int2 post, int lodShift, float z)
{
    const float step = gTerrParams.x * (float)(1u << (uint)lodShift);
    return float3((float)post.x * step - gTerrCamPos.x,
                  (float)post.y * step - gTerrCamPos.y,
                  z - gTerrCamPos.z);
}

// Elevation colour, matching the legacy path's untextured fallback exactly.
float3 ElevationColour(float z)
{
    const float elev = -z;
    const float s = clamp(0.45f + elev / 14000.0f, 0.30f, 1.00f)
                  * gTerrParams.y;
    return float3(s * 155.0f, s * 135.0f, s * 100.0f) / 255.0f;
}

// Bounds live per toroidal TILE, not per ring chunk -- the ring moves every
// frame, the tiles do not, so a tile's min/max survives until its posts change.
uint BoundsIndex(uint slice, int2 chunkOriginPost)
{
    const int tiles = (int)gTerrParams.w / CHUNK_QUADS;
    int tr = (chunkOriginPost.x / CHUNK_QUADS) % tiles;
    int tc = (chunkOriginPost.y / CHUNK_QUADS) % tiles;
    tr = (tr < 0) ? tr + tiles : tr;
    tc = (tc < 0) ? tc + tiles : tc;
    return slice * (uint)(tiles * tiles) + (uint)(tr * tiles + tc);
}

// Decode a global chunk id into its clip slice and chunk origin post.
bool DecodeChunk(uint chunkId, out uint slice, out int2 originPost)
{
    slice = 0;
    originPost = int2(0, 0);

    const uint lodCount = gTerrFlags.y;
    for (uint l = 0; l < lodCount; ++l)
    {
        const int first = gClip[l].chunkSpan.x;
        const int count = gClip[l].chunkSpan.z;
        if ((int)chunkId >= first && (int)chunkId < first + count)
        {
            const int local = (int)chunkId - first;
            const int perRow = max(gClip[l].chunkSpan.y, 1);
            slice = l;
            // Chunks start on a post that is a multiple of CHUNK_QUADS, so one
            // chunk maps onto exactly one bounds tile (originPost.zw).
            originPost = int2(gClip[l].originPost.z + (local / perRow) * CHUNK_QUADS,
                              gClip[l].originPost.w + (local % perRow) * CHUNK_QUADS);
            return true;
        }
    }
    return false;
}

//========================= Amplification shader ==============================

// Artscout - 2026: one thread per candidate chunk -- frustum-cull it against the
// bounds the clipmap upload recorded, and emit a mesh group only for survivors.
groupshared ChunkPayload s_payload;
groupshared uint s_survivors;

[numthreads(AS_GROUP, 1, 1)]
void AS_Terrain(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID)
{
    bool visible = false;
    const uint chunkId = dtid;

    if (chunkId < gTerrFlags.w)
    {
        uint slice;
        int2 origin;
        if (DecodeChunk(chunkId, slice, origin))
        {
            const int shift = (int)(gTerrFlags.z + slice);
            const float step = gTerrParams.x * (float)(1u << (uint)shift);
            const float2 bounds = gChunkBounds[BoundsIndex(slice, origin)];

            // Camera-relative AABB of the chunk (z DOWN, so minZ is the high side).
            const float3 lo = float3((float)origin.x * step - gTerrCamPos.x,
                                     (float)origin.y * step - gTerrCamPos.y,
                                     bounds.x - gTerrCamPos.z);
            const float3 hi = float3(lo.x + step * CHUNK_QUADS,
                                     lo.y + step * CHUNK_QUADS,
                                     bounds.y - gTerrCamPos.z);

            visible = true;
            [unroll]
            for (int p = 0; p < 6; ++p)
            {
                // Farthest corner along the plane normal: outside => whole box out.
                const float3 n = gTerrFrustum[p].xyz;
                const float3 farCorner = float3(n.x >= 0.0f ? hi.x : lo.x,
                                          n.y >= 0.0f ? hi.y : lo.y,
                                          n.z >= 0.0f ? hi.z : lo.z);
                if (dot(n, farCorner) + gTerrFrustum[p].w < 0.0f)
                    visible = false;
            }
        }
    }

    // Compact the survivors so the mesh groups are dense. A groupshared counter,
    // NOT wave intrinsics: the group is only guaranteed to be one wave when the
    // wave is at least AS_GROUP wide (it is 16 on some hardware).
    if (gtid == 0)
        s_survivors = 0;
    GroupMemoryBarrierWithGroupSync();

    uint slotIndex = 0;
    if (visible)
        InterlockedAdd(s_survivors, 1, slotIndex);
    if (visible)
        s_payload.chunkId[slotIndex] = chunkId;
    GroupMemoryBarrierWithGroupSync();

    DispatchMesh(s_survivors, 1, 1, s_payload);
}

//============================== Mesh shader ==================================

// One resolved post, shared by the (up to four) quads that touch it.
struct PostCache
{
    float3 wpos;    // camera-relative world position, morphed height
    float3 normal;
    float2 uv;      // tile uv at this post
    float d;        // tile uv step per post
    uint info;
};

groupshared float s_rawZ[BORDER_POSTS * BORDER_POSTS];
groupshared PostCache s_post[CHUNK_POSTS * CHUNK_POSTS];

[outputtopology("triangle")]
[numthreads(MS_GROUP, 1, 1)]
void MS_Terrain(uint gtid : SV_GroupThreadID,
                uint gid : SV_GroupID,
                uint viewId : SV_ViewID,
                in payload ChunkPayload payload,
                out vertices TerrVertex verts[CHUNK_VERTS],
                out indices uint3 tris[CHUNK_TRIS])
{
    const uint chunkId = payload.chunkId[gid];

    uint slice;
    int2 origin;
    // Uniform across the group (one chunk per group), so the single
    // SetMeshOutputCounts below covers the dead-chunk case too -- calling it
    // twice is a validation error.
    const bool live = DecodeChunk(chunkId, slice, origin);

    SetMeshOutputCounts(live ? CHUNK_VERTS : 0, live ? CHUNK_TRIS : 0);
    if (!live)
        return;

    const uint lodCount = gTerrFlags.y;
    const int shift = (int)(gTerrFlags.z + slice);
    const int4 outer = gClip[slice].ringOuter;
    const int4 inner = gClip[slice].ringInner;
    const bool hasInner = inner.w >= 0;
    const bool doMorph = (slice + 1) < lodCount;

    // ---- stage 1: raw heights over the chunk PLUS a one-post border, so the
    // normals below are central differences without re-fetching per quad ----
    for (uint b = gtid; b < BORDER_POSTS * BORDER_POSTS; b += MS_GROUP)
    {
        const int2 off = int2((int)(b / BORDER_POSTS) - 1,
                              (int)(b % BORDER_POSTS) - 1);
        s_rawZ[b] = LoadHeight(origin + off, slice);
    }
    GroupMemoryBarrierWithGroupSync();

    // ---- stage 2: the chunk's own posts -- morphed height, normal, tile ----
    const float step = gTerrParams.x * (float)(1u << (uint)shift);
    for (uint p = gtid; p < CHUNK_POSTS * CHUNK_POSTS; p += MS_GROUP)
    {
        const int pr = (int)(p / CHUNK_POSTS);
        const int pc = (int)(p % CHUNK_POSTS);
        const int2 post = origin + int2(pr, pc);
        const int bi = (pr + 1) * BORDER_POSTS + (pc + 1);

        float z = s_rawZ[bi];
        if (doMorph)
        {
            const float a = MorphAlpha(post, outer);
            float coarseZ;
            // No coarse data yet -> keep the fine height. A seam is far better
            // than a patch sinking to zero.
            if (a > 0.0f && CoarseHeight(post, slice + 1, coarseZ))
                z = lerp(z, coarseZ, a);
        }

        const float4 pd = LoadPost(post, slice);
        const uint info = LoadInfo(post, slice);

        PostCache pc2;
        pc2.wpos = PostToWorld(post, shift, z);
        pc2.uv = pd.yz;
        pc2.d = pd.w;
        pc2.info = info;
        // Central difference over the raw grid; z points DOWN, hence the sign.
        pc2.normal = normalize(float3(s_rawZ[bi + BORDER_POSTS] - s_rawZ[bi - BORDER_POSTS],
                                      s_rawZ[bi + 1] - s_rawZ[bi - 1],
                                      -2.0f * step));
        s_post[p] = pc2;
    }
    GroupMemoryBarrierWithGroupSync();

    // ---- stage 3: one quad per thread -- 4 vertices, 2 triangles ----
    // Its tile and uv origin come from the quad's own corner post, so the four
    // vertices cannot be shared with the neighbouring quads.
    const uint q = gtid;
    const int qi = (int)(q / CHUNK_QUADS);
    const int qj = (int)(q % CHUNK_QUADS);
    const int2 p0 = origin + int2(qi, qj);

    bool keep = (p0.x >= outer.x) && (p0.x + 1 <= outer.y)
             && (p0.y >= outer.z) && (p0.y + 1 <= outer.w);

    // The finer LOD owns everything inside the inner box.
    if (keep && hasInner)
    {
        const bool insideInner = (p0.x >= inner.x) && (p0.x + 1 <= inner.y)
                              && (p0.y >= inner.z) && (p0.y + 1 <= inner.w);
        keep = !insideInner;
    }

    const uint c00 = (uint)(qi * CHUNK_POSTS + qj);
    const uint c10 = c00 + CHUNK_POSTS;
    const uint c01 = c00 + 1;
    const uint c11 = c10 + 1;

    if (keep)
    {
        keep = ((s_post[c00].info & s_post[c10].info
               & s_post[c01].info & s_post[c11].info) & PI_VALID) != 0;
    }

    const PostCache corner = s_post[c00];
    const uint slot = ((corner.info & PI_HASTILE) != 0)
                    ? (corner.info & PI_SLOT_MASK) : ~0u;
    // uv follows the legacy quad exactly: +d per column, -d per ROW.
    const uint vb = q * 4;
    const uint idx[4] = {c00, c10, c01, c11};
    const float2 uvOff[4] = {float2(0, 0), float2(0, -1),
                             float2(1, 0), float2(1, -1)};

    [unroll]
    for (uint k = 0; k < 4; ++k)
    {
        const PostCache src = s_post[idx[k]];
        TerrVertex o;
        o.wpos = src.wpos;
        o.pos = mul(mul(float4(src.wpos, 1.0f), gTerrView[viewId]),
                    gTerrProj[viewId]);
        o.uv = corner.uv + uvOff[k] * corner.d;
        o.normal = src.normal;
        o.slot = slot;
        o.lod = slice;
        verts[vb + k] = o;
    }

    // A dropped quad emits two degenerate triangles: SetMeshOutputCounts is
    // uniform for the group, so the count cannot shrink per quad.
    // Same winding as the legacy index pair (a,b,e) / (a,e,d).
    const uint t = q * 2;
    tris[t + 0] = keep ? uint3(vb + 0, vb + 2, vb + 3) : uint3(0, 0, 0);
    tris[t + 1] = keep ? uint3(vb + 0, vb + 3, vb + 1) : uint3(0, 0, 0);
}

//============================== Pixel shader =================================

float4 PS_Terrain(TerrVertex i) : SV_Target
{
    float3 rgb;

    if ((gTerrFlags.x & TF_TEXTURED) != 0 && i.slot != ~0u)
    {
#if defined(FF_BINDLESS_HEAP)
        // NonUniformResourceIndex is REQUIRED: the slot differs per pixel across
        // a wave (neighbouring quads use different tiles). Without it the index
        // is undefined per wave and NVIDIA returns black -- the Vulkan branch
        // below always had it, which is why only D3D12 lost its ground.
        Texture2D tile = ResourceDescriptorHeap[NonUniformResourceIndex(i.slot)];
        rgb = tile.Sample(gTileSampler, i.uv).rgb;
#elif defined(FF_BINDLESS_ARRAY)
        rgb = gTiles[NonUniformResourceIndex(i.slot)].Sample(gTileSampler, i.uv).rgb;
#else
        rgb = float3(1.0f, 1.0f, 1.0f);
#endif
        rgb *= gTerrParams.y;   // day/night level
    }
    else
    {
        rgb = ElevationColour(i.wpos.z + gTerrCamPos.z);
    }

    // Sun shading is skipped in the sensor image: a thermal view has no sun
    // shadows, and shading THEN taking luma drove the ground dark twice over.
    if ((gTerrFlags.x & TF_LIGHTING) != 0 && (gTerrFlags.x & TF_IRGREY) == 0)
    {
        // Lambert off the post normals. gSunDir is the RAY direction, same
        // convention as the object shader, so negate to face the sun.
        const float ndl = saturate(dot(normalize(i.normal), -gSunDir.xyz));
        rgb *= saturate(gSunAmbient.rgb + gSunColor.rgb * ndl);
    }

    if ((gTerrFlags.x & TF_WIREOVERLAY) != 0)
    {
        const float3 lodTint[4] = { float3(1, 0.3f, 0.3f), float3(0.3f, 1, 0.3f),
                                    float3(0.3f, 0.5f, 1), float3(1, 1, 0.3f) };
        rgb = lerp(rgb, lodTint[i.lod & 3], 0.35f);
    }

    if ((gTerrFlags.x & TF_FOG) != 0)
    {
        const float dist = length(i.wpos);
        const float f = saturate((gTerrFog.y - dist)
                                 / max(gTerrFog.y - gTerrFog.x, 1.0f));
        rgb = lerp(gTerrFogColor.rgb, rgb, f);
    }

    // #A5 sensor pass -> Rec.601 luma, same as the object shader does.
    if ((gTerrFlags.x & TF_IRGREY) != 0)
        rgb = dot(rgb, float3(0.299f, 0.587f, 0.114f)).xxx;

    // #97 NVG: green phosphor with tube gain, scanlines, grain and vignette --
    // matched to PS_Object so the ground and the objects on it agree.
    if ((gTerrFlags.x & TF_NVG) != 0)
    {
        const float kNvgGain = 4.0f;
        float lum = dot(rgb, float3(0.30f, 0.59f, 0.11f));
        lum = 1.0f - exp(-lum * kNvgGain);

        const float scan = 0.92f + 0.08f * sin(i.pos.y * 3.14159f);
        const float2 np = i.pos.xy + gTerrMisc.z * 37.0f;
        const float grain =
            frac(sin(dot(np, float2(12.9898f, 78.233f))) * 43758.5453f);
        const float noise = 0.91f + 0.09f * grain;
        const float2 vc =
            i.pos.xy / max(gTerrMisc.xy, float2(1.0f, 1.0f)) - 0.5f;
        const float vig = saturate(1.0f - dot(vc, vc) * 1.35f);

        lum *= scan * noise * vig;
        rgb = float3(0.10f, 1.0f, 0.28f) * lum;
    }

    return float4(rgb, 1.0f);
}
