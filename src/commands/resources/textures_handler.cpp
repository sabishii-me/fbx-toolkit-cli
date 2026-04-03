#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class TexturesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "textures";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Load textures but skip embedded data and animations
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int textureCount = scene->GetTextureCount();

        // Query specific texture by name
        if (pathSegments.size() >= 2) {
            std::string textureName = pathSegments[1];

            // Find the texture
            for (int i = 0; i < textureCount; i++) {
                FbxTexture* texture = scene->GetTexture(i);
                if (std::string(texture->GetName()) == textureName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"texture\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"name\": \"" << texture->GetName() << "\",\n";
                    json << "  \"type\": \"" << texture->GetClassId().GetName() << "\",\n";

                    // If it's a file texture, get file info
                    FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
                    if (fileTexture) {
                        json << "  \"file_path\": \"" << fileTexture->GetFileName() << "\",\n";
                        json << "  \"relative_path\": \"" << fileTexture->GetRelativeFileName() << "\",\n";
                        json << "  \"wrap_mode_u\": \"" << (fileTexture->GetWrapModeU() == FbxTexture::eRepeat ? "repeat" : "clamp") << "\",\n";
                        json << "  \"wrap_mode_v\": \"" << (fileTexture->GetWrapModeV() == FbxTexture::eRepeat ? "repeat" : "clamp") << "\",\n";
                        json << "  \"scale_u\": " << fileTexture->GetScaleU() << ",\n";
                        json << "  \"scale_v\": " << fileTexture->GetScaleV() << "\n";
                    } else {
                        json << "  \"details\": \"procedural or layered texture\"\n";
                    }

                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Texture not found\"}";
        }

        // List all textures (brief)
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"textures\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"textures\": [\n";

        for (int i = 0; i < textureCount; i++) {
            FbxTexture* texture = scene->GetTexture(i);
            FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"name\": \"" << texture->GetName() << "\",\n";
            json << "      \"type\": \"" << texture->GetClassId().GetName() << "\"";

            if (fileTexture) {
                json << ",\n";
                json << "      \"file_path\": \"" << fileTexture->GetFileName() << "\"";
            }

            json << "\n    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(TexturesHandler)
