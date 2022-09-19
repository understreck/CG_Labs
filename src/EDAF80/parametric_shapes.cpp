#include "parametric_shapes.hpp"
#include "core/Log.h"

#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

bonobo::mesh_data
parametric_shapes::createQuad(
        float const width,
        float const height,
        unsigned int const horizontal_split_count,
        unsigned int const vertical_split_count)
{
    auto const vertices = std::array<glm::vec3, 4>{
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(width, 0.0f, 0.0f),
            glm::vec3(width, height, 0.0f),
            glm::vec3(0.0f, height, 0.0f)};

    auto const index_sets = std::array<glm::uvec3, 2>{
            glm::uvec3(0u, 1u, 2u),
            glm::uvec3(0u, 2u, 3u)};

    bonobo::mesh_data data;

    if(horizontal_split_count > 0u || vertical_split_count > 0u) {
        LogError(
                "parametric_shapes::createQuad() does not support tesselation.");
        return data;
    }

    //
    // NOTE:
    //
    // Only the values preceeded by a `\todo` tag should be changed, the
    // other ones are correct!
    //

    // Create a Vertex Array Object: it will remember where we stored the
    // data on the GPU, and  which part corresponds to the vertices, which
    // one for the normals, etc.
    //
    // The following function will create new Vertex Arrays, and pass their
    // name in the given array (second argument). Since we only need one,
    // pass a pointer to `data.vao`.
    glGenVertexArrays(1, &data.vao);

    // To be able to store information, the Vertex Array has to be bound
    // first.
    glBindVertexArray(data.vao);

    // To store the data, we need to allocate buffers on the GPU. Let's
    // allocate a first one for the vertices.
    //
    // The following function's syntax is similar to `glGenVertexArray()`:
    // it will create multiple OpenGL objects, in this case buffers, and
    // return their names in an array. Have the buffer's name stored into
    // `data.bo`.
    glGenBuffers(1, &data.bo);

    // Similar to the Vertex Array, we need to bind it first before storing
    // anything in it. The data stored in it can be interpreted in
    // different ways. Here, we will say that it is just a simple 1D-array
    // and therefore bind the buffer to the corresponding target.
    glBindBuffer(GL_ARRAY_BUFFER, data.bo);

    glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(decltype(vertices)::value_type),
            /* where is the data stored on the CPU? */ vertices.data(),
            /* inform OpenGL that the data is modified once, but used often */
            GL_STATIC_DRAW);

    // Vertices have been just stored into a buffer, but we still need to
    // tell Vertex Array where to find them, and how to interpret the data
    // within that buffer.
    //
    // You will see shaders in more detail in lab 3, but for now they are
    // just pieces of code running on the GPU and responsible for moving
    // all the vertices to clip space, and assigning a colour to each pixel
    // covered by geometry.
    // Those shaders have inputs, some of them are the data we just stored
    // in a buffer object. We need to tell the Vertex Array which inputs
    // are enabled, and this is done by the following line of code, which
    // enables the input for vertices:
    glEnableVertexAttribArray(
            static_cast<unsigned int>(bonobo::shader_bindings::vertices));

    // Once an input is enabled, we need to explain where the data comes
    // from, and how it interpret it. When calling the following function,
    // the Vertex Array will automatically use the current buffer bound to
    // GL_ARRAY_BUFFER as its source for the data. How to interpret it is
    // specified below:
    glVertexAttribPointer(
            static_cast<unsigned int>(bonobo::shader_bindings::vertices),
            decltype(vertices)::value_type::length(),
            /* what is the type of each component? */ GL_FLOAT,
            /* should it automatically normalise the values stored */ GL_FALSE,
            /* once all components of a vertex have been read, how far away (in
               bytes) is the next vertex? */
            0,
            /* how far away (in bytes) from the start of the buffer is the first
               vertex? */
            reinterpret_cast<GLvoid const*>(0x0));

    // Now, let's allocate a second one for the indices.
    //
    // Have the buffer's name stored into `data.ibo`.
    glGenBuffers(1, &data.ibo);

    // We still want a 1D-array, but this time it should be a 1D-array of
    // elements, aka. indices!
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);

    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            index_sets.size() * sizeof(decltype(index_sets)::value_type),
            /* where is the data stored on the CPU? */ index_sets.data(),
            /* inform OpenGL that the data is modified once, but used often */
            GL_STATIC_DRAW);

    data.indices_nb =
            index_sets.size() * decltype(index_sets)::value_type::length();

    // All the data has been recorded, we can unbind them.
    glBindVertexArray(0u);
    glBindBuffer(GL_ARRAY_BUFFER, 0u);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

    return data;
}

