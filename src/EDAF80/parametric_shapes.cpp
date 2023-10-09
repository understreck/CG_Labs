#include "parametric_shapes.hpp"
#include "core/Log.h"
#include "core/helpers.hpp"

#include <cstdio>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <vector>

bonobo::mesh_data
parametric_shapes::createQuad(float const width, float const height,
                              unsigned int const horizontal_split_count,
                              unsigned int const vertical_split_count) {
  auto const rowCount = horizontal_split_count + 2;
  auto const columnCount = vertical_split_count + 2;

  auto vertices = std::vector<glm::vec3>{};
  vertices.reserve(rowCount * columnCount);

  for (auto row = 0u; row < rowCount; row++) {
    auto const columnStep = width / (columnCount - 1);
    auto const rowStep = height / (rowCount - 1);

    for (auto column = 0u; column < columnCount; column++) {
      vertices.push_back({columnStep * column, 0.0f, rowStep * row});
    }
  }

  auto const rowQuadCount = rowCount - 1;
  auto const columnQuadCount = columnCount - 1;
  auto const triangleCount = rowQuadCount * columnQuadCount * 2;

  auto indexSets = std::vector<glm::uvec3>{};
  indexSets.reserve(triangleCount);

  for (auto row = 0u; row < rowQuadCount; row++) {
    auto const topOffset = columnCount * row;
    auto const bottomOffset = topOffset + columnCount;

    for (auto column = 0u; column < columnQuadCount; column++) {
      auto const topLeft = topOffset + column;
      auto const topRight = topLeft + 1;
      auto const bottomLeft = bottomOffset + column;
      auto const bottomRight = bottomLeft + 1;

      /* printf("Top left: %d:{%f, %f}, Top right: %d:{%f, %f}\nBottom left: "
             "%d:{%f, %f}, Bottom "
             "right: %d:{%f, %f}\n\n",
             topLeft, vertices[topLeft][0], vertices[topLeft][1], topRight,
             vertices[topRight][0], vertices[topRight][1], bottomLeft,
             vertices[bottomLeft][0], vertices[bottomLeft][1], bottomRight,
             vertices[bottomRight][0], vertices[bottomRight][1]);
*/
      indexSets.push_back({topLeft, bottomLeft, topRight});
      indexSets.push_back({topRight, bottomLeft, bottomRight});
    }
  }

  bonobo::mesh_data data;

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
      GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]),
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
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3,
      /* what is the type of each component? */ GL_FLOAT,
      /* should it automatically normalise the values stored */ GL_FALSE,
      /* once all components of a vertex have been read, how far away (in bytes)
         is the next vertex? */
      0,
      /* how far away (in bytes) from the start of the buffer is the first
         vertex? */
      reinterpret_cast<GLvoid const *>(0x0));

  // Now, let's allocate a second one for the indices.
  //
  // Have the buffer's name stored into `data.ibo`.
  glGenBuffers(1, &data.ibo);

  // We still want a 1D-array, but this time it should be a 1D-array of
  // elements, aka. indices!
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);

  glBufferData(
      GL_ELEMENT_ARRAY_BUFFER, indexSets.size() * sizeof(indexSets[0]),
      /* where is the data stored on the CPU? */ indexSets.data(),
      /* inform OpenGL that the data is modified once, but used often */
      GL_STATIC_DRAW);

  data.indices_nb = indexSets.size() * 3;

  // All the data has been recorded, we can unbind them.
  glBindVertexArray(0u);
  glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

  return data;
}

