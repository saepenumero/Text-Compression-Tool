#include <bits/stdc++.h>
#include "huffman.hpp"
#include "utils.hpp"
using namespace std;

TreeNode::TreeNode(char c)
{
    this->c = c;
    this->is_char = true;
    this->left = nullptr;
    this->right = nullptr;
    this->char_sum = c;
}

TreeNode::TreeNode()
{
    this->is_char = false;
    this->left = nullptr;
    this->right = nullptr;
}

TreeNode *gen_huffman_tree(vector<pair<char, long>> &vec)
{
    stable_priority_queue<pair<long, TreeNode *>> spq;
    for (auto p : vec)
    {
        TreeNode *temp = new TreeNode(p.first);
        spq.emplace(p.second, temp);
    }
    while (spq.size() > 1)
    {
        auto n1 = spq.top();
        spq.pop();
        auto n2 = spq.top();
        spq.pop();
        TreeNode *temp = new TreeNode();
        temp->left = n1.second;
        temp->right = n2.second;
        temp->char_sum = temp->left->char_sum + temp->right->char_sum;
        spq.emplace(n1.first + n2.first, temp);
    }
    return spq.top().second;
}
void gen_huffman_dictionary(TreeNode *root, string prev, unordered_map<char, string> &dict)
{
    if (root->is_char)
    {
        dict[root->c] = prev;
    }
    if (root->left)
    {
        gen_huffman_dictionary(root->left, prev + "0", dict);
    }
    if (root->right)
    {
        gen_huffman_dictionary(root->right, prev + "1", dict);
    }
}

string decode_huffman(string input, int char_count, unordered_map<string, char> &dict)
{

    string decoded_string = "";
    BitReader br(input);
    while (char_count > 0)
    {
        string cur = "";
        while (dict.find(cur) == dict.end())
        {
            cur.push_back(br.get_bit());
        }
        decoded_string.push_back(dict[cur]);
        char_count--;
    }
    return decoded_string;
}

string read_huffman(ifstream &ifile)
{
    long no_characters, total_count;
    ifile >> total_count;
    char throwaway;
    ifile >> throwaway;
    ifile >> no_characters;
    ifile >> throwaway;
    vector<pair<char, long>> vec;
    for (int i = 0; i < no_characters; i++)
    {
        char c;
        long l;
        ifile >> c;
        ifile >> l;
        ifile >> throwaway;
        vec.push_back({c, l});
    }
    sort(vec.begin(), vec.end());
    auto root = gen_huffman_tree(vec);
    unordered_map<char, string> dict;
    gen_huffman_dictionary(root, "", dict);
    unordered_map<string, char> inverted_dict;
    for (auto p : dict)
    {
        inverted_dict[p.second] = p.first;
    }
    ifile >> throwaway;
    string input;
    char c;
    while (ifile >> c)
    {
        input.push_back(c);
    }
    return decode_huffman(input, total_count, inverted_dict);
}

string serialise_tree(unordered_map<char, long> &umap)
{
    string ser = " ";
    ser += to_string(umap.size());
    ser += " ";
    long total = 0;
    for (auto p : umap)
    {
        total += p.second;
        ser.push_back(p.first);
        ser += to_string(p.second);
        ser.push_back(' ');
    }
    return to_string(total) + ser;
}

string bitwise_huffman(string input, unordered_map<char, string> &dict)
{
    BitWriter bw;
    for (char c : input)
    {
        string code = dict[c];
        for (char b : code)
        {
            bw.write_bit(b);
        }
    }
    bw.flush_byte();
    return bw.get_encoded_string();
}

string encode_huffman(string input)
{
    unordered_map<char, long> umap;
    for (char c : input)
    {
        if (umap.find(c) == umap.end())
        {
            umap[c] = 1;
        }
        else
        {
            umap[c]++;
        }
    }
    vector<pair<char, long>> vec;
    for (auto p : umap)
    {
        vec.push_back({p.first, p.second});
    }
    sort(vec.begin(), vec.end());
    auto root = gen_huffman_tree(vec);
    unordered_map<char, string> dict;
    gen_huffman_dictionary(root, "", dict);
    return serialise_tree(umap) + " " + bitwise_huffman(input, dict);
}