auto
print(glm::vec3 v)
{
    std::cerr << v.x << " : " << v.y << " : " << v.z << "\n";
}

auto
generate_edges(float radius, float theta, float phi)
{
    auto const sinTheta = std::sin(theta);
    auto const cosTheta = std::cos(theta);

    auto const sinPhi = std::sin(phi);
    auto const cosPhi = std::cos(phi);

    struct Edges {
        glm::vec3 position;
        glm::vec3 tangent;
        glm::vec3 binormal;
    };
    return Edges{
            .position =
                    {radius * sinTheta * sinPhi,
                     -radius * cosPhi,
                     radius * cosTheta * sinPhi},
            .tangent =
                    {radius * cosTheta * sinPhi,
                     0,
                     -radius * sinTheta * sinPhi},
            .binormal = {
                    radius * sinTheta * cosPhi,
                    radius * sinPhi,
                    radius * cosTheta * cosPhi}};
}

bonobo::mesh_data
parametric_shapes::createSphere(
        float const radius,
        unsigned int const longitude_split_count,
        unsigned int const latitude_split_count)
{
    // auto const triangleCount = longitude_split_count * 2;
    auto const edgesPerPole       = longitude_split_count;
    auto const longitudeEdgeCount = longitude_split_count + 1;
    auto const middleEdgeCount    = longitudeEdgeCount * latitude_split_count;
    auto const edgeCount          = middleEdgeCount + edgesPerPole * 2;

    auto vertices  = std::vector<glm::vec3>{edgeCount};
    auto tangents  = std::vector<glm::vec3>{edgeCount};
    auto binormals = std::vector<glm::vec3>{edgeCount};
    auto normals   = std::vector<glm::vec3>{edgeCount};
    auto texCoords = std::vector<glm::vec3>{edgeCount};

    // Generate pole vertices
    for(auto round = 0; round <= 1; ++round) {
        auto const edgeOffset = round == 0 ? 0 : edgesPerPole;

        auto const phi = round == 0 ? 0.f : glm::pi<float>();

        auto const xTextStep  = 1.f / edgesPerPole;
        auto const xTexOffset = xTextStep / 2.f;

        auto const sinPhi = std::sin(phi);
        auto const cosPhi = std::cos(phi);

        for(auto edge = 0; edge < edgesPerPole; ++edge) {
            auto const thetaStep   = glm::two_pi<float>() / edgesPerPole;
            auto const thetaOffset = thetaStep / 2.f;
            auto const theta       = thetaOffset + thetaStep * edge;
            auto const sinTheta    = std::sin(theta);
            auto const cosTheta    = std::cos(theta);

            vertices[edgeOffset + edge] = {
                    radius * sinTheta * sinPhi,
                    -radius * cosPhi,
                    radius * cosTheta * sinPhi};
            tangents[edgeOffset + edge] = {
                    radius * cosTheta,
                    0,
                    -radius * sinTheta};
            // {0,0,0}
            binormals[edgeOffset + edge] =
                    glm::vec3{radius * sinTheta, 0, radius * cosTheta}
                    * (1.f - 2.0f * round);
            normals[edgeOffset + edge] = glm::cross(
                    tangents[edgeOffset + edge],
                    binormals[edgeOffset + edge]);

            texCoords[edgeOffset + edge] = {
                    xTexOffset + xTextStep * edge,
                    static_cast<float>(round),
                    0.f};
            // print(vertices[edgeOffset + edge]);
        }
    }

    // Generate middle vertices
    for(auto lat = 0; lat < latitude_split_count; ++lat) {
        auto const phiStep   = glm::pi<float>() / (latitude_split_count + 1.f);
        auto const phiOffset = phiStep;
        auto const phi       = phiOffset + phiStep * lat;

        auto const xTextStep  = 1.f / longitudeEdgeCount;
        auto const xTexOffset = xTextStep / 2.f;
        auto const yTexStep   = 1.f / latitude_split_count;

        auto const edgeOffset = 2 * edgesPerPole + longitudeEdgeCount * lat;
        for(auto lon = 0; lon < longitudeEdgeCount; ++lon) {
            auto const thetaStep = glm::two_pi<float>() / longitude_split_count;
            auto const theta     = thetaStep * lon;

            auto&& [position, tangent, binormal] =
                    generate_edges(radius, theta, phi);

            vertices[edgeOffset + lon]  = position;
            tangents[edgeOffset + lon]  = tangent;
            binormals[edgeOffset + lon] = binormal;
            normals[edgeOffset + lon]   = glm::cross(tangent, binormal);

            texCoords[edgeOffset + lon] = {
                    xTexOffset + xTextStep * lon,
                    yTexStep * lat,
                    0.f};
            // std::cerr << "Edge: " << edge << " : ";
            // print(vertices[edge]);
        }
    }

    auto const trianglesPerPole = edgesPerPole;
    auto const quadsPerRow      = longitude_split_count;
    auto const quadRows         = latitude_split_count - 1;
    auto const middleQuadCount  = quadsPerRow * quadRows;
    auto const triangleCount    = middleQuadCount * 2 + trianglesPerPole * 2;

    auto triangles = std::vector<glm::uvec3>{triangleCount};

    std::cerr << triangles.size() << "\n\n\n";

    // Generate pole triangles
    for(auto round = 0; round <= 1; ++round) {
        auto const topOffset      = edgesPerPole * round;
        auto const triangleOffset = topOffset;
        auto const bottomOffset =
                round == 0 ? edgesPerPole * 2 : edgeCount - longitudeEdgeCount;

        for(auto triangle = 0; triangle < trianglesPerPole; ++triangle) {
            auto const topVertice = topOffset + triangle;

            auto const bottomLeftVertice  = bottomOffset + triangle;
            auto const bottomRightVertice = bottomOffset + triangle + 1;

            triangles[triangleOffset + triangle] = {
                    topVertice,
                    round == 0 ? bottomRightVertice : bottomLeftVertice,
                    round == 0 ? bottomLeftVertice : bottomRightVertice};

            // print(triangles[triangleOffset + triangle]);
        }
    }
    for(auto row = 0; row < quadRows; ++row) {
        auto const triangleOffset =
                trianglesPerPole * 2 + quadsPerRow * row * 2;

        auto const topOffset = trianglesPerPole * 2 + (quadsPerRow + 1) * row;
        auto const bottomOffset = topOffset + longitudeEdgeCount;

        for(auto quad = 0; quad < quadsPerRow; ++quad) {
            auto const topLeft     = topOffset + quad;
            auto const topRight    = topOffset + quad + 1;
            auto const bottomLeft  = bottomOffset + quad;
            auto const bottomRight = bottomOffset + quad + 1;

            // Both triangles make up a quad
            //  Top left triangle
            triangles[triangleOffset + quad * 2] = {
                    topLeft,
                    topRight,
                    bottomLeft};

            // Bottom tight triangle
            triangles[triangleOffset + quad * 2 + 1] = {
                    topRight,
                    bottomRight,
                    bottomLeft};

            // print(triangles[triangleOffset + quad * 2]);
            // print(triangles[triangleOffset + quad * 2 + 1]);
        }
    }

    auto meshData = bonobo::mesh_data();

    glGenVertexArrays(1, &meshData.vao);
    assert(meshData.vao != 0u);
    glBindVertexArray(meshData.vao);

    auto const subBufferSize = sizeof(glm::vec3) * edgeCount;
    auto const bufferSize    = subBufferSize * 5;

    auto const verticesOffset  = 0u;
    auto const tangentsOffset  = verticesOffset + subBufferSize;
    auto const binormalsOffset = tangentsOffset + subBufferSize;
    auto const normalsOffset   = binormalsOffset + subBufferSize;
    auto const texCoordsOffset = normalsOffset + subBufferSize;

    glGenBuffers(1, &meshData.bo);
    assert(meshData.bo != 0);
    glBindBuffer(GL_ARRAY_BUFFER, meshData.bo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_STATIC_DRAW);

    glBufferSubData(
            GL_ARRAY_BUFFER,
            verticesOffset,
            subBufferSize,
            vertices.data());
    glEnableVertexAttribArray(
            static_cast<GLuint>(bonobo::shader_bindings::vertices));
    glVertexAttribPointer(
            static_cast<GLuint>(bonobo::shader_bindings::vertices),
            glm::vec3::length(),
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(verticesOffset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            binormalsOffset,
            subBufferSize,
            binormals.data());
    glEnableVertexAttribArray(
            static_cast<GLuint>(bonobo::shader_bindings::binormals));
    glVertexAttribPointer(
            static_cast<GLuint>(bonobo::shader_bindings::binormals),
            glm::vec3::length(),
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(binormalsOffset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            tangentsOffset,
            subBufferSize,
            tangents.data());
    glEnableVertexAttribArray(
            static_cast<GLuint>(bonobo::shader_bindings::tangents));
    glVertexAttribPointer(
            static_cast<GLuint>(bonobo::shader_bindings::tangents),
            glm::vec3::length(),
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(tangentsOffset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            normalsOffset,
            subBufferSize,
            normals.data());
    glEnableVertexAttribArray(
            static_cast<GLuint>(bonobo::shader_bindings::normals));
    glVertexAttribPointer(
            static_cast<GLuint>(bonobo::shader_bindings::normals),
            glm::vec3::length(),
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(normalsOffset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            texCoordsOffset,
            subBufferSize,
            texCoords.data());
    glEnableVertexAttribArray(
            static_cast<GLuint>(bonobo::shader_bindings::texcoords));
    glVertexAttribPointer(
            static_cast<GLuint>(bonobo::shader_bindings::texcoords),
            glm::vec3::length(),
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(texCoordsOffset));

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    meshData.indices_nb = triangleCount * 3;    // 3 indices per triangle
    glGenBuffers(1, &meshData.ibo);
    assert(meshData.ibo != 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.ibo);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            triangles.size() * sizeof(glm::uvec3),
            triangles.data(),
            GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return meshData;
    // return {};
}

bonobo::mesh_data
parametric_shapes::createTorus(
        float const major_radius,
        float const minor_radius,
        unsigned int const major_split_count,
        unsigned int const minor_split_count)
{
    //! \todo (Optional) Implement this function
    return bonobo::mesh_data();
}

bonobo::mesh_data
parametric_shapes::createCircleRing(
        float const radius,
        float const spread_length,
        unsigned int const circle_split_count,
        unsigned int const spread_split_count)
{
    auto const circle_slice_edges_count    = circle_split_count + 1u;
    auto const spread_slice_edges_count    = spread_split_count + 1u;
    auto const circle_slice_vertices_count = circle_slice_edges_count + 1u;
    auto const spread_slice_vertices_count = spread_slice_edges_count + 1u;
    auto const vertices_nb =
            circle_slice_vertices_count * spread_slice_vertices_count;

    auto vertices  = std::vector<glm::vec3>(vertices_nb);
    auto normals   = std::vector<glm::vec3>(vertices_nb);
    auto texcoords = std::vector<glm::vec3>(vertices_nb);
    auto tangents  = std::vector<glm::vec3>(vertices_nb);
    auto binormals = std::vector<glm::vec3>(vertices_nb);

    float const spread_start = radius - 0.5f * spread_length;
    float const d_theta      = glm::two_pi<float>()
                          / (static_cast<float>(circle_slice_edges_count));
    float const d_spread =
            spread_length / (static_cast<float>(spread_slice_edges_count));

    // generate vertices iteratively
    size_t index = 0u;
    float theta  = 0.0f;
    for(unsigned int i = 0u; i < circle_slice_vertices_count; ++i) {
        float const cos_theta = std::cos(theta);
        float const sin_theta = std::sin(theta);

        float distance_to_centre = spread_start;
        for(unsigned int j = 0u; j < spread_slice_vertices_count; ++j) {
            // vertex
            vertices[index] = glm::vec3(
                    distance_to_centre * cos_theta,
                    distance_to_centre * sin_theta,
                    0.0f);

            // texture coordinates
            texcoords[index] = glm::vec3(
                    static_cast<float>(j)
                            / (static_cast<float>(spread_slice_vertices_count)),
                    static_cast<float>(i)
                            / (static_cast<float>(circle_slice_vertices_count)),
                    0.0f);

            // tangent
            auto const t    = glm::vec3(cos_theta, sin_theta, 0.0f);
            tangents[index] = t;

            // binormal
            auto const b     = glm::vec3(-sin_theta, cos_theta, 0.0f);
            binormals[index] = b;

            // normal
            auto const n   = glm::cross(t, b);
            normals[index] = n;

            distance_to_centre += d_spread;
            ++index;
        }

        theta += d_theta;
    }

    // create index array
    auto index_sets = std::vector<glm::uvec3>(
            2u * circle_slice_edges_count * spread_slice_edges_count);

    // generate indices iteratively
    index = 0u;
    for(unsigned int i = 0u; i < circle_slice_edges_count; ++i) {
        for(unsigned int j = 0u; j < spread_slice_edges_count; ++j) {
            index_sets[index] = glm::uvec3(
                    spread_slice_vertices_count * (i + 0u) + (j + 0u),
                    spread_slice_vertices_count * (i + 0u) + (j + 1u),
                    spread_slice_vertices_count * (i + 1u) + (j + 1u));
            ++index;

            index_sets[index] = glm::uvec3(
                    spread_slice_vertices_count * (i + 0u) + (j + 0u),
                    spread_slice_vertices_count * (i + 1u) + (j + 1u),
                    spread_slice_vertices_count * (i + 1u) + (j + 0u));
            ++index;
        }
    }

    bonobo::mesh_data data;
    glGenVertexArrays(1, &data.vao);
    assert(data.vao != 0u);
    glBindVertexArray(data.vao);

    auto const vertices_offset = 0u;
    auto const vertices_size =
            static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
    auto const normals_offset = vertices_size;
    auto const normals_size =
            static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
    auto const texcoords_offset = normals_offset + normals_size;
    auto const texcoords_size =
            static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
    auto const tangents_offset = texcoords_offset + texcoords_size;
    auto const tangents_size =
            static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
    auto const binormals_offset = tangents_offset + tangents_size;
    auto const binormals_size =
            static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
    auto const bo_size = static_cast<GLsizeiptr>(
            vertices_size + normals_size + texcoords_size + tangents_size
            + binormals_size);
    glGenBuffers(1, &data.bo);
    assert(data.bo != 0u);
    glBindBuffer(GL_ARRAY_BUFFER, data.bo);
    glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

    glBufferSubData(
            GL_ARRAY_BUFFER,
            vertices_offset,
            vertices_size,
            static_cast<GLvoid const*>(vertices.data()));
    glEnableVertexAttribArray(
            static_cast<unsigned int>(bonobo::shader_bindings::vertices));
    glVertexAttribPointer(
            static_cast<unsigned int>(bonobo::shader_bindings::vertices),
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(0x0));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            normals_offset,
            normals_size,
            static_cast<GLvoid const*>(normals.data()));
    glEnableVertexAttribArray(
            static_cast<unsigned int>(bonobo::shader_bindings::normals));
    glVertexAttribPointer(
            static_cast<unsigned int>(bonobo::shader_bindings::normals),
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(normals_offset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            texcoords_offset,
            texcoords_size,
            static_cast<GLvoid const*>(texcoords.data()));
    glEnableVertexAttribArray(
            static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
    glVertexAttribPointer(
            static_cast<unsigned int>(bonobo::shader_bindings::texcoords),
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(texcoords_offset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            tangents_offset,
            tangents_size,
            static_cast<GLvoid const*>(tangents.data()));
    glEnableVertexAttribArray(
            static_cast<unsigned int>(bonobo::shader_bindings::tangents));
    glVertexAttribPointer(
            static_cast<unsigned int>(bonobo::shader_bindings::tangents),
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(tangents_offset));

    glBufferSubData(
            GL_ARRAY_BUFFER,
            binormals_offset,
            binormals_size,
            static_cast<GLvoid const*>(binormals.data()));
    glEnableVertexAttribArray(
            static_cast<unsigned int>(bonobo::shader_bindings::binormals));
    glVertexAttribPointer(
            static_cast<unsigned int>(bonobo::shader_bindings::binormals),
            3,
            GL_FLOAT,
            GL_FALSE,
            0,
            reinterpret_cast<GLvoid const*>(binormals_offset));

    glBindBuffer(GL_ARRAY_BUFFER, 0u);

    data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
    glGenBuffers(1, &data.ibo);
    assert(data.ibo != 0u);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)),
            reinterpret_cast<GLvoid const*>(index_sets.data()),
            GL_STATIC_DRAW);

    glBindVertexArray(0u);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

    return data;
}
