#include "command.h"
#include <fbxsdk.h>
#include <iostream>
#include <string>

class CreateCommand : public Command {
public:
    const char* GetName() const override {
        return "create";
    }

    const char* GetDescription() const override {
        return "Create FBX files with test primitives and scenes";
    }

    const char* GetUsage() const override {
        return "create <output.fbx> --primitive <type> | --skeleton <type> | --scene <type>";
    }

    int Execute(const std::vector<std::string>& args) override {
        if (args.size() < 3) {
            std::cerr << "Usage: " << GetUsage() << std::endl;
            std::cerr << "\nPrimitives: cube, sphere, plane" << std::endl;
            std::cerr << "Skeletons: simple (Root->Spine->Head), biped" << std::endl;
            std::cerr << "Scenes: test (full test scene with mesh+skeleton+animation)" << std::endl;
            return 1;
        }

        std::string outputPath = args[0];
        std::string option = args[1];
        std::string type = args[2];

        // Initialize FBX SDK
        FbxManager* manager = FbxManager::Create();
        FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
        manager->SetIOSettings(ios);

        FbxScene* scene = FbxScene::Create(manager, "CreatedScene");

        // Set scene properties
        FbxGlobalSettings& globalSettings = scene->GetGlobalSettings();
        globalSettings.SetSystemUnit(FbxSystemUnit::m);
        globalSettings.SetAxisSystem(FbxAxisSystem::MayaYUp);

        FbxNode* rootNode = scene->GetRootNode();

        if (option == "--primitive") {
            if (type == "cube") {
                CreateCube(scene, rootNode);
            } else if (type == "sphere") {
                CreateSphere(scene, rootNode);
            } else if (type == "plane") {
                CreatePlane(scene, rootNode);
            } else {
                std::cerr << "Unknown primitive: " << type << std::endl;
                manager->Destroy();
                return 1;
            }
        } else if (option == "--skeleton") {
            if (type == "simple") {
                CreateSimpleSkeleton(scene, rootNode);
            } else if (type == "biped") {
                CreateBipedSkeleton(scene, rootNode);
            } else {
                std::cerr << "Unknown skeleton: " << type << std::endl;
                manager->Destroy();
                return 1;
            }
        } else if (option == "--scene") {
            if (type == "test") {
                CreateTestScene(scene, rootNode);
            } else {
                std::cerr << "Unknown scene: " << type << std::endl;
                manager->Destroy();
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << option << std::endl;
            manager->Destroy();
            return 1;
        }

        // Export
        FbxExporter* exporter = FbxExporter::Create(manager, "");
        if (!exporter->Initialize(outputPath.c_str(), -1, manager->GetIOSettings())) {
            std::cerr << "Failed to initialize exporter: " << exporter->GetStatus().GetErrorString() << std::endl;
            manager->Destroy();
            return 1;
        }

        if (!exporter->Export(scene)) {
            std::cerr << "Failed to export: " << exporter->GetStatus().GetErrorString() << std::endl;
            exporter->Destroy();
            manager->Destroy();
            return 1;
        }

        std::cout << "Created FBX: " << outputPath << std::endl;

        exporter->Destroy();
        manager->Destroy();
        return 0;
    }

private:
    void CreateCube(FbxScene* scene, FbxNode* parentNode) const {
        FbxMesh* mesh = FbxMesh::Create(scene, "Cube");

        // 8 vertices
        mesh->InitControlPoints(8);
        FbxVector4* vertices = mesh->GetControlPoints();
        vertices[0].Set(-0.5, -0.5, -0.5);
        vertices[1].Set( 0.5, -0.5, -0.5);
        vertices[2].Set( 0.5,  0.5, -0.5);
        vertices[3].Set(-0.5,  0.5, -0.5);
        vertices[4].Set(-0.5, -0.5,  0.5);
        vertices[5].Set( 0.5, -0.5,  0.5);
        vertices[6].Set( 0.5,  0.5,  0.5);
        vertices[7].Set(-0.5,  0.5,  0.5);

        // 6 quad faces
        int faces[6][4] = {
            {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
            {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0}
        };

        for (int i = 0; i < 6; i++) {
            mesh->BeginPolygon();
            for (int j = 0; j < 4; j++) {
                mesh->AddPolygon(faces[i][j]);
            }
            mesh->EndPolygon();
        }

        // Normals
        FbxGeometryElementNormal* normalElement = mesh->CreateElementNormal();
        normalElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
        normalElement->SetReferenceMode(FbxGeometryElement::eDirect);

        FbxVector4 normals[6] = {
            FbxVector4(0, 0, -1), FbxVector4(0, 0, 1), FbxVector4(-1, 0, 0),
            FbxVector4(1, 0, 0), FbxVector4(0, 1, 0), FbxVector4(0, -1, 0)
        };

        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 4; j++) {
                normalElement->GetDirectArray().Add(normals[i]);
            }
        }

        FbxNode* meshNode = FbxNode::Create(scene, "CubeNode");
        meshNode->SetNodeAttribute(mesh);
        parentNode->AddChild(meshNode);
    }

