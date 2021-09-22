#include <myrt/sfml/sfml.hpp>
#include <myrt/sfml/gl.hpp>
#include <thread>
#include <source_location>
#include <rnu/math/math.hpp>
#include <mutex>
#include <myrt/sfml/utils.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class ffmpeg_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

void fail(std::string msg, std::source_location location = std::source_location::current()) {
  std::format_to(std::ostreambuf_iterator(std::cerr), "critical error [{} at {}:{}]: ", location.function_name(), location.line(), location.column());
  std::cerr << msg << '\n';
  throw ffmpeg_error(std::move(msg));
}
//
//template<size_t S>
//void fail(char const(&msg)[S], std::source_location location = std::source_location::current()) {
//  fail(std::string_view(msg), location);
//}

template<typename Fun>
struct on_exit_t {
  Fun callback;

  ~on_exit_t() {
    callback();
  }
};

template<typename Fun>
on_exit_t<Fun> on_exit(Fun&& f)
{
  return on_exit_t<Fun>{std::forward<Fun>(f)};
}

struct video_transform
{
  void set_video_size(int vw, int vh)
  {
    video_width = vw;
    video_height = vh;
  }

  void set_window_size(int ww, int wh)
  {
    window_width = ww;
    window_height = wh;
  }

  void move(int x, int y)
  {
    float winx = - (x / window_width);
    float winy = (y / window_height);
    rnu::vec3 pt(winx, winy, 1);
    translate *= rnu::mat3(rnu::vec3(1, 0, 0), rnu::vec3(0, 1, 0), pt);
  }

  rnu::mat3 compute_transform() {

    float aspectw = (window_height / video_height) / window_width * video_width;
    float aspecth = 1.f;
    if (aspectw < 1.0f)
    {
      aspecth = 1.0f / aspectw;
      aspectw = 1.0f;
    }
    rnu::mat3 center_frame_mat;
    center_frame_mat[0][0] = aspecth;
    center_frame_mat[1][1] = aspectw;
    center_frame_mat[2][0] = (1 - aspecth) / 2;
    center_frame_mat[2][1] = (1 - aspectw) / 2;

    matrix = center_frame_mat;
    return matrix * zoom_translate * translate;
  }

  void zoom_at(float by, int x, int y)
  {
    float winx = (x / window_width);
    float winy = 1 - (y / window_height);

    rnu::vec3 point = rnu::vec3(winx, winy, 1);
    rnu::mat3 translate;
    translate[2][0] = -point[0] / point[2];
    translate[2][1] = -point[1] / point[2];
    translate[2][2] = 1;
    translate = translate * inverse( this->translate );
    rnu::mat3 translate2 = inverse(translate);

    zoom_translate *= translate2 * rnu::mat3(1 / by, 0, 0, 0, 1 / by, 0, 0, 0, 1) * translate;
  }

  float video_width;
  float video_height;
  float window_width;
  float window_height;

  rnu::mat3 translate;
  rnu::mat3 zoom_translate;
  rnu::mat3 matrix;
} transform;

float zoom = 1.0f;

std::filesystem::path video_file = myrt::gl::resources_dir / "yo.mp4";

void render_function(std::stop_token stop_token, sf::RenderWindow* window);
int main(int argc, char** argv)
{
  std::span<char*> args(argv, argc);

  if (args.size() > 1)
    video_file = args.back();

  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 6;
  settings.attributeFlags |= sf::ContextSettings::Debug;

  sf::RenderWindow window(sf::VideoMode(1600, 1200), "MyRT", sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);
  window.setActive(false);

  std::jthread render_thread(&render_function, &window);

  for (auto event : myrt::sfml::poll_event(window)) {

    switch (event.get().type) {
    case sf::Event::EventType::MouseWheelScrolled:
      transform.zoom_at(pow(1.1, event.get().mouseWheelScroll.delta), event.get().mouseWheelScroll.x, event.get().mouseWheelScroll.y);
      break;
    case sf::Event::EventType::Closed:
      render_thread.request_stop();
      break;
    default:
      break;
    }
  }
  render_thread.request_stop();
}

template<typename DataType>
struct frame_data_t
{
  frame_data_t(int w, int h)
    : width(w), height(h), data(w* h)
  {

  }

  void resize(int w, int h)
  {
    width = w;
    height = h;
    data.resize(w * h);
  }

  int width;
  int height;
  std::vector<DataType> data;
};

std::string averror(int errnum) {
  std::string err(AV_ERROR_MAX_STRING_SIZE, '\0');
  av_make_error_string(err.data(), AV_ERROR_MAX_STRING_SIZE, errnum);
  return err;
}

