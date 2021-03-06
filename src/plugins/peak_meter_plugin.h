/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Audio processor with event output example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PEAK_METER_PLUGIN_H
#define SUSHI_PEAK_METER_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr int MAX_METERED_CHANNELS = 2;

class PeakMeterPlugin : public InternalPlugin
{
public:
    PeakMeterPlugin(HostControl host_control);

    virtual ~PeakMeterPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    void _update_refresh_interval(float sample_rate);

    FloatParameterValue* _left_level;
    FloatParameterValue* _right_level;
    int _refresh_interval;
    int _sample_count{0};
    float _smoothing_coef{0.0f};
    std::array<float, MAX_METERED_CHANNELS> _smoothed{ {0.0f} };
};

}// namespace peak_meter_plugin
}// namespace sushi

#endif //SUSHI_PEAK_METER_PLUGIN_H
