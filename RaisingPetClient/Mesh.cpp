//-----------------------------------------------------------------------------
// File: Mesh.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Mesh.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////

CPolygon::CPolygon(int nVertices)
{
	m_nVertices = nVertices;
	m_pVertices = new CVertex[nVertices];
}

CPolygon::~CPolygon()
{
	if (m_pVertices) delete[] m_pVertices;
}

void CPolygon::SetVertex(int nIndex, CVertex& vertex)
{
	if ((0 <= nIndex) && (nIndex < m_nVertices) && m_pVertices)
	{
		m_pVertices[nIndex] = vertex;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

CMesh::CMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, char *pstrFileName)
{
	if (pstrFileName) LoadMeshFromFile(pd3dDevice, pd3dCommandList, pstrFileName);
}
CMesh::CMesh(int nPolygons)
{
	m_nPolygons = nPolygons;
	m_ppPolygons = new CPolygon * [nPolygons];
}

CMesh::~CMesh()
{
	if (m_pxmf3Positions) delete[] m_pxmf3Positions;
	if (m_pxmf3Normals) delete[] m_pxmf3Normals;
	if (m_pxmf2TextureCoords) delete[] m_pxmf2TextureCoords;

	if (m_pnIndices) delete[] m_pnIndices;

	if (m_pd3dVertexBufferViews) delete[] m_pd3dVertexBufferViews;

	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
	if (m_pd3dTextureCoordBuffer) m_pd3dTextureCoordBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CMesh::ReleaseUploadBuffers() 
{
	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	if (m_pd3dTextureCoordUploadBuffer) m_pd3dTextureCoordUploadBuffer->Release();
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();

	m_pd3dPositionUploadBuffer = NULL;
	m_pd3dNormalUploadBuffer = NULL;
	m_pd3dTextureCoordUploadBuffer = NULL;
	m_pd3dIndexUploadBuffer = NULL;
};

void CMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, m_nVertexBufferViews, m_pd3dVertexBufferViews);
	if (m_pd3dIndexBuffer)
	{
		pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

void CMesh::LoadMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, char* filename)
{
	std::ifstream file(filename);
	if (!file.is_open()) return;

	struct OBJ_VERTEX_INDEX
	{
		int position = -1;
		int textureCoord = -1;
	};

	std::vector<XMFLOAT3> sourcePositions;
	std::vector<XMFLOAT2> sourceTextureCoords;
	std::vector<XMFLOAT3> vertices;
	std::vector<XMFLOAT2> textureCoords;
	std::vector<UINT> indices;

	auto ResolveOBJIndex = [](int index, size_t count) -> int
	{
		return (index < 0) ? static_cast<int>(count) + index : index - 1;
	};

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v")
		{
			float x, y, z;
			iss >> x >> y >> z;
			sourcePositions.emplace_back(x, y, z);
		}
		else if (prefix == "vt")
		{
			float u, v;
			iss >> u >> v;
			sourceTextureCoords.emplace_back(u, 1.0f - v);
		}
		else if (prefix == "f")
		{
			std::vector<OBJ_VERTEX_INDEX> face;
			std::string token;
			while (iss >> token)
			{
				OBJ_VERTEX_INDEX vertexIndex;
				std::istringstream tokenStream(token);
				std::string value;

				if (std::getline(tokenStream, value, '/') && !value.empty())
					vertexIndex.position = ResolveOBJIndex(std::stoi(value), sourcePositions.size());
				if (std::getline(tokenStream, value, '/') && !value.empty())
					vertexIndex.textureCoord = ResolveOBJIndex(std::stoi(value), sourceTextureCoords.size());

				face.push_back(vertexIndex);
			}

			for (size_t i = 1; i + 1 < face.size(); ++i)
			{
				const OBJ_VERTEX_INDEX triangle[3] = { face[0], face[i], face[i + 1] };
				for (const OBJ_VERTEX_INDEX& vertexIndex : triangle)
				{
					if (vertexIndex.position < 0 || vertexIndex.position >= static_cast<int>(sourcePositions.size())) continue;
					vertices.push_back(sourcePositions[vertexIndex.position]);

					if (vertexIndex.textureCoord >= 0 && vertexIndex.textureCoord < static_cast<int>(sourceTextureCoords.size()))
						textureCoords.push_back(sourceTextureCoords[vertexIndex.textureCoord]);
					else
						textureCoords.emplace_back(0.0f, 0.0f);

					indices.push_back(static_cast<UINT>(indices.size()));
				}
			}
		}
	}
	file.close();

	if (vertices.empty()) return;

	m_nVertices = static_cast<UINT>(vertices.size());
	m_nIndices = static_cast<UINT>(indices.size());

	m_pxmf3Positions = new XMFLOAT3[m_nVertices];
	memcpy(m_pxmf3Positions, vertices.data(), sizeof(XMFLOAT3) * m_nVertices);
	m_pxmf2TextureCoords = new XMFLOAT2[m_nVertices];
	memcpy(m_pxmf2TextureCoords, textureCoords.data(), sizeof(XMFLOAT2) * m_nVertices);
	m_pnIndices = new UINT[m_nIndices];
	memcpy(m_pnIndices, indices.data(), sizeof(UINT) * m_nIndices);

	UINT vbSize = sizeof(XMFLOAT3) * m_nVertices;
	m_pd3dPositionBuffer = CreateBufferResource(device, cmdList, m_pxmf3Positions, vbSize,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	UINT uvbSize = sizeof(XMFLOAT2) * m_nVertices;
	m_pd3dTextureCoordBuffer = CreateBufferResource(device, cmdList, m_pxmf2TextureCoords, uvbSize,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoordUploadBuffer);

	m_nVertexBufferViews = 2;
	m_pd3dVertexBufferViews = new D3D12_VERTEX_BUFFER_VIEW[m_nVertexBufferViews];
	m_pd3dVertexBufferViews[0].BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_pd3dVertexBufferViews[0].StrideInBytes = sizeof(XMFLOAT3);
	m_pd3dVertexBufferViews[0].SizeInBytes = vbSize;
	m_pd3dVertexBufferViews[1].BufferLocation = m_pd3dTextureCoordBuffer->GetGPUVirtualAddress();
	m_pd3dVertexBufferViews[1].StrideInBytes = sizeof(XMFLOAT2);
	m_pd3dVertexBufferViews[1].SizeInBytes = uvbSize;

	UINT ibSize = sizeof(UINT) * m_nIndices;
	m_pd3dIndexBuffer = CreateBufferResource(device, cmdList, m_pnIndices, ibSize,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = ibSize;

	XMFLOAT3 minimum = vertices[0], maximum = vertices[0];
	for (const XMFLOAT3& vertex : vertices)
	{
		minimum.x = min(minimum.x, vertex.x);
		minimum.y = min(minimum.y, vertex.y);
		minimum.z = min(minimum.z, vertex.z);
		maximum.x = max(maximum.x, vertex.x);
		maximum.y = max(maximum.y, vertex.y);
		maximum.z = max(maximum.z, vertex.z);
	}

	XMFLOAT3 center = {
		(minimum.x + maximum.x) * 0.5f,
		(minimum.y + maximum.y) * 0.5f,
		(minimum.z + maximum.z) * 0.5f
	};
	XMFLOAT3 extent = {
		(maximum.x - minimum.x) * 0.5f,
		(maximum.y - minimum.y) * 0.5f,
		(maximum.z - minimum.z) * 0.5f
	};
	m_xmOOBB = BoundingOrientedBox(center, extent, XMFLOAT4(0, 0, 0, 1));
}

void CMesh::SetPolygon(int nIndex, CPolygon* pPolygon)
{
	if ((0 <= nIndex) && (nIndex < m_nPolygons)) m_ppPolygons[nIndex] = pPolygon;
}

int CMesh::CheckRayIntersection(XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection, float* pfNearHitDistance)
{
	if (!m_pxmf3Positions || !m_pnIndices || m_nIndices < 3) return(0);

	int nHits = 0;
	float fNearestHit = FLT_MAX;
	for (UINT i = 0; i + 2 < m_nIndices; i += 3)
	{
		const UINT i0 = m_pnIndices[i];
		const UINT i1 = m_pnIndices[i + 1];
		const UINT i2 = m_pnIndices[i + 2];
		if (i0 >= m_nVertices || i1 >= m_nVertices || i2 >= m_nVertices) continue;

		const XMVECTOR v0 = XMLoadFloat3(&m_pxmf3Positions[i0]);
		const XMVECTOR v1 = XMLoadFloat3(&m_pxmf3Positions[i1]);
		const XMVECTOR v2 = XMLoadFloat3(&m_pxmf3Positions[i2]);

		float fHitDistance = 0.0f;
		if (TriangleTests::Intersects(xmvPickRayOrigin, xmvPickRayDirection, v0, v1, v2, fHitDistance))
		{
			++nHits;
			if (fHitDistance < fNearestHit) fNearestHit = fHitDistance;
		}
	}

	if (pfNearHitDistance && nHits > 0) *pfNearHitDistance = fNearestHit;
	return(nHits);
}
BOOL CMesh::RayIntersectionByTriangle(XMVECTOR& xmRayOrigin, XMVECTOR& xmRayDirection, XMVECTOR v0, XMVECTOR v1, XMVECTOR v2, float* pfNearHitDistance)
{
	float fHitDistance;
	BOOL bIntersected = TriangleTests::Intersects(xmRayOrigin, xmRayDirection, v0, v1, v2, fHitDistance);
	if (bIntersected && (fHitDistance < *pfNearHitDistance)) *pfNearHitDistance = fHitDistance;

	return(bIntersected);
}