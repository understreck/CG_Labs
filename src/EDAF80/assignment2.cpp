#include "assignment2.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/ShaderProgramManager.hpp"
#include "core/node.hpp"
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <clocale>
#include <cstdlib>
#include <iterator>
#include <stdexcept>

edaf80::Assignment2::Assignment2(WindowManager &windowManager)
    : mCamera(0.5f * glm::half_pi<float>(),
              static_cast<float>(config::resolution_x) /
                  static_cast<float>(config::resolution_y),
              0.01f, 1000.0f),
      inputHandler(), mWindowManager(windowManager), window(nullptr) {
  WindowManager::WindowDatum window_datum{inputHandler,
                                          mCamera,
                                          config::resolution_x,
                                          config::resolution_y,
                                          0,
                                          0,
                                          0,
                                          0};

  window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 2", window_datum,
                                           config::msaa_rate);
  if (window == nullptr) {
    throw std::runtime_error("Failed to get a window: aborting!");
  }

  bonobo::init();
}

edaf80::Assignment2::~Assignment2() { bonobo::deinit(); }

void edaf80::Assignment2::run() {
  // Set up the camera
  mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 1.0f, 9.0f));
  mCamera.mMouseSensitivity = glm::vec2(0.003f);
  mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

  // Create the shader programs
  ShaderProgramManager program_manager;
  GLuint fallback_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Fallback",
      {{ShaderType::vertex, "common/fallback.vert"},
       {ShaderType::fragment, "common/fallback.frag"}},
      fallback_shader);
  if (fallback_shader == 0u) {
    LogError("Failed to load fallback shader");
    return;
  }

  GLuint phong_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Phong",
      {{ShaderType::vertex, "EDAF80/phong.vert"},
       {ShaderType::fragment, "EDAF80/phong.frag"}},
      phong_shader);
  if (phong_shader == 0u) {
    LogError("Failed to load phong shader");
    return;
  }

  GLuint diffuse_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Diffuse",
      {{ShaderType::vertex, "EDAF80/diffuse.vert"},
       {ShaderType::fragment, "EDAF80/diffuse.frag"}},
      diffuse_shader);
  if (diffuse_shader == 0u)
    LogError("Failed to load diffuse shader");

  GLuint normal_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Normal",
      {{ShaderType::vertex, "EDAF80/normal.vert"},
       {ShaderType::fragment, "EDAF80/normal.frag"}},
      normal_shader);
  if (normal_shader == 0u)
    LogError("Failed to load normal shader");

  GLuint tangent_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Tangent",
      {{ShaderType::vertex, "EDAF80/tangent.vert"},
       {ShaderType::fragment, "EDAF80/tangent.frag"}},
      tangent_shader);
  if (tangent_shader == 0u)
    LogError("Failed to load tangent shader");

  GLuint binormal_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Bitangent",
      {{ShaderType::vertex, "EDAF80/binormal.vert"},
       {ShaderType::fragment, "EDAF80/binormal.frag"}},
      binormal_shader);
  if (binormal_shader == 0u)
    LogError("Failed to load binormal shader");

  GLuint texcoord_shader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Texture coords",
      {{ShaderType::vertex, "EDAF80/texcoord.vert"},
       {ShaderType::fragment, "EDAF80/texcoord.frag"}},
      texcoord_shader);
  if (texcoord_shader == 0u)
    LogError("Failed to load texcoord shader");

  auto const light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
  auto const set_uniforms = [&light_position](GLuint program) {
    glUniform3fv(glGetUniformLocation(program, "light_position"), 1,
                 glm::value_ptr(light_position));
  };

  // Set whether the default interpolation algorithm should be the linear one;
  // it can always be changed at runtime through the "Scene Controls" window.
  bool use_linear = true;

  // Set whether to interpolate the position of an object or not; it can
  // always be changed at runtime through the "Scene Controls" window.
  bool interpolate = true;

  // Set whether to show the control points or not; it can always be changed
  // at runtime through the "Scene Controls" window.
  bool show_control_points = true;

  bool use_normal_mapping = false;
  auto camera_position = mCamera.mWorld.GetTranslation();
  auto const phong_set_uniforms = [&use_normal_mapping, &light_position,
                                   &camera_position](GLuint program) {
    glUniform1i(glGetUniformLocation(program, "use_normal_mapping"),
                use_normal_mapping ? 1 : 0);
    glUniform3fv(glGetUniformLocation(program, "light_position"), 1,
                 glm::value_ptr(light_position));
    glUniform3fv(glGetUniformLocation(program, "camera_position"), 1,
                 glm::value_ptr(camera_position));
  };

  auto demo_shape = parametric_shapes::createSphere(0.25f, 40u, 40u);
  if (demo_shape.vao == 0u) {
    LogError("Failed to retrieve the mesh for the demo sphere");
    return;
  }

  bonobo::material_data demo_material;
  demo_material.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
  demo_material.diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
  demo_material.specular = glm::vec3(1.0f, 1.0f, 1.0f);
  demo_material.shininess = 10.0f;

  auto diffuse_texture =
      bonobo::loadTexture2D("res/textures/leather_red_02_coll1_2k.jpg");
  auto specular_texture =
      bonobo::loadTexture2D("res/textures/leather_red_02_rough_2k.jpg");
  auto normal_texture =
      bonobo::loadTexture2D("res/textures/leather_red_02_nor_2k.jpg");

  Node demo_sphere;
  demo_sphere.set_geometry(demo_shape);
  demo_sphere.set_material_constants(demo_material);
  demo_sphere.set_program(&phong_shader, phong_set_uniforms);

  demo_sphere.add_texture("diffuse_texture", diffuse_texture, GL_TEXTURE_2D);
  demo_sphere.add_texture("specular_texture", specular_texture, GL_TEXTURE_2D);
  demo_sphere.add_texture("normal_texture", normal_texture, GL_TEXTURE_2D);
  //! \todo Create a tesselated sphere and a tesselated torus

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  auto const control_point_sphere =
      parametric_shapes::createSphere(0.1f, 10u, 10u);

  std::array<glm::vec3, 9> control_point_locations = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(1.0f, 1.8f, 1.0f),
      glm::vec3(2.0f, 1.2f, 2.0f),    glm::vec3(3.0f, 3.0f, 3.0f),
      glm::vec3(3.0f, 0.0f, 3.0f),    glm::vec3(-2.0f, -1.0f, 3.0f),
      glm::vec3(-3.0f, -3.0f, -3.0f), glm::vec3(-2.0f, -1.2f, -2.0f),
      glm::vec3(-1.0f, -1.8f, -1.0f)};
  std::array<Node, control_point_locations.size()> control_points;
  for (std::size_t i = 0; i < control_point_locations.size(); ++i) {
    auto &control_point = control_points[i];
    control_point.set_geometry(control_point_sphere);
    control_point.set_program(&diffuse_shader, set_uniforms);
    control_point.get_transform().SetTranslate(control_point_locations[i]);
  }

  auto lastTime = std::chrono::high_resolution_clock::now();

  std::int32_t program_index = 1;
  float elapsed_time_s = 0.0f;
  auto cull_mode = bonobo::cull_mode_t::disabled;
  auto polygon_mode = bonobo::polygon_mode_t::fill;
  bool show_logs = true;
  bool show_gui = true;
  bool show_basis = false;
  float basis_thickness_scale = 1.0f;
  float basis_length_scale = 1.0f;

  changeCullMode(cull_mode);

  auto followingIndex = 0;

  auto constexpr nextFollowingIndex =
      [end = control_point_locations.size()](auto index) {
        return (index + 1) < end ? (index + 1) : 0;
      };
  auto constexpr prevFollowingIndex = [last = control_point_locations.size() -
                                              1](auto index) {
    return (index - 1) >= 0 ? (index - 1) : last;
  };

  auto lerpSpeed = 1.0f;
  auto catmullRomTension = 0.5f;
  auto catmullRomSpeed = 1.f;
  auto terpLocation = 0.0f;

  while (!glfwWindowShouldClose(window)) {
    auto const nowTime = std::chrono::high_resolution_clock::now();
    auto const deltaTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                              lastTime);
    lastTime = nowTime;

    auto &io = ImGui::GetIO();
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

    glfwPollEvents();
    inputHandler.Advance();
    mCamera.Update(deltaTimeUs, inputHandler);
    camera_position = mCamera.mWorld.GetTranslation();
    elapsed_time_s += std::chrono::duration<float>(deltaTimeUs).count();

    if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
      show_logs = !show_logs;
    if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
      show_gui = !show_gui;
    if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
      mWindowManager.ToggleFullscreenStatusForWindow(window);

    // Retrieve the actual framebuffer size: for HiDPI monitors,
    // you might end up with a framebuffer larger than what you
    // actually asked for. For example, if you ask for a 1920x1080
    // framebuffer, you might get a 3840x2160 one instead.
    // Also it might change as the user drags the window between
    // monitors with different DPIs, or if the fullscreen status is
    // being toggled.
    int framebuffer_width, framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0, 0, framebuffer_width, framebuffer_height);

    mWindowManager.NewImGuiFrame();

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    bonobo::changePolygonMode(polygon_mode);

    if (interpolate) {
      if (use_linear) {
        auto to = interpolation::evalLERP(
            control_point_locations[prevFollowingIndex(followingIndex)],
            control_point_locations[followingIndex], terpLocation);

        demo_sphere.get_transform().SetTranslate(to);

        terpLocation +=
            std::chrono::duration<float>(deltaTimeUs).count() * lerpSpeed;
      } else {
        auto to = interpolation::evalCatmullRom(
            control_point_locations[prevFollowingIndex(followingIndex)],
            control_point_locations[followingIndex],
            control_point_locations[nextFollowingIndex(followingIndex)],
            control_point_locations[nextFollowingIndex(
                nextFollowingIndex(followingIndex))],
            catmullRomTension, terpLocation);

        demo_sphere.get_transform().SetTranslate(to);

        terpLocation +=
            std::chrono::duration<float>(deltaTimeUs).count() * catmullRomSpeed;
      }

      if (terpLocation > 1.f) {
        terpLocation = 0.f;
        followingIndex = nextFollowingIndex(followingIndex);
      }
    }

    demo_sphere.render(mCamera.GetWorldToClipMatrix());
    if (show_control_points) {
      for (auto const &control_point : control_points) {
        control_point.render(mCamera.GetWorldToClipMatrix());
      }
    }

    bool const opened =
        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
    if (opened) {
      auto const cull_mode_changed =
          bonobo::uiSelectCullMode("Cull mode", cull_mode);
      if (cull_mode_changed) {
        changeCullMode(cull_mode);
      }
      bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
      auto selection_result =
          program_manager.SelectProgram("Shader", program_index);
      if (selection_result.was_selection_changed) {
        demo_sphere.set_program(selection_result.program, set_uniforms);
      }
      ImGui::Separator();
      ImGui::Checkbox("Show control points", &show_control_points);
      ImGui::Checkbox("Enable interpolation", &interpolate);
      ImGui::Checkbox("Use linear interpolation", &use_linear);
      ImGui::SliderFloat("LERP speed", &lerpSpeed, 0.0f, 5.0f);
      ImGui::SliderFloat("Catmull-Rom tension", &catmullRomTension, 0.0f, 1.0f);
      ImGui::SliderFloat("Catmull-Rom speed", &catmullRomSpeed, 0.0f, 5.0f);
      ImGui::Separator();
      ImGui::Checkbox("Show basis", &show_basis);
      ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f,
                         100.0f);
      ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f,
                         100.0f);
    }
    ImGui::End();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (show_basis)
      bonobo::renderBasis(basis_thickness_scale, basis_length_scale,
                          mCamera.GetWorldToClipMatrix());
    if (show_logs)
      Log::View::Render();
    mWindowManager.RenderImGuiFrame(show_gui);

    glfwSwapBuffers(window);
  }
}

int main() {
  std::setlocale(LC_ALL, "");

  Bonobo framework;

  try {
    edaf80::Assignment2 assignment2(framework.GetWindowManager());
    assignment2.run();
  } catch (std::runtime_error const &e) {
    LogError(e.what());
  }
}
