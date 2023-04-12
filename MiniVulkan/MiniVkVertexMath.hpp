#pragma once
#ifndef MINIVULKAN_MINIVKMATH
#define MINIVULKAN_MINIVKMATH
    #include "./MiniVK.hpp"

    namespace MINIVULKAN_NAMESPACE {
	    class MiniVkMath {
	    public:
            const static glm::mat4 Project2D(double width, double height, double camerax, double cameray, double znear = 1.0, double zfar = 0.0) {
                //return glm::transpose(glm::mat4(
                //    2.0/(width - 1), 0.0, 0.0, -1.0,
                //    0.0, 2.0/(height-1), 0.0, -1.0,
                //    0.0, 0.0, -2.0/(zfar-znear), -((zfar+znear)/(znear-zfar)),
                //    0.0, 0.0, 0.0, 1.0));
                // Defining the TOP and BOTTOM upside down will provide the proper translation transform for scaling
                // with Vulkan due to Vulkan's inverted Y-Axis without having to transpose the matrix.
                glm::mat4 projection = glm::ortho(0.0, width, 0.0, height, znear, zfar);
                return glm::translate(projection, glm::vec3(-camerax, -cameray, 0.0));

            }
            
            const static glm::vec2 GetUVCoords(glm::vec2 xy, glm::vec2 wh, bool forceClamp = true) {
                if (forceClamp)
                    xy = glm::clamp(xy, glm::vec2(0.0, 0.0), wh);

                return glm::vec2(xy.x * (1.0 / wh.x), xy.y * (1.0 / wh.y));
            }

            const static glm::vec2 GetXYCoords(glm::vec2 uv, glm::vec2 wh, bool forceClamp = true) {
                if (forceClamp)
                    uv = glm::clamp(uv, glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0));

                return glm::vec2(uv.x * wh.x, uv.y * wh.y);
            }
	    };

        struct MiniVkVertex {
            glm::vec2 texcoord;
            glm::vec3 position;
            glm::vec4 color;

            MiniVkVertex(glm::vec2 tex, glm::vec3 pos, glm::vec4 col) : texcoord(tex), position(pos), color(col) {}

            static MiniVkVertexDescription GetVertexDescription() {
                return MiniVkVertexDescription(GetBindingDescription(), GetAttributeDescriptions());
            }

            static VkVertexInputBindingDescription GetBindingDescription() {
                VkVertexInputBindingDescription bindingDescription {};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(MiniVkVertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return bindingDescription;
            }

            static const std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
                std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(MiniVkVertex, texcoord);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(MiniVkVertex, position);

                attributeDescriptions[2].binding = 0;
                attributeDescriptions[2].location = 2;
                attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
                attributeDescriptions[2].offset = offsetof(MiniVkVertex, color);
                return attributeDescriptions;
            }
        };

        struct MiniVkQuad {
        public:
            static const std::vector<glm::vec4> defvcolors;

            static std::vector<MiniVkVertex> CreateExt(glm::vec3 whd, const std::vector<glm::vec4> vcolors = defvcolors) {
                return {
                    MiniVkVertex({0.0,0.0}, {0.0, 0.0, whd.z}, vcolors[0]),
                    MiniVkVertex({1.0,0.0}, {whd.x, 0.0, whd.z}, vcolors[1]),
                    MiniVkVertex({1.0,1.0}, {whd.x, whd.y, whd.z}, vcolors[2]),
                    MiniVkVertex({0.0,1.0}, {0.0, whd.y, whd.z}, vcolors[3]),
                };
            }

            static std::vector<MiniVkVertex> CreateWithOffsetExt(glm::vec2 xy, glm::vec3 whd, const std::vector<glm::vec4> vcolors = defvcolors) {
                return {
                    MiniVkVertex({0.0,0.0}, {xy.x, xy.y, whd.z}, vcolors[0]),
                    MiniVkVertex({1.0,0.0}, {xy.x + whd.x, xy.y, whd.z}, vcolors[1]),
                    MiniVkVertex({1.0,1.0}, {xy.x + whd.x, xy.y + whd.y, whd.z}, vcolors[2]),
                    MiniVkVertex({0.0,1.0}, {xy.x, xy.y + whd.y, whd.z}, vcolors[3]),
                };
            }

            static std::vector<MiniVkVertex> CreateFromAtlasExt(glm::vec2 xy, glm::vec3 whd, glm::vec2 atlaswh, const std::vector<glm::vec4> vcolors = defvcolors) {
                glm::vec2 uv1 = { xy.x / whd.x, xy.y / whd.y };
                glm::vec2 uv2 = uv1 + glm::vec2(whd.x / atlaswh.x, whd.y / atlaswh.y);

                return {
                    MiniVkVertex({uv1.x, uv1.y}, {xy.x, xy.y, whd.z}, vcolors[0]),
                    MiniVkVertex({uv2.x, uv1.y}, {xy.x + whd.x, xy.y, whd.z}, vcolors[1]),
                    MiniVkVertex({uv2.x, uv2.y}, {xy.x + whd.x, xy.y + whd.y, whd.z}, vcolors[2]),
                    MiniVkVertex({uv1.x, uv2.y}, {xy.x, xy.y + whd.y, whd.z}, vcolors[3]),
                };
            }

            static std::vector<MiniVkVertex> Create(glm::vec3 whd, const glm::vec4 vcolor = defvcolors[0]) {
                return CreateExt(whd, { vcolor,vcolor,vcolor,vcolor });
            }

            static std::vector<MiniVkVertex> CreateWithOffset(glm::vec2 xy, glm::vec3 whd, const glm::vec4 vcolor = defvcolors[0]) {
                return CreateWithOffsetExt(xy, whd, { vcolor,vcolor,vcolor,vcolor });
            }

            static std::vector<MiniVkVertex> CreateFromAtlas(glm::vec2 xy, glm::vec3 whd, glm::vec2 atlaswh, const glm::vec4 vcolor = defvcolors[0]) {
                return CreateFromAtlasExt(xy, whd, atlaswh, {vcolor,vcolor,vcolor,vcolor});
            }

            static void RotateFromOrigin(std::vector<MiniVkVertex>& quad, glm::vec3 origin, glm::float32 radians) {
                glm::mat2 rotation = glm::mat2(glm::cos(radians), -glm::sin(radians), glm::sin(radians), glm::cos(radians));
                glm::vec2 pivot = origin;
                glm::vec2 position;

                for (size_t i = 0; i < quad.size(); i++) {
                    position = quad[i].position;
                    
                    position -= pivot;
                    position = rotation * position;
                    position += pivot;

                    quad[i].position = glm::vec3(position, quad[i].position.z);
                }
            }

            static void ScaleFromOrigin(std::vector<MiniVkVertex>& quad, glm::vec3 origin, glm::float32 scale) {
                glm::vec2 pivot = origin;
                glm::vec2 position;

                for (size_t i = 0; i < quad.size(); i++) {
                    position = quad[i].position;

                    position -= pivot;
                    position = scale * position;
                    position += pivot;

                    quad[i].position = glm::vec3(position, quad[i].position.z);
                }
            }

            static void RotateScaleFromOrigin(std::vector<MiniVkVertex>& quad, glm::vec3 origin, glm::float32 radians, glm::float32 scale) {
                glm::mat2 rotation = glm::mat2(glm::cos(radians), -glm::sin(radians), glm::sin(radians), glm::cos(radians));
                glm::vec2 pivot = origin;
                glm::vec2 position;

                for (size_t i = 0; i < quad.size(); i++) {
                    position = quad[i].position;

                    position -= pivot;
                    position = rotation * scale * position;
                    position += pivot;

                    quad[i].position = glm::vec3(position, quad[i].position.z);
                }
            }
        };

        const std::vector<glm::vec4> MiniVkQuad::defvcolors = {
            {1.0,1.0,1.0,1.0},{1.0,1.0,1.0,1.0},{1.0,1.0,1.0,1.0},{1.0,1.0,1.0,1.0},
        };
    }
#endif