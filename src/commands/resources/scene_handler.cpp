#include "resource_handler.h"
#include "resource_registry.h"
#include <sstream>

class SceneHandler : public ResourceHandler {
public:
    const char* GetResourceName() const override {
        return "scene";
    }

    LoadStrategy GetLoadStrategy(const std::vector<std::string>& pathSegments) const override {
        return LoadStrategy::MINIMAL;  // Need to import for scene statistics
    }

    void ConfigureImport(FbxIOSettings* ios, const std::vector<std::string>& pathSegments) const override {
        // Skip heavy data - we just need structure/counts
        ios->SetBoolProp(IMP_FBX_EXTRACT_EMBEDDED_DATA, false);
        ios->SetBoolProp(IMP_FBX_ANIMATION, false);
    }

    std::string HandleScene(FbxScene* scene, const std::vector<std::string>& pathSegments) const override {
        FbxGlobalSettings& settings = scene->GetGlobalSettings();
        FbxSystemUnit unit = settings.GetSystemUnit();

        std::ostringstream json;
        json << "{\n";
        json << "  \"resource_type\": \"scene\",\n";
        json << "  \"format_version\": \"1.0\",\n";

        // Scene info
        FbxDocumentInfo* info = scene->GetSceneInfo();
        if (info) {
            json << "  \"title\": \"" << (info->mTitle.IsEmpty() ? "Untitled" : info->mTitle.Buffer()) << "\",\n";
            json << "  \"author\": \"" << (info->mAuthor.IsEmpty() ? "" : info->mAuthor.Buffer()) << "\",\n";
        }

        // Coordinate system
        FbxAxisSystem axisSystem = settings.GetAxisSystem();
        int upSign;
        FbxAxisSystem::EUpVector upVector = axisSystem.GetUpVector(upSign);
        json << "  \"coordinate_system\": {\n";
        json << "    \"up_axis\": \"" << (upVector == FbxAxisSystem::eYAxis ? "Y" : (upVector == FbxAxisSystem::eZAxis ? "Z" : "X")) << "\",\n";
        json << "    \"coord_system\": \"" << (axisSystem.GetCoorSystem() == FbxAxisSystem::eRightHanded ? "RightHanded" : "LeftHanded") << "\"\n";
        json << "  },\n";

        // Units
        json << "  \"units\": {\n";
        json << "    \"scale_factor\": " << unit.GetScaleFactor() << ",\n";
        json << "    \"name\": \"" << unit.GetScaleFactorAsString().Buffer() << "\"\n";
        json << "  },\n";

        // Time mode
        FbxTime::EMode timeMode = settings.GetTimeMode();
        const double sdkTimelineRate = FbxTime::GetFrameRate(timeMode);
        const double declaredRate = timeMode == FbxTime::eCustom
            ? settings.GetCustomFrameRate()
            : sdkTimelineRate;
        json << "  \"time\": {\n";
        json << "    \"mode\": " << static_cast<int>(timeMode) << ",\n";
        json << "    \"fps\": " << declaredRate << ",\n";
        json << "    \"sdk_timeline_fps\": " << sdkTimelineRate << "\n";
        json << "  },\n";

        // Statistics
        int nodeCount = scene->GetNodeCount();
        int meshCount = scene->GetSrcObjectCount<FbxMesh>();
        int materialCount = scene->GetMaterialCount();
        int textureCount = scene->GetTextureCount();

        json << "  \"statistics\": {\n";
        json << "    \"nodes\": " << nodeCount << ",\n";
        json << "    \"meshes\": " << meshCount << ",\n";
        json << "    \"materials\": " << materialCount << ",\n";
        json << "    \"textures\": " << textureCount << "\n";
        json << "  }\n";
        json << "}";

        return json.str();
    }
};

// Auto-register
REGISTER_RESOURCE_HANDLER(SceneHandler)
