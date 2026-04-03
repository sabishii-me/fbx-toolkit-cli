#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class LightsHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "lights";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::MINIMAL;
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        int lightCount = scene->GetSrcObjectCount<FbxLight>();

        // Query specific light by name
        if (pathSegments.size() >= 2) {
            std::string lightName = pathSegments[1];

            for (int i = 0; i < lightCount; i++) {
                FbxLight* light = scene->GetSrcObject<FbxLight>(i);
                if (std::string(light->GetName()) == lightName) {
                    std::ostringstream json;
                    json << "{\n";
                    json << "  \"resource_type\": \"light\",\n";
                    json << "  \"format_version\": \"1.0\",\n";
                    json << "  \"fbx_unique_id\": " << light->GetUniqueID() << ",\n";
                    json << "  \"name\": \"" << light->GetName() << "\",\n";

                    // Light position (from parent node)
                    FbxNode* node = light->GetNode();
                    if (node) {
                        FbxVector4 translation = node->LclTranslation.Get();
                        FbxVector4 rotation = node->LclRotation.Get();
                        json << "  \"position\": [" << translation[0] << ", " << translation[1] << ", " << translation[2] << "],\n";
                        json << "  \"rotation\": [" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << "],\n";
                    }

                    // Light type
                    const char* lightType = "Unknown";
                    switch (light->LightType.Get()) {
                        case FbxLight::ePoint: lightType = "Point"; break;
                        case FbxLight::eDirectional: lightType = "Directional"; break;
                        case FbxLight::eSpot: lightType = "Spot"; break;
                        case FbxLight::eArea: lightType = "Area"; break;
                        case FbxLight::eVolume: lightType = "Volume"; break;
                    }

                    json << "  \"type\": \"" << lightType << "\",\n";
                    json << "  \"cast_light\": " << (light->CastLight.Get() ? "true" : "false") << ",\n";

                    // Color and intensity
                    FbxDouble3 color = light->Color.Get();
                    json << "  \"color\": [" << color[0] << ", " << color[1] << ", " << color[2] << "],\n";
                    json << "  \"intensity\": " << light->Intensity.Get() << ",\n";

                    // Spot light properties
                    if (light->LightType.Get() == FbxLight::eSpot) {
                        json << "  \"inner_angle\": " << light->InnerAngle.Get() << ",\n";
                        json << "  \"outer_angle\": " << light->OuterAngle.Get() << ",\n";
                    }

                    // Shadow properties
                    json << "  \"cast_shadows\": " << (light->CastShadows.Get() ? "true" : "false") << "\n";

                    json << "}";
                    return json.str();
                }
            }

            return "{\"error\": \"Light not found\"}";
        }

        // List all lights
        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"lights\",\n";
        json << "  \"format_version\": \"1.0\",\n";
        json << "  \"lights\": [\n";

        for (int i = 0; i < lightCount; i++) {
            FbxLight* light = scene->GetSrcObject<FbxLight>(i);

            const char* lightType = "Unknown";
            switch (light->LightType.Get()) {
                case FbxLight::ePoint: lightType = "Point"; break;
                case FbxLight::eDirectional: lightType = "Directional"; break;
                case FbxLight::eSpot: lightType = "Spot"; break;
                case FbxLight::eArea: lightType = "Area"; break;
                case FbxLight::eVolume: lightType = "Volume"; break;
            }

            if (i > 0) json << ",\n";
            json << "    {\n";
            json << "      \"fbx_unique_id\": " << light->GetUniqueID() << ",\n";
            json << "      \"name\": \"" << light->GetName() << "\",\n";
            json << "      \"type\": \"" << lightType << "\",\n";
            json << "      \"intensity\": " << light->Intensity.Get() << "\n";
            json << "    }";
        }

        json << "\n  ]\n";
        json << "}";

        return json.str();
    }
};

REGISTER_RESOURCE_HANDLER(LightsHandler)
