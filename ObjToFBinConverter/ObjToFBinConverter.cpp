#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

#pragma pack(push, 1)
struct FBinHeader
{
	char magic[8] = { 'R', 'P', 'F', 'B', 'I', 'N', 0, 0 };
	std::uint32_t version = 1;
	std::uint32_t headerSize = sizeof(FBinHeader);
	std::uint32_t flags = 1;
	std::uint32_t vertexStride = 20;
	std::uint32_t vertexCount = 0;
	std::uint32_t indexCount = 0;
	float boundsMinimum[3] = {};
	float boundsMaximum[3] = {};
	std::uint64_t verticesOffset = sizeof(FBinHeader);
	std::uint64_t indicesOffset = 0;
	std::uint64_t fileSize = 0;
};

struct FBinVertex
{
	float position[3];
	float textureCoordinate[2];
};
#pragma pack(pop)

static_assert(sizeof(FBinHeader) == 80);
static_assert(sizeof(FBinVertex) == 20);

struct Float3 { float x, y, z; };
struct Float2 { float x, y; };

struct ObjVertexKey
{
	int positionIndex = -1;
	int textureCoordinateIndex = -1;
	bool operator==(const ObjVertexKey& rhs) const noexcept
	{
		return positionIndex == rhs.positionIndex
			&& textureCoordinateIndex == rhs.textureCoordinateIndex;
	}
};

struct ObjVertexKeyHash
{
	std::size_t operator()(const ObjVertexKey& key) const noexcept
	{
		const std::uint64_t position = static_cast<std::uint32_t>(key.positionIndex);
		const std::uint64_t texture = static_cast<std::uint32_t>(key.textureCoordinateIndex);
		return std::hash<std::uint64_t>{}((position << 32) | texture);
	}
};

struct ConvertedMesh
{
	std::vector<FBinVertex> vertices;
	std::vector<std::uint32_t> indices;
	Float3 minimum = {};
	Float3 maximum = {};
};

int ResolveObjIndex(int objIndex, std::size_t elementCount)
{
	if (objIndex == 0) return -1;
	const std::int64_t resolved = (objIndex > 0)
		? static_cast<std::int64_t>(objIndex) - 1
		: static_cast<std::int64_t>(elementCount) + objIndex;
	return (resolved >= 0 && resolved < static_cast<std::int64_t>(elementCount))
		? static_cast<int>(resolved) : -1;
}

ObjVertexKey ParseFaceVertex(const std::string& token, std::size_t positionCount,
	std::size_t textureCoordinateCount)
{
	ObjVertexKey result;
	const std::size_t firstSlash = token.find('/');
	const std::string positionText = token.substr(0, firstSlash);
	if (positionText.empty()) throw std::runtime_error("face vertex has no position index");
	result.positionIndex = ResolveObjIndex(std::stoi(positionText), positionCount);
	if (result.positionIndex < 0) throw std::runtime_error("face position index is out of range");

	if (firstSlash != std::string::npos)
	{
		const std::size_t secondSlash = token.find('/', firstSlash + 1);
		const std::string textureText = token.substr(firstSlash + 1,
			secondSlash == std::string::npos ? std::string::npos : secondSlash - firstSlash - 1);
		if (!textureText.empty())
		{
			result.textureCoordinateIndex = ResolveObjIndex(std::stoi(textureText), textureCoordinateCount);
			if (result.textureCoordinateIndex < 0)
				throw std::runtime_error("face texture-coordinate index is out of range");
		}
	}
	return result;
}

