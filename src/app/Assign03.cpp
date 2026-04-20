#include <iostream>
#include <string>
#include "pro/Prometheus.hpp"
#include <cmath>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// STRUCTS
///////////////////////////////////////////////////////////////////////////////

struct ProVertex {
    glm::vec3 pos;
    glm::vec4 color;
    glm::vec3 normal;
};

struct UPushVertex {
    alignas(16) glm::mat4 modelMat;
    alignas(16) float useNormalAsColor;
};

///////////////////////////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////////////////////////////

bool didWindowResize = false;

float rotAngleZ = 0.0f;
float useNormalAsColor = 1.0f;
float zOffset = 0.5f; 
bool flipWindingOrder = true;

///////////////////////////////////////////////////////////////////////////////
// GLFW CALLBACKS
///////////////////////////////////////////////////////////////////////////////

// Note: static prevents name conflicts in other cpp files

// When the window resizes/minimizes...
static void window_resize_callback(GLFWwindow* window, int width, int height) {
    didWindowResize = true;    
}

// When key events occur...
static void key_callback(   GLFWwindow *window,
                            int key,
                            int scancode,
                            int action,
                            int mods) {

    if(action == GLFW_PRESS || action == GLFW_REPEAT) {
        if(key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
        else if (key == GLFW_KEY_J) {
            rotAngleZ += 1.0;
        }
        else if (key == GLFW_KEY_K) {
            rotAngleZ -= 1.0;
        }
        else if (key == GLFW_KEY_O) {
            useNormalAsColor = glm::clamp(useNormalAsColor + 0.1f, 0.0f, 1.0f);
        }
        else if (key == GLFW_KEY_I) {
            useNormalAsColor = glm::clamp(useNormalAsColor - 0.1f, 0.0f, 1.0f);
        }
        else if (key == GLFW_KEY_U) {
            zOffset += 0.05;
        }
        else if (key == GLFW_KEY_Y) {
            zOffset -= 0.05;
        }
    }
}

// When the mouse moves...
static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
    glm::vec2 curMouse = glm::vec2(xpos, ypos);
    //cout << "Mouse pos: " << glm::to_string(curMouse) << endl;
}

// When a mouse button is pressed/released/clicked...
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cout << "Left mouse press." << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////
// PER-FRAME "DRAWING" FUNCTION
///////////////////////////////////////////////////////////////////////////////

void recordFrame(   pro::VulkanInitData &vkInitData, 
                    pro::FrameCommandData &cd,
                    const pro::VulkanSwapImage &swapImage,
                    const pro::VulkanImage &depthImage,
                    pro::VulkanPipelineData &pipelineData,
                    vector<pro::VulkanMesh> allMeshes,
                    const aiScene *scene,
                    vk::ClearColorValue &backgroundColor) {

    // Reset our command pool so it's cleared and ready to go
    vkInitData.device().resetCommandPool(cd.commandPool);
     
    // Begin recording
    cd.commandBuffer.begin(vk::CommandBufferBeginInfo());

    // Transition swap image from undefined to color buffer
    performVulkanImageTransition(cd.commandBuffer, swapImage.image, pro::IMAGE_TRANSITION_TYPE::UNDEF_TO_COLOR);
    
    // Define behavior for the color attachment (including clear color)
    vk::RenderingAttachmentInfoKHR colorAtt = pro::createColorAttachment(
        swapImage.view, 
        backgroundColor);

    // Define behavior for the depth attachment
    vk::RenderingAttachmentInfoKHR depthAtt = pro::createDepthAttachment(depthImage.view);
        
    // Set rendering info and begin (dynamic) rendering
    vk::RenderingInfoKHR ri{};
    ri.setRenderArea(vk::Rect2D{ {0,0}, vkInitData.swapchain().extent })
        .setLayerCount(1)
        .setColorAttachments(colorAtt)
        .setPDepthAttachment(&depthAtt);

    cd.commandBuffer.beginRendering(ri);
    
    // Bind pipeline
    cd.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineData.pipeline);
    
    // Set up viewport and scissors
    vk::Viewport viewports[] = { pro::makeDefaultViewport(vkInitData) };    
    cd.commandBuffer.setViewport(0, viewports);
    
    vk::Rect2D scissors[] = { pro::makeDefaultScissors(vkInitData) };
    cd.commandBuffer.setScissor(0, scissors);

    if (scene && scene->mRootNode) {
        renderSceneNode(
            cd.commandBuffer,
            pipelineData,
            allMeshes,
            scene->mRootNode,
            glm::mat4(1.0f),
            0);
    }
    
    // End rendering
    cd.commandBuffer.endRendering();
   
    // Transition swap image from color buffer to presentation
    pro::performVulkanImageTransition(cd.commandBuffer, swapImage.image, pro::IMAGE_TRANSITION_TYPE::COLOR_TO_PRESENT);
    

    // End recording
    cd.commandBuffer.end();
}   


