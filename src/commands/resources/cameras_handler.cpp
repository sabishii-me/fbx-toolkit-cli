#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class CamerasHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "cameras";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::MINIMAL;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int cameraCount = scene->GetSrcObjectCount<FbxCamera>();

        // Query specific camera by name
        if (pathSegments.size() >= 2) {
            std::string cameraName = pathSegments[1];

            for (int i = 0; i < cameraCount; i++) {
                FbxCamera* camera = scene->GetSrcObject<FbxCamera>(i);
                if (std::string(camera->GetName()) == cameraName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"camera\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"fbx_unique_id\": " << camera->GetUniqueID() << ",\n";
                    json << "  \"name\": \"" << camera->GetName() << "\",\n";

                    // Camera position (from parent node)
                    FbxNode* node = camera->GetNode();
                    if (node) {
                        FbxVector4 translation = node->LclTranslation.Get();
                        FbxVector4 rotation = node->LclRotation.Get();
                        json << "  \"position\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
                        json << "  \"rotation\": [" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << "],\n";
                    }

                    // Camera properties
                    json << "  \"projection\": \"" << (camera->ProjectionType.Get() == FbxCamera::ePerspective ? "Perspective" : "Orthographic") << "\",\n";
                    json << "  \"fov\": " << camera->FieldOfView.Get() << ",\n";
                    json << "  \"near_plane\": " << camera->NearPlane.Get() << ",\n";
                    json << "  \"far_plane\": " << camera->FarPlane.Get() << ",\n";
                    json << "  \"aspect_width\": " << camera->AspectWidth.Get() << ",\n";
                    json << "  \"aspect_height\": " << camera->AspectHeight.Get() << ",\n";
                    json << "  \"film_width\": " << camera->FilmWidth.Get() << ",\n";
                    json << "  \"film_height\": " << camera->FilmHeight.Get() << ",\n";
                    json << "  \"focal_length\": " << camera->FocalLength.Get() << "\n";
                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Camera not found\"}";
        }

        // List all cameras
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"cameras\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"cameras\": [\n";

        for (int i = 0; i < cameraCount; i++) {
            FbxCamera* camera = scene->GetSrcObject<FbxCamera>(i);

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"fbx_unique_id\": " << camera->GetUniqueID() << ",\n";
            json << "      \"name\": \"" << camera->GetName() << "\",\n";
            json << "      \"projection\": \"" << (camera->ProjectionType.Get() == FbxCamera::ePerspective ? "Perspective" : "Orthographic") << "\",\n";
            json << "      \"fov\": " << camera->FieldOfView.Get() << "\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(CamerasHandler)
