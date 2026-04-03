#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class MeshesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "meshes";
    }

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

            // Find the mesh
            for (int i = 0; i < meshCount; i++) {
                FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
                if (std::string(mesh->GetName()) == meshName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"mesh\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"name\": \"" << mesh->GetName() << "\",\n";
                    json << "  \"vertices\": " << mesh->GetControlPointsCount() << ",\n";
                    json << "  \"polygons\": " << mesh->GetPolygonCount() << ",\n";
                    json << "  \"has_normals\": " << (mesh->GetElementNormalCount() > 0 ? "true" : "false") << ",\n";
                    json << "  \"has_uvs\": " << (mesh->GetElementUVCount() > 0 ? "true" : "false") << ",\n";
                    json << "  \"has_vertex_colors\": " << (mesh->GetElementVertexColorCount() > 0 ? "true" : "false") << ",\n";

                    // Get attached materials
                    FbxNode* node = mesh->GetNode();
                    if (node) {
                        int materialCount = node->GetMaterialCount();
                        json << "  \"materials\": [";
                        for (int m = 0; m < materialCount; m++) {
                            FbxSurfaceMaterial* material = node->GetMaterial(m);
                            if (m > 0) json << ", ";
                            json << "\"" << material->GetName() << "\"";
                        }
                        json << "]\n";
                    } else {
                        json << "  \"materials\": []\n";
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
