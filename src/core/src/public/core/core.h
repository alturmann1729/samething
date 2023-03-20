// SPDX-License-Identifier: MIT
//
// Copyright 2023 alturmann1729
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cmath>
#include <cstring>

namespace samething {
struct HeaderInfo {
  /// How many seconds will the attention signal last?
  int attention_signal_num_secs;

  /// Location code - Indicates the geographic area affected by the EAS alert.
  /// There may be 31 location codes in an EAS alert.
  ///
  /// P - County subdivision
  /// SS - State
  /// CCC - County or City
  char PSSCCC[31][6];

  /// This is the identification of the EAS participant, NWS office, etc.,
  /// transmitting or retransmitting the message. These codes will be
  /// automatically affixed to all outgoing messages by the EAS encoder.
  char LLLLLLLL[8];

  /// This is the day in Julian Calendar days (JJJ) of the year and the time in
  /// hours and minutes (HHMM) when the message was initially released by the
  /// originator using 24 hour Universal Coordinated Time (UTC).
  char JJJHHMM[7];

  /// +TTTT: Valid time period of a message in 15 minute segments up to one hour
  /// and then in 30 minute segments beyond one hour.
  char TTTT[4];

  /// Originator code - indicates who originally initiated the activation of the
  /// EAS.
  char ORG[3];

  /// Event code - Indicates the nature of the EAS activation.
  char EEE[3];
};

enum class ReturnCodes {
  kNoError,
#ifdef SAMETHING_ENFORCE_MSG_VALIDITY
  kNoLocationCodes,

  /// The application has requested an attention signal duration outside of the
  /// range of 8 and 25.
  kInvalidAttentionSignalDuration,

  /// The application has requested a non-standard originator code.
  kInvalidORG,

  /// The application has requested a non-standard event code.
  kInvalidEEE,

  /// The application has requested a non-standard county subdivision.
  kInvalidP,

  /// The application has requested a non-standard state.
  kInvalidSS,

  /// The application has requested a non-standard county or city code (unused).
  kInvalidCCC,

  /// The application has requested an invalid time period.
  kInvalidTTTT,

  /// The application has specified an invalid originator release time.
  kInvalidJJJHHMM,

