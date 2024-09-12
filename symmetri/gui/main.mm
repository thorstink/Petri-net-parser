#define IMGUI_DEFINE_MATH_OPERATORS

#include <stdio.h>

#include <chrono>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include "rpp/rpp.hpp"
#include "rxdispatch.h"
#include "rximgui.h"
using namespace rximgui;
#include <iostream>
#include "draw_ui.h"
#include "model.h"
#include "util.h"
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char **) {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  io.Fonts->AddFontFromFileTTF(
      "/Users/thomashorstink/Projects/Symmetri/symmetri/gui/imgui/misc/fonts/DroidSans.ttf", 15);

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  // Setup style
  // ImGui::StyleColorsDark();
  ImGui::StyleColorsLight();

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) return 1;

  // Create window with graphics context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "Dear ImGui GLFW+Metal example", nullptr, nullptr);
  if (window == nullptr) return 1;

  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  id<MTLCommandQueue> commandQueue = [device newCommandQueue];

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplMetal_Init(device);

  NSWindow *nswin = glfwGetCocoaWindow(window);
  CAMetalLayer *layer = [CAMetalLayer layer];
  layer.device = device;
  layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  nswin.contentView.layer = layer;
  nswin.contentView.wantsLayer = YES;

  MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor new];

  auto reducers = rpp::source::create<model::Reducer>(&rxdispatch::dequeue) |
                  rpp::operators::subscribe_on(rpp::schedulers::new_thread{});

  auto models =
      reducers |
      rpp::operators::scan(model::initializeModel(), [](model::Model &&m, const model::Reducer &f) {
        static size_t i = 0;
        std::cout << "update " << i++ << ", ref: " << m.data.use_count() << std::endl;
        try {
          m.data->timestamp = std::chrono::steady_clock::now();
          return f(std::move(m));
        } catch (const std::exception &e) {
          printf("%s", e.what());
          return std::move(m);
        }
      });

  auto root_subscription =
      frames | rpp::operators::subscribe_on(rximgui::rl) |
      rpp::operators::with_latest_from(
          rxu::take_at<1>(),
          models | rpp::operators::map([](const model::Model &m) { return model::ViewModel{m}; })) |
      rpp::operators::tap(&draw_everything) | rpp::operators::subscribe_with_disposable();

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    @autoreleasepool {
      glfwPollEvents();

      int width, height;
      glfwGetFramebufferSize(window, &width, &height);
      layer.drawableSize = CGSizeMake(width, height);
      id<CAMetalDrawable> drawable = [layer nextDrawable];

      id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
      renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
      renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
      renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
      id<MTLRenderCommandEncoder> renderEncoder =
          [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
      // [renderEncoder pushDebugGroup:@"ImGui demo"];

      // Start the Dear ImGui frame
      ImGui_ImplMetal_NewFrame(renderPassDescriptor);
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      while (!rl.is_empty()) {
        rl.dispatch();
      };
      sendframe();
      while (!rl.is_empty()) {
        rl.dispatch();
      };

      // Rendering
      ImGui::Render();
      ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

      // [renderEncoder popDebugGroup]; ????
      [renderEncoder endEncoding];

      [commandBuffer presentDrawable:drawable];
      [commandBuffer commit];
    }
  }
  rxdispatch::unsubscribe();
  root_subscription.dispose();

  ImGui_ImplMetal_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
