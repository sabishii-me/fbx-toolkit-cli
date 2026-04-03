#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class MeshesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "meshes";
    }

private:
    const char* GetMappingModeName(FbxGeometryElement::EMappingMode mode) const {
        switch (mode) {
            case FbxGeometryElement::eNone: return "None";
            case FbxGeometryElement::eByControlPoint: return "ByControlPoint";
            case FbxGeometryElement::eByPolygonVertex: return "ByPolygonVertex";
            case FbxGeometryElement::eByPolygon: return "ByPolygon";
            case FbxGeometryElement::eByEdge: return "ByEdge";
            case FbxGeometryElement::eAllSame: return "AllSame";
            default: return "Unknown";
        }
    }

    const char* GetReferenceModeName(FbxGeometryElement::EReferenceMode mode) const {
        switch (mode) {
            case FbxGeometryElement::eDirect: return "Direct";
            case FbxGeometryElement::eIndex: return "Index";
            case FbxGeometryElement::eIndexToDirect: return "IndexToDirect";
            default: return "Unknown";
        }
    }

public:

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Load meshes but skip textures and animations
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int meshCount = scene->GetSrcObjectCount<FbxMesh>();

        // Query specific mesh by name
        if (pathSegments.size() >= 2) {
            std::string meshName = pathSegments[1];
            bool detailed = (pathSegments.size() >= 3 && pathSegments[2] == "**");

            // Find the mesh
            for (int i = 0; i < meshCount; i++) {
                FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
                if (std::string(mesh->GetName()) == meshName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"mesh\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"fbx_unique_id\": " << mesh->GetUniqueID() << ",\n";
                    json << "  \"name\": \"" << mesh->GetName() << "\",\n";
                    json << "  \"vertices\": " << mesh->GetControlPointsCount() << ",\n";
                    json << "  \"polygons\": " << mesh->GetPolygonCount() << ",\n";

                    // Geometry element counts (raw FBX structure)
                    json << "  \"elements\": {\n";
                    json << "    \"normals\": " << mesh->GetElementNormalCount() << ",\n";
                    json << "    \"tangents\": " << mesh->GetElementTangentCount() << ",\n";
                    json << "    \"binormals\": " << mesh->GetElementBinormalCount() << ",\n";
                    json << "    \"uvs\": " << mesh->GetElementUVCount() << ",\n";
                    json << "    \"vertex_colors\": " << mesh->GetElementVertexColorCount() << ",\n";
                    json << "    \"materials\": " << mesh->GetElementMaterialCount() << ",\n";
                    json << "    \"polygon_groups\": " << mesh->GetElementPolygonGroupCount() << "\n";
                    json << "  },\n";

                    // Deformers (skinning)
                    int deformerCount = mesh->GetDeformerCount();
                    json << "  \"deformers\": " << deformerCount << ",\n";
                    json << "  \"deformer_types\": [";
                    for (int d = 0; d < deformerCount; d++) {
                        FbxDeformer* deformer = mesh->GetDeformer(d);
                        if (d > 0) json << ", ";
                        json << "\"" << deformer->GetTypeName() << "\"";
                    }
                    json << "],\n";

                    // Blend shapes
                    int blendShapeCount = mesh->GetShapeCount();
                    json << "  \"blend_shapes\": " << blendShapeCount << ",\n";

                    // Materials
                    FbxNode* node = mesh->GetNode();
                    if (node) {
                        int materialCount = node->GetMaterialCount();
                        json << "  \"materials\": [";
                        for (int m = 0; m < materialCount; m++) {
                            FbxSurfaceMaterial* material = node->GetMaterial(m);
                            if (m > 0) json << ", ";
                            json << "{\"name\": \"" << material->GetName() << "\", ";
                            json << "\"fbx_unique_id\": " << material->GetUniqueID() << "}";
                        }
                        json << "]\n";
                    } else {
                        json << "  \"materials\": []\n";
                    }

                    // If detailed flag, add element details
                    if (detailed) {
                        json << ",\n  \"element_details\": {\n";

                        // UV layers
                        json << "    \"uv_layers\": [\n";
                        for (int u = 0; u < mesh->GetElementUVCount(); u++) {
                            FbxGeometryElementUV* uvElement = mesh->GetElementUV(u);
                            if (u > 0) json << ",\n";
                            json << "      {\n";
                            json << "        \"name\": \"" << uvElement->GetName() << "\",\n";
                            json << "        \"mapping_mode\": \"" << GetMappingModeName(uvElement->GetMappingMode()) << "\",\n";
                            json << "        \"reference_mode\": \"" << GetReferenceModeName(uvElement->GetReferenceMode()) << "\",\n";
                            json << "        \"count\": " << uvElement->GetDirectArray().GetCount() << "\n";
                            json << "      }";
                        }
                        json << "\n    ]\n";

                        json << "  }\n";
                    }

                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Mesh not found\"}";
        }

        // List all meshes (brief)
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"meshes\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"meshes\": [\n";

        for (int i = 0; i < meshCount; i++) {
            FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"name\": \"" << mesh->GetName() << "\",\n";
            json << "      \"vertices\": " << mesh->GetControlPointsCount() << ",\n";
            json << "      \"polygons\": " << mesh->GetPolygonCount() << "\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(MeshesHandler)
