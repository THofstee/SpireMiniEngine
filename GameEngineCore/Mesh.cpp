#include "Mesh.h"
#include "CoreLib/LibIO.h"
#include "Skeleton.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace VectorMath;

namespace GameEngine
{
	void Mesh::LoadFromFile(const CoreLib::Basic::String & fileName)
	{
		RefPtr<FileStream> stream = new FileStream(fileName);
		LoadFromStream(stream.Ptr());
		stream->Close();
	}

	void Mesh::LoadFromStream(Stream * stream)
	{
		auto reader = BinaryReader(stream);
		int typeId = reader.ReadInt32();
		vertexFormat = MeshVertexFormat(typeId);
		vertCount = reader.ReadInt32();
		int indexCount = reader.ReadInt32();
		reader.Read(&Bounds, 1);
		AllocVertexBuffer(vertCount);
		Indices.SetSize(indexCount);
		reader.Read((char*)GetVertexBuffer(), vertCount * vertexFormat.GetVertexSize());
		reader.Read(Indices.Buffer(), indexCount);
		reader.ReleaseStream();
	}

	void Mesh::SaveToStream(Stream * stream)
	{
		auto writer = BinaryWriter(stream);
		writer.Write(GetVertexTypeId());
		writer.Write(vertCount);
		writer.Write(Indices.Count());
		writer.Write(&Bounds, 1);
		writer.Write((char*)GetVertexBuffer(), vertCount * GetVertexSize());
		writer.Write(Indices.Buffer(), Indices.Count());
		writer.ReleaseStream();
	}

	void Mesh::SaveToFile(const CoreLib::String & fileName)
	{
		RefPtr<FileStream> stream = new FileStream(fileName, FileMode::Create);
		SaveToStream(stream.Ptr());
		stream->Close();
	}

	union MeshVertexFormatTypeIdConverter
	{
		struct Fields
		{
			unsigned int hasSkinning : 1;
			unsigned int hasTangent : 1;
			unsigned int numUVs : 4;
			unsigned int numColors : 4;
		} f;
		int typeId;
	};

	MeshVertexFormat::MeshVertexFormat(int typeId)
	{
		MeshVertexFormatTypeIdConverter convertor;
		convertor.typeId = typeId;
		hasSkinning = convertor.f.hasSkinning;
		hasTangent = convertor.f.hasTangent;
		numUVs = (int)convertor.f.numUVs;
		numColors = (int)convertor.f.numColors;
		vertSize = CalcVertexSize();
	}