bonobo::mesh_data
parametric_shapes::createSphere(float const radius,
                                unsigned int const longitude_split_count,
                                unsigned int const latitude_split_count) {
  auto const edgesPerPole = longitude_split_count;
  auto const longitudeEdgeCount = longitude_split_count + 1;
  auto const middleEdgeCount = longitudeEdgeCount * latitude_split_count;
  auto const edgeCount = middleEdgeCount + edgesPerPole * 2;

  // NOTE: Generate vertices
  auto vertices = std::vector<glm::vec3>{};
  vertices.reserve(edgeCount);
  auto tangents = std::vector<glm::vec3>{};
  tangents.reserve(edgeCount);
  auto normals = std::vector<glm::vec3>{};
  normals.reserve(edgeCount);
  auto binormals = std::vector<glm::vec3>{};
  binormals.reserve(edgeCount);
  auto textureCoords = std::vector<glm::vec2>{};
  textureCoords.reserve(edgeCount);

  // NOTE: South pole vertices
  for (auto i = 0u; i < edgesPerPole; i++) {
    auto const thetaStep = glm::two_pi<float>() / longitude_split_count;
    auto const theta = thetaStep * i;

    vertices.push_back({0.0f, -radius, 0.0f});
    tangents.push_back(glm::normalize(
        glm::vec3{radius * std::cos(theta), 0.0f, -radius * std::sin(theta)}));
    normals.push_back(glm::normalize(vertices[vertices.size() - 1]));
    binormals.push_back(glm::normalize(
        glm::vec3{radius * std::sin(theta), 0.0f, radius * std::cos(theta)}));

    textureCoords.push_back({theta / glm::two_pi<float>(), 0.f});
  }

  // NOTE: Middle vertices
  for (auto latitude = 0u; latitude < latitude_split_count; latitude++) {
    auto const phiStep = glm::pi<float>() / (latitude_split_count + 1);
    auto const thetaStep = glm::two_pi<float>() / longitude_split_count;

    for (auto longitude = 0u; longitude < longitudeEdgeCount; longitude++) {
      auto const theta = thetaStep * longitude;
      auto const phi = phiStep * (latitude + 1);

      vertices.push_back({radius * std::sin(theta) * std::sin(phi),
                          -radius * std::cos(phi),
                          radius * std::cos(theta) * std::sin(phi)});

      tangents.push_back(glm::normalize(
          glm::vec3{radius * std::cos(theta) * std::sin(phi), 0.0f,
                    -radius * std::sin(theta) * std::sin(phi)}));

      binormals.push_back(glm::normalize(glm::vec3{
          radius * std::sin(theta) * std::cos(phi), radius * std::sin(phi),
          radius * std::cos(theta) * std::cos(phi)}));

      normals.push_back(glm::cross(tangents[tangents.size() - 1],
                                   binormals[binormals.size() - 1]));

      textureCoords.push_back(
          {theta / glm::two_pi<float>(), phi / glm::pi<float>()});
    }
  }

  // NOTE: North pole vertices
  for (auto i = 0u; i < edgesPerPole; i++) {
    auto const thetaStep = glm::two_pi<float>() / longitude_split_count;
    auto const theta = thetaStep * i;

    vertices.push_back({0.0f, radius, 0.0f});
    tangents.push_back(glm::normalize(
        glm::vec3{radius * std::cos(theta), 0.0f, -radius * std::sin(theta)}));
    normals.push_back(glm::normalize(vertices[vertices.size() - 1]));
    binormals.push_back(glm::normalize(
        glm::vec3{-radius * std::sin(theta), 0.0f, -radius * std::cos(theta)}));

    textureCoords.push_back({theta / glm::two_pi<float>(), 1.f});
  }

  // NOTE: Generate vertex index sets
  auto const trianglesPerPole = edgesPerPole;
  auto const middleTriangleCount =
      (latitude_split_count - 1) * longitudeEdgeCount * 2;
  auto const triangleCount = trianglesPerPole * 2 + middleTriangleCount;

  auto vertexIndices = std::vector<glm::uvec3>{};
  vertexIndices.reserve(triangleCount);

  // NOTE: South pole triangle index sets
  for (auto triangle = 0u; triangle < trianglesPerPole; triangle++) {
    auto const bottomOffset = edgesPerPole;
    auto const top = triangle;
    auto const bottomLeft = bottomOffset + triangle;
    auto const bottomRight = bottomLeft + 1;
    vertexIndices.push_back({top, bottomLeft, bottomRight});
  }

  // NOTE: Middle triangle index sets
  for (auto row = 0u; row < (latitude_split_count - 1); row++) {
    auto const topOffset = edgesPerPole + 1 + (longitudeEdgeCount * row);
    auto const bottomOffset = topOffset - 1 + longitudeEdgeCount;

    for (auto longitude = 0u; longitude < longitude_split_count; longitude++) {
      auto topRight = topOffset + longitude;
      auto topLeft = topRight - 1;
      auto bottomLeft = bottomOffset + longitude;
      auto bottomRight = bottomLeft + 1;

      vertexIndices.push_back({topRight, topLeft, bottomLeft});
      vertexIndices.push_back({topRight, bottomLeft, bottomRight});
    }
  }

  // NOTE: North pole triangle index sets
  for (auto triangle = 0u; triangle < trianglesPerPole; triangle++) {
    auto const bottomOffset = edgeCount - edgesPerPole;
    auto const topOffset = bottomOffset - longitudeEdgeCount;

    auto const bottom = bottomOffset + triangle;
    auto const topLeft = topOffset + triangle;
    auto const topRight = topLeft + 1;
    vertexIndices.push_back({bottom, topRight, topLeft});
  }

  auto data = bonobo::mesh_data{};

  glGenVertexArrays(1, &data.vao);
  glBindVertexArray(data.vao);

  glGenBuffers(1, &data.bo);
  glBindBuffer(GL_ARRAY_BUFFER, data.bo);

  auto const vertexOffset = 0u;
  auto const vertexSize = vertices.size() * sizeof(vertices[0]);

  auto const tangentOffset = vertexOffset + vertexSize;
  auto const tangentSize = tangents.size() * sizeof(tangents[0]);

  auto const normalOffset = tangentOffset + tangentSize;
  auto const normalSize = normals.size() * sizeof(normals[0]);

  auto const binormalOffset = normalOffset + normalSize;
  auto const binormalSize = binormals.size() * sizeof(binormals[0]);

  auto const textureCoordOffset = binormalOffset + binormalSize;
  auto const textureCoordSize = textureCoords.size() * sizeof(textureCoords[0]);

  auto const bufferSize =
      vertexSize + tangentSize + normalSize + binormalSize + textureCoordSize;

  glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STATIC_DRAW);

  glBufferSubData(GL_ARRAY_BUFFER, vertexOffset, vertexSize, vertices.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  glBufferSubData(GL_ARRAY_BUFFER, tangentOffset, tangentSize, tangents.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangentOffset));

  glBufferSubData(GL_ARRAY_BUFFER, normalOffset, normalSize, normals.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::normals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normalOffset));

  glBufferSubData(GL_ARRAY_BUFFER, binormalOffset, binormalSize,
                  binormals.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3,
      GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(binormalOffset));

  glBufferSubData(GL_ARRAY_BUFFER, textureCoordOffset, textureCoordSize,
                  textureCoords.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 2,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(textureCoordOffset));

  glGenBuffers(1, &data.ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               vertexIndices.size() * sizeof(vertexIndices[0]),
               vertexIndices.data(), GL_STATIC_DRAW);
  data.indices_nb = vertexIndices.size() * 3;

  glBindVertexArray(0u);
  glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

  return data;
}

