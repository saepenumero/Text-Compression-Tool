#include <bits/stdc++.h>
using namespace std;

const int SIZE_OF_ALPHABET = 256;
const unsigned long SIZE_OF_BWT_BLOCK = 10000;

pair<string, int> encode_BWT(string input);

vector<int> get_transformation_matrix(string &f_string, string &l_string);

string decode_BWT(string &encoded_string, int primary_index);

string encode_MTF(string input);

string decode_MTF(string encoded_string);