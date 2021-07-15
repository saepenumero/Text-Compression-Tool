#include <bits/stdc++.h>
using namespace std;

class TreeNode
{
public:
    char c;
    bool is_char;
    int char_sum;
    TreeNode *left;
    TreeNode *right;
    TreeNode(char c);
    TreeNode();
};

TreeNode *gen_huffman_tree(vector<pair<char, long>> &vec);

void gen_huffman_dictionary(TreeNode *root, string prev, unordered_map<char, string> &dict);

string decode_huffman(string input, int char_count, unordered_map<string, char> &dict);

string read_huffman(ifstream &ifile);

string serialise_tree(unordered_map<char, long> &umap);

string bitwise_huffman(string input, unordered_map<char, string> &dict);

string encode_huffman(string input);