pro::HostMesh<ProVertex> createShape(int maxSub = 5, bool flipWinding = false) {
    pro::HostMesh<ProVertex> triangleFan {};
    const float PI = 3.14159265f;
    const float RADIUS = 0.5f;
    triangleFan.vertices.push_back(
        {{0.0f, 0.0f, 0.5f}, {0.5f, 0.5f, 0.5f, 1.0f}} // center vertex
    );

    for(unsigned int i = 0; i <= maxSub; i++) {
        float angle_i = 2 * PI * i / maxSub; // angle of the current outer vertex around the circle
        float x = RADIUS * cos(angle_i);
        float y = RADIUS * sin(angle_i);
        float r = (cos(angle_i)+1)/2; // range: [0,1]
        float g = (sin(angle_i)+1)/2;
        float b = (cos(angle_i + PI)+1)/2;
        triangleFan.vertices.push_back({
            {x, y, 0.5f}, // position
            {r, g, b, 1.0f} // color
        });
        
    };
    if (flipWinding) { // clockwise
        for(unsigned int i = 0; i <= maxSub; i++) {
        triangleFan.indices.push_back(0); // center vertex
        triangleFan.indices.push_back(i + 2);
        triangleFan.indices.push_back(i + 1);
        };
    }
    else { // counter clockwise
        for(unsigned int i = 0; i <= maxSub; i++) {
        triangleFan.indices.push_back(0);
        triangleFan.indices.push_back(i + 1);
        triangleFan.indices.push_back(i + 2);
        };
    }
    
    return triangleFan;
};

glm::mat4 assimpToGLM(aiMatrix4x4 &a) {
    glm::mat4 m(1.0f); 
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            m[k][i] = a[i][k]; // convert from row-major to column-major
        }
    }
    return m;
}

glm::mat4 makeRotateZ(glm::vec3 offset) {
    glm::mat4 negOffset = glm::translate(glm::mat4(1.0f), -offset); // negative offset translation
    glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotAngleZ), glm::vec3(0, 0, 1));
    glm::mat4 translateOffset = glm::translate(glm::mat4(1.0f), offset);
    glm::mat4 compositeTransform = translateOffset * rotateZ * negOffset; // right to left
    return compositeTransform;
}