bonobo::mesh_data
parametric_shapes::createTorus(float const major_radius,
                               float const minor_radius,
                               unsigned int const major_split_count,
                               unsigned int const minor_split_count) {
  auto const majorEdgeCount = major_split_count + 1;
  auto const minorEdgeCount = minor_split_count + 1;
  auto const edgeCount = majorEdgeCount * minorEdgeCount;

  // NOTE: Generate vertices
  auto vertices = std::vector<glm::vec3>{};
  vertices.reserve(edgeCount);
  auto tangents = std::vector<glm::vec3>{};
  tangents.reserve(edgeCount);
  auto normals = std::vector<glm::vec3>{};
  normals.reserve(edgeCount);
  auto binormals = std::vector<glm::vec3>{};
  binormals.reserve(edgeCount);
  auto textureCoords = std::vector<glm::vec2>{};
  textureCoords.reserve(edgeCount);

  // NOTE: Vertices
  for (auto minor = 0u; minor < minorEdgeCount; minor++) {
    auto const phiStep = glm::two_pi<float>() / major_split_count;
    auto const thetaStep = glm::two_pi<float>() / minor_split_count;

    for (auto major = 0u; major < majorEdgeCount; major++) {
      auto const phi = phiStep * major;
      auto const theta = thetaStep * minor;

      vertices.push_back(
          {(major_radius + minor_radius * std::cos(theta)) * std::cos(phi),
           (major_radius + minor_radius * std::cos(theta)) * std::sin(phi),
           -minor_radius * std::sin(theta)});

      tangents.push_back(glm::normalize(
          glm::vec3{-minor_radius * std::sin(theta) * std::cos(phi),
                    -minor_radius * std::sin(theta) * std::sin(phi),
                    -minor_radius * std::cos(theta)}));

      binormals.push_back(glm::normalize(glm::vec3{
          -(major_radius + minor_radius * std::cos(theta)) * sin(phi),
          (major_radius + minor_radius * std::cos(theta)) * cos(phi), 0.0f}));

      normals.push_back(glm::cross(tangents[tangents.size() - 1],
                                   binormals[binormals.size() - 1]));

      textureCoords.push_back(
          {theta / glm::two_pi<float>(), phi / glm::pi<float>()});
    }
  }

  // NOTE: Generate vertex index sets
  auto const trianglesPerRow = 2 * major_split_count;
  auto const triangleCount = trianglesPerRow * minor_split_count;

  auto vertexIndices = std::vector<glm::uvec3>{};
  vertexIndices.reserve(triangleCount);

  // NOTE: Triangle index sets
  for (auto row = 0u; row < minor_split_count; row++) {
    auto const topOffset = majorEdgeCount * row;
    auto const bottomOffset = majorEdgeCount * (row + 1);

    for (auto column = 0u; column < major_split_count; column++) {
      auto topLeft = topOffset + column;
      auto topRight = topLeft + 1;
      auto bottomLeft = bottomOffset + column;
      auto bottomRight = bottomLeft + 1;

      vertexIndices.push_back({topLeft, bottomLeft, topRight});
      vertexIndices.push_back({topRight, bottomLeft, bottomRight});
    }
  }
  auto data = bonobo::mesh_data{};

  glGenVertexArrays(1, &data.vao);
  glBindVertexArray(data.vao);

  glGenBuffers(1, &data.bo);
  glBindBuffer(GL_ARRAY_BUFFER, data.bo);

  auto const vertexOffset = 0u;
  auto const vertexSize = vertices.size() * sizeof(vertices[0]);

  auto const tangentOffset = vertexOffset + vertexSize;
  auto const tangentSize = tangents.size() * sizeof(tangents[0]);

  auto const normalOffset = tangentOffset + tangentSize;
  auto const normalSize = normals.size() * sizeof(normals[0]);

  auto const binormalOffset = normalOffset + normalSize;
  auto const binormalSize = binormals.size() * sizeof(binormals[0]);

  auto const textureCoordOffset = binormalOffset + binormalSize;
  auto const textureCoordSize = textureCoords.size() * sizeof(textureCoords[0]);

  auto const bufferSize =
      vertexSize + tangentSize + normalSize + binormalSize + textureCoordSize;

  glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STATIC_DRAW);

  glBufferSubData(GL_ARRAY_BUFFER, vertexOffset, vertexSize, vertices.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  glBufferSubData(GL_ARRAY_BUFFER, tangentOffset, tangentSize, tangents.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangentOffset));

  glBufferSubData(GL_ARRAY_BUFFER, normalOffset, normalSize, normals.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::normals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normalOffset));

  glBufferSubData(GL_ARRAY_BUFFER, binormalOffset, binormalSize,
                  binormals.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3,
      GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const *>(binormalOffset));

  glBufferSubData(GL_ARRAY_BUFFER, textureCoordOffset, textureCoordSize,
                  textureCoords.data());
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 2,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(textureCoordOffset));

  glGenBuffers(1, &data.ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               vertexIndices.size() * sizeof(vertexIndices[0]),
               vertexIndices.data(), GL_STATIC_DRAW);
  data.indices_nb = vertexIndices.size() * 3;

  glBindVertexArray(0u);
  glBindBuffer(GL_ARRAY_BUFFER, 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

  return data;
}

