#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class BlendShapesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "blendshapes";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        ios->SetBoolProp(IMP_FBX_MODEL, true);
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        // Query blend shapes for a specific mesh
        if (pathSegments.size() >= 2) {
            std::string meshName = pathSegments[1];

            // Find the mesh
            int meshCount = scene->GetSrcObjectCount<FbxMesh>();
            for (int i = 0; i < meshCount; i++) {
                FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
                if (std::string(mesh->GetName()) == meshName) {
                    return GetMeshBlendShapes(mesh);
                }
            }

            return "{\"error\": \"Mesh not found\"}";
        }

        // List all meshes with blend shapes
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"blendshapes_summary\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"meshes_with_blendshapes\": [\n";

        int meshCount = scene->GetSrcObjectCount<FbxMesh>();
        bool first = true;

        for (int i = 0; i < meshCount; i++) {
            FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(i);
            int blendShapeDeformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);

            if (blendShapeDeformerCount > 0) {
                if (!first) json << ",\n";
                first = false;

                json << "    {\n";
                json << "      \"mesh_name\": \"" << mesh->GetName() << "\",\n";
                json << "      \"mesh_id\": " << mesh->GetUniqueID() << ",\n";
                json << "      \"blendshape_deformers\": " << blendShapeDeformerCount << ",\n";

                // Count total channels
                int totalChannels = 0;
                for (int d = 0; d < blendShapeDeformerCount; d++) {
                    FbxBlendShape* blendShape = (FbxBlendShape*)mesh->GetDeformer(d, FbxDeformer::eBlendShape);
                    totalChannels += blendShape->GetBlendShapeChannelCount();
                }

                json << "      \"total_channels\": " << totalChannels << "\n";
                json << "    }";
            }
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }

private:
    std::string GetMeshBlendShapes(FbxMesh* mesh) const {
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"blendshapes\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"mesh_name\": \"" << mesh->GetName() << "\",\n";
        json << "  \"mesh_id\": " << mesh->GetUniqueID() << ",\n";
        json << "  \"deformers\": [\n";

        int deformerCount = mesh->GetDeformerCount(FbxDeformer::eBlendShape);

        for (int d = 0; d < deformerCount; d++) {
            FbxBlendShape* blendShape = (FbxBlendShape*)mesh->GetDeformer(d, FbxDeformer::eBlendShape);

            if (d > 0) json << ",\n";
            json << "    {\n";
            json << "      \"deformer_name\": \"" << blendShape->GetName() << "\",\n";
            json << "      \"deformer_id\": " << blendShape->GetUniqueID() << ",\n";
            json << "      \"channels\": [\n";

            int channelCount = blendShape->GetBlendShapeChannelCount();
            for (int c = 0; c < channelCount; c++) {
                FbxBlendShapeChannel* channel = blendShape->GetBlendShapeChannel(c);

                if (c > 0) json << ",\n";
                json << "        {\n";
                json << "          \"channel_name\": \"" << channel->GetName() << "\",\n";
                json << "          \"channel_id\": " << channel->GetUniqueID() << ",\n";
                json << "          \"deform_percent\": " << channel->DeformPercent.Get() << ",\n";

                // List target shapes
                int targetShapeCount = channel->GetTargetShapeCount();
                json << "          \"target_shapes\": [\n";

                for (int t = 0; t < targetShapeCount; t++) {
                    FbxShape* shape = channel->GetTargetShape(t);

                    if (t > 0) json << ",\n";
                    json << "            {\n";
                    json << "              \"shape_name\": \"" << shape->GetName() << "\",\n";
                    json << "              \"shape_id\": " << shape->GetUniqueID() << ",\n";
                    json << "              \"control_points\": " << shape->GetControlPointsCount() << "\n";
                    json << "            }";
                }

                json << "\n          ]\n";
                json << "        }";
            }

            json << "\n      ]\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(BlendShapesHandler)
