# Direct Renderer Design

Direct renderer is designed to be most applicable to medium or some low-end devices.

# Optimization

some optimization points in use are listed here:

## Non-instancing Optimization

If the device reports no support for instancing then the following techniques will be applied:

### 1. One buffer per frame containing per-object data.

refer to [Descriptor and buffer management](https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/advanced/descriptor_management/descriptor_management_tutorial.html) for details.

One buffer per frame stores all objects' data. Each object updates its own data by offset. The same one descriptor created for this buffer is used by all objects reducing the number of total descriptor sets and descriptor set bindings.

One buffer and push constant to index in shader or bind with offset.
### 2. One deviceLocal Buffer for static meshes.

Device local buffer provides faster access performance compared to host visible buffer. For static meshes, we can load and save in device local buffer once for all. Such static mesh then doesn't need per-frame descriptor sets and never update.

### 3. One buffer per frame for dynamic meshes either host or device.

The same one buffer per frame optimization is applied here. What is different is the type of the buffer can be either host visible or device local because of different update methods. Host visible mesh buffer can be modified in CPU which is easy to use but less efficient. Device local mesh buffer can only be updated in GPU either by compute shaders or CUDA.

### 4. Using one global array of textures

refer to [Array Of Textures](http://kylehalladay.com/blog/tutorial/vulkan/2018/01/28/Textue-Arrays-Vulkan.html) for details. Specialization Constants will be used to specify the max number of textures. If the array of textures need to be bound before bindig descriptor sets, then they can be bound to one empty texture and update later.

## Instancing Optimization

If the device supports instancing then a few more optimization can be taken:

### 5. Draw multi indirect

If the device supports `draw multi indirect`

## Descriptor Indexing Optimization

If the device supports descriptor indexing  then a few more optimization can be taken:

## Compute Pipeline Optimization

If the device supports compute pipeline then a few more optimization can be taken:

## Mesh Shader Optimization

If the device supports mesh shader then a few more optimization can be taken:

## Ray Tracing Pipeline Optimization

If the device supports ray tracing pipeline then a few more optimization can be taken: