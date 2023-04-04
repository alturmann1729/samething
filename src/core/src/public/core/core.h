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
/// This class allows the end-user to generate Specific Area Message Encoding
/// (SAME) headers. This is the class that all applications should use.
///
/// Some points to make note of here:
///
/// a) The sample rate is fixed to 44100Hz. There's really no good reason to go
///    above or below it. Unfortunately, the sample rate is not defined in the
///    specification.
///
/// b) Since we do not use any dynamic memory allocation, whenever a sample is
///    generated a function is called to handle the sample as the application
///    sees fit. Since the length of the header can vary, there's little choice
///    here.
///
/// c) Single-precision floating point is enforced; double precision is not
///    necessary and many embedded targets do not have double-precision FPUs
///    which would lead to an unnecessary performance drop and code size
///    increase as softfloat would be required.
///
/// \tparam SampleGeneratedCallback The function to call when a sample has been
/// generated. We do not use a function pointer as there's no good reason for a
/// user to change what function is called. This would also pessimize
/// optimization opportunities.
template <typename SampleGeneratedCallback>
class SAMEGenerator final {
 public:
  /// The maximum number of counties for each state.
  static constexpr int kCountiesNumMax = 70;

  /// The number of states we support.
  static constexpr int kStatesNumMax = 50;

  /// The shortest duration of an attention signal.
  static constexpr int kAttentionSignalMin = 8;

  /// The longest duration of an attention signal.
  static constexpr int kAttentionSignalMax = 25;

  /// The longest valid time period possible (hh'mm).
  static constexpr int kLongestValidTimePeriod = 24'00;

  /// This monstrocity of a table should be used to populate user interfaces.
  /// The interface should be designed such that a change here is automatically
  /// reflected in a user interface.
  static constexpr struct {
    const char* const name;
    int num_counties;
    const char* const county_names[kCountiesNumMax];
  } state_county_map[kStatesNumMax] = {
      {"Alabama",
       67,
       {"Autauga",    "Baldwin",    "Barbour",    "Bibb",       "Blount",
        "Bullock",    "Butler",     "Calhoun",    "Chambers",   "Cherokee",
        "Chilton",    "Choctaw",    "Clarke",     "Clay",       "Cleburne",
        "Coffee",     "Colbert",    "Conecuh",    "Coosa",      "Covington",
        "Crenshaw",   "Cullman",    "Dale",       "Dallas",     "DeKalb",
        "Elmore",     "Escambia",   "Etowah",     "Fayette",    "Franklin",
        "Geneva",     "Greene",     "Hale",       "Henry",      "Houston",
        "Jackson",    "Jefferson",  "Lamar",      "Lauderdale", "Lawrence",
        "Lee",        "Limestone",  "Lowndes",    "Macon",      "Madison",
        "Marengo",    "Marion",     "Marshall",   "Mobile",     "Monroe",
        "Montgomery", "Morgan",     "Perry",      "Pickens",    "Pike",
        "Randolph",   "Russell",    "St.Clair",   "Shelby",     "Sumter",
        "Talladega",  "Tallapoosa", "Tuscaloosa", "Walker",     "Washington",
        "Wilcox",     "Winston"}},
      {"Alaska",
       31,
       {"Aleutians East Borough",
        "Aleutians West Census Area",
        "Anchorage Municipality",
        "Bethel Census Area",
        "Bristol Bay Borough",
        "Chugach Census Area",
        "Copper River Census Area",
        "Denali Borough",
        "Dillingham Census Area",
        "Fairbanks North Star Borough",
        "Haines Borough",
        "Hoonah-Angoon Census Area",
        "Juneau City and Borough",
        "Kenai Peninsulua Borough",
        "Ketchikan Gateway Borough",
        "Kodiak Island Borough",
        "Kusilvak",
        "Lake and Peninsula Borough",
        "Matanuska-Susitna Borough",
        "Nome Census Area",
        "North Slope Borough",
        "Northwest Arctic Borough",
        "Petersburg Borough",
        "Prince of Wales-Hyder Census Area",
        "Sitka City and Borough",
        "Skagway Municipality",
        "Southeast Fairbanks Census Area",
        "Valdez-Cordova Census Area",
        "Wrangell City and Borough",
        "Yakutat City and Borough",
        "Yukon-Koyukuk Census Area"}},
      {"Arizona",
       15,
       {"Apache", "Cochise", "Coconino", "Gila", "Graham", "Greenlee", "La Paz",
        "Maricopa", "Mohave", "Navajo", "Pima", "Pinal", "Santa Cruz",
        "Yavapai", "Yuma"}}};

  /// Sets the attention signal duration.
  ///
  /// If this method fails, the original attention signal duration will be
  /// preserved.
  ///
  /// \param duration The desired duration of the attention signal.
  /// \return `false` if the attention signal duration is out of bounds, or
  /// `true` otherwise indicating success.
  [[nodiscard]] constexpr bool AttentionSignalDurationIsValid(
      const int duration) noexcept {
    if ((duration < kAttentionSignalMin) || (duration > kAttentionSignalMax)) {
      return false;
    }
    attention_signal_duration_ = duration;
    return true;
  }

  /// Sets the valid time period of a message.
  ///
  /// \param time_period The desired valid time period.
  /// \return `false` if either the time period exceeds 2400 (24 hours) or the
  [[nodiscard]] constexpr bool TimePeriodSet(const int time_period) noexcept {
    if (time_period > kLongestValidTimePeriod) {
      return false;
    }

    const int segments = (time_period <= 100) ? 15 : 30;
    const int minutes = time_period % 100;

    if ((minutes % segments) == 0) {
      return false;
    }

    valid_time_period_[0] = ((time_period / 1000) % 10) + '0';
    valid_time_period_[1] = ((time_period / 100) % 10) + '0';
    valid_time_period_[2] = ((time_period / 10) % 10) + '0';
    valid_time_period_[3] = (time_period % 10) + '0';
  }

  /// Generates a SAME header.
  constexpr void Generate() noexcept {
    // The transmission is as follows:
    //
    // [PREAMBLE]ZCZC-ORG-EEE-PSSCCC+TTTT-JJJHHMM-LLLLLLLL-(1 second silence)
    // [PREAMBLE]ZCZC-ORG-EEE-PSSCCC+TTTT-JJJHHMM-LLLLLLLL-(1 second silence)
    // [PREAMBLE]ZCZC-ORG-EEE-PSSCCC+TTTT-JJJHHMM-LLLLLLLL-(1 second silence)
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
    constexpr uint8_t PREAMBLE = 0b1010'1011;
    constexpr int kNumBursts = 3;

    uint8_t header_data_[252] = {};

    constexpr uint8_t eom[] = {PREAMBLE, PREAMBLE, PREAMBLE, PREAMBLE, PREAMBLE,
                               PREAMBLE, PREAMBLE, PREAMBLE, PREAMBLE, PREAMBLE,
                               PREAMBLE, PREAMBLE, PREAMBLE, PREAMBLE, PREAMBLE,
                               PREAMBLE, 'N',      'N',      'N',      'N'};

    // Start from here
    int header_offset = 29;

#if 0
    for (int i = 0; i < info.PSCC; ++i) {
      memcpy(&header_data[header_offset], info.PSSCCC[i], 6);
      header_offset += 7;
    }
    memcpy(&header_data[40], valid_time_period_, sizeof(valid_time_period_));
    memcpy(&header_data[45], info.JJJHHMM, sizeof(info.JJJHHMM));
#endif

    for (int bursts = 0; bursts < kNumBursts; ++bursts) {
      AFSKGenerate(header_data_, header_offset);
      SilenceGenerate();
    }
    AttentionSignalGenerate();
    SilenceGenerate();

    for (int bursts = 0; bursts < kNumBursts; ++bursts) {
      AFSKGenerate(eom, 20);
      SilenceGenerate();
    }
  }

 private:
  static constexpr int kSSCCCLength = 5;

  static constexpr const char ssccc_map[kStatesNumMax][kCountiesNumMax][kSSCCCLength] = {
      { // Alabama
       {'0', '1', '0', '0', '1'}, // Autauga
       {'0', '1', '0', '0', '3'}, // Baldwin
       {'0', '1', '0', '0', '5'}, // Barbour
       {'0', '1', '0', '0', '7'}, // Bibb
       {'0', '1', '0', '0', '9'}, // Blount
       {'0', '1', '0', '1', '1'}, // Bullock
       {'0', '1', '0', '1', '3'}, // Butler
       {'0', '1', '0', '1', '5'}, // Calhoun
       {'0', '1', '0', '1', '7'}, // Chambers
      }
  };

  // The Preamble and EAS codes must use Audio Frequency Shift Keying at a
  // rate of 520.83 bits per second to transmit the codes.
  static constexpr float kBitRate = 520.83F;

  // Mark and space time must be 1.92 milliseconds.
  static constexpr float kBitDuration = 1.0F / kBitRate;

  // The maximum number of samples we will ever output.
  static constexpr float kSampleRate = 44100.0F;

  constexpr void AFSKGenerate(const uint8_t* const data,
                              const int data_size) noexcept {
    constexpr float kMarkFreq = 2083.3F;
    constexpr float kSpaceFreq = 1562.5F;

    for (int i = 0; i < data_size; ++i) {
      for (int bit_pos = 0; bit_pos < 8; ++bit_pos) {
        const int bit = (data[i] >> bit_pos) & 1;
        SineGenerate(kBitDuration, bit ? kMarkFreq : kSpaceFreq);
      }
    }
  }

  // Generates an attention signal.
  constexpr void AttentionSignalGenerate() noexcept {
    const int num_samples =
        static_cast<int>(attention_signal_duration_ * kSampleRate);

    for (int sample_num = 0; sample_num < num_samples; ++sample_num) {
      ;
    }
  }

  /// Generates a sine wave.
  ///
  /// \param duration How many seconds the sine wave will last for.
  /// \param freq The frequency of the sine wave.
  constexpr void SineGenerate(const float duration, const float freq) noexcept {
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

  // The function to call when a sample has been generated.
  SampleGeneratedCallback sample_generated_cb_;

  int attention_signal_duration_;
};
}  // namespace samething
