# RaisingPet FBin mesh format (version 1)

`ObjToFBinConverter` reads every top-level `objMeshs/*.obj` file and recreates
`binMeshs/<same-name>.bin`. Existing output files are truncated and overwritten.

All integers and IEEE-754 floats use little-endian byte order. Structures are packed
with 1-byte alignment. File offsets are absolute offsets from the beginning of the file.

## File layout

```text
[FBinHeader: 80 bytes]
[FBinVertex × vertexCount]
[uint32 index × indexCount]
```

## FBinHeader (80 bytes)

| Offset | Type | Name | Value/meaning |
|---:|---|---|---|
| 0 | char[8] | magic | `R P F B I N 00 00` |
| 8 | uint32 | version | `1` |
| 12 | uint32 | headerSize | `80` |
| 16 | uint32 | flags | Bit 0: texture coordinates are present |
| 20 | uint32 | vertexStride | `20` |
| 24 | uint32 | vertexCount | Number of `FBinVertex` records |
| 28 | uint32 | indexCount | Number of uint32 indices; multiple of 3 |
| 32 | float[3] | boundsMinimum | Local-space AABB minimum XYZ |
| 44 | float[3] | boundsMaximum | Local-space AABB maximum XYZ |
| 56 | uint64 | verticesOffset | Normally `80` |
| 64 | uint64 | indicesOffset | First byte of index data |
| 72 | uint64 | fileSize | Expected complete file size |

## FBinVertex (20 bytes)

| Offset | Type | Name |
|---:|---|---|
| 0 | float[3] | position XYZ |
| 12 | float[2] | texture coordinate UV |

The converter flips OBJ V coordinates using `1.0 - v`, matching the current
`CMesh::LoadMeshFromFile` behavior. OBJ normals and material declarations are ignored
because the current RaisingPet mesh pipeline consumes only position and UV streams.
Polygons with more than three vertices are converted to a triangle fan. Vertices are
deduplicated by the pair `(OBJ position index, OBJ texture-coordinate index)`.

## Reader validation requirements

Before allocating or uploading data, the game reader should validate:

1. `magic`, `version`, `headerSize`, and `vertexStride`.
2. `indexCount % 3 == 0`.
3. Every multiplication/addition used to calculate byte ranges for overflow.
4. Vertex and index ranges are inside `fileSize` and the actual file length.
5. Every index is less than `vertexCount`.

Version 1 intentionally contains no normals, animation, materials, or embedded textures.
