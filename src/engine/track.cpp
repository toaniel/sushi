
#include "track.h"
#include "logging.h"
#include <iostream>
#include <string>

MIND_GET_LOGGER;

namespace sushi {
namespace engine {


Track::Track(int channels) : _input_buffer{std::max(channels, 2)},
                             _output_buffer{std::max(channels, 2)},
                             _input_busses{1},
                             _output_busses{1},
                             _multibus{false}
{
    _max_input_channels = channels;
    _max_output_channels = channels;
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init();
}

Track::Track(int input_busses, int output_busses) :  _input_buffer{std::max(input_busses, output_busses) * 2},
                                                     _output_buffer{std::max(input_busses, output_busses) * 2},
                                                     _input_busses{input_busses},
                                                     _output_busses{output_busses},
                                                     _multibus{(input_busses > 1 || output_busses > 1)}
{
    int channels = std::max(input_busses, output_busses) * 2;
    _max_input_channels = channels;
    _max_output_channels = channels;
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init();
}


bool Track::add(Processor* processor)
{
    if (_processors.size() >= TRACK_MAX_PROCESSORS || processor == this)
    {
        // If a track adds itself to its process chain, endless loops can arrise
        return false;
    }
    _processors.push_back(processor);
    processor->set_event_output(this);
    update_channel_config();
    return true;
}

bool Track::remove(ObjectId processor)
{
    for (auto plugin = _processors.begin(); plugin != _processors.end(); ++plugin)
    {
        if ((*plugin)->id() == processor)
        {
            (*plugin)->set_event_output(nullptr);
            _processors.erase(plugin);
            update_channel_config();
            return true;
        }
    }
    return false;
}

void Track::render()
{
    process_audio(_input_buffer, _output_buffer);
    for (int bus = 0; bus < _output_busses; ++bus)
    {
        apply_pan_and_gain(_output_buffer, _gain_parameters[bus]->value(), _pan_parameters[bus]->value());
    }
}

void Track::process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out)
{
    for (auto &processor : _processors)
    {
        while (!_event_buffer.empty()) // This should only contain keyboard/note events
        {
            RtEvent event;
            if (_event_buffer.pop(event))
            {
                processor->process_event(event);
            }
        }
        ChunkSampleBuffer in_bfr = ChunkSampleBuffer::create_non_owning_buffer(in, 0, processor->input_channels());
        ChunkSampleBuffer out_bfr = ChunkSampleBuffer::create_non_owning_buffer(out, 0, processor->output_channels());
        processor->process_audio(in_bfr, out_bfr);
        std::swap(in_bfr, out_bfr);
    }
    int output_channels = _processors.empty() ? _current_output_channels : _processors.back()->output_channels();
    ChunkSampleBuffer output = ChunkSampleBuffer::create_non_owning_buffer(in, 0, output_channels);
    out = output;

    /* If there are keyboard events not consumed, pass them on upwards
     * Rewrite the processor id of the events with that of the track. */
    while (!_event_buffer.empty())
    {
        RtEvent event;
        if (_event_buffer.pop(event))
        {
            switch (event.type())
            {
                case RtEventType::NOTE_ON:
                    output_event(RtEvent::make_note_on_event(this->id(), event.sample_offset(),
                                                             event.keyboard_event()->note(),
                                                             event.keyboard_event()->velocity()));
                    break;
                case RtEventType::NOTE_OFF:
                    output_event(RtEvent::make_note_off_event(this->id(), event.sample_offset(),
                                                              event.keyboard_event()->note(),
                                                              event.keyboard_event()->velocity()));
                    break;
                case RtEventType::NOTE_AFTERTOUCH:
                    output_event(RtEvent::make_note_aftertouch_event(this->id(), event.sample_offset(),
                                                                     event.keyboard_event()->note(),
                                                                     event.keyboard_event()->velocity()));
                    break;
                case RtEventType::WRAPPED_MIDI_EVENT:
                    output_event(RtEvent::make_wrapped_midi_event(this->id(), event.sample_offset(),
                                                                  event.wrapped_midi_event()->midi_data()));
                    break;

                default:
                    output_event(event);
            }
        }
    }
}

void Track::update_channel_config()
{
    int input_channels = _current_input_channels;
    int output_channels;

    for (unsigned int i = 0; i < _processors.size(); ++i)
    {
        input_channels = std::min(input_channels, _processors[i]->max_input_channels());
        if (input_channels != _processors[i]->input_channels())
        {
            _processors[i]->set_input_channels(input_channels);
        }
        if (i < _processors.size() - 1)
        {
            output_channels = std::min(_max_output_channels, std::min(_processors[i]->max_output_channels(),
                                                                      _processors[i+1]->max_input_channels()));
        }
        else
        {
            output_channels = std::min(_max_output_channels, std::min(_processors[i]->max_output_channels(),
                                                                      _current_output_channels));
        }
        if (output_channels != _processors[i]->output_channels())
        {
            _processors[i]->set_output_channels(output_channels);
        }
        input_channels = output_channels;
    }

    if (!_processors.empty())
    {
        auto last = _processors.back();
        int track_outputs = std::min(_current_output_channels, last->output_channels());
        if (track_outputs != last->output_channels())
        {
            last->set_output_channels(track_outputs);
        }
    }
}

void Track::process_event(RtEvent event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the first processor in the track */
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::WRAPPED_MIDI_EVENT:
            _event_buffer.push(event);
            break;

        default:
           break;
    }
}

