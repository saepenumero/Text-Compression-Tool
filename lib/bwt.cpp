#include <bits/stdc++.h>
#include "bwt.hpp"
using namespace std;

pair<string, int> encode_BWT(string input)
{
    vector<pair<string, int>> rotations_list;
    rotations_list.push_back({input, 0});
    for (int i = 1; i < input.size(); i++)
    {
        auto temp = rotations_list.back().first;
        rotate(temp.begin(), temp.begin() + 1, temp.end());
        rotations_list.push_back({temp, i});
    }

    sort(rotations_list.begin(), rotations_list.end(), [](const auto &ele1, const auto &ele2) {
        return ele1.first < ele2.first;
    });

    string ret;
    int primary_index = -1;
    for (int i = 0; i < rotations_list.size(); i++)
    {
        ret.push_back(rotations_list[i].first.back());
        if (rotations_list[i].second == 1)
        {
            primary_index = i;
        }
    }
    return {ret, primary_index};
}

vector<int> get_transformation_matrix(string &f_string, string &l_string)
{
    vector<int> trans_mat;
    vector<pair<vector<int>, int>> store(SIZE_OF_ALPHABET, {{}, 0});
    for (int i = 0; i < l_string.size(); i++)
    {
        store[(int)l_string[i] + 128].first.push_back(i);
    }
    for (int i = 0; i < f_string.size(); i++)
    {
        auto &vec = store[(int)f_string[i] + 128].first;
        auto ind = store[(int)f_string[i] + 128].second;
        trans_mat.push_back(vec[ind]);
        store[(int)f_string[i] + 128].second++;
    };
    return trans_mat;
}

string decode_BWT(string &encoded_string, int primary_index)
{
    string f_string(encoded_string);
    sort(f_string.begin(), f_string.end());
    string decoded_string;
    int index = primary_index;
    vector<int> trans_mat = get_transformation_matrix(f_string, encoded_string);
    for (int i = 0; i < encoded_string.size(); i++)
    {
        decoded_string.push_back(encoded_string[index]);
        index = trans_mat[index];
    }
    return decoded_string;
}

string encode_MTF(string input)
{
    if (input.empty())
    {
        return input;
    }
    string encoded_string;
    encoded_string.push_back(input[0]);
    for (int i = 1; i < input.size(); i++)
    {
        int t1, t2;
        t2 = input[i];
        t1 = input[i - 1];
        t2 -= t1;
        encoded_string.push_back((char)t2);
    }
    return encoded_string;
}

string decode_MTF(string encoded_string)
{
    if (encoded_string.empty())
    {
        return encoded_string;
    }
    string decoded_string;
    decoded_string.push_back(encoded_string[0]);
    for (int i = 1; i < encoded_string.size(); i++)
    {
        decoded_string.push_back(((char)(int)decoded_string[i - 1] + (int)encoded_string[i]));
    }
    return decoded_string;
}
