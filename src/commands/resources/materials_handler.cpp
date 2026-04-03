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
        int materialCount = scene->GetMaterialCount();

        // Query specific material by name
        if (pathSegments.size() >= 2) {
            std::string materialName = pathSegments[1];

            // Find the material
            for (int i = 0; i < materialCount; i++) {
                FbxSurfaceMaterial* material = scene->GetMaterial(i);
                if (std::string(material->GetName()) == materialName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"material\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"name\": \"" << material->GetName() << "\",\n";
                    json << "  \"shading_model\": \"" << material->ShadingModel.Get().Buffer() << "\",\n";

                    // Get textures
                    json << "  \"textures\": {\n";
                    bool firstTexture = true;

                    auto addTexture = [&](const char* propName, const FbxProperty& prop) {
                        if (prop.IsValid()) {
                            int textureCount = prop.GetSrcObjectCount<FbxTexture>();
                            if (textureCount > 0) {
                                if (!firstTexture) json << ",\n";
                                firstTexture = false;

                                json << "    \"" << propName << "\": [";
                                for (int t = 0; t < textureCount; t++) {
                                    FbxTexture* texture = prop.GetSrcObject<FbxTexture>(t);
                                    if (t > 0) json << ", ";
                                    json << "\"" << texture->GetName() << "\"";

                                    // If it's a file texture, add file path
                                    FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
                                    if (fileTexture) {
                                        json << " (\"" << fileTexture->GetFileName() << "\")";
                                    }
                                }
                                json << "]";
                            }
                        }
                    };

                    addTexture("diffuse", material->FindProperty(FbxSurfaceMaterial::sDiffuse));
                    addTexture("specular", material->FindProperty(FbxSurfaceMaterial::sSpecular));
                    addTexture("emissive", material->FindProperty(FbxSurfaceMaterial::sEmissive));
                    addTexture("normal", material->FindProperty(FbxSurfaceMaterial::sNormalMap));
                    addTexture("bump", material->FindProperty(FbxSurfaceMaterial::sBump));
                    addTexture("ambient", material->FindProperty(FbxSurfaceMaterial::sAmbient));

                    json << "\n  }\n";
                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Material not found\"}";
        }

        // List all materials (brief)
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"materials\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"materials\": [\n";

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
