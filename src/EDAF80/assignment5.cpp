#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/ShaderProgramManager.hpp"
#include "core/helpers.hpp"
#include "core/node.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <random>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

edaf80::Assignment5::Assignment5(WindowManager &windowManager)
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

  window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum,
                                           config::msaa_rate);
  if (window == nullptr) {
    throw std::runtime_error("Failed to get a window: aborting!");
  }

  bonobo::init();
}

edaf80::Assignment5::~Assignment5() { bonobo::deinit(); }

class Boat {
  std::vector<bonobo::mesh_data> m_meshes;
  std::array<Node, 3> m_boat;
  std::array<Node, 4> m_sail;

public:
  Boat(GLuint const *const shaderID,
       std::function<void(GLuint)> const &set_uniforms)
      : m_meshes{bonobo::loadObjects("./res/game/boat.obj")}, m_boat{},
        m_sail{} {
    m_boat[0].set_geometry(m_meshes[0]);
    m_boat[0].set_program(shaderID, set_uniforms);
    m_boat[0].set_material_constants(m_meshes[0].material);

    m_boat[1].set_geometry(m_meshes[1]);
    m_boat[1].set_program(shaderID, set_uniforms);
    m_boat[1].set_material_constants(m_meshes[1].material);

    m_boat[2].set_geometry(m_meshes[2]);
    m_boat[2].set_program(shaderID, set_uniforms);
    m_boat[2].set_material_constants(m_meshes[2].material);

    m_sail[0].set_geometry(m_meshes[3]);
    m_sail[0].set_program(shaderID, set_uniforms);
    m_sail[0].set_material_constants(m_meshes[3].material);

    m_sail[1].set_geometry(m_meshes[4]);
    m_sail[1].set_program(shaderID, set_uniforms);
    m_sail[1].set_material_constants(m_meshes[4].material);

    m_sail[2].set_geometry(m_meshes[5]);
    m_sail[2].set_program(shaderID, set_uniforms);
    m_sail[2].set_material_constants(m_meshes[5].material);

    m_sail[3].set_geometry(m_meshes[6]);
    m_sail[3].set_program(shaderID, set_uniforms);
    m_sail[3].set_material_constants(m_meshes[6].material);
  }

  auto render(glm::mat4 const &world, glm::mat4 const &transform) {
    for (auto &&node : m_boat) {
      node.render(world, transform);
    }

    for (auto &&node : m_sail) {
      node.render(world, transform);
    }
  }
};