ConvertedMesh ConvertObj(const fs::path& sourcePath)
{
	std::ifstream input(sourcePath);
	if (!input) throw std::runtime_error("cannot open input file");

	std::vector<Float3> sourcePositions;
	std::vector<Float2> sourceTextureCoordinates;
	ConvertedMesh mesh;
	std::unordered_map<ObjVertexKey, std::uint32_t, ObjVertexKeyHash> vertexLookup;
	std::string line;
	std::size_t lineNumber = 0;

	auto getVertexIndex = [&](const ObjVertexKey& key) -> std::uint32_t
	{
		const auto found = vertexLookup.find(key);
		if (found != vertexLookup.end()) return found->second;
		if (mesh.vertices.size() >= std::numeric_limits<std::uint32_t>::max())
			throw std::runtime_error("vertex count exceeds UINT32_MAX");
		const Float3& position = sourcePositions[key.positionIndex];
		Float2 textureCoordinate = {};
		if (key.textureCoordinateIndex >= 0)
			textureCoordinate = sourceTextureCoordinates[key.textureCoordinateIndex];
		const FBinVertex vertex = {
			{ position.x, position.y, position.z },
			{ textureCoordinate.x, textureCoordinate.y }
		};
		const std::uint32_t index = static_cast<std::uint32_t>(mesh.vertices.size());
		mesh.vertices.push_back(vertex);
		vertexLookup.emplace(key, index);
		return index;
	};

	while (std::getline(input, line))
	{
		++lineNumber;
		std::istringstream stream(line);
		std::string prefix;
		stream >> prefix;
		if (prefix.empty() || prefix[0] == '#') continue;
		try
		{
			if (prefix == "v")
			{
				Float3 position;
				if (!(stream >> position.x >> position.y >> position.z))
					throw std::runtime_error("invalid position");
				sourcePositions.push_back(position);
			}
			else if (prefix == "vt")
			{
				Float2 textureCoordinate;
				if (!(stream >> textureCoordinate.x >> textureCoordinate.y))
					throw std::runtime_error("invalid texture coordinate");
				textureCoordinate.y = 1.0f - textureCoordinate.y;
				sourceTextureCoordinates.push_back(textureCoordinate);
			}
			else if (prefix == "f")
			{
				std::vector<ObjVertexKey> face;
				std::string token;
				while (stream >> token)
					face.push_back(ParseFaceVertex(token, sourcePositions.size(),
						sourceTextureCoordinates.size()));
				if (face.size() < 3) throw std::runtime_error("face has fewer than three vertices");
				for (std::size_t i = 1; i + 1 < face.size(); ++i)
				{
					mesh.indices.push_back(getVertexIndex(face[0]));
					mesh.indices.push_back(getVertexIndex(face[i]));
					mesh.indices.push_back(getVertexIndex(face[i + 1]));
				}
			}
		}
		catch (const std::exception& error)
		{
			throw std::runtime_error("line " + std::to_string(lineNumber) + ": " + error.what());
		}
	}

	if (mesh.vertices.empty() || mesh.indices.empty())
		throw std::runtime_error("mesh has no renderable triangles");
	mesh.minimum = mesh.maximum = {
		mesh.vertices[0].position[0], mesh.vertices[0].position[1], mesh.vertices[0].position[2]
	};
	for (const FBinVertex& vertex : mesh.vertices)
	{
		mesh.minimum.x = (std::min)(mesh.minimum.x, vertex.position[0]);
		mesh.minimum.y = (std::min)(mesh.minimum.y, vertex.position[1]);
		mesh.minimum.z = (std::min)(mesh.minimum.z, vertex.position[2]);
		mesh.maximum.x = (std::max)(mesh.maximum.x, vertex.position[0]);
		mesh.maximum.y = (std::max)(mesh.maximum.y, vertex.position[1]);
		mesh.maximum.z = (std::max)(mesh.maximum.z, vertex.position[2]);
	}
	return mesh;
}

void WriteFBin(const fs::path& outputPath, const ConvertedMesh& mesh)
{
	FBinHeader header;
	header.vertexCount = static_cast<std::uint32_t>(mesh.vertices.size());
	header.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
	header.boundsMinimum[0] = mesh.minimum.x;
	header.boundsMinimum[1] = mesh.minimum.y;
	header.boundsMinimum[2] = mesh.minimum.z;
	header.boundsMaximum[0] = mesh.maximum.x;
	header.boundsMaximum[1] = mesh.maximum.y;
	header.boundsMaximum[2] = mesh.maximum.z;
	header.indicesOffset = header.verticesOffset + sizeof(FBinVertex) * mesh.vertices.size();
	header.fileSize = header.indicesOffset + sizeof(std::uint32_t) * mesh.indices.size();

	std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
	if (!output) throw std::runtime_error("cannot create output file");
	output.write(reinterpret_cast<const char*>(&header), sizeof(header));
	output.write(reinterpret_cast<const char*>(mesh.vertices.data()),
		static_cast<std::streamsize>(sizeof(FBinVertex) * mesh.vertices.size()));
	output.write(reinterpret_cast<const char*>(mesh.indices.data()),
		static_cast<std::streamsize>(sizeof(std::uint32_t) * mesh.indices.size()));
	if (!output) throw std::runtime_error("failed while writing output file");
}

fs::path FindConverterRoot()
{
	fs::path candidate = fs::current_path();
	for (int i = 0; i < 6; ++i)
	{
		if (fs::is_directory(candidate / "objMeshs")) return candidate;
		if (!candidate.has_parent_path()) break;
		candidate = candidate.parent_path();
	}
	throw std::runtime_error("objMeshs directory was not found from the working directory or its parents");
}

int main()
{
	try
	{
		const fs::path root = FindConverterRoot();
		const fs::path inputDirectory = root / "objMeshs";
		const fs::path outputDirectory = root / "binMeshs";
		fs::create_directories(outputDirectory);

		std::vector<fs::path> sourceFiles;
		for (const fs::directory_entry& entry : fs::directory_iterator(inputDirectory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".obj")
				sourceFiles.push_back(entry.path());
		}
		std::sort(sourceFiles.begin(), sourceFiles.end());

		std::size_t succeeded = 0;
		std::size_t failed = 0;
		for (const fs::path& sourcePath : sourceFiles)
		{
			try
			{
				const ConvertedMesh mesh = ConvertObj(sourcePath);
				const fs::path outputPath = outputDirectory / sourcePath.filename().replace_extension(".bin");
				WriteFBin(outputPath, mesh);
				++succeeded;
				std::wcout << L"[OK] " << sourcePath.filename().wstring() << L" -> "
					<< outputPath.filename().wstring() << L" (vertices=" << mesh.vertices.size()
					<< L", indices=" << mesh.indices.size() << L")\n";
			}
			catch (const std::exception& error)
			{
				++failed;
				std::wcerr << L"[FAILED] " << sourcePath.filename().wstring() << L": ";
				std::cerr << error.what() << '\n';
			}
		}
		std::cout << "Completed: " << succeeded << " succeeded, " << failed
			<< " failed, " << sourceFiles.size() << " total.\n";
		return failed == 0 ? 0 : 1;
	}
	catch (const std::exception& error)
	{
		std::cerr << "Fatal error: " << error.what() << '\n';
		return 1;
	}
}