    void CreateSphere(FbxScene* scene, FbxNode* parentNode) const {
        FbxMesh* mesh = FbxMesh::Create(scene, "Sphere");

        // Simple UV sphere: 8 segments, 6 rings
        int segments = 8;
        int rings = 6;
        int vertexCount = (rings - 1) * segments + 2; // +2 for poles

        mesh->InitControlPoints(vertexCount);
        FbxVector4* vertices = mesh->GetControlPoints();

        double radius = 0.5;
        int index = 0;

        // Top pole
        vertices[index++].Set(0, radius, 0);

        // Rings
        for (int r = 1; r < rings; r++) {
            double theta = (M_PI * r) / rings;
            double y = radius * cos(theta);
            double ringRadius = radius * sin(theta);

            for (int s = 0; s < segments; s++) {
                double phi = (2.0 * M_PI * s) / segments;
                double x = ringRadius * cos(phi);
                double z = ringRadius * sin(phi);
                vertices[index++].Set(x, y, z);
            }
        }

        // Bottom pole
        vertices[index++].Set(0, -radius, 0);

        // Create faces
        // Top cap
        for (int s = 0; s < segments; s++) {
            mesh->BeginPolygon();
            mesh->AddPolygon(0);
            mesh->AddPolygon(1 + s);
            mesh->AddPolygon(1 + (s + 1) % segments);
            mesh->EndPolygon();
        }

        // Middle rings
        for (int r = 0; r < rings - 2; r++) {
            for (int s = 0; s < segments; s++) {
                int current = 1 + r * segments + s;
                int next = 1 + r * segments + (s + 1) % segments;
                int below = 1 + (r + 1) * segments + s;
                int belowNext = 1 + (r + 1) * segments + (s + 1) % segments;

                mesh->BeginPolygon();
                mesh->AddPolygon(current);
                mesh->AddPolygon(below);
                mesh->AddPolygon(belowNext);
                mesh->AddPolygon(next);
                mesh->EndPolygon();
            }
        }

        // Bottom cap
        int lastRingStart = 1 + (rings - 2) * segments;
        int bottomPole = vertexCount - 1;
        for (int s = 0; s < segments; s++) {
            mesh->BeginPolygon();
            mesh->AddPolygon(lastRingStart + s);
            mesh->AddPolygon(bottomPole);
            mesh->AddPolygon(lastRingStart + (s + 1) % segments);
            mesh->EndPolygon();
        }

        FbxNode* meshNode = FbxNode::Create(scene, "SphereNode");
        meshNode->SetNodeAttribute(mesh);
        parentNode->AddChild(meshNode);
    }

    void CreatePlane(FbxScene* scene, FbxNode* parentNode) const {
        FbxMesh* mesh = FbxMesh::Create(scene, "Plane");

        // 4 vertices
        mesh->InitControlPoints(4);
        FbxVector4* vertices = mesh->GetControlPoints();
        vertices[0].Set(-1, 0, -1);
        vertices[1].Set( 1, 0, -1);
        vertices[2].Set( 1, 0,  1);
        vertices[3].Set(-1, 0,  1);

        // 1 quad face
        mesh->BeginPolygon();
        mesh->AddPolygon(0);
        mesh->AddPolygon(1);
        mesh->AddPolygon(2);
        mesh->AddPolygon(3);
        mesh->EndPolygon();

        // Normal
        FbxGeometryElementNormal* normalElement = mesh->CreateElementNormal();
        normalElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
        normalElement->SetReferenceMode(FbxGeometryElement::eDirect);
        for (int i = 0; i < 4; i++) {
            normalElement->GetDirectArray().Add(FbxVector4(0, 1, 0));
        }

        FbxNode* meshNode = FbxNode::Create(scene, "PlaneNode");
        meshNode->SetNodeAttribute(mesh);
        parentNode->AddChild(meshNode);
    }

    void CreateSimpleSkeleton(FbxScene* scene, FbxNode* parentNode) const {
        // Root -> Spine -> Head
        FbxSkeleton* rootSkeleton = FbxSkeleton::Create(scene, "Root_Skeleton");
        rootSkeleton->SetSkeletonType(FbxSkeleton::eRoot);
        FbxNode* rootBone = FbxNode::Create(scene, "Root");
        rootBone->SetNodeAttribute(rootSkeleton);
        rootBone->LclTranslation.Set(FbxDouble3(0, 0, 0));
        parentNode->AddChild(rootBone);

        FbxSkeleton* spineSkeleton = FbxSkeleton::Create(scene, "Spine_Skeleton");
        spineSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
        FbxNode* spineBone = FbxNode::Create(scene, "Spine");
        spineBone->SetNodeAttribute(spineSkeleton);
        spineBone->LclTranslation.Set(FbxDouble3(0, 1, 0));
        rootBone->AddChild(spineBone);

        FbxSkeleton* headSkeleton = FbxSkeleton::Create(scene, "Head_Skeleton");
        headSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
        FbxNode* headBone = FbxNode::Create(scene, "Head");
        headBone->SetNodeAttribute(headSkeleton);
        headBone->LclTranslation.Set(FbxDouble3(0, 1.5, 0));
        spineBone->AddChild(headBone);

        // Create bind pose
        FbxPose* bindPose = FbxPose::Create(scene, "BindPose");
        bindPose->SetIsBindPose(true);
        bindPose->Add(rootBone, rootBone->EvaluateGlobalTransform());
        bindPose->Add(spineBone, spineBone->EvaluateGlobalTransform());
        bindPose->Add(headBone, headBone->EvaluateGlobalTransform());
    }