void Track::set_bypassed(bool bypassed)
{
    for (auto& processor : _processors)
    {
        processor->set_bypassed(bypassed);
    }
    Processor::set_bypassed(bypassed);
}

void Track::send_event(RtEvent event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the next processor in the track */
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::WRAPPED_MIDI_EVENT:
            _event_buffer.push(event);
            break;

            /* Other events are passed on upstream unprocessed */
        default:
            output_event(event);
    }
}

void Track::_common_init()
{
    _processors.reserve(TRACK_MAX_PROCESSORS);
    _gain_parameters[0]  = register_float_parameter("gain_main", "Gain", 0.0f, -120.0f, 24.0f, new dBToLinPreProcessor(-120.0f, 24.0f));
    _pan_parameters[0]  = register_float_parameter("pan_main", "Pan", 0.0f, -1.0f, 1.0f, new FloatParameterPreProcessor(-1.0f, 1.0f));
    for (int bus = 1 ; bus < _output_busses; ++bus)
    {
        _gain_parameters[bus]  = register_float_parameter("gain_sub_" + std::to_string(bus), "Gain", 0.0f, -120.0f, 24.0f, new dBToLinPreProcessor(-120.0f, 24.0f));
        _pan_parameters[bus]  = register_float_parameter("pan_sub_" + std::to_string(bus), "Pan", 0.0f, -1.0f, 1.0f, new FloatParameterPreProcessor(-1.0f, 1.0f));
    }
}

void apply_pan_and_gain(ChunkSampleBuffer& buffer, float gain, float pan)
{
    float left_gain, right_gain;
    ChunkSampleBuffer left = ChunkSampleBuffer::create_non_owning_buffer(buffer, LEFT_CHANNEL_INDEX, 1);
    ChunkSampleBuffer right = ChunkSampleBuffer::create_non_owning_buffer(buffer, RIGHT_CHANNEL_INDEX, 1);
    if (pan < 0.0f) // Audio panned left
    {
        left_gain = gain * (1.0f + pan - PAN_GAIN_3_DB * pan);
        right_gain = gain * (1.0f + pan);
    }
    else            // Audio panned right
    {
        left_gain = gain * (1.0f - pan);
        right_gain = gain * (1.0f - pan + PAN_GAIN_3_DB * pan);
    }
    left.apply_gain(left_gain);
    right.apply_gain(right_gain);
}


} // namespace engine
} // namespace sushi
