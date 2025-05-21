//
// Created by user on 3/14/25.
//
#include "webrtc_apm_wrapper.h"

#include <modules/audio_processing/include/audio_processing.h>

class WebRTCApm {
public:
  ~WebRTCApm() {
    delete apm;
  }
  webrtc::AudioProcessing* apm{nullptr};
  webrtc::StreamConfig input_stream_config;
  webrtc::StreamConfig output_stream_config;
};

void* webrtc_apm_create()
{
  auto *handle = new WebRTCApm();
  handle->apm = webrtc::AudioProcessingBuilder().Create();
  return handle;
}

void webrtc_apm_destroy(void* apm) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (handle) {
    delete handle;
  }
}

void webrtc_apm_prepare(void* apm, int sample_rate,int channels) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return;
  }

  handle->input_stream_config = webrtc::StreamConfig(sample_rate, channels);
  handle->output_stream_config = webrtc::StreamConfig(sample_rate, channels);
}

void webrtc_apm_apply_config(void *apm, const APMConfig *config) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle || !config) {
    return;
  }

  webrtc::AudioProcessing::Config cfg;

  // Echo canceller
  cfg.echo_canceller.enabled = config->echo_canceller.enabled;
  cfg.echo_canceller.mobile_mode = config->echo_canceller.mobile_mode;
  cfg.echo_canceller.export_linear_aec_output =
      config->echo_canceller.export_linear_aec_output;
  cfg.echo_canceller.enforce_high_pass_filtering =
      config->echo_canceller.enforce_high_pass_filtering;

  // Noise suppression
  cfg.noise_suppression.enabled = config->noise_suppression.enabled;
  cfg.noise_suppression.level = static_cast<webrtc::AudioProcessing::Config::NoiseSuppression::Level>(
      config->noise_suppression.level);

  // High-pass filter
  cfg.high_pass_filter.enabled = config->high_pass_filter.enabled;

  // Gain controller 1 (classic AGC)
  cfg.gain_controller1.enabled = config->gain_controller.enabled;
  cfg.gain_controller1.mode = static_cast<webrtc::AudioProcessing::Config::GainController1::Mode>(
      config->gain_controller.mode);
  cfg.gain_controller1.target_level_dbfs = config->gain_controller.target_level_dbfs;
  cfg.gain_controller1.compression_gain_db = config->gain_controller.compression_gain_db;
  cfg.gain_controller1.enable_limiter = config->gain_controller.enable_limiter;

  // Voice detection
  cfg.voice_detection.enabled = config->voice_detection.enabled;

  // Transient suppression
  cfg.transient_suppression.enabled = config->transient_suppression.enabled;

  // Residual echo detector
  cfg.residual_echo_detector.enabled = config->residual_echo_detector.enabled;

  // Level estimation
  cfg.level_estimation.enabled = config->level_estimation.enabled;

  handle->apm->ApplyConfig(cfg);
}

void webrtc_apm_process_reverse_stream(void* apm, const float* const* src, float* const* dest) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return;
  }

  handle->apm->ProcessReverseStream(src, handle->input_stream_config, handle->output_stream_config, dest);
}

void webrtc_apm_process_stream(void* apm, const float* const* src, float* const* dest) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return;
  }

  handle->apm->ProcessStream(src, handle->input_stream_config, handle->output_stream_config, dest);
}

// --------------- 新增接口实现 ----------------
void webrtc_apm_set_stream_analog_level(void *apm, int level) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return;
  }
  handle->apm->set_stream_analog_level(level);
}

int webrtc_apm_get_stream_analog_level(void *apm) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return 0;
  }
  return handle->apm->recommended_stream_analog_level();
}

bool webrtc_apm_voice_detected(void *apm) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return false;
  }
  auto stats = handle->apm->GetStatistics();
  if (stats.voice_detected) {
    return *stats.voice_detected;
  }
  return false;
}

// 延迟补偿接口实现
void webrtc_apm_set_stream_delay_ms(void *apm, int delay_ms) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return;
  }
  handle->apm->set_stream_delay_ms(delay_ms);
}

int webrtc_apm_get_stream_delay_ms(void *apm) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return 0;
  }
  return handle->apm->stream_delay_ms();
}

// 键盘/按键提示接口实现
void webrtc_apm_set_key_pressed(void *apm, bool key_pressed) {
  auto* handle = static_cast<WebRTCApm*>(apm);
  if (!handle) {
    return;
  }
  handle->apm->set_stream_key_pressed(key_pressed);
}