    void CreateBipedSkeleton(FbxScene* scene, FbxNode* parentNode) const {
        // Simple biped: Hips -> Spine -> LeftArm, RightArm, LeftLeg, RightLeg
        FbxSkeleton* hipsSkeleton = FbxSkeleton::Create(scene, "Hips_Skeleton");
        hipsSkeleton->SetSkeletonType(FbxSkeleton::eRoot);
        FbxNode* hips = FbxNode::Create(scene, "Hips");
        hips->SetNodeAttribute(hipsSkeleton);
        hips->LclTranslation.Set(FbxDouble3(0, 1, 0));
        parentNode->AddChild(hips);

        FbxSkeleton* spineSkeleton = FbxSkeleton::Create(scene, "Spine_Skeleton");
        spineSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
        FbxNode* spine = FbxNode::Create(scene, "Spine");
        spine->SetNodeAttribute(spineSkeleton);
        spine->LclTranslation.Set(FbxDouble3(0, 0.3, 0));
        hips->AddChild(spine);

        // Arms
        const char* armNames[2] = {"LeftArm", "RightArm"};
        double armOffsets[2] = {0.5, -0.5};
        for (int i = 0; i < 2; i++) {
            FbxSkeleton* armSkeleton = FbxSkeleton::Create(scene, (std::string(armNames[i]) + "_Skeleton").c_str());
            armSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
            FbxNode* arm = FbxNode::Create(scene, armNames[i]);
            arm->SetNodeAttribute(armSkeleton);
            arm->LclTranslation.Set(FbxDouble3(armOffsets[i], 0.5, 0));
            spine->AddChild(arm);
        }

        // Legs
        const char* legNames[2] = {"LeftLeg", "RightLeg"};
        double legOffsets[2] = {0.2, -0.2};
        for (int i = 0; i < 2; i++) {
            FbxSkeleton* legSkeleton = FbxSkeleton::Create(scene, (std::string(legNames[i]) + "_Skeleton").c_str());
            legSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
            FbxNode* leg = FbxNode::Create(scene, legNames[i]);
            leg->SetNodeAttribute(legSkeleton);
            leg->LclTranslation.Set(FbxDouble3(legOffsets[i], -0.3, 0));
            hips->AddChild(leg);
        }
    }

    void CreateTestScene(FbxScene* scene, FbxNode* parentNode) const {
        // Create skeleton
        CreateSimpleSkeleton(scene, parentNode);

        // Create mesh
        CreateCube(scene, parentNode);

        // Create animation
        FbxAnimStack* animStack = FbxAnimStack::Create(scene, "TestAnimation");
        FbxAnimLayer* animLayer = FbxAnimLayer::Create(scene, "BaseLayer");
        animStack->AddMember(animLayer);

        // Find root bone
        FbxNode* rootBone = nullptr;
        for (int i = 0; i < parentNode->GetChildCount(); i++) {
            FbxNode* child = parentNode->GetChild(i);
            if (std::string(child->GetName()) == "Root") {
                rootBone = child;
                break;
            }
        }

        if (rootBone) {
            // Animate root bone rotation
            FbxAnimCurve* curveY = rootBone->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
            FbxTime time;

            time.SetFrame(0);
            int keyIndex = curveY->KeyAdd(time);
            curveY->KeySetValue(keyIndex, 0.0);

            time.SetFrame(30);
            keyIndex = curveY->KeyAdd(time);
            curveY->KeySetValue(keyIndex, 90.0);

            time.SetFrame(60);
            keyIndex = curveY->KeyAdd(time);
            curveY->KeySetValue(keyIndex, 0.0);

            // Set time span
            FbxTimeSpan timeSpan;
            timeSpan.SetStart(FbxTime::SetFrame(0));
            timeSpan.SetStop(FbxTime::SetFrame(60));
            animStack->SetLocalTimeSpan(timeSpan);
        }

        // Create material
        FbxSurfacePhong* material = FbxSurfacePhong::Create(scene, "RedMaterial");
        material->Diffuse.Set(FbxDouble3(0.8, 0.2, 0.2));
        material->Specular.Set(FbxDouble3(1.0, 1.0, 1.0));
        material->Shininess.Set(20.0);

        // Apply material to mesh
        FbxNode* cubeNode = nullptr;
        for (int i = 0; i < parentNode->GetChildCount(); i++) {
            FbxNode* child = parentNode->GetChild(i);
            if (std::string(child->GetName()) == "CubeNode") {
                cubeNode = child;
                break;
            }
        }
        if (cubeNode) {
            cubeNode->AddMaterial(material);
        }
    }
};