void renderSceneNode(vk::CommandBuffer &commandBuffer, pro::VulkanPipelineData 
&pipelineData, vector<pro::VulkanMesh> &allMeshes, aiNode *node, glm::mat4 parentMat, 
int level) {
    glm::mat4 nodeT = assimpToGLM(node->mTransformation);
    glm::mat4 modelMat = parentMat * nodeT;
    glm::vec3 pos = glm::vec3(modelMat[3]); // [3] is last column
    glm::mat4 R = makeRotateZ(pos);
    R = glm::translate(glm::vec3(0, 0, zOffset)) * R;
    glm::mat4 tmpModel = R * modelMat;
    UPushVertex upv = {};
    upv.modelMat = tmpModel;
    upv.useNormalAsColor = useNormalAsColor;

    commandBuffer.pushConstants(
                pipelineData.layout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(UPushVertex),
                &upv
    );

    for (int i = 0; i < node->mNumMeshes; i++) {
        int index = node->mMeshes[i];
        pro::recordDrawVulkanMesh(commandBuffer, allMeshes.at(index));
    }
    for (int i = 0; i < node->mNumChildren; i++) {
        renderSceneNode(commandBuffer, pipelineData, allMeshes, node->mChildren[i], modelMat, level + 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    cout << "BEGIN PROGRAM..." << endl;

    // Set app name
    string appName = "Assign03";
    string windowName = appName + ": dematta";

    ///////////////////////////////////////////////////////////////////////
    // GLFW
    ///////////////////////////////////////////////////////////////////////

    // Initialize GLFW
    if(!glfwInit()) {
        cerr << "ERROR: Cannot start GLFW!" << endl;
        exit(1);
    }
    
    // Create GLFW window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);
    GLFWwindow *window = glfwCreateWindow(800, 600, windowName.c_str(), nullptr, nullptr);

    // Was window successfully created?
    if(!window) {
        cerr << "ERROR: Cannot create GLFW window!" << endl;
        glfwTerminate();
        exit(1);
    }

    // Define GLFW callback functions
    glfwSetFramebufferSizeCallback(window, window_resize_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    // Create scope for Vulkan Init Data (to ensure proper cleanup)
    {
        ///////////////////////////////////////////////////////////////////////
        // VULKAN INIT DATA
        ///////////////////////////////////////////////////////////////////////

        // Creation information for basic Vulkan components
        pro::VulkanInitCreateInfo createInfo {};
        createInfo.appName = appName;
        // If you encounter errors with instance creation, try requesting Vulkan 1.3:
        createInfo.requestedAppVulkanVersionMinor = 3;
        
        // If you encounter errors with compute and/or transfer queue creation, try these:
        createInfo.requireComputeQueue = false;
        createInfo.requireTransferQueue = false;

        createInfo.createSurfaceFunc = [window](VkInstance instance, VkSurfaceKHR& surface) {            
            return glfwCreateWindowSurface(instance, window, nullptr, &surface);
        };

        createInfo.getCurrentWindowSizeFunc = [window](int &width, int &height) {
            glfwGetFramebufferSize(window, &width, &height);
        };
    
        // Create the basic Vulkan components
        pro::VulkanInitData vkInitData(createInfo);
        
        cout << "** Chosen Physical Device: *********" << endl;        
        pro::printPhysicalDeviceProperties(vkInitData.physicalDevice());
        vkInitData.printQueues();

        // Create depth image(s)
        vector<pro::VulkanImage> allDepthImages {};
        int numberOfFramesInFlight = 1;
        pro::recreateAllVulkanDepthImages(vkInitData, allDepthImages, numberOfFramesInFlight);
        
        // Define resize function
        pro::OnResizeFunc resizeFunc = [&vkInitData, window, &allDepthImages, numberOfFramesInFlight]() {            
            int width = 0;
            int height = 0;

            do {
                glfwGetFramebufferSize(window, &width, &height);
                // If minimized, this will be 0,0. Block until restored.
                glfwWaitEvents(); // Actually waits/sleeps/blocks until an event happens
            } while (width == 0 || height == 0);
        
            // Safe to recreate
            vkInitData.recreateVulkanSwapchain();
            recreateAllVulkanDepthImages(vkInitData, allDepthImages, numberOfFramesInFlight);

            cout << "Swapchain recreated..." << endl;
        };

        ///////////////////////////////////////////////////////////////////////
        // VULKAN COMMAND DATA
        ///////////////////////////////////////////////////////////////////////

        // Create command data
        pro::FrameCommandData commandData = pro::createFrameCommandData(vkInitData);

        ///////////////////////////////////////////////////////////////////////
        // VULKAN GRAPHICS PIPELINE
        ///////////////////////////////////////////////////////////////////////

        // Set up creation info for pipeline
        pro::VulkanPipelineCreateInfo pipelineCreateInfo(vkInitData);

        // Create shader info
        pipelineCreateInfo.shaderInfo = {
            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.vert.spv",
                vk::ShaderStageFlagBits::eVertex
            ),

            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.frag.spv",
                vk::ShaderStageFlagBits::eFragment
            )
        };

        // Set up vertex information
        pipelineCreateInfo.bindDesc = vk::VertexInputBindingDescription(
            0, sizeof(ProVertex), vk::VertexInputRate::eVertex);

        // POSITION
        pipelineCreateInfo.attribDesc.push_back(vk::VertexInputAttributeDescription(
            0, // location
            0, // binding
            vk::Format::eR32G32B32Sfloat,  // format
            offsetof(ProVertex, pos) // offset
        ));
        
        // COLOR
        pipelineCreateInfo.attribDesc.push_back(vk::VertexInputAttributeDescription(
            1, // location
            0, // binding
            vk::Format::eR32G32B32A32Sfloat,  // format
            offsetof(ProVertex, color) // offset
        ));

        // Actually create the pipeline data
        pro::VulkanPipelineData pipelineData = createVulkanPipeline(vkInitData, pipelineCreateInfo);

        ///////////////////////////////////////////////////////////////////////
        // MESH CREATION
        ///////////////////////////////////////////////////////////////////////

        // Create host data  
        vector<pro::HostMesh<ProVertex>> allHostMeshes {};
        
        allHostMeshes.push_back(createShape(300, false));
        
        // pro::HostMesh<ProVertex> createShape(int maxSub = 5, bool flipWinding = false); 
        
        // pro::HostMesh<ProVertex> simpleQuad {};
        // simpleQuad.vertices = {
        //     {{-0.5f, -0.5f, 0.5f},  {1,0,0,1}},
        //     {{0.5f, -0.5f, 0.5f},   {0,1,0,1}},
        //     {{0.5f, 0.5f, 0.5f},    {0,0,1,1}},
        //     {{-0.5f, 0.5f, 0.5f},   {1,1,1,1}}
        // };
        // simpleQuad.indices = { 0, 1, 2, 0, 2, 3 };        
        // allHostMeshes.push_back(simpleQuad);

        // Create the Vulkan meshes
        vector<pro::VulkanMesh> allMeshes {};    
        allMeshes.resize(allHostMeshes.size());       
        for(unsigned int i = 0; i < allMeshes.size(); i++) {
            allMeshes[i] = pro::createVulkanMesh(vkInitData, allHostMeshes[i], false);
            pro::copyToHostVisibleVulkanMesh(vkInitData, allMeshes[i], allHostMeshes[i]);            
        }

        // Background color data
        vk::ClearColorValue backgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};
        const glm::vec3 first_color(0.12f, 0.25f, 0.60f);
        const glm::vec3 second_color(0.90f, 0.50f, 0.20f);
        const float switchSeconds = 3.0f;
        bool useFirst = true; // use first_color
        auto lastSwitchTime = pro::getTime();



        ///////////////////////////////////////////////////////////////////////
        // MAIN RENDER LOOP
        ///////////////////////////////////////////////////////////////////////
        
        // While the window is still open...
        while (!glfwWindowShouldClose(window)) { 
            // Check for window/keyboard/mouse events...	
            glfwPollEvents();	

            // Did the window resize?
            if(didWindowResize) {
                didWindowResize = false;
                resizeFunc();
            }

            // Set frame-in-flight index (only one for now)
            unsigned int indexFlight = 0;

            // Acquire swap image
            unsigned int indexSwap = pro::acquireNextSwapImage(vkInitData, commandData, resizeFunc);

            // Record a frame
            recordFrame(
                vkInitData, 
                commandData, 
                vkInitData.swapchain().swaps[indexSwap], 
                allDepthImages[indexFlight],
                pipelineData,
                allMeshes,
                // scene,
                backgroundColor);
                    
            // Submit to queue
            pro::submitToGraphicsQueue(vkInitData, commandData, indexSwap, resizeFunc);

            // Present
            if(!pro::presentSwapImage(vkInitData, commandData, indexSwap, resizeFunc)) {
                cout << "Warning: Presentation was not successful." << endl;
            };

            // Background color change logic
            auto now = pro::getTime();
            float time_elapsed = pro::getElapsedSeconds(lastSwitchTime, now);
            if (time_elapsed >= switchSeconds) {
                useFirst = !useFirst; // use second_color
                lastSwitchTime = now;
            }
            glm::vec3 color = useFirst ? first_color: second_color; 
            backgroundColor = {color.r, color.g, color.b, 1.0f};
            
    

        }
        
        ///////////////////////////////////////////////////////////////////////
        // CLEANUP
        ///////////////////////////////////////////////////////////////////////

        // Wait until device is completely idle
        vkInitData.device().waitIdle();

        // Cleanup assets
        for(auto &mesh : allMeshes) {
            pro::cleanupVulkanMesh(vkInitData, mesh);
        }
        allMeshes.clear();
        
        // Cleanup Vulkan-related stuff
        cleanupVulkanPipeline(vkInitData, pipelineData);
        cleanupFrameCommandData(vkInitData, commandData);
        cleanupAllVulkanDepthImages(vkInitData, allDepthImages);

        // VulkanInitData will be cleaned up automatically when it falls out of scope.
    }
    
    ///////////////////////////////////////////////////////////////////////
    // GLFW CLEANUP
    ///////////////////////////////////////////////////////////////////////

    glfwDestroyWindow(window);
    glfwTerminate();   

    // End program successfully
    return 0;
}