bonobo::mesh_data
parametric_shapes::createCircleRing(float const radius,
                                    float const spread_length,
                                    unsigned int const circle_split_count,
                                    unsigned int const spread_split_count) {
  auto const circle_slice_edges_count = circle_split_count + 1u;
  auto const spread_slice_edges_count = spread_split_count + 1u;
  auto const circle_slice_vertices_count = circle_slice_edges_count + 1u;
  auto const spread_slice_vertices_count = spread_slice_edges_count + 1u;
  auto const vertices_nb =
      circle_slice_vertices_count * spread_slice_vertices_count;

  auto vertices = std::vector<glm::vec3>(vertices_nb);
  auto normals = std::vector<glm::vec3>(vertices_nb);
  auto texcoords = std::vector<glm::vec3>(vertices_nb);
  auto tangents = std::vector<glm::vec3>(vertices_nb);
  auto binormals = std::vector<glm::vec3>(vertices_nb);

  float const spread_start = radius - 0.5f * spread_length;
  float const d_theta =
      glm::two_pi<float>() / (static_cast<float>(circle_slice_edges_count));
  float const d_spread =
      spread_length / (static_cast<float>(spread_slice_edges_count));

  // generate vertices iteratively
  size_t index = 0u;
  float theta = 0.0f;
  for (unsigned int i = 0u; i < circle_slice_vertices_count; ++i) {
    float const cos_theta = std::cos(theta);
    float const sin_theta = std::sin(theta);

    float distance_to_centre = spread_start;
    for (unsigned int j = 0u; j < spread_slice_vertices_count; ++j) {
      // vertex
      vertices[index] = glm::vec3(distance_to_centre * cos_theta,
                                  distance_to_centre * sin_theta, 0.0f);

      // texture coordinates
      texcoords[index] =
          glm::vec3(static_cast<float>(j) /
                        (static_cast<float>(spread_slice_vertices_count)),
                    static_cast<float>(i) /
                        (static_cast<float>(circle_slice_vertices_count)),
                    0.0f);

      // tangent
      auto const t = glm::vec3(cos_theta, sin_theta, 0.0f);
      tangents[index] = t;

      // binormal
      auto const b = glm::vec3(-sin_theta, cos_theta, 0.0f);
      binormals[index] = b;

      // normal
      auto const n = glm::cross(t, b);
      normals[index] = n;

      distance_to_centre += d_spread;
      ++index;
    }

    theta += d_theta;
  }

  // create index array
  auto index_sets = std::vector<glm::uvec3>(2u * circle_slice_edges_count *
                                            spread_slice_edges_count);

  // generate indices iteratively
  index = 0u;
  for (unsigned int i = 0u; i < circle_slice_edges_count; ++i) {
    for (unsigned int j = 0u; j < spread_slice_edges_count; ++j) {
      index_sets[index] =
          glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
                     spread_slice_vertices_count * (i + 0u) + (j + 1u),
                     spread_slice_vertices_count * (i + 1u) + (j + 1u));
      ++index;

      index_sets[index] =
          glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
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
  auto const bo_size =
      static_cast<GLsizeiptr>(vertices_size + normals_size + texcoords_size +
                              tangents_size + binormals_size);
  glGenBuffers(1, &data.bo);
  assert(data.bo != 0u);
  glBindBuffer(GL_ARRAY_BUFFER, data.bo);
  glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

  glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size,
                  static_cast<GLvoid const *>(vertices.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(0x0));

  glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size,
                  static_cast<GLvoid const *>(normals.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::normals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(normals_offset));

  glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size,
                  static_cast<GLvoid const *>(texcoords.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(texcoords_offset));

  glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size,
                  static_cast<GLvoid const *>(tangents.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT,
      GL_FALSE, 0, reinterpret_cast<GLvoid const *>(tangents_offset));

  glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size,
                  static_cast<GLvoid const *>(binormals.data()));
  glEnableVertexAttribArray(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals));
  glVertexAttribPointer(
      static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3,
      GL_FLOAT, GL_FALSE, 0,
      reinterpret_cast<GLvoid const *>(binormals_offset));

  glBindBuffer(GL_ARRAY_BUFFER, 0u);

  data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
  glGenBuffers(1, &data.ibo);
  assert(data.ibo != 0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)),
               reinterpret_cast<GLvoid const *>(index_sets.data()),
               GL_STATIC_DRAW);

  glBindVertexArray(0u);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

  return data;
}
