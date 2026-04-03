#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>
#include <map>

class PosesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "poses";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Poses need full model import
        ios->SetBoolProp(IMP_FBX_MODEL, true);
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

private:
    // Compute bind pose from skin deformers when explicit pose isn't available
    FbxPose* ComputeBindPoseFromSkin(FbxScene* scene) const {
        FbxPose* pose = FbxPose::Create(scene, "ComputedBindPose");
        pose->SetIsBindPose(true);

        // Collect all bones from all skin deformers
        std::map<FbxNode*, FbxAMatrix> boneTransforms;

        int meshCount = scene->GetSrcObjectCount<FbxMesh>();
        for (int m = 0; m < meshCount; m++) {
            FbxMesh* mesh = scene->GetSrcObject<FbxMesh>(m);
            int deformerCount = mesh->GetDeformerCount(FbxDeformer::eSkin);

            for (int d = 0; d < deformerCount; d++) {
                FbxSkin* skin = (FbxSkin*)mesh->GetDeformer(d, FbxDeformer::eSkin);
                int clusterCount = skin->GetClusterCount();

                for (int c = 0; c < clusterCount; c++) {
                    FbxCluster* cluster = skin->GetCluster(c);
                    FbxNode* link = cluster->GetLink();

                    if (link && boneTransforms.find(link) == boneTransforms.end()) {
                        // Get bind pose transform from cluster
                        FbxAMatrix transformLinkMatrix;
                        cluster->GetTransformLinkMatrix(transformLinkMatrix);
                        boneTransforms[link] = transformLinkMatrix;
                    }
                }
            }
        }

        // Add bones to pose
        for (auto& pair : boneTransforms) {
            pose->Add(pair.first, pair.second, false); // false = global matrix
        }

        return pose->GetCount() > 0 ? pose : nullptr;
    }

public:

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int poseCount = scene->GetPoseCount();
        bool computedFromSkin = false;

        // If no explicit poses, try to compute from skin deformers
        FbxPose* computedPose = nullptr;
        if (poseCount == 0) {
            computedPose = ComputeBindPoseFromSkin(scene);
            if (computedPose) {
                computedFromSkin = true;
                poseCount = 1;
            }
        }

        // Query specific pose by name or index
        if (pathSegments.size() >= 2) {
            std::string poseName = pathSegments[1];

            // Try as index first
            int poseIndex = -1;
            try {
                poseIndex = std::stoi(poseName);
            } catch (...) {
                // Not an index, search by name
            }

            FbxPose* pose = nullptr;
            if (computedFromSkin && poseIndex == 0) {
                pose = computedPose;
            } else if (poseIndex >= 0 && poseIndex < scene->GetPoseCount()) {
                pose = scene->GetPose(poseIndex);
            } else {
                // Search by name
                if (computedFromSkin && poseName == computedPose->GetName()) {
                    pose = computedPose;
                } else {
                    for (int i = 0; i < scene->GetPoseCount(); i++) {
                        FbxPose* p = scene->GetPose(i);
                        if (std::string(p->GetName()) == poseName) {
                            pose = p;
                            break;
                        }
                    }
                }
            }

            if (pose) {
                std::ostringstream json;
                json << "{\n";
                json << "  \"resource_type\": \"pose\",\n";
                json << "  \"format_version\": \"1.0\",\n";
                json << "  \"name\": \"" << pose->GetName() << "\",\n";
                json << "  \"is_bind_pose\": " << (pose->IsBindPose() ? "true" : "false") << ",\n";
                json << "  \"is_rest_pose\": " << (pose->IsRestPose() ? "true" : "false") << ",\n";
                json << "  \"node_count\": " << pose->GetCount() << ",\n";
                json << "  \"nodes\": [\n";

                for (int i = 0; i < pose->GetCount(); i++) {
                    FbxNode* node = pose->GetNode(i);
                    FbxMatrix matrix = pose->GetMatrix(i);

                    // Convert to affine matrix for decomposition
                    FbxAMatrix aMatrix;
                    aMatrix.SetIdentity();
                    for (int r = 0; r < 4; r++) {
                        for (int c = 0; c < 4; c++) {
                            aMatrix.mData[r][c] = matrix.mData[r][c];
                        }
                    }

                    if (i > 0) json << ",\n";
                    json << "    {\n";
                    json << "      \"node_name\": \"" << node->GetName() << "\",\n";
                    json << "      \"fbx_unique_id\": " << node->GetUniqueID() << ",\n";
                    json << "      \"is_local\": " << (pose->IsLocalMatrix(i) ? "true" : "false") << ",\n";

                    // Matrix transform
                    FbxVector4 trans = aMatrix.GetT();
                    FbxVector4 rot = aMatrix.GetR();
                    FbxVector4 scale = aMatrix.GetS();

                    json << "      \"translation\": [" << trans[0] << ", " << trans[1] << ", " << trans[2] << "],\n";
                    json << "      \"rotation\": [" << rot[0] << ", " << rot[1] << ", " << rot[2] << "],\n";
                    json << "      \"scale\": [" << scale[0] << ", " << scale[1] << ", " << scale[2] << "]\n";
                    json << "    }";
                }

                json << "\n  ]\n";
                json << "}";
                return json.str();
            }

            return "{\"error\": \"Pose not found\"}";
        }

        // List all poses
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"poses\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        if (computedFromSkin) {
            json << "  \"note\": \"Bind pose computed from skin deformers\",\n";
        }
        json << "  \"poses\": [\n";

        if (computedFromSkin) {
            json << "    {\n";
            json << "      \"index\": 0,\n";
            json << "      \"name\": \"" << computedPose->GetName() << "\",\n";
            json << "      \"is_bind_pose\": true,\n";
            json << "      \"is_rest_pose\": false,\n";
            json << "      \"node_count\": " << computedPose->GetCount() << ",\n";
            json << "      \"computed\": true\n";
            json << "    }";
        } else {
            for (int i = 0; i < poseCount; i++) {
                FbxPose* pose = scene->GetPose(i);

                if (i > 0) json << ",\n";
                json << "    {\n";
                json << "      \"index\": " << i << ",\n";
                json << "      \"name\": \"" << pose->GetName() << "\",\n";
                json << "      \"is_bind_pose\": " << (pose->IsBindPose() ? "true" : "false") << ",\n";
                json << "      \"is_rest_pose\": " << (pose->IsRestPose() ? "true" : "false") << ",\n";
                json << "      \"node_count\": " << pose->GetCount() << ",\n";
                json << "      \"computed\": false\n";
                json << "    }";
            }
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(PosesHandler)
