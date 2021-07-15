#pragma once
#include <bits/stdc++.h>
#include "utils.hpp"
using namespace std;

BitWriter::BitWriter()
{
    cur_byte = 0;
    pos = 0;
    encoded_string = "";
}
void BitWriter::write_bit(char bit)
{
    cur_byte <<= 1;
    if (bit == '1')
    {
        cur_byte |= 1;
    }
    else
    {
        cur_byte |= 0;
    }
    pos++;
    if (pos == 8)
    {
        encoded_string.push_back(cur_byte);
        cur_byte = 0;
        pos = 0;
    }
}
void BitWriter::flush_byte()
{
    if (pos == 0)
    {
        return;
    }
    while (pos < 8)
    {
        cur_byte <<= 1;
        pos++;
    }
    encoded_string.push_back(cur_byte);
    cur_byte = 0;
    pos = 0;
}
string BitWriter::get_encoded_string()
{
    return encoded_string;
}

BitReader::BitReader(string &input)
{
    this->input = input;
    pos = 8;
    index = 0;
}
char BitReader::get_bit()
{
    if (pos == 8)
    {
        cur_byte = input[index];
        index++;
        pos = 0;
    }
    char ret = 0 | (1 << 7);
    pos++;
    if (ret & cur_byte)
    {
        cur_byte <<= 1;
        return '1';
    }
    else
    {
        cur_byte <<= 1;
        return '0';
    }
}

SimpleBitInputStream::SimpleBitInputStream(string input)
{
    this->input = input;
    index = 0;
    bit_count = 0;
    buffer = 0;
}

int SimpleBitInputStream::get_bit()
{
    if (bit_count == 0)
    {
        buffer = (int)input[index];
        index++;
        if (index == input.size())
            return 0;
        else
            bit_count = 8;
    }
    return (buffer >> --bit_count) & 1;
}