void edaf80::Assignment5::run() {
  // Set up the camera
  mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 30.0f, 0.0f));
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

  GLuint wavesShader = 0u;
  program_manager.CreateAndRegisterProgram(
      "Waves",
      {{ShaderType::vertex, "game/game_water.vert"},
       {ShaderType::fragment, "game/game_water.frag"}},
      wavesShader);
  if (wavesShader == 0u) {
    LogError("Failed to load fallback shader");
    return;
  }

  auto wirlPoolBuffer = 0u;
  glGenBuffers(1, &wirlPoolBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, wirlPoolBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec2) * 10, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, wirlPoolBuffer);

  auto whirlPools = std::array<glm::vec2, 10>{};
  auto mt = std::mt19937{std::random_device{}()};
  auto distribution = std::uniform_int_distribution{-50, 50};

  for (auto &&whirlPool : whirlPools) {
    whirlPool = {distribution(mt), distribution(mt)};
  }

  glBindBuffer(GL_UNIFORM_BUFFER, wirlPoolBuffer);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::vec2) * 10,
                  whirlPools.data());

  auto elapsedTimeSeconds = 0.0f;

  auto island = glm::vec3{0.0, 0.0, 10.0f};

  struct Wave {
    glm::vec2 direction;
    float amplitude;
    float frequency;
    float phase;
    float sharpness;
  } mainWave{{-1.0, 0.0}, 0.0, 0.2, 0.5, 2.0};

  auto terrainProgram = [&elapsedTimeSeconds, &camera = mCamera, &mainWave,
                         &island](GLuint program) {
    glUniform1f(glGetUniformLocation(program, "elapsedTimeSeconds"),
                elapsedTimeSeconds);
    glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1,
                 glm::value_ptr(camera.mWorld.GetTranslation()));

    glUniform2fv(glGetUniformLocation(program, "mainWave.direction"), 1,
                 glm::value_ptr(mainWave.direction));
    glUniform1f(glGetUniformLocation(program, "mainWave.amplitude"),
                mainWave.amplitude);
    glUniform1f(glGetUniformLocation(program, "mainWave.frequency"),
                mainWave.frequency);
    glUniform1f(glGetUniformLocation(program, "mainWave.phase"),
                mainWave.phase);
    glUniform1f(glGetUniformLocation(program, "mainWave.sharpness"),
                mainWave.sharpness);

    glUniform3fv(glGetUniformLocation(program, "island"), 1,
                 glm::value_ptr(island));
  };

  auto terrainNoiseTexture = bonobo::loadTexture2D("res/textures/waves.png");

  auto terrainGeometry =
      parametric_shapes::createQuad(100.0, 100.0, 1000, 1000);
  auto terrainNode = Node{};
  terrainNode.set_geometry(terrainGeometry);
  terrainNode.get_transform().Translate(glm::vec3{-50.0f, 0.0, -50.0f});
  terrainNode.add_texture("normalTexture", terrainNoiseTexture, GL_TEXTURE_2D);
  terrainNode.set_program(&wavesShader, terrainProgram);

  glClearDepthf(1.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);

  auto lastTime = std::chrono::high_resolution_clock::now();

  bool show_logs = true;
  bool show_gui = true;
  bool shader_reload_failed = false;
  bool show_basis = false;
  float basis_thickness_scale = 1.0f;
  float basis_length_scale = 1.0f;

  auto boat = Boat{&fallback_shader, [](GLuint) {}};

  while (!glfwWindowShouldClose(window)) {
    auto const nowTime = std::chrono::high_resolution_clock::now();
    auto const deltaTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(nowTime -
                                                              lastTime);
    elapsedTimeSeconds += std::chrono::duration<float>(deltaTimeUs).count();
    lastTime = nowTime;

    auto &io = ImGui::GetIO();
    inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

    glfwPollEvents();
    inputHandler.Advance();
    mCamera.Update(deltaTimeUs, inputHandler);

    if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
      shader_reload_failed = !program_manager.ReloadAllPrograms();
      if (shader_reload_failed)
        tinyfd_notifyPopup("Shader Program Reload Error",
                           "An error occurred while reloading shader programs; "
                           "see the logs for details.\n"
                           "Rendering is suspended until the issue is solved. "
                           "Once fixed, just reload the shaders again.",
                           "error");
    }
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

    //
    // Todo: If you need to handle inputs, you can do it here
    //

    mWindowManager.NewImGuiFrame();

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    if (!shader_reload_failed) {
      //
      // Todo: Render all your geometry here.
      //
      terrainNode.render(mCamera.GetWorldToClipMatrix());
      boat.render(mCamera.GetWorldToClipMatrix(),
                  glm::rotate(glm::mat4{1.0},
                              glm::pi<float>() * elapsedTimeSeconds * 0.1f,
                              glm::vec3{0.0, 1.0, 0.0}));
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //
    // Todo: If you want a custom ImGUI window, you can set it up
    //       here
    //
    bool const opened =
        ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
    if (opened) {
      ImGui::Checkbox("Show basis", &show_basis);
      ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f,
                         100.0f);
      ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f,
                         100.0f);
    }
    ImGui::End();

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
    edaf80::Assignment5 assignment5(framework.GetWindowManager());
    assignment5.run();
  } catch (std::runtime_error const &e) {
    LogError(e.what());
  }
}
