#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class DeformersHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "deformers";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::MINIMAL;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        // Query deformers for a specific mesh
        if (pathSegments.size() >= 2) {
            std::string meshName = pathSegments[1];

            // Find the mesh
            int meshCount = scene->GetSrcObjectCount<FbxMesh>();
            for (int i = 0; i < meshCount; i++) {
                FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
                if (std::string(mesh->GetName()) == meshName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"deformers\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"mesh_name\": \"" << meshName << "\",\n";
                    json << "  \"deformers\": [\n";

                    int deformerCount = mesh->GetDeformerCount();
                    for (int d = 0; d < deformerCount; d++) {
                        FbxDeformer* deformer = mesh->GetDeformer(d);

                        if (d > 0) json << ",\n";
                        json << "    {\n";
                        json << "      \"fbx_unique_id\": " << deformer->GetUniqueID() << ",\n";
                        json << "      \"name\": \"" << deformer->GetName() << "\",\n";
                        json << "      \"type\": \"" << deformer->GetTypeName() << "\"";

                        // If it's a skin deformer, list clusters (bones)
                        if (deformer->GetDeformerType() == FbxDeformer::eSkin) {
                            FbxSkin* skin = (FbxSkin*)deformer;
                            int clusterCount = skin->GetClusterCount();

                            json << ",\n";
                            json << "      \"clusters\": " << clusterCount << ",\n";
                            json << "      \"bones\": [\n";

                            for (int c = 0; c < clusterCount; c++) {
                                FbxCluster* cluster = skin->GetCluster(c);
                                FbxNode* link = cluster->GetLink();

                                if (c > 0) json << ",\n";
                                json << "        {\n";
                                json << "          \"bone_name\": \"" << (link ? link->GetName() : "null") << "\",\n";
                                json << "          \"bone_id\": " << (link ? link->GetUniqueID() : 0) << ",\n";
                                json << "          \"control_point_indices_count\": " << cluster->GetControlPointIndicesCount() << ",\n";

                                const char* linkMode = "Unknown";
                                switch (cluster->GetLinkMode()) {
                                    case FbxCluster::eNormalize: linkMode = "Normalize"; break;
                                    case FbxCluster::eAdditive: linkMode = "Additive"; break;
                                    case FbxCluster::eTotalOne: linkMode = "TotalOne"; break;
                                }
                                json << "          \"link_mode\": \"" << linkMode << "\"\n";
                                json << "        }";
                            }

                            json << "\n      ]";
                        }

                        // If it's a blend shape deformer
                        if (deformer->GetDeformerType() == FbxDeformer::eBlendShape) {
                            FbxBlendShape* blendShape = (FbxBlendShape*)deformer;
                            int channelCount = blendShape->GetBlendShapeChannelCount();

                            json << ",\n";
                            json << "      \"channels\": " << channelCount << ",\n";
                            json << "      \"targets\": [\n";

                            for (int ch = 0; ch < channelCount; ch++) {
                                FbxBlendShapeChannel* channel = blendShape->GetBlendShapeChannel(ch);

                                if (ch > 0) json << ",\n";
                                json << "        {\n";
                                json << "          \"channel_name\": \"" << channel->GetName() << "\",\n";
                                json << "          \"target_shape_count\": " << channel->GetTargetShapeCount() << ",\n";
                                json << "          \"deform_percent\": " << channel->DeformPercent.Get() << "\n";
                                json << "        }";
                            }

                            json << "\n      ]";
                        }

                        json << "\n    }";
                    }

                    json << "\n  ]\n";
                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Mesh not found\"}";
        }

        // List all meshes with deformers
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"deformers_summary\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"meshes_with_deformers\": [\n";

        int meshCount = scene->GetSrcObjectCount<FbxMesh>();
        bool first = true;

        for (int i = 0; i < meshCount; i++) {
            FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
            int deformerCount = mesh->GetDeformerCount();

            if (deformerCount > 0) {
                if (!first) json << ",\n";
                first = false;

                json << "    {\n";
                json << "      \"mesh_name\": \"" << mesh->GetName() << "\",\n";
                json << "      \"deformer_count\": " << deformerCount << ",\n";
                json << "      \"deformer_types\": [";

                for (int d = 0; d < deformerCount; d++) {
                    if (d > 0) json << ", ";
                    json << "\"" << mesh->GetDeformer(d)->GetTypeName() << "\"";
                }

                json << "]\n";
                json << "    }";
            }
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(DeformersHandler)
