#include "stdafx.h"
#include "Mesh.h"

namespace
{
#pragma pack(push, 1)
struct FBIN_HEADER
{
	char magic[8];
	UINT32 version;
	UINT32 headerSize;
	UINT32 flags;
	UINT32 vertexStride;
	UINT32 vertexCount;
	UINT32 indexCount;
	float boundsMinimum[3];
	float boundsMaximum[3];
	UINT64 verticesOffset;
	UINT64 indicesOffset;
	UINT64 fileSize;
};

struct FBIN_VERTEX
{
	float position[3];
	float textureCoordinate[2];
};
#pragma pack(pop)

static_assert(sizeof(FBIN_HEADER) == 80, "FBin v1 header size must be 80 bytes.");
static_assert(sizeof(FBIN_VERTEX) == 20, "FBin v1 vertex size must be 20 bytes.");

bool IsRangeInsideFile(UINT64 offset, UINT64 size, UINT64 fileSize)
{
	return(offset <= fileSize && size <= fileSize - offset);
}

void ReportBinLoadError(const char* fileName, const char* reason)
{
	const std::string message = std::string("[LoadMeshFromBin] ") + fileName + ": " + reason + "\n";
	OutputDebugStringA(message.c_str());
}
}

void CMesh::LoadMeshFromBin(ID3D12Device* device, ID3D12GraphicsCommandList* commandList,
	char* fileName)
{
	std::ifstream input(fileName, std::ios::binary | std::ios::ate);
	if (!input)
	{
		ReportBinLoadError(fileName, "file could not be opened");
		return;
	}

	const std::streamoff streamSize = input.tellg();
	if (streamSize < static_cast<std::streamoff>(sizeof(FBIN_HEADER)))
	{
		ReportBinLoadError(fileName, "file is smaller than the FBin header");
		return;
	}
	const UINT64 actualFileSize = static_cast<UINT64>(streamSize);
	input.seekg(0, std::ios::beg);

	FBIN_HEADER header = {};
	input.read(reinterpret_cast<char*>(&header), sizeof(header));
	const char expectedMagic[8] = { 'R', 'P', 'F', 'B', 'I', 'N', 0, 0 };
	if (!input || memcmp(header.magic, expectedMagic, sizeof(expectedMagic)) != 0
		|| header.version != 1 || header.headerSize != sizeof(FBIN_HEADER)
		|| header.vertexStride != sizeof(FBIN_VERTEX))
	{
		ReportBinLoadError(fileName, "header signature, version, or stride is invalid");
		return;
	}
	if (header.vertexCount == 0 || header.indexCount == 0 || (header.indexCount % 3) != 0)
	{
		ReportBinLoadError(fileName, "vertex or triangle count is invalid");
		return;
	}
	if (header.vertexCount > UINT_MAX / sizeof(XMFLOAT3)
		|| header.vertexCount > UINT_MAX / sizeof(XMFLOAT2)
		|| header.indexCount > UINT_MAX / sizeof(UINT))
	{
		ReportBinLoadError(fileName, "mesh buffers exceed the supported UINT byte size");
		return;
	}
	if (header.fileSize != actualFileSize)
	{
		ReportBinLoadError(fileName, "recorded file size does not match the actual file size");
		return;
	}

	const UINT64 vertexDataSize = static_cast<UINT64>(header.vertexCount) * sizeof(FBIN_VERTEX);
	const UINT64 indexDataSize = static_cast<UINT64>(header.indexCount) * sizeof(UINT32);
	if (!IsRangeInsideFile(header.verticesOffset, vertexDataSize, actualFileSize)
		|| !IsRangeInsideFile(header.indicesOffset, indexDataSize, actualFileSize)
		|| header.verticesOffset < sizeof(FBIN_HEADER)
		|| header.indicesOffset < header.verticesOffset + vertexDataSize)
	{
		ReportBinLoadError(fileName, "vertex or index byte range is outside the file");
		return;
	}

	std::vector<FBIN_VERTEX> vertices(header.vertexCount);
	std::vector<UINT32> indices(header.indexCount);
	input.seekg(static_cast<std::streamoff>(header.verticesOffset), std::ios::beg);
	input.read(reinterpret_cast<char*>(vertices.data()), static_cast<std::streamsize>(vertexDataSize));
	input.seekg(static_cast<std::streamoff>(header.indicesOffset), std::ios::beg);
	input.read(reinterpret_cast<char*>(indices.data()), static_cast<std::streamsize>(indexDataSize));
	if (!input)
	{
		ReportBinLoadError(fileName, "vertex or index data could not be read completely");
		return;
	}
	for (UINT32 index : indices)
	{
		if (index >= header.vertexCount)
		{
			ReportBinLoadError(fileName, "an index references a vertex outside the vertex array");
			return;
		}
	}

	m_nVertices = header.vertexCount;
	m_nIndices = header.indexCount;
	m_pxmf3Positions = new XMFLOAT3[m_nVertices];
	m_pxmf2TextureCoords = new XMFLOAT2[m_nVertices];
	m_pnIndices = new UINT[m_nIndices];
	for (UINT i = 0; i < m_nVertices; ++i)
	{
		m_pxmf3Positions[i] = XMFLOAT3(vertices[i].position[0],
			vertices[i].position[1], vertices[i].position[2]);
		m_pxmf2TextureCoords[i] = XMFLOAT2(vertices[i].textureCoordinate[0],
			vertices[i].textureCoordinate[1]);
	}
	memcpy(m_pnIndices, indices.data(), static_cast<size_t>(indexDataSize));

	const UINT positionBufferSize = sizeof(XMFLOAT3) * m_nVertices;
	m_pd3dPositionBuffer = CreateBufferResource(device, commandList, m_pxmf3Positions,
		positionBufferSize, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);
	const UINT textureCoordinateBufferSize = sizeof(XMFLOAT2) * m_nVertices;
	m_pd3dTextureCoordBuffer = CreateBufferResource(device, commandList, m_pxmf2TextureCoords,
		textureCoordinateBufferSize, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoordUploadBuffer);

	m_nVertexBufferViews = 2;
	m_pd3dVertexBufferViews = new D3D12_VERTEX_BUFFER_VIEW[m_nVertexBufferViews];
	m_pd3dVertexBufferViews[0].BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_pd3dVertexBufferViews[0].StrideInBytes = sizeof(XMFLOAT3);
	m_pd3dVertexBufferViews[0].SizeInBytes = positionBufferSize;
	m_pd3dVertexBufferViews[1].BufferLocation = m_pd3dTextureCoordBuffer->GetGPUVirtualAddress();
	m_pd3dVertexBufferViews[1].StrideInBytes = sizeof(XMFLOAT2);
	m_pd3dVertexBufferViews[1].SizeInBytes = textureCoordinateBufferSize;

	const UINT indexBufferSize = sizeof(UINT) * m_nIndices;
	m_pd3dIndexBuffer = CreateBufferResource(device, commandList, m_pnIndices,
		indexBufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER,
		&m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = indexBufferSize;

	const XMFLOAT3 minimum(header.boundsMinimum[0], header.boundsMinimum[1], header.boundsMinimum[2]);
	const XMFLOAT3 maximum(header.boundsMaximum[0], header.boundsMaximum[1], header.boundsMaximum[2]);
	const XMFLOAT3 center((minimum.x + maximum.x) * 0.5f,
		(minimum.y + maximum.y) * 0.5f, (minimum.z + maximum.z) * 0.5f);
	const XMFLOAT3 extents((maximum.x - minimum.x) * 0.5f,
		(maximum.y - minimum.y) * 0.5f, (maximum.z - minimum.z) * 0.5f);
	m_xmBoundingBox = BoundingBox(center, extents);
	m_xmOOBB = BoundingOrientedBox(center, extents, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
}
