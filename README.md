# Vulkan Engine
The purpose of the project is to build a rendering engine on Vulkan with the main focus on streamlining development experience, 
make bringing new features to life as fast as possible while still allowing for a good portion of low level control.

### Features

#### Declarative shader inputs
Shader inputs such as Descriptors, Attachments and Vertex Attributes are declared using X Macro pattern. All necessary code is generated allowing for 
quck iterations (see [examples](./examples) for usage)
```c++
#define BLOCK_NAME Vertex
#define BLOCK \
VEC3(position, 0) \
VEC3(color, 1) \
VEC2(uv, 2)
#include <gen_vertex_data.h>

/*
expands to:
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;

    //... other required binding information
};
*/

#define BLOCK_NAME ShaderInput
#define BLOCK \
UNIFORM_BUFFER(transforms, 0, Stage::Vertex) \
IMAGE_SAMPLER(img, 1, Stage::Fragment)
#include <gen_bindings.h>

/*
expands to:
struct ShaderInput {
    BufferRegion transforms;
    ResourceRef<Image> img;
    ResourceRef<Sampler> img_sampler;

    //... other required binding information
};
*/

#define BLOCK_NAME Attachments
#define BLOCK \
COLOR(color, LoadOp::Clear) FINAL_LAYOUT(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) \
DEPTH(depth, LoadOp::Clear) 
#include <gen_attachments.h>

/*
expands to:
struct ShaderInput {
    ResourceRef<Image> color;
    ResourceRef<Image> depthl

    struct Formats {
        VkFormat color;
        VkFormat depth;
    };

    //... other required binding information
};
*/
// Application: 

void Example() {
  ShaderInput data{};
  data.transforms = uniformBuffer;
  data.img = image;
  data.img_sampler = sampler;
  
  ShaderInputInstance input = context.Get<Descriptors>().BorrowDescriptorSet(data);
  // and then
  commandBuffer.BindShaderInput({input});
}
```

#### Render graph
To streamline development experience the recommended way to use the engine is via render graph. Work can be scheduled on multiple queues and synchronized via semaphores. 
Resource states and synchronizations are managed by the graph. 
There are a couple of render nodes premade:
* graphics node
* compute node
* present node
* ImGui node

With possibility to make custom nodes. Examples:
* [render graph](./examples/render_graph_draw.cpp)
* [particles](./examples/particles.cpp)

#### Other features
* **ImGUI integration**
* **Resource managment**
* **Dynamic uniform buffers**
* **And More**