void render_function(std::stop_token stop_token, sf::RenderWindow* window)
{
  myrt::gl::start(*window);

  frame_data_t<rnu::vec4ui8> frame_data(1, 1);

  std::mutex frame_data_mutex;
  std::atomic_bool image_changed = false;
  std::atomic_bool size_changed = false;
  std::atomic_int64_t seek_ts = -1;
  std::atomic_int64_t last_pts = -1;
  std::atomic_int64_t duration = 1;

  std::atomic_bool play = true;

  std::jthread frame_provider([&](std::stop_token stop) {
    AVFormatContext* avformat_context = nullptr;

    if (avformat_open_input(&avformat_context, video_file.string().c_str(), nullptr, nullptr) != 0)
    {
      fail("Failed to open av format context.");
    }
    auto close_avformat_context = on_exit([&] { avformat_free_context(avformat_context); });

    AVCodec* codec = nullptr;
    AVCodecParameters* params = nullptr;
    AVRational time_base;

    std::optional<unsigned> video_stream = std::nullopt;
    for (unsigned i = 0; i < avformat_context->nb_streams; ++i)
    {
      auto stream = avformat_context->streams[i];
      params = stream->codecpar;
      if (!params) continue;
      codec = avcodec_find_decoder(params->codec_id);
      if (!codec) continue;
      time_base = stream->time_base;

      if (params->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        video_stream = i;
        duration = stream->duration;
        break;
      }
    }

    if (!video_stream)
    {
      fail("Could not find a suitable video stream.");
    }

    AVCodecContext* avcodec_context = avcodec_alloc_context3(codec);
    if (!avcodec_context || avcodec_parameters_to_context(avcodec_context, params) < 0)
      fail("Could not open avcodec context.");
    avcodec_context->thread_count = 10;
    avcodec_context->thread_type = FF_THREAD_FRAME;

    auto close_avcodec_context = on_exit([&] { avcodec_free_context(&avcodec_context); });

    if (avcodec_open2(avcodec_context, codec, nullptr) < 0)
      fail("Could not open codec.");

    auto close_codec = on_exit([&] { avcodec_close(avcodec_context); });

    AVFrame* frame = av_frame_alloc();
    auto free_frame = on_exit([&] { av_frame_free(&frame); });
    AVPacket* packet = av_packet_alloc();
    auto free_packet = on_exit([&] { av_packet_free(&packet); });

    while (av_read_frame(avformat_context, packet) >= 0)
    {
      if (packet->stream_index != *video_stream) {
        continue;
      }

      int frame_finished = 0;
      int response = avcodec_decode_video2(avcodec_context, frame, &frame_finished, packet);
      if (response < 0) {
        fail(std::format("Failed to decode packet: {}\n", averror(response)));
      }

      if (!frame_finished)
        continue;

      /*int response = avcodec_send_packet(avcodec_context, packet);
      if (response < 0) {
        fail(std::format("Failed to decode packet: {}\n", averror(response)));
      }

      response = avcodec_receive_frame(avcodec_context, frame);
      if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
        continue;
      }
      else if (response < 0) {
        fail(std::format("Failed to decode packet: {}\n", averror(response)));
      }*/

      av_packet_unref(packet);
      break;
    }
    {
      std::unique_lock lock(frame_data_mutex);
      frame_data.resize(frame->width, frame->height);
      size_changed = true;
      image_changed = true;
    }

    SwsContext* sws_context = sws_getContext(frame->width, frame->height, avcodec_context->pix_fmt,
      frame->width, frame->height, AV_PIX_FMT_RGB0, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    if (!sws_context)
      fail("Failed to allocate scaler context.");

    auto close_scaler = on_exit([&] { sws_freeContext(sws_context); });

    uint8_t* dest[4] = { frame_data.data[0].data(), NULL, NULL, NULL };
    int dest_linesize[4] = { frame->width * 4, 0, 0, 0 };
    {
      std::unique_lock lock(frame_data_mutex);
      sws_scale(sws_context, frame->data, frame->linesize, 0, frame->height, dest, dest_linesize);
      image_changed = true;
    }
    last_pts = frame->pts;
    int64_t last_pts_delta = 400;
    while (!stop.stop_requested())
    {
      auto frame_begin = std::chrono::steady_clock::now();

      auto n = std::chrono::steady_clock::now();
      int64_t ts = seek_ts.exchange(-1);
      if (ts != -1)
      {
        last_pts = ts;
        av_seek_frame(avformat_context, *video_stream, ts, AVSEEK_FLAG_BACKWARD);
      }

      while (av_read_frame(avformat_context, packet) >= 0)
      {
        if (packet->stream_index != *video_stream) {
          continue;
        }

        int frame_finished = 0;
        int response = avcodec_decode_video2(avcodec_context, frame, &frame_finished, packet);
        if (response < 0) {
          fail(std::format("Failed to decode packet: {}\n", averror(response)));
        }

        if (!frame_finished)
          continue;
      /*  response = avcodec_receive_frame(avcodec_context, frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
          continue;
        }
        else if (response < 0) {
          fail(std::format("Failed to decode packet: {}\n", averror(response)));
        }
        */
        
        av_packet_unref(packet);
        break;
      }
      {
        image_changed = false;
        sws_scale(sws_context, frame->data, frame->linesize, 0, frame->height, dest, dest_linesize);
        //std::unique_lock lock(frame_data_mutex);

        image_changed = true;
      }
      auto pts_delta = frame->pts - last_pts;
      last_pts = frame->pts;
      last_pts_delta = pts_delta;
      auto frame_end = frame_begin + std::chrono::duration<double>(pts_delta * time_base.num / float(time_base.den));
      std::this_thread::sleep_until(frame_end);
      std::format_to(std::ostreambuf_iterator(std::cout), "Rescale {}", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - n));

      while (!stop.stop_requested() && !play && seek_ts == -1)
      {
        std::this_thread::sleep_for(std::chrono::duration<double>(last_pts_delta * time_base.num / float(time_base.den)));
        std::this_thread::yield();
      }
    }
    });

  std::uint32_t texture = 0;
  std::uint32_t sampler = 0;
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);
  glTextureStorage2D(texture, 1, GL_RGBA8, frame_data.width, frame_data.height);
  glCreateSamplers(1, &sampler);
  glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
  glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  float color[4] = { 0,0,0,1 };
  glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, color);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  auto prog = myrt::make_program(
    "#version 460 core\nlayout(location=0) out vec2 uv;void main() { gl_Position = vec4(3*(gl_VertexID&1)-1, 3*(gl_VertexID&2)-1,0,1); uv=gl_Position.xy/2+0.5; }",
    R"(#version 460 core
layout(location=0) in vec2 uv;
      layout(location=0) uniform mat3 mat = mat3(1.0);
      layout(location=1) uniform vec2 kappa = vec2(0);
      layout(location=0) out vec4 col;layout(binding=0) uniform sampler2D img;void main() {
      
      vec2 u = (mat * vec3(uv, 1)).xy;

      vec2 dir = u - vec2(0.5);
      float r2 = dot(dir, dir);
      float r = sqrt(r2);
      dir = dir / r;
      
      float kappa1 = kappa[0];    
      float kappa2 = kappa[1];

      float rd = r * (1 + kappa1 * r2 + kappa2 * r2 * r2);
      vec2 p = vec2(0.5) + rd * dir;
      
      vec2 rem = p; col = texture(img, vec2(rem.x, 1-rem.y)); 
    })"
  );

 /* GLuint pbo = 0;
  glCreateBuffers(1, &pbo);*/

  std::uint32_t vao = 0;
  glCreateVertexArrays(1, &vao);

  std::uint32_t fbo = 0;
  glCreateFramebuffers(1, &fbo);
  glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, texture, 0);

  rnu::mat3 center_frame_mat(
    1, 0, 0,
    0, 1, 0,
    0, 1, 1
  );

  rnu::mat3 zoom_mat;

  rnu::vec2 kappa(0, 0);

  for (auto f : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      break;

    transform.set_window_size(window->getSize().x, window->getSize().y);
    
    if (size_changed.exchange(false))
    {
      std::unique_lock lock(frame_data_mutex);

      glDeleteTextures(1, &texture);
      glCreateTextures(GL_TEXTURE_2D, 1, &texture);
      glTextureStorage2D(texture, 8, GL_RGBA8, frame_data.width, frame_data.height);
    }

    if(image_changed.exchange(false)){
      //std::unique_lock lock(frame_data_mutex);
      transform.set_video_size(frame_data.width, frame_data.height);
      glTextureSubImage2D(texture, 0, 0, 0, frame_data.width, frame_data.height, GL_RGBA, GL_UNSIGNED_BYTE, frame_data.data.data());
      //lock.unlock();
      glGenerateTextureMipmap(texture);
    }

    auto vp = window->getView().getViewport();

    glViewport(0, 0, window->getSize().x, window->getSize().y);

    static std::optional<sf::Vector2i> mposstart;
    if (!mposstart && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
      if(!ImGui::GetIO().WantCaptureMouse)
        mposstart = sf::Mouse::getPosition();
    }
    else if (mposstart && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
      mposstart = std::nullopt;
    }
    else if(mposstart) {
      auto pos = sf::Mouse::getPosition();
      sf::Vector2i delta = pos - *mposstart;
      mposstart = pos;
      transform.move(delta.x, delta.y);
    }

    ImGui::Begin("Controls");
    bool p = play;
    ImGui::Checkbox("Play", &p);
    play = p;

    if (ImGui::Button("NPREV"))
    {
      seek_ts = std::max(0ll, last_pts - 10000);
    }
    if (ImGui::Button("PREV"))
    {
      seek_ts = std::max(0ll, last_pts - 1000);
    }
    if (ImGui::Button("NNEXT"))
    {
      seek_ts = last_pts + 10000;
    }
    if (ImGui::Button("NEXT"))
    {
      seek_ts = last_pts + 1000;
    }

    int pos = last_pts / 100;
    if (ImGui::SliderInt("LP", &pos, 0, duration / 100))
    {
      seek_ts = pos * 100;
    }

    ImGui::DragFloat2("Kappa", kappa.data(), 0.001, -10.0f, 10.0f);

    ImGui::End();

    auto const mat = transform.compute_transform();

    glBindTextureUnit(0, texture);
    glBindSampler(0, sampler);
    glUseProgram(*prog);
    glUniformMatrix3fv(0, 1, false, mat.data());
    glUniform2f(1, kappa[0], kappa[1]);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
}