#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class MaterialsHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "materials";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Load materials but skip embedded textures and animations
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"materials\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"materials\": [\n";

        int materialCount = scene->GetMaterialCount();
        for (int i = 0; i < materialCount; i++) {
            FbxSurfaceMaterial* material = scene->GetMaterial(i);

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"name\": \"" << material->GetName() << "\",\n";
            json << "      \"shading_model\": \"" << material->ShadingModel.Get().Buffer() << "\"\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(MaterialsHandler)