	String MeshVertexFormat::GetShaderDefinition()
	{
		if (shaderDef.Length() == 0)
		{
			StringBuilder sb;
			sb << "module VertexAttributes\n{\n";
			sb << "public @MeshVertex vec3 vertPos;\n";
			for (int i = 0; i < numUVs; i++)
				sb << "public @MeshVertex vec2 vertUV" << i << ";\n";
			for (int i = numUVs; i < 8; i++)
				sb << "public inline vec2 vertUV" << i << " = vec2(0.0);\n";
			sb << "public vec2 vertUV = vertUV0;\n";
			if (hasTangent)
			{
				sb << R"(
				@MeshVertex uint tangentFrame;
				vec4 tangentFrameQuaternion
				{
					vec4 result;
					float inv255 = 2.0 / 255.0;
					result.x = float(tangentFrame & 255) * inv255 - 1.0;
					result.y = float((tangentFrame >> 8) & 255) * inv255 - 1.0;
					result.z = float((tangentFrame >> 16) & 255) * inv255 - 1.0;
					result.w = float((tangentFrame >> 24) & 255) * inv255 - 1.0;
					return result;
				}
				public @CoarseVertex vec3 vertNormal
				{
					return normalize(QuaternionRotate(tangentFrameQuaternion, vec3(0.0, 1.0, 0.0)));
				}
				public @CoarseVertex vec3 vertTangent
				{
					return normalize(QuaternionRotate(tangentFrameQuaternion, vec3(1.0, 0.0, 0.0)));
				}
				public vec3 vertBinormal = cross(vertTangent, vertNormal);
				)";
			}
			else
			{
				sb << R"(
				public @CoarseVertex vec3 vertNormal = vec3(0.0, 1.0, 0.0);
				public @CoarseVertex vec3 vertTangent = vec3(1.0, 0.0, 0.0);
				public vec3 vertBinormal = vec3(0.0, 0.0, 1.0);
				)";
			}
			for (int i = 0; i < numColors; i++)
				sb << "public @MeshVertex vec4 vertColor" << i << ";\n";
			for (int i = numColors; i < 8; i++)
				sb << "public inline vec4 vertColor" << i << " = vec4(0.0);\n";
			if (hasSkinning)
			{
				sb << "public @MeshVertex uint boneIds;\n";
				sb << "public @MeshVertex uint boneWeights;\n";
			}
			else
			{
				sb << "public inline uint boneIds = 255;\n";
				sb << "public inline uint boneWeights = 0;\n";
			}
			sb << "}\n";
			shaderDef = sb.ProduceString();
		}
		return shaderDef;
	}

	int MeshVertexFormat::GetTypeId()
	{
		MeshVertexFormatTypeIdConverter convertor;
		convertor.typeId = 0;
		convertor.f.hasSkinning = hasSkinning;
		convertor.f.hasTangent = hasTangent;
		convertor.f.numUVs = numUVs;
		convertor.f.numColors = numColors;
		return convertor.typeId;
	}

	struct SkeletonMeshVertex
	{
		Vec3 pos;
		Quaternion tangentFrame;
		int boneId;
	};
	void Mesh::FromSkeleton(Skeleton * skeleton, float width)
	{
		Bounds.Init();
		SetVertexFormat(MeshVertexFormat(0, 0, true, true));
		List<SkeletonMeshVertex> vertices;
		List<Matrix4> forwardTransforms;
		List<Vec3> positions;
		positions.SetSize(skeleton->Bones.Count());
		forwardTransforms.SetSize(skeleton->Bones.Count());
		for (int i = 0; i < skeleton->Bones.Count(); i++)
		{
			forwardTransforms[i] = skeleton->Bones[i].BindPose.ToMatrix();
			if (skeleton->Bones[i].ParentId != -1)
				Matrix4::Multiply(forwardTransforms[i], forwardTransforms[skeleton->Bones[i].ParentId], forwardTransforms[i]);
			positions[i] = Vec3::Create(forwardTransforms[i].values[12], forwardTransforms[i].values[13], forwardTransforms[i].values[14]);
		}
		for (int i = 0; i < skeleton->Bones.Count(); i++)
		{
			int parent = skeleton->Bones[i].ParentId;
			Vec3 bonePos = positions[i];
			Vec3 parentPos = parent == -1 ? bonePos : positions[parent];
			if (parent == -1)
			{
				bonePos.y -= width;
				parentPos.y += width;
			}
			else
			{
				float length = (bonePos - parentPos).Length();
				if (length < width * 2.0f)
					width = length * 0.5f;
			}
			Bounds.Union(bonePos);
			Bounds.Union(parentPos);

			Vec3 dir = (bonePos - parentPos).Normalize();
			Vec3 xAxis, yAxis; 
			GetOrthoVec(xAxis, dir);
			Vec3::Cross(yAxis, dir, xAxis);
			int vCoords[] = { 0, 1, 3, 2 };
			for (int j = 0; j < 4; j++)
			{
				int vCoord = vCoords[j];
				int vCoord1 = vCoords[(j + 1) & 3];
				Vec3 v0 = parentPos + dir * width + xAxis * (width * ((float)(vCoord & 1) - 0.5f)) 
					+ yAxis * (width * ((float)((vCoord >> 1) & 1) - 0.5f));
				Vec3 v1 = parentPos + dir * width + xAxis * (width * ((float)(vCoord1 & 1) - 0.5f)) 
					+ yAxis * (width * ((float)((vCoord1 >> 1) & 1) - 0.5f));
				Bounds.Union(v0);

				// triangle1: v1->v0->parent
				{
					Vec3 normal1 = Vec3::Cross(v0 - v1, parentPos - v1).Normalize();
					Vec3 tangent1 = (v1 - v0).Normalize();
					Vec3 binormal1 = Vec3::Cross(tangent1, normal1).Normalize();
					Quaternion q = Quaternion::FromCoordinates(tangent1, normal1, binormal1);
					SkeletonMeshVertex vert;
					vert.pos = v1;
					vert.tangentFrame = q;
					vert.boneId = (parent == -1 ? i : parent);
					Indices.Add(vertices.Count());
					vertices.Add(vert);
					vert.pos = v0;
					Indices.Add(vertices.Count());
					vertices.Add(vert);
					vert.pos = parentPos;
					Indices.Add(vertices.Count());
					vertices.Add(vert);
				}
				// triangle2: v0->v1->bone
				{
					Vec3 normal1 = Vec3::Cross(v1 - v0, bonePos - v0).Normalize();
					Vec3 tangent1 = (v1 - v0).Normalize();
					Vec3 binormal1 = Vec3::Cross(tangent1, normal1).Normalize();
					Quaternion q = Quaternion::FromCoordinates(tangent1, normal1, binormal1);
					SkeletonMeshVertex vert;
					vert.pos = v0;
					vert.tangentFrame = q;
					vert.boneId = (parent == -1 ? i : parent);
					Indices.Add(vertices.Count());
					vertices.Add(vert);
					vert.pos = v1;
					Indices.Add(vertices.Count());
					vertices.Add(vert);
					vert.pos = bonePos;
					Indices.Add(vertices.Count());
					vertices.Add(vert);
				}
			}
		}
		vertexData.SetSize(vertices.Count() * vertexFormat.GetVertexSize());
		for (int i = 0; i<vertices.Count(); i++)
		{
			SetVertexPosition(i, vertices[i].pos);
			SetVertexTangentFrame(i, vertices[i].tangentFrame);
			SetVertexSkinningBinding(i, MakeArrayView(vertices[i].boneId), MakeArrayView(1.0f));
		}
		vertCount = vertices.Count();
		
	}
	Mesh Mesh::CreateBox(VectorMath::Vec3 vmin, VectorMath::Vec3 vmax)
	{
		Mesh rs;
		rs.SetVertexFormat(MeshVertexFormat(0, 1, true, false));
		rs.AllocVertexBuffer(24);
		
		Quaternion tangentFrame;
		// top
		tangentFrame = Quaternion::FromCoordinates(Vec3::Create(1.0f, 0.0f, 0.0f), Vec3::Create(0.0f, 1.0f, 0.0f), Vec3::Create(0.0f, 0.0f, 1.0f));
		rs.SetVertexPosition(0, Vec3::Create(vmax.x, vmax.y, vmin.z)); rs.SetVertexUV(0, 0, Vec2::Create(0.0f, 0.0f)); rs.SetVertexTangentFrame(0, tangentFrame);
		rs.SetVertexPosition(1, Vec3::Create(vmin.x, vmax.y, vmin.z)); rs.SetVertexUV(1, 0, Vec2::Create(1.0f, 0.0f)); rs.SetVertexTangentFrame(1, tangentFrame);
		rs.SetVertexPosition(2, Vec3::Create(vmin.x, vmax.y, vmax.z)); rs.SetVertexUV(2, 0, Vec2::Create(1.0f, 1.0f)); rs.SetVertexTangentFrame(2, tangentFrame);
		rs.SetVertexPosition(3, Vec3::Create(vmax.x, vmax.y, vmax.z)); rs.SetVertexUV(3, 0, Vec2::Create(0.0f, 1.0f)); rs.SetVertexTangentFrame(3, tangentFrame);
		rs.Indices.Add(0);	rs.Indices.Add(1);	rs.Indices.Add(2);
		rs.Indices.Add(0);	rs.Indices.Add(2);	rs.Indices.Add(3);

		// bottom
		tangentFrame = Quaternion::FromCoordinates(Vec3::Create(-1.0f, 0.0f, 0.0f), Vec3::Create(0.0f, -1.0f, 0.0f), Vec3::Create(0.0f, 0.0f, 1.0f));
		rs.SetVertexPosition(4, Vec3::Create(vmin.x, vmin.y, vmax.z)); rs.SetVertexUV(4, 0, Vec2::Create(0.0f, 0.0f)); rs.SetVertexTangentFrame(4, tangentFrame);
		rs.SetVertexPosition(5, Vec3::Create(vmin.x, vmin.y, vmin.z)); rs.SetVertexUV(5, 0, Vec2::Create(0.0f, 1.0f)); rs.SetVertexTangentFrame(5, tangentFrame);
		rs.SetVertexPosition(6, Vec3::Create(vmax.x, vmin.y, vmin.z)); rs.SetVertexUV(6, 0, Vec2::Create(1.0f, 1.0f)); rs.SetVertexTangentFrame(6, tangentFrame);
		rs.SetVertexPosition(7, Vec3::Create(vmax.x, vmin.y, vmax.z)); rs.SetVertexUV(7, 0, Vec2::Create(1.0f, 0.0f)); rs.SetVertexTangentFrame(7, tangentFrame);
		rs.Indices.Add(4);	rs.Indices.Add(5);	rs.Indices.Add(6);
		rs.Indices.Add(4);	rs.Indices.Add(6);	rs.Indices.Add(7);

		// front
		tangentFrame = Quaternion::FromCoordinates(Vec3::Create(1.0f, 0.0f, 0.0f), Vec3::Create(0.0f, 0.0f, 1.0f), Vec3::Create(0.0f, -1.0f, 0.0f));
		rs.SetVertexPosition(8,  Vec3::Create(vmin.x, vmin.y, vmax.z)); rs.SetVertexUV(8,  0, Vec2::Create(0.0f, 0.0f)); rs.SetVertexTangentFrame(8,  tangentFrame);
		rs.SetVertexPosition(9,  Vec3::Create(vmax.x, vmin.y, vmax.z)); rs.SetVertexUV(9,  0, Vec2::Create(1.0f, 0.0f)); rs.SetVertexTangentFrame(9,  tangentFrame);
		rs.SetVertexPosition(10, Vec3::Create(vmax.x, vmax.y, vmax.z)); rs.SetVertexUV(10, 0, Vec2::Create(1.0f, 1.0f)); rs.SetVertexTangentFrame(10, tangentFrame);
		rs.SetVertexPosition(11, Vec3::Create(vmin.x, vmax.y, vmax.z)); rs.SetVertexUV(11, 0, Vec2::Create(0.0f, 1.0f)); rs.SetVertexTangentFrame(11, tangentFrame);
		rs.Indices.Add(8);	rs.Indices.Add(9);	rs.Indices.Add(10);
		rs.Indices.Add(8);	rs.Indices.Add(10);	rs.Indices.Add(11);

		// back
		tangentFrame = Quaternion::FromCoordinates(Vec3::Create(-1.0f, 0.0f, 0.0f), Vec3::Create(0.0f, 0.0f, -1.0f), Vec3::Create(0.0f, -1.0f, 0.0f));
		rs.SetVertexPosition(12, Vec3::Create(vmin.x, vmin.y, vmin.z)); rs.SetVertexUV(12, 0, Vec2::Create(0.0f, 0.0f)); rs.SetVertexTangentFrame(12, tangentFrame);
		rs.SetVertexPosition(13, Vec3::Create(vmin.x, vmax.y, vmin.z)); rs.SetVertexUV(13, 0, Vec2::Create(0.0f, 1.0f)); rs.SetVertexTangentFrame(13, tangentFrame);
		rs.SetVertexPosition(14, Vec3::Create(vmax.x, vmax.y, vmin.z)); rs.SetVertexUV(14, 0, Vec2::Create(1.0f, 1.0f)); rs.SetVertexTangentFrame(14, tangentFrame);
		rs.SetVertexPosition(15, Vec3::Create(vmax.x, vmin.y, vmin.z)); rs.SetVertexUV(15, 0, Vec2::Create(1.0f, 0.0f)); rs.SetVertexTangentFrame(15, tangentFrame);
		rs.Indices.Add(12);	rs.Indices.Add(13);	rs.Indices.Add(14);
		rs.Indices.Add(12);	rs.Indices.Add(14);	rs.Indices.Add(15);

		// left
		tangentFrame = Quaternion::FromCoordinates(Vec3::Create(0.0f, 1.0f, 0.0f), Vec3::Create(-1.0f, 0.0f, 0.0f), Vec3::Create(0.0f, 0.0f, 1.0f));
		rs.SetVertexPosition(16, Vec3::Create(vmin.x, vmin.y, vmin.z)); rs.SetVertexUV(16, 0, Vec2::Create(0.0f, 0.0f)); rs.SetVertexTangentFrame(16, tangentFrame);
		rs.SetVertexPosition(17, Vec3::Create(vmin.x, vmin.y, vmax.z)); rs.SetVertexUV(17, 0, Vec2::Create(0.0f, 1.0f)); rs.SetVertexTangentFrame(17, tangentFrame);
		rs.SetVertexPosition(18, Vec3::Create(vmin.x, vmax.y, vmax.z)); rs.SetVertexUV(18, 0, Vec2::Create(1.0f, 1.0f)); rs.SetVertexTangentFrame(18, tangentFrame);
		rs.SetVertexPosition(19, Vec3::Create(vmin.x, vmax.y, vmin.z)); rs.SetVertexUV(19, 0, Vec2::Create(1.0f, 0.0f)); rs.SetVertexTangentFrame(19, tangentFrame);
		rs.Indices.Add(16);	rs.Indices.Add(17);	rs.Indices.Add(18);
		rs.Indices.Add(16);	rs.Indices.Add(18);	rs.Indices.Add(19);

		// right
		tangentFrame = Quaternion::FromCoordinates(Vec3::Create(0.0f, 1.0f, 0.0f), Vec3::Create(1.0f, 0.0f, 0.0f), Vec3::Create(0.0f, 0.0f, -1.0f));
		rs.SetVertexPosition(20, Vec3::Create(vmax.x, vmin.y, vmax.z)); rs.SetVertexUV(20, 0, Vec2::Create(0.0f, 0.0f)); rs.SetVertexTangentFrame(20, tangentFrame);
		rs.SetVertexPosition(21, Vec3::Create(vmax.x, vmin.y, vmin.z)); rs.SetVertexUV(21, 0, Vec2::Create(0.0f, 1.0f)); rs.SetVertexTangentFrame(21, tangentFrame);
		rs.SetVertexPosition(22, Vec3::Create(vmax.x, vmax.y, vmin.z)); rs.SetVertexUV(22, 0, Vec2::Create(1.0f, 1.0f)); rs.SetVertexTangentFrame(22, tangentFrame);
		rs.SetVertexPosition(23, Vec3::Create(vmax.x, vmax.y, vmax.z)); rs.SetVertexUV(23, 0, Vec2::Create(1.0f, 0.0f)); rs.SetVertexTangentFrame(23, tangentFrame);
		rs.Indices.Add(20);	rs.Indices.Add(21);	rs.Indices.Add(22);
		rs.Indices.Add(20);	rs.Indices.Add(22);	rs.Indices.Add(23);

		return rs;
	}
}

