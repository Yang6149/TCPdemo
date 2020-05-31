#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _Unassembled(), _firstUnassembled(0), _nUnassembled(0), _capacity(capacity), _eof(0) {
    std::cout<<"data"<<"\t"<<"index"<<"\n";
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.

void StreamReassembler::push_substring(const std::string &data, const size_t index, const bool eof) {
    std::cout<<data<<"\t"<<index<<"\n";
    _eof = _eof | eof;
    if (data.size() > _capacity) {
        _eof = 0;
    }
    if (data.empty() || index + data.size() <= _firstUnassembled) {
        if (_eof)
            _output.end_input();
        return;
    }
    //这一次最大能存到的index
    size_t firstUnacceptable = _firstUnassembled + (_capacity - _output.buffer_size());

    //不能直接影响_output的情形，储存的数据提前优化处理
    std::set<typeUnassembled>::iterator iter2;
    size_t resIndex = index;
    auto resData = std::string(data);
    //把resData 接收过的部分cut掉
    if (resIndex < _firstUnassembled) {
        resData = resData.substr(_firstUnassembled - resIndex);
        resIndex = _firstUnassembled;
    }
    //如果这一次的数据大于最大能存到的数据，也截掉一部分

    //检查set当中是否有刚好可以连接在一起的segment，有的话连接在一起
    for (iter2 = _Unassembled.begin(); iter2 != _Unassembled.end();) {
        if (size_t deleteNum = merge_substring(resIndex, resData, iter2)) {  //返回值是删掉重合的bytes数
            _Unassembled.erase(iter2++);
            _nUnassembled -= deleteNum;
        } else
            iter2++;
    }
    //如果当前Data里面的数据比capacity-size要大的话，就进行消减然后Insert到Set中
    if (resIndex + resData.size() > firstUnacceptable){
        _Unassembled.insert(typeUnassembled(firstUnacceptable, resData.substr(firstUnacceptable - resIndex)));
        _nUnassembled += resData.substr(firstUnacceptable - resIndex).size();
        resData = resData.substr(0, firstUnacceptable - resIndex);
    }
    //再次判断如果当前index小于第一个没重组的index，就输出
    if (resIndex <= _firstUnassembled) {
        size_t wSize = _output.write(string(resData.begin() + _firstUnassembled - resIndex, resData.end()));
        if (wSize == resData.size() && eof)
            _output.end_input();
        _firstUnassembled += wSize;
    } else {

        //大于的话，就放入set当中，等待被其他的连接再输出
        _Unassembled.insert(typeUnassembled(resIndex, resData));
        _nUnassembled += resData.size();
    }
    //  当该没有非重组的数据并且eof关闭时，就把数据流也关闭了。
    if (empty() && _eof) {
        _output.end_input();
    }
    return;
}
int StreamReassembler::merge_substring(size_t &index, std::string &data, std::set<typeUnassembled>::iterator iter2) {
    // return value: 1:successfully merge; 0:fail to merge
    std::string data2 = (*iter2).data;
    size_t l2 = (*iter2).index, r2 = l2 + data2.size() - 1;
    size_t l1 = index, r1 = l1 + data.size() - 1;
    if (l2 > r1 + 1 || l1 > r2 + 1)
        return 0;
    index = min(l1, l2);
    size_t deleteNum = data2.size();
    if (l1 <= l2) {
        if (r2 > r1)
            data += std::string(data2.begin() + r1 - l2 + 1, data2.end());
    } else {
        if (r1 > r2)
            data2 += std::string(data.begin() + r2 - l1 + 1, data.end());
        data.assign(data2);
    }
    return deleteNum;
}

size_t StreamReassembler::unassembled_bytes() const { return _nUnassembled; }

bool StreamReassembler::empty() const { return _nUnassembled == 0; }
