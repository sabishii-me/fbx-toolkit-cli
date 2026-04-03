#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>
#include <functional>

class NodesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "nodes";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::MINIMAL;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Nodes need structure, skip animations/textures
        ios->SetBoolProp(IMP_FBX_TEXTURE, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"nodes\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"nodes\": [\n";

        FbxNode* rootNode = scene->GetRootNode();
        bool first = true;

        std::function<void(FbxNode*, int)> printNode = [&](FbxNode* node, int depth) {
            if (!node) return;

            if (!first) json << ",\n";
            first = false;

            json << "    {\n";
            json << "      \"name\": \"" << node->GetName() << "\",\n";
            json << "      \"type\": \"" << node->GetTypeName() << "\",\n";

            FbxDouble3 translation = node->LclTranslation.Get();
            FbxDouble3 rotation = node->LclRotation.Get();
            FbxDouble3 scale = node->LclScaling.Get();

            json << "      \"transform\": {\n";
            json << "        \"translation\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
            json << "        \"rotation\": [" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << "],\n";
            json << "        \"scale\": [" << scale[0] << ", " << scale[1] << ", " << scale[2] << "]\n";
            json << "      },\n";

            json << "      \"children\": [";
            for (int i = 0; i < node->GetChildCount(); i++) {
                if (i > 0) json << ", ";
                json << "\"" << node->GetChild(i)->GetName() << "\"";
            }
            json << "]\n";
            json << "    }";

            for (int i = 0; i < node->GetChildCount(); i++) {
                printNode(node->GetChild(i), depth + 1);
            }
        };

        if (rootNode) {
            for (int i = 0; i < rootNode->GetChildCount(); i++) {
                printNode(rootNode->GetChild(i), 0);
            }
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(NodesHandler)
