// sound_immersive_test.cpp — Immersive Sound (§16.4.2 AudioClip, §16.4.17 Sound)
// on the built-in backend: the ellipsoid and Buffer fixtures shared with the
// swap-test, the WAV decoder, and a Sound{AudioClip} scene through SoundSystem
// (url -> resolver -> decoder -> Buffer; isActive/isPaused/pitch -> playback).

#include "AudioBackend.hpp"
#include "SoundSystem.hpp"
#include "RecordingBackend.hpp"
#include "MediaTimeSystem.hpp"
#include "WavDecoder.hpp"
#include "dsp/BuiltinDspBackend.hpp"
#include "tests/dsp_metrics.hpp"
#include "tests/immersive_fixtures.hpp"

#include "X3DExecutionContext.hpp"

#include "x3d/nodes/AudioClip.hpp"
#include "x3d/nodes/MovieTexture.hpp"
#include "x3d/nodes/Sound.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

using namespace x3d::core;
using namespace x3d::nodes;
using namespace x3d::runtime;
using x3d::test::goertzel;
using x3d::test::rmsStereo;

static int g_failures = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__);    \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

// A RIFF/WAVE file: `channels` interleaved 16-bit PCM of a 1 kHz sine at `rate`.
static std::vector<std::uint8_t> makeWav16(int rate, int channels, int frames) {
  std::vector<std::uint8_t> b;
  auto u32 = [&](std::uint32_t v) { for (int i = 0; i < 4; ++i) b.push_back(std::uint8_t(v >> (8 * i))); };
  auto u16 = [&](std::uint16_t v) { b.push_back(std::uint8_t(v)); b.push_back(std::uint8_t(v >> 8)); };
  const std::uint32_t data = std::uint32_t(frames * channels * 2);
  b.insert(b.end(), {'R', 'I', 'F', 'F'}); u32(36 + data);
  b.insert(b.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '}); u32(16);
  u16(1); u16(std::uint16_t(channels)); u32(std::uint32_t(rate));
  u32(std::uint32_t(rate * channels * 2)); u16(std::uint16_t(channels * 2)); u16(16);
  b.insert(b.end(), {'d', 'a', 't', 'a'}); u32(data);
  for (int i = 0; i < frames; ++i) {
    const auto s = std::int16_t(std::lround(16384.0 * std::sin(2.0 * 3.14159265358979 * 1000.0 * i / rate)));
    for (int c = 0; c < channels; ++c) u16(std::uint16_t(s));
  }
  return b;
}

static void testWav() {
  const DecodedAudio a = io::wav::decodeWav(makeWav16(8000, 2, 4000));
  CHECK(a.ok, "wav: 16-bit stereo decodes");
  CHECK(a.samples.size() == 4000, "wav: stereo downmixed to one sample per frame");
  CHECK(a.sampleRate == 8000.0f, "wav: sample rate read from fmt");
  CHECK(std::fabs(a.duration() - 0.5) < 1e-9, "wav: duration = frames / rate");
  CHECK(std::fabs(a.samples[2] - 16384.0f / 32768.0f) < 1e-3f, "wav: 16-bit scaled to [-1,1]");
  CHECK(!io::wav::decodeWav({'n', 'o', 'p', 'e'}).ok, "wav: garbage is rejected");
  std::vector<std::uint8_t> trunc = makeWav16(8000, 1, 100);
  trunc.resize(30);
  CHECK(!io::wav::decodeWav(trunc).ok, "wav: a truncated header is rejected");
}

static void testSoundClipScene() {
  auto clip = std::make_shared<AudioClip>();
  clip->setUrl(MFString{"missing.wav", "tone.wav"});
  auto snd = std::make_shared<Sound>();
  snd->setSource(clip);

  const std::vector<std::uint8_t> wav = makeWav16(8000, 1, 8000);
  int fetches = 0;
  SoundSystem sys(std::make_shared<BuiltinDspBackend>());
  sys.setAssetResolver([&](const std::string &url, extract::AssetKind kind) {
    ++fetches;
    CHECK(kind == extract::AssetKind::Audio, "scene: clip fetched as AssetKind::Audio");
    return url == "tone.wav" ? extract::AssetResult::makeReady(wav) : extract::AssetResult::makeFailed();
  });
  sys.setAudioDecoder(io::wav::makeWavDecoder());
  X3DExecutionContext ctx;  // no Viewpoint: listener at the origin facing -Z
  sys.attach(snd.get(), ctx);
  CHECK(fetches == 2, "scene: url list tried in order until one loads");
  CHECK(sys.destinationCount() == 1, "scene: Sound owns a stereo destination");

  std::vector<float> lr;
  sys.update(0.0, ctx);
  sys.renderStereo(x3d::test::kImmFrames, x3d::test::kImmSR, lr);
  CHECK(rmsStereo(lr) < 1e-6, "scene: an inactive AudioClip is silent");

  clip->emitIsActive(true);
  sys.update(0.1, ctx);
  sys.renderStereo(x3d::test::kImmFrames, x3d::test::kImmSR, lr);
  const double level = rmsStereo(lr);
  CHECK(level > 0.05, "scene: isActive plays the clip (listener inside minFront)");
  std::vector<float> left(lr.size() / 2);
  for (std::size_t i = 0; i < left.size(); ++i) left[i] = lr[2 * i];
  CHECK(goertzel(left, 1000.0, x3d::test::kImmSR) > 20.0 * goertzel(left, 2500.0, x3d::test::kImmSR),
        "scene: the decoded 1 kHz tone reaches the output");

  // Move the Sound 5 m down its own +Z (the listener is at the origin): between
  // the ellipsoids, -20 dB * 4/9.
  snd->setLocation(SFVec3f{0, 0, -5});
  sys.update(0.2, ctx);
  sys.renderStereo(x3d::test::kImmFrames, x3d::test::kImmSR, lr);
  const double want = x3d::test::ellipsoidExpected(5.0, 1.0, 10.0);
  CHECK(std::fabs(rmsStereo(lr) / level - want) < 0.05 * want,
        "scene: Sound.location moves the source through the ellipsoid falloff");

  clip->emitIsPaused(true);
  sys.update(0.3, ctx);
  sys.renderStereo(x3d::test::kImmFrames, x3d::test::kImmSR, lr);
  CHECK(rmsStereo(lr) < 1e-6, "scene: isPaused silences the clip");
}

static void testActivePitchStaysAtActivationRate() {
  auto clip = std::make_shared<AudioClip>();
  clip->setUrl(MFString{"tone.wav"});
  auto snd = std::make_shared<Sound>();
  snd->setSource(clip);
  auto backend = std::make_shared<RecordingBackend>();
  SoundSystem sound(backend);
  sound.setAssetResolver([&](const std::string &, extract::AssetKind) {
    return extract::AssetResult::makeReady(makeWav16(8000, 1, 48000));
  });
  sound.setAudioDecoder(io::wav::makeWavDecoder());
  X3DExecutionContext ctx;
  auto media = std::make_shared<MediaTimeSystem>();
  media->attach(clip.get(), ctx);
  ctx.addSystem(media);
  sound.attach(snd.get(), ctx);
  ctx.tick(0.0);
  sound.update(0.0, ctx);
  clip->setPitch(3.0f); // direct write cannot change an active clip's playback rate.
  ctx.tick(1.0);
  sound.update(1.0, ctx);
  float rate = -1.0f;
  for (const auto &p : backend->setParams)
    if (p.param == Param::PlaybackRate) rate = p.value;
  CHECK(rate == 1.0f, "scene: active AudioClip retains its activation pitch in the backend");
}

static void testMovieTextureSourceLifecycle() {
  auto movie = std::make_shared<MovieTexture>();
  movie->setUrl(MFString{"movie.mpg"});
  movie->setLoad(false);
  movie->setStartTime(1.0);
  movie->setStopTime(4.0);
  movie->setSpeed(2.0f);
  auto snd = std::make_shared<Sound>();
  snd->setSource(movie);
  SoundSystem sound(std::make_shared<BuiltinDspBackend>());
  int fetches = 0;
  sound.setAssetResolver([&](const std::string &, extract::AssetKind kind) {
    ++fetches;
    CHECK(kind == extract::AssetKind::Movie, "movie source resolves as movie media");
    return extract::AssetResult::makeReady({1});
  });
  sound.setMovieAudioDecoder([](const std::vector<std::uint8_t> &) {
    DecodedAudio audio;
    audio.ok = true;
    audio.sampleRate = 48000.0f;
    audio.samples.resize(48000);
    for (std::size_t i = 0; i < audio.samples.size(); ++i)
      audio.samples[i] = 0.5f * std::sin(2.0 * 3.141592653589793 * 1000.0 * i / 48000.0);
    return audio;
  });
  X3DExecutionContext ctx;
  auto media = std::make_shared<MediaTimeSystem>();
  media->attach(movie.get(), ctx);
  ctx.addSystem(media);
  reportMovieDuration(ctx, *movie, 6.0);
  ctx.process();
  sound.attach(snd.get(), ctx);
  std::vector<float> lr;
  auto levelAt = [&](double t) {
    ctx.tick(t);
    sound.update(t, ctx);
    sound.renderStereo(x3d::test::kImmFrames, x3d::test::kImmSR, lr);
    return rmsStereo(lr);
  };
  CHECK(levelAt(0.0) < 1e-6, "movie audio is silent before activation");
  CHECK(fetches == 0, "load FALSE defers movie audio fetch");
  movie->setLoad(true);
  CHECK(levelAt(1.0) > 0.05, "movie audio plays while active");
  CHECK(fetches == 1, "movie audio is fetched once when load becomes TRUE");
  movie->setSpeed(4.0f); // active speed remains captured at 2.
  CHECK(levelAt(1.5) > 0.05, "movie audio keeps playing after active speed write");
  movie->setPauseTime(2.0);
  CHECK(levelAt(2.0) < 1e-6, "movie audio pauses with MovieTexture");
  movie->setResumeTime(3.0);
  CHECK(levelAt(3.0) > 0.05, "movie audio resumes with MovieTexture");
  CHECK(levelAt(4.0) < 1e-6, "movie audio stops with MovieTexture");
}

// ADR-0051 detach: a MovieTexture whose audio is still pending (load FALSE)
// is removed with its Inline and freed; later ticks must not touch it (the
// sanitizer build turns a stale pending entry into a use-after-free report).
static void testDetachDropsPendingMovie() {
  auto movie = std::make_shared<MovieTexture>();
  movie->setUrl(MFString{"movie.mpg"});
  movie->setLoad(false);
  auto snd = std::make_shared<Sound>();
  snd->setSource(movie);
  SoundSystem sound(std::make_shared<BuiltinDspBackend>());
  int fetches = 0;
  sound.setAssetResolver([&](const std::string &, extract::AssetKind) {
    ++fetches;
    return extract::AssetResult::makeReady({1});
  });
  sound.setMovieAudioDecoder([](const std::vector<std::uint8_t> &) { return DecodedAudio{}; });
  X3DExecutionContext ctx;
  sound.attach(snd.get(), ctx);
  sound.update(0.0, ctx);
  sound.detach(movie.get(), ctx);
  sound.detach(snd.get(), ctx);
  snd->setSource(nullptr);
  movie.reset();  // the unloaded content is freed
  sound.update(1.0, ctx);
  CHECK(fetches == 0, "a detached pending movie is never retried");
}

int main() {
  x3d::test::runImmersiveFixtures([] { return std::make_shared<BuiltinDspBackend>(); },
                                  [](bool ok, const char *msg) { CHECK(ok, msg); });
  testWav();
  testSoundClipScene();
  testActivePitchStaysAtActivationRate();
  testMovieTextureSourceLifecycle();
  testDetachDropsPendingMovie();
  if (g_failures == 0) std::fprintf(stderr, "sound_immersive_test: ALL PASS\n");
  return g_failures == 0 ? 0 : 1;
}
