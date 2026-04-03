#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>
#include <functional>

class SkeletonHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "skeleton";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::MINIMAL;  // Need scene structure, skip heavy data
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Skeleton structure only - skip animations, meshes, materials, textures
        ios->SetBoolProp(IMP_FBX_TEXTURE, false);
        ios->SetBoolProp(IMP_FBX_MATERIAL, false);
        ios->SetBoolProp(IMP_FBX_MODEL, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
    }

    std::string HandleScene(FbxScene* scene,
                           const std::vector<std::string>& pathSegments) const override {
        // Check if requesting full tree (dangerous for agents!)
        bool isRecursive = (pathSegments.size() > 1 && pathSegments.back() == "**");

        if (isRecursive || pathSegments.size() > 1) {
            return BuildFullTree(scene);
        } else {
            return BuildBrief(scene);
        }
    }

private:
    std::string BuildBrief(FbxScene* scene) const {
        FbxNode* rootNode = scene->GetRootNode();

        // Count skeletons and bones
        int skeletonCount = 0;
        int totalBones = 0;
        std::vector<std::string> skeletonNames;

        std::function<void(FbxNode*)> findSkeletons = [&](FbxNode* node) {
            if (!node) return;
            FbxSkeleton* skel = node->GetSkeleton();
            if (skel) {
                totalBones++;
                if (skel->IsSkeletonRoot()) {
                    skeletonCount++;
                    skeletonNames.push_back(node->GetName());
                }
            }
            for (int i = 0; i < node->GetChildCount(); i++) {
                findSkeletons(node->GetChild(i));
            }
        };

        if (rootNode) findSkeletons(rootNode);

        // Brief output: just counts and names
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"skeleton\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"skeleton_count\": " << skeletonCount << ",\n";
        json << "  \"total_bones\": " << totalBones << ",\n";
        json << "  \"skeletons\": [";
        for (size_t i = 0; i < skeletonNames.size(); i++) {
            if (i > 0) json << ", ";
            json << "\"" << skeletonNames[i] << "\"";
        }
        json << "],\n";
        json << "  \"hint\": \"Use skeleton/0/** or -r for full tree (large output)\"\n";
        json << "}";

        return json.str();
    }

    std::string BuildFullTree(FbxScene* scene) const {
        FbxNode* rootNode = scene->GetRootNode();

        // Count skeletons and bones
        int skeletonCount = 0;
        int totalBones = 0;
        std::function<void(FbxNode*)> countSkeletons = [&](FbxNode* node) {
            if (!node) return;
            FbxSkeleton* skel = node->GetSkeleton();
            if (skel) {
                totalBones++;
                if (skel->IsSkeletonRoot()) skeletonCount++;
            }
            for (int i = 0; i < node->GetChildCount(); i++) {
                countSkeletons(node->GetChild(i));
            }
        };
        if (rootNode) countSkeletons(rootNode);

        // Recursive function to print bone tree
        std::function<void(FbxNode*, std::ostringstream&, int)> printBoneTree =
            [&](FbxNode* node, std::ostringstream& out, int depth) {
            if (!node) return;

            FbxSkeleton* skel = node->GetSkeleton();
            if (skel) {
                FbxDouble3 translation = node->LclTranslation.Get();
                FbxDouble3 rotation = node->LclRotation.Get();

                out << "{\n";
                out << "      \"name\": \"" << node->GetName() << "\",\n";
                out << "      \"type\": \"" << (skel->IsSkeletonRoot() ? "Root" : "Bone") << "\",\n";
                out << "      \"transform\": {\n";
                out << "        \"translation\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
                out << "        \"rotation\": [" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << "]\n";
                out << "      },\n";
                out << "      \"children\": [";

                // Print children
                bool firstChild = true;
                for (int i = 0; i < node->GetChildCount(); i++) {
                    FbxNode* child = node->GetChild(i);
                    if (child && child->GetSkeleton()) {
                        if (!firstChild) out << ",";
                        firstChild = false;
                        out << "\n        ";
                        printBoneTree(child, out, depth + 1);
                    }
                }

                out << "\n      ]\n";
                out << "    }";
            }
        };

        // Output JSON
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"skeleton\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"skeleton_count\": " << skeletonCount << ",\n";
        json << "  \"total_bones\": " << totalBones << ",\n";
        json << "  \"skeletons\": [\n";

        // Find and print all skeleton roots
        bool firstSkeleton = true;
        std::function<void(FbxNode*)> findSkeletonRoots = [&](FbxNode* node) {
            if (!node) return;
            FbxSkeleton* skel = node->GetSkeleton();
            if (skel && skel->IsSkeletonRoot()) {
                if (!firstSkeleton) json << ",\n";
                firstSkeleton = false;
                json << "    ";
                printBoneTree(node, json, 0);
            }
            for (int i = 0; i < node->GetChildCount(); i++) {
                findSkeletonRoots(node->GetChild(i));
            }
        };

        if (rootNode) {
            findSkeletonRoots(rootNode);
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

// Auto-register
REGISTER_RESOURCE_HANDLER(SkeletonHandler)