  /// The application has specified an invalid station identification.
  kInvalidLLLLLLLL
#endif  // SAMETHING_ENFORCE_MSG_VALIDITY
};

/// This class allows the end-user to generate Specific Area Message Encoding
/// (SAME) headers. This is the class that all applications should use.
///
/// Some points to make note of here:
///
/// a) The sample rate is fixed to 44100Hz. There's really no good reason to go
///    above or below it.
///
/// b) Since we do not use any dynamic memory allocation, whenever a sample is
///    generated a function is called to handle the sample as the application
///    sees fit. Since the length of the header can vary, there's little choice
///    here. Otherwise, we would've just dumped everything into a fixed-size
///    buffer.
///
/// c) In the interest of reducing code size and improving performance, checking
///    to see if the given header contains standard compliant header codes (with
///    the exception of the CCC field) is disabled by default.
///
///    However, this can be enabled at compile-time by defining
///    SAMETHING_ENFORCE_MSG_VALIDITY. The idea is that the application
///    author(s) test their code to ensure a user can only pass standard codes.
///    If one doesn't care about that though, then it's in their best interest
///    to leave this undefined.
///
/// d) Single-precision floating point is enforced; double precision is not
///    necessary and many embedded targets do not have double-precision FPUs
///    which would lead to an unnecessary performance drop and code size
///    increase as softfloat would be required.
///
/// \tparam SampleGeneratedCallback The function to call when a sample has been
/// generated. We do not use a function pointer as there's no good reason for a
/// user to change what function is called. This would also pessimize
/// optimization opportunities.
template <typename SampleGeneratedCallback>
class SAMEGenerator {
 public:
  /// Generates a SAME header.
  ///
  /// \param info The header.
  /// \return ReturnCodes::kNoError if no error occurred.
  [[nodiscard]] constexpr ReturnCodes Generate(HeaderInfo& info) noexcept {
#ifdef SAMETHING_ENFORCE_MSG_VALIDITY
    if (!AttentionSignalDurationValid(info.attention_signal_num_secs)) {
      return ReturnCodes::kInvalidAttentionSignalDuration;
    }

    if (!ORGCodeValid(info.ORG)) {
      return ReturnCodes::kInvalidORG;
    }

    if (!EEECodeValid(info.EEE)) {
      return ReturnCodes::kInvalidEEE;
    }

    if (!TTTTValid(info.TTTT)) {
      return ReturnCodes::kInvalidTTTT;
    }

    // XXX: We don't check for PSSCCC validity here, that's done after this.

    if (!JJJHHMMValid(info.JJJHHMM)) {
      return ReturnCodes::kInvalidJJJHHMM;
    }

    if (!LLLLLLLLValid(info.LLLLLLLL)) {
      return ReturnCodes::kInvalidLLLLLLLL;
    }

    // XXX: County ANSI numbers are contained in the State EAS Mapbook; we won't
    // bother checking those for now as there are simply far too many to go
    // through.
#endif  // SAMETHING_ENFORCE_MSG_VALIDITY

    // The transmission is as follows:
    //
    // [PREAMBLE]ZCZC-ORG-EEE-PSSCCC + TTTT-JJJHHMM-LLLLLLLL-(1 second silence)
    // [PREAMBLE]ZCZC-ORG-EEE-PSSCCC + TTTT-JJJHHMM-LLLLLLLL-(1 second silence)
    // [PREAMBLE]ZCZC-ORG-EEE-PSSCCC + TTTT-JJJHHMM-LLLLLLLL-(1 second silence)
    // (transmission of 8 to 25 seconds of Attention Signal)
    // (transmission of audio, video or text messages) (not implemented)
    // (at least a one second pause)
    // [PREAMBLE]NNNN (one second pause)
    // [PREAMBLE]NNNN (one second pause)
    // [PREAMBLE]NNNN (at least one second pause)
    //
    // PREAMBLE is a consecutive string of bits (sixteen bytes of AB hexadecimal
    // [8 bit byte 10101011]) sent to clear the system, set AGC and set
    // asynchronous decoder clocking cycles. The preamble must be transmitted
    // before each header and End of Message code.

    uint8_t header_data[80] = {
        0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011,
        0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011,
        0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011,
        0b1010'1011, 'Z',         'C',         'Z',         'C',
        '-',         'O',         'R',         'G',         '-',
        'E',         'E',         'E',         '-',         'P',
        'S',         'S',         'C',         'C',         'C',
        '-'};

    constexpr uint8_t eom[] = {
        0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011,
        0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011,
        0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011, 0b1010'1011,
        0b1010'1011, 'N',         'N',         'N',         'N'};

    int header_offset = 29;

    for (int i = 0; i < 31; ++i) {
#ifdef SAMETHING_ENFORCE_MSG_VALIDITY
      if (!*info.PSSCCC[i]) {
        if (i == 0) {
          // The end of location codes marker was seen, but we're at the first
          // entry of the PSSCCC field. Therefore, no locations were specified.
          return ReturnCodes::kNoLocationCodes;
        }
      }

      if ((info.PSSCCC[i][0] - '0') > 9) {
        return ReturnCodes::kInvalidP;
      }
#endif  // SAMETHING_ENFORCE_MSG_VALIDITY
      memcpy(&header_data[header_offset], info.PSSCCC[i], 6);
      header_offset += 7;
    }

    memcpy(&header_data[22], info.ORG, sizeof(info.ORG));
    memcpy(&header_data[26], info.EEE, sizeof(info.EEE));
    memcpy(&header_data[40], info.TTTT, sizeof(info.TTTT));
    memcpy(&header_data[45], info.JJJHHMM, sizeof(info.JJJHHMM));

    for (int i = 0; i < 3; ++i) {
      AFSKGenerate(header_data, header_offset);
      SilenceGenerate();
    }
    AttentionSignalGenerate(info.attention_signal_num_secs);
    SilenceGenerate();

    for (int i = 0; i < 3; ++i) {
      AFSKGenerate(eom, 20);
      SilenceGenerate();
    }
    return ReturnCodes::kNoError;
  }

 private:
  // The Preamble and EAS codes must use Audio Frequency Shift Keying at a
  // rate of 520.83 bits per second to transmit the codes.
  static constexpr float kBitRate = 520.83F;

  // Mark and space time must be 1.92 milliseconds.
  static constexpr float kBitDuration = 1.0F / kBitRate;

  // The maximum number of samples we will ever output.
  static constexpr float kSampleRate = 44100.0F;

  constexpr void AFSKGenerate(const std::uint8_t* const data,
                              const size_t data_size) noexcept {
    constexpr float kMarkFreq = 2083.3F;
    constexpr float kSpaceFreq = 1562.5F;

    for (size_t i = 0; i < data_size; ++i) {
      for (int bit_pos = 0; bit_pos < 8; ++bit_pos) {
        const int bit = (data[i] >> bit_pos) & 1;
        SineGenerate(kBitDuration, bit ? kMarkFreq : kSpaceFreq);
      }
    }
  }

  /// Generates an attention signal.
  ///
  /// \param num_secs How long will the attention signal last for in seconds?
  constexpr void AttentionSignalGenerate(const float num_secs) noexcept {
    const int num_samples = static_cast<int>(num_secs * kSampleRate);

    for (int sample_num = 0; sample_num < num_samples; ++sample_num) {
      ;
    }
  }

  /// Generates a sine wave.
  ///
  /// \param duration How many seconds the sine wave will last for.
  /// \param freq The frequency of the sine wave.
  constexpr void SineGenerate(float duration, float freq) noexcept {
    const int num_samples = static_cast<int>(duration * kSampleRate);

    for (int sample_num = 0; sample_num < num_samples; ++sample_num) {
      const float t = static_cast<float>(sample_num) / kSampleRate;
      const float wave = sinf(M_PI * 2 * t * freq);

      sample_generated_cb_(wave);
    }
  }

  constexpr void SilenceGenerate() noexcept {
    const int num_samples = static_cast<int>(kSampleRate * kBitDuration);

    for (int sample_num = 0; sample_num < num_samples; ++sample_num) {
      sample_generated_cb_(0.0F);
    }
  }

#ifdef SAMETHING_ENFORCE_MSG_VALIDITY
  /// Helper function to determine if a string exists within an array.
  ///
  /// \param str The string to check for in the array.
  /// \param array The array to search in.
  /// \param array_size The size of the array we're searching in.
  [[nodiscard]] constexpr bool StringExistsIn(
      const char* const str, const char* const array,
      const int array_size) const noexcept {
    assert(str);
    assert(array);
    assert(array_size > 0);

    if (!*str) {
      return false;
    }

    for (int i = 0; i < array_size; ++i) {
      if (strcmp(&array[i], str) == 0) {
        return true;
      }
    }
    return false;
  }

  /// Verifies that the given attention signal duration is valid.
  ///
  /// A valid attention signal duration is between the range of 8 to 25.
  ///
  /// \param attention_signal_range The given attention signal duration.
  /// \return `true` if the attention signal duration is valid, `false`
  /// otherwise.
  [[nodiscard]] constexpr bool AttentionSignalDurationValid(
      const int attention_signal_duration) const noexcept {
    return (attention_signal_duration >= 8) &&
           (attention_signal_duration <= 25);
  }

  /// Verifies that the given originator code (ORG) is valid.
  ///
  /// \param org The originator code.
  /// \return `true` if the originator code is valid, `false` otherwise.
  [[nodiscard]] constexpr bool ORGCodeValid(
      const char* const org) const noexcept {
    assert(org);

    constexpr const char* valid_org_codes[] = {"EAS", "CIV", "WXR", "PEP"};
    return StringExistsIn(org, *valid_org_codes, 4);
  }

  /// Verifies that the given event code (EEE) is valid.
  ///
  /// \param eee The event code.
  /// \return `true` if the event code is valid, `false` otherwise.
  [[nodiscard]] constexpr bool EEECodeValid(
      const char* const eee) const noexcept {
    assert(eee);

    constexpr const char* valid_eee_codes[] = {
        "EAN", "NPT", "RMT", "RWT", "ADR", "AVW", "AVA", "BZW", "BLU", "CAE",
        "CDW", "CEM", "CFW", "CFA", "DSW", "EQW", "EVI", "EWW", "FRW", "FFW",
        "FFA", "FFS", "FLW", "FLA", "FLS", "HMW", "HWW", "HWA", "HUW", "HUA",
        "HLS", "LEW", "LAE", "NMN", "TOE", "NUW", "DMO", "RHW", "SVR", "SVA",
        "SVS", "SPW", "SMW", "SPS", "SSA", "SSW", "TOR", "TOA", "TRW", "TRA",
        "TSW", "TSA", "VOW", "WSW", "WSA"};
    return StringExistsIn(eee, *valid_eee_codes, 55);
  }

  /// Verifies that the given TTTT (valid time period) is valid.
  ///
  /// \param tttt The time period.
  /// \return `true` if the time period is valid, `false` otherwise.
  [[nodiscard]] constexpr bool TTTTValid(
      const char* const tttt) const noexcept {
    assert(tttt);

    if (!*tttt) {
      return false;
    }

    const long value = strtol(tttt, nullptr, 10);

    // XXX: I really doubt that the TTTT field is supposed to be greater than 24
    // hours, but I have no proof of that. We'll leave it alone for now.

    const int segments = (value <= 100) ? 15 : 30;
    const long minutes = value % 100;

    return (minutes % segments) == 0;
  }

  [[nodiscard]] constexpr bool JJJHHMMValid(
      const char* const jjjhhmm) const noexcept {
    assert(jjjhhmm);

    if (!*jjjhhmm) {
      return false;
    }
    return true;
  }

  [[nodiscard]] constexpr bool LLLLLLLLValid(
      const char* const llllllll) const noexcept {
    assert(llllllll);

    if (!*llllllll) {
      return false;
    }
    return true;
  }
#endif  // SAMETHING_ENFORCE_MSG_VALIDITY

  /// The function to call when a sample has been generated.
  SampleGeneratedCallback sample_generated_cb_;
};
}  // namespace samething
