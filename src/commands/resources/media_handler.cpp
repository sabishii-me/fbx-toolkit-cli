#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

class MediaHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "media";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        // Check if dumping data
        if (pathSegments.size() >= 3 && pathSegments[2] == "data") {
            return LoadStrategy::FULL;  // Need embedded data
        }
        return LoadStrategy::SELECTIVE;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Extract embedded data if dumping
        if (pathSegments.size() >= 3 && pathSegments[2] == "data") {
            ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, true);
        } else {
            ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        }
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int videoCount = scene->GetSrcObjectCount<FbxVideo>();

        // Query specific media by name
        if (pathSegments.size() >= 2) {
            std::string mediaName = pathSegments[1];
            bool dumpData = (pathSegments.size() >= 3 && pathSegments[2] == "data");

            // Find the video/media
            for (int i = 0; i < videoCount; i++) {
                FbxVideo* video = scene->GetSrcObject<FbxVideo>(i);
                if (std::string(video->GetName()) == mediaName) {

                    // Dump binary data
                    if (dumpData) {
                        const char* filePath = video->GetFileName();

                        // Try to find extracted file
                        std::ifstream file;

                        // 1. Try the SDK-reported path (where it should be extracted)
                        if (filePath && strlen(filePath) > 0) {
                            file.open(filePath, std::ios::binary);
                        }

                        if (!file.is_open()) {
                            std::cerr << "Error: Cannot find extracted media file\n";
                            std::cerr << "Expected path: " << (filePath ? filePath : "unknown") << "\n";
                            std::cerr << "Note: Media should be extracted during scene import with IMP_FBX_EXTRACT_EMBEDDED_DATA=true\n";
                            return "{\"error\": \"Extracted media file not found\"}";
                        }

                        // Set stdout to binary mode on Windows to prevent CRLF conversion
                        #ifdef _WIN32
                        _setmode(_fileno(stdout), _O_BINARY);
                        #endif

                        // Copy binary data directly to stdout
                        // Use stream buffer for efficient copy
                        std::cout << file.rdbuf();

                        if (!file.good() && !file.eof()) {
                            std::cerr << "Error: Failed to read media file\n";
                            return "{\"error\": \"Failed to read media file\"}";
                        }

                        return "";  // No JSON, just binary
                    }

                    // Return media info
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"media\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"name\": \"" << video->GetName() << "\",\n";
                    json << "  \"filename\": \"" << video->GetFileName() << "\",\n";
                    json << "  \"relative_filename\": \"" << video->GetRelativeFileName() << "\",\n";
                    json << "  \"width\": " << video->Width.Get() << ",\n";
                    json << "  \"height\": " << video->Height.Get() << "\n";
                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Media not found\"}";
        }

        // List all media (brief)
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"media\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"media\": [\n";

        for (int i = 0; i < videoCount; i++) {
            FbxVideo* video = scene->GetSrcObject<FbxVideo>(i);

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"name\": \"" << video->GetName() << "\",\n";
            json << "      \"filename\": \"" << video->GetFileName() << "\",\n";
            json << "      \"width\": " << video->Width.Get() << ",\n";
            json << "      \"height\": " << video->Height.Get() << "\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(MediaHandler)
