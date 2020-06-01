#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
// bool _is_send_syn,_is_finished ;
// uint16_t _window_size;
// size_t _time_to_out,_time_out;
// std::queue<TCPSegment> _outstanding{};
// uint64_t _ack_seq;
// unsigned int _consecutive_count;
using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _is_send_syn(0)
    , _is_finished(0)
    , _window_size(1)
    , _time_to_out(retx_timeout)
    , _time_out(retx_timeout)
    , _ack_seq(0)
    , _consecutive_count(0){}

uint64_t TCPSender::bytes_in_flight() const {
    return _next_seqno-_ack_seq;
}

//1. 从bytestream中读取数据
//2. 每个Segment尽量大不超过TCPConfig::MAX PAYLOAD SIZE.
void TCPSender::fill_window() {
    if(_is_finished)
        return;

    //1. 判断是syn
    if(!_is_send_syn){
        _is_send_syn = 1;
        TCPSegment segment;
        segment.header().syn = 1;
        segment.header().seqno = next_seqno();
        _next_seqno++;
        segments_out().push(segment);
        _outstanding.push(segment);
        return;
    }
    //2. 填满outstanding和segments_out
    //auto left = _ack_seq;
    auto right = _ack_seq+_window_size;
    while(_next_seqno<right){

        //拿去可用的buffer，放进滑动窗口内
        TCPSegment segment;
        segment.header().seqno = next_seqno();
        auto buf_size = min(MAX_PAYLOAD_SIZE,stream_in().buffer_size());
        buf_size = min(buf_size,right-next_seqno_absolute());
        Buffer buf{stream_in().read(buf_size)} ;
        segment.payload() = buf;
        _next_seqno+=buf_size;

        //判断是否结束
        if(stream_in().input_ended()&&stream_in().buffer_size()==0){
            _is_finished = 1;
            //TCPSegment segment;
            segment.header().fin = 1;
            //segment.header().seqno = next_seqno();
            _next_seqno++;
            segments_out().push(segment);
            _outstanding.push(segment);
            return;
        }else{
            if(buf_size==0){
                return;
            }
            segments_out().push(segment);
            _outstanding.push(segment);
        }
    }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    //1. 获得absAck
    auto absAck = unwrap(ackno, _isn, next_seqno_absolute());

    //无效判负
    if(absAck>_next_seqno){
        return false;
    }
    if (absAck < _ack_seq ){

        return true;
    }
    //2. 更新 window_size
    _window_size = window_size;
    //3. 更新 akcSeq，防止远古ack干扰
    _ack_seq = max(_ack_seq,absAck);

    //4. 维护segment队列
    while(_outstanding.size()){
        auto segment = _outstanding.front();
        if (unwrap(segment.header().seqno,_isn,next_seqno_absolute())+segment.length_in_sequence_space()<=_ack_seq){
            _outstanding.pop();
            //更新ack,清零consecutive,更新timeout
            _consecutive_count = 0;
            _time_out = _initial_retransmission_timeout;
            _time_to_out = _time_out;
        }else{
            break;
        }
    }
    //fill_window();
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {

    if(_time_to_out<=ms_since_last_tick){
        _consecutive_count++;
        _time_out*=2;
        auto segment = _outstanding.front();
        segments_out().push(segment);
        _time_to_out=_time_out;
    }else{
        _time_to_out -= ms_since_last_tick;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_count; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    segments_out().push(segment);
}
