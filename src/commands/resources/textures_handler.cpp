#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>

class TexturesHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "textures";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // If dumping texture data, extract embedded data
        if (pathSegments.size() >= 3 && pathSegments[2] == "data") {
            ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, true);
        } else {
            ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        }
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int textureCount = scene->GetTextureCount();

        // Query specific texture by name
        if (pathSegments.size() >= 2) {
            std::string textureName = pathSegments[1];
            bool dumpData = (pathSegments.size() >= 3 && pathSegments[2] == "data");

            // Find the texture
            for (int i = 0; i < textureCount; i++) {
                FbxTexture* texture = scene->GetTexture(i);
                if (std::string(texture->GetName()) == textureName) {
                    FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);

                    // Dump binary texture data
                    if (dumpData) {
                        if (!fileTexture) {
                            std::cerr << "Error: Texture is not a file texture\n";
                            return "{\"error\": \"Not a file texture\"}";
                        }

                        // Try external file path
                        const char* filePath = fileTexture->GetFileName();
                        std::ifstream file(filePath, std::ios::binary | std::ios::ate);

                        if (!file.is_open()) {
                            // Try relative path
                            const char* relPath = fileTexture->GetRelativeFileName();
                            if (relPath && strlen(relPath) > 0) {
                                file.open(relPath, std::ios::binary | std::ios::ate);
                            }
                        }

                        if (!file.is_open()) {
                            std::cerr << "Error: Cannot open texture file: " << filePath << "\n";
                            std::cerr << "Note: Texture path may be absolute from source system\n";
                            std::cerr << "Relative path: " << fileTexture->GetRelativeFileName() << "\n";
                            return "{\"error\": \"Cannot open texture file\"}";
                        }

                        std::streamsize size = file.tellg();
                        file.seekg(0, std::ios::beg);

                        // Read and output binary data to stdout
                        char buffer[8192];
                        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                            std::cout.write(buffer, file.gcount());
                        }

                        return "";  // No JSON, just binary data
                    }

                    // Return texture info (not data)
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"texture\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"name\": \"" << texture->GetName() << "\",\n";
                    json << "  \"type\": \"" << texture->GetClassId().GetName() << "\",\n";

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
