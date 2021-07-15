#include <bits/stdc++.h>
#include "lib/bwt.hpp"
#include "lib/huffman.hpp"
#include "lib/utils.hpp"
using namespace std;

void print_usage()
{
    cout << "Invalid number of arguments." << endl;
    cout << "Usage:" << endl;
    cout << "To compress a file: ./cmp c input output" << endl;
    cout << "To decompress a file: ./cmp d input output" << endl;
    cout << "To benchmark compression methods: ./cmp b test_input test_output_file" << endl;
    cout << "Aborting." << endl;
}

pair<unsigned long, unsigned long> BWT_huffman_compress(ifstream &ifile, ofstream &ofile)
{
    ifile >> noskipws;
    char c;
    string input;
    while (ifile >> c)
    {
        input.push_back(c);
    }
    unsigned long ini_size = input.size();
    int index = 0;
    string output = "";
    while (index < input.size())
    {
        int len = min(SIZE_OF_BWT_BLOCK, input.size() - index);
        string buf = input.substr(index, len);
        auto p = encode_BWT(buf);
        p.first = encode_MTF(p.first);
        index += SIZE_OF_BWT_BLOCK;
        output += (to_string(p.second) + " " + p.first);
    }
    ofile << 'h';
    output = encode_huffman(output);
    ofile << output;
    unsigned long fin_size = output.size();
    return {ini_size, fin_size};
}

void BWT_huffman_decompress(ifstream &ifile, ofstream &ofile)
{
    ifile >> noskipws;
    //ofile << read_huffman(ifile);
    auto ss = stringstream(read_huffman(ifile));
    ss >> noskipws;
    while (ss)
    {
        int i = 0;
        string buf;
        int num;
        ss >> num;
        //cout << num << endl;
        char c;
        ss >> c;
        while (i < SIZE_OF_BWT_BLOCK && ss >> c)
        {
            buf.push_back(c);
            i++;
        }
        auto st = decode_MTF(buf);
        ofile << decode_BWT(st, num);
    }
}

class ArithmeticEncoder;
class ArithmeticDecoder;

class CircularBuffer
{
public:
    static const long SIZE = 8192;

private:
    unsigned char data[SIZE];
    long insert_pos;

public:
    CircularBuffer();
    unsigned char at(long i);  /* return char at position i */
    void add(unsigned char c); /* add a char at the inserting position */
    long get_insert_pos() { return insert_pos; }
    void reset() { insert_pos = 0; }
};

class ContextTrie
{
public:
    static const long NODES = 2000000;

private:
    struct Node
    {
        short int symbol;
        unsigned long count; /* symbol count */
        long down;           /* pointer to next level */
        long right;          /* pointer to the next node on the same level */
        long vine;           /* the vine pointer */
        void set(int, unsigned long, long, long, long);
    } nodes[NODES];
    short int exclusion_list[Settings::SYMBOLS];
    int exclusion_pos;
    int max_index;
    long base;        /* the base pointer */
    long insert_node; /* the (root) node where new child will be inserted */
    int depth;        /* the trie depth varies from 0 to Model::ORDER + 1 */
    long vine;
    long last;
    long get_start();
    bool search_exclusion(short int c, int max_index);
    /* Search all childs of node n for symbol s and, if successful, store
	 * the statistics to the pointers and return the number of this node.
	 * Otherwise store the statistics of the escape symbol and return -1. */
    long search_by_symbol(long n, int s, unsigned long *sym_low, unsigned long *sym_high, unsigned long *context_totals);
    long search_by_count(long n, unsigned long count, unsigned long *sym_low, unsigned long *sym_high);

public:
    ContextTrie();
    unsigned long get_context_totals();
    bool is_full() { return (insert_node == NODES); }
    /* Initialize the trie and if buffer is not NULL, build the trie
	 * according to the last CircularBuffer::SIZE input characters. */
    void build(CircularBuffer *buffer);
    /* Try to add a symbol to the trie and if the pointers are not NULL, 
	 * store the symbol statistics. Return false if the symbol is not
	 * found in the current context (the pointers will then contain the
	 * statistics for the escape symbol instead). Otherwise return true. */
    bool add_by_symbol(int s, unsigned long *sym_low, unsigned long *sym_high, unsigned long *context_totals);
    int add_by_count(unsigned long count, unsigned long *sym_low, unsigned long *sym_high, unsigned long context_totals);
    void flush();
};

class Model
{
    ContextTrie *trie;
    CircularBuffer *buffer;

public:
    static const int ID_EOF = Settings::SYMBOLS;
    static const int ID_FLUSH = Settings::SYMBOLS + 1;
    static const int ID_ESCAPE = Settings::SYMBOLS + 2;
    Model();
    ~Model();
    void add(ArithmeticEncoder *enc, int s, unsigned long *sym_low, unsigned long *sym_high, unsigned long *context_totals);
    /* add a symbol to the model and return its statistics; used when dempressing */
    int add(ArithmeticDecoder *dec);
    void flush()
    {
        trie->flush();
    };
};

class ArithmeticEncoder
{
    Model *model;
    BitWriter bw;
    unsigned long low;
    unsigned long high;
    unsigned long bits_count;
    unsigned long total_bits;
    unsigned long length_of_input;
    unsigned long ini_size;
    unsigned long fin_size;
    void check_ratio();

public:
    ArithmeticEncoder();
    ~ArithmeticEncoder();
    void encode_symbol(unsigned long sym_low, unsigned long sym_high, unsigned long context_totals);
    void encode(ifstream &ifile);
    void write(ofstream &ofile);
    string ret_encoded_string();
    unsigned long ret_length_of_input();
    pair<unsigned long, unsigned long> ret_sizes();
};

class ArithmeticDecoder
{
    //BitReader *br;
    SimpleBitInputStream *br;
    Model *model;
    string decoded_string;
    unsigned long low;    /* lower bound of the encoding interval */
    unsigned long high;   /* upper half of the encoding interval */
    unsigned long buffer; /* the first 31 bits of the encoded number */
    unsigned long len;

public:
    ArithmeticDecoder(string input);
    ~ArithmeticDecoder();
    unsigned long get_count(unsigned long context_totals);
    void decode_symbol(unsigned long sym_low, unsigned long sym_high, unsigned long context_totals);
    void decode();
    void set_length(unsigned long length);
    string ret_decoded_string();
};

pair<unsigned long, unsigned long> PPM_compress(ifstream &ifile, ofstream &ofile)
{
    ifile >> noskipws;
    unique_ptr<ArithmeticEncoder> en(new ArithmeticEncoder);
    en->encode(ifile);
    ofile << 'p';
    ofile << en->ret_encoded_string();
    return en->ret_sizes();
}

void PPM_decompress(ifstream &ifile, ofstream &ofile)
{
    ifile >> noskipws;
    unsigned long length;
    ifile >> length;
    char c;
    ifile >> c;
    string input;
    while (ifile >> c)
    {
        input.push_back(c);
    }
    unique_ptr<ArithmeticDecoder> de(new ArithmeticDecoder(input));
    de->set_length(length);
    de->decode();
    ofile << de->ret_decoded_string();
}

int main(int argc, char *argv[])
{
    ifstream ifile;
    ofstream ofile;
    if (argc != 4)
    {
        print_usage();
        exit(1);
    }
    if (argv[1][0] == 'c')
    {
        string input(argv[2]);
        string output(argv[3]);
        ifile.open(input);
        if (!ifile.is_open())
        {
            cout << "Unable to open file: " << input << " for reading, check permissions.";
            cout << "Aborting..." << endl;
            exit(1);
        }
        ofile.open(output);
        if (!ofile.is_open())
        {
            cout << "Unable to open file: " << output << " for writing, check permissions. Aborting..." << endl;
            exit(1);
        }
        cout << "Press h to use BWT followed by Huffman coding." << endl;
        cout << "Press p to use PPM compression." << endl;
        cout << "Press anything else to exit." << endl;
        char c;
        cin >> c;
        if (c == 'h')
        {
            BWT_huffman_compress(ifile, ofile);
            ifile.close();
            ofile.close();
        }
        else if (c == 'p')
        {
            PPM_compress(ifile, ofile);
            ifile.close();
            ofile.close();
        }
        else
        {
            ifile.close();
            ofile.close();
            exit(0);
        }
    }
    else if (argv[1][0] == 'd')
    {
        string input(argv[2]);
        string output(argv[3]);
        ifile.open(input);
        if (!ifile.is_open())
        {
            cout << "Unable to open file: " << input << " for reading, check permissions.";
            cout << "Aborting..." << endl;
            exit(1);
        }
        ofile.open(output);
        if (!ofile.is_open())
        {
            cout << "Unable to open file: " << output << " for writing, check permissions. Aborting..." << endl;
            exit(1);
        }
        char c;
        ifile >> noskipws;
        ifile >> c;
        if (c == 'h')
        {
            BWT_huffman_decompress(ifile, ofile);
            ifile.close();
            ofile.close();
        }
        else
        {
            PPM_decompress(ifile, ofile);
            ifile.close();
            ofile.close();
        }
    }
    else if (argv[1][0] == 'b')
    {
        string input(argv[2]);
        string output(argv[3]);
        ifile.open(input);
        if (!ifile.is_open())
        {
            cout << "Unable to open file: " << input << " for reading, check permissions.";
            cout << "Aborting..." << endl;
            exit(1);
        }
        ofile.open(output);
        if (!ofile.is_open())
        {
            cout << "Unable to open file: " << output << " for writing, check permissions. Aborting..." << endl;
            exit(1);
        }
        auto start1 = chrono::high_resolution_clock::now();
        auto p1 = BWT_huffman_compress(ifile, ofile);
        auto stop1 = chrono::high_resolution_clock::now();
        ifile.close();
        ofile.close();
        cout << "BWT+Huffman compression complete." << endl;
        cout << "Time taken: " << chrono::duration_cast<chrono::microseconds>(stop1 - start1).count() / 1000.0 << " ms" << endl;
        cout << "Initial file size: " << p1.first << " bytes." << endl;
        cout << "Compressed file size: " << p1.second << " bytes." << endl;
        cout << "Compression ratio: " << p1.first / (double)p1.second << endl
             << endl;

        ifile.open(output);
        ofile.open(input);
        char tempc;
        ifile >> noskipws;
        auto start2 = chrono::high_resolution_clock::now();
        ifile >> tempc;
        BWT_huffman_decompress(ifile, ofile);
        auto stop2 = chrono::high_resolution_clock::now();
        ifile.close();
        ofile.close();
        cout << "BWT+Huffman decompression complete." << endl;
        cout << "Time taken: " << chrono::duration_cast<chrono::microseconds>(stop2 - start2).count() / 1000.0 << " ms" << endl;
        cout << endl;

        ifile.open(input);
        ofile.open(output);
        auto start3 = chrono::high_resolution_clock::now();
        auto p2 = PPM_compress(ifile, ofile);
        auto stop3 = chrono::high_resolution_clock::now();
        ifile.close();
        ofile.close();
        cout << "PPM compression complete." << endl;
        cout << "Time taken: " << chrono::duration_cast<chrono::microseconds>(stop3 - start3).count() / 1000.0 << " ms" << endl;
        cout << "Initial file size: " << p2.first << " bytes." << endl;
        cout << "Compressed file size: " << p2.second << " bytes." << endl;
        cout << "Compression ratio: " << p2.first / (double)p2.second << endl
             << endl;

        ifile.open(output);
        ofile.open(input);
        ifile >> noskipws;
        auto start4 = chrono::high_resolution_clock::now();
        ifile >> tempc;
        PPM_decompress(ifile, ofile);
        auto stop4 = chrono::high_resolution_clock::now();
        ifile.close();
        ofile.close();
        cout << "PPM decompression complete." << endl;
        cout << "Time taken: " << chrono::duration_cast<chrono::microseconds>(stop4 - start4).count() / 1000.0 << " ms" << endl;
    }
    else
    {
        print_usage();
        exit(1);
    }
    return 0;
}

void ArithmeticEncoder::check_ratio()
{
    static const long CHECK_INTERVAL = 16384;
    static int check_cnt = 0;
    if (check_cnt++ == CHECK_INTERVAL)
    {
        if (((total_bits >> 3) / ((float)CHECK_INTERVAL)) > 0.8)
        {
            unsigned long sym_low, sym_high, context_totals;
            model->add(this, Model::ID_FLUSH, &sym_low, &sym_high, &context_totals);
            encode_symbol(sym_low, sym_high, context_totals);
            model->flush();
        }
        check_cnt = 0;
        total_bits = 0;
    }
}
ArithmeticEncoder::ArithmeticEncoder()
{
    model = new Model();
    low = 0;
    high = 0x7FFFFFFF; /* maximal value that fits in 31 bits */
    bits_count = 0;
    total_bits = 0;
}

ArithmeticEncoder::~ArithmeticEncoder()
{
    if (model)
        delete model;
}
void ArithmeticEncoder::encode_symbol(unsigned long sym_low, unsigned long sym_high, unsigned long context_totals)
{
    unsigned long range = (high - low + 1) / context_totals;
    high = low + sym_high * range - 1;
    low = low + sym_low * range;
    /* while the MSB of high and low match, output them and rescale low and high */
    while ((low & 0x40000000) == (high & 0x40000000))
    {
        //out->put_bit(low >> 30);
        if ((low >> 30) & 1)
        {
            bw.write_bit('1');
        }
        else
        {
            bw.write_bit('0');
        }
        total_bits++;
        while (bits_count > 0)
        {
            //out->put_bit((~low & 0x7FFFFFFF) >> 30);
            if ((~low & 0x7FFFFFFF) >> 30 & 1)
            {
                bw.write_bit('1');
            }
            else
            {
                bw.write_bit('0');
            }
            total_bits++;
            bits_count--;
        }
        low <<= 1;
        low &= 0x7FFFFFFF;
        high <<= 1;
        high |= 1;
        high &= 0x7FFFFFFF;
    }
    /* if there is a danger of underflow, increase the underflow counter and rescale low and high */
    while (((low & 0x20000000) != 0) && ((high & 0x20000000) == 0))
    {
        bits_count++;
        low &= 0x1FFFFFFF;
        low <<= 1;
        high |= 0x20000000;
        high <<= 1;
        high |= 1;
        high &= 0x7FFFFFFF;
    }
}

void ArithmeticEncoder::encode(ifstream &ifile)
{
    int symbol;
    char c;
    unsigned long sym_low, sym_high, context_totals;
    this->length_of_input = 0;
    while (ifile >> c)
    {
        this->length_of_input++;
        symbol = c;
        check_ratio();
        model->add(this, symbol, &sym_low, &sym_high, &context_totals);
        encode_symbol(sym_low, sym_high, context_totals);
    }
    /* output a special EOF marker */
    model->add(this, Model::ID_EOF, &sym_low, &sym_high, &context_totals);
    encode_symbol(sym_low, sym_high, context_totals);
    /* output the remaining MSBs */
    //out->put_bit((low & 0x20000000) >> 29);
    if (((low & 0x20000000) >> 29) & 1)
    {
        bw.write_bit('1');
    }
    else
    {
        bw.write_bit('0');
    }
    bits_count++;
    while (bits_count-- > 0)
    {
        //out->put_bit((~low & 0x20000000) >> 29);
        if (((~low & 0x20000000) >> 29) & 1)
        {
            bw.write_bit('1');
        }
        else
        {
            bw.write_bit('0');
        }
    }
    bw.flush_byte();
    ini_size = this->length_of_input;
}

void ArithmeticEncoder::write(ofstream &ofile)
{
    string wr = bw.get_encoded_string();
    ofile << wr;
}

CircularBuffer::CircularBuffer()
{
    insert_pos = 0;
}

inline unsigned char CircularBuffer::at(long i)
{
    return data[i];
}

inline void CircularBuffer::add(unsigned char c)
{
    data[insert_pos] = c;
    insert_pos++;
    insert_pos &= SIZE - 1;
}

ContextTrie::ContextTrie()
{
    build(NULL);
}

void ContextTrie::Node::set(int _symbol, unsigned long _count, long _down, long _right, long _vine)
{
    symbol = _symbol;
    count = _count;
    down = _down;
    right = _right;
    vine = _vine;
}

void ContextTrie::build(CircularBuffer *buffer)
{
    base = 0;
    depth = 0;
    insert_node = 1; /* index 0 is reserved for the trie root, so we begin inserting from index 1 */
    vine = -2;
    last = -1;
    for (long i = 0; i < NODES; i++)
    {
        nodes[i].set(0, 0, -1, -1, -1);
    }
    if (buffer != NULL)
    {
        unsigned long dummy;
        for (int i = 0; i < buffer->get_insert_pos(); i++)
            while (!add_by_symbol(buffer->at(i), &dummy, &dummy, &dummy))
                ;
        for (int i = buffer->get_insert_pos(); i < CircularBuffer::SIZE; i++)
            while (!add_by_symbol(buffer->at(i), &dummy, &dummy, &dummy))
                ;
        buffer->reset();
    }
}

bool ContextTrie::search_exclusion(short int c, int max_index)
{
    for (int i = 0; i < max_index; i++)
        if (exclusion_list[i] == c)
            return true;
    return false;
}

long ContextTrie::search_by_symbol(long n, int s, unsigned long *sym_low, unsigned long *sym_high, unsigned long *context_totals)
{
    *sym_low = 0;
    long i, j;
    int max_index = exclusion_pos;
    for (i = nodes[n].down; i != -1 && nodes[i].symbol != s; i = nodes[i].right)
        if (!search_exclusion(nodes[i].symbol, max_index))
        {
            *sym_low += nodes[i].count;
            exclusion_list[exclusion_pos++] = nodes[i].symbol;
        }
    *context_totals = *sym_low;
    if (i != -1)
    {
        *sym_high = *sym_low + nodes[i].count;
        /* continue with addition till the end to obtain total range at current level */
        for (j = i; j != -1; j = nodes[j].right)
        {
            if (!search_exclusion(nodes[j].symbol, max_index))
            {
                *context_totals += nodes[j].count;
                exclusion_list[exclusion_pos++] = nodes[j].symbol;
            }
        }
    }
    else
        *sym_high = *sym_low + 1;
    *context_totals += 1; /* include the escape symbol, which has count equal one */
    return i;
}

long ContextTrie::get_start()
{
    long vine;
    if (depth < Settings::ORDER + 1)
    {
        vine = base;
        depth++;
    }
    else
    {
        vine = nodes[base].vine;
    }
    return vine;
}

bool ContextTrie::add_by_symbol(int s, unsigned long *sym_low, unsigned long *sym_high, unsigned long *context_totals)
{
    /* Vine pointer for current addition. -2 means new search with new symbol, -1 means that the
	 * model fell off to the order special order -1 model, i.e. it  has not been found neither in
	 * any context nor as a standalone character. */
    if (vine == -2)
    { /* if new search, determine the new value of vine */
        vine = get_start();
        last = -1;
        exclusion_pos = 0;
    }
    else if (vine == -1)
    {
        *sym_low = s;
        *sym_high = s + 1;
        *context_totals = Settings::SYMBOLS + 2; /* including eof marker and flush marker */
        assert(last != -1);
        nodes[last].vine = 0;
        vine = -2;
        return true;
    }
    long n = search_by_symbol(vine, s, sym_low, sym_high, context_totals);
    if (n == -1)
    {
        nodes[insert_node].set(s, 1, -1, nodes[vine].down, -1);
        nodes[vine].down = insert_node;
        if (last == -1)
            base = insert_node;
        else
            nodes[last].vine = insert_node;
        last = insert_node;
        vine = nodes[vine].vine;
        insert_node++;
        return false;
    }
    else
    {
        nodes[n].count++;
        if (last == -1)
            base = n;
        else
            nodes[last].vine = n;
        vine = -2;
        return true;
    }
}
unsigned long ContextTrie::get_context_totals()
{
    if (vine == -2)
    {
        vine = get_start();
        last = -1;
        exclusion_pos = 0;
    }
    else if (vine == -1)
    {
        return Settings::SYMBOLS + 2;
    }
    max_index = exclusion_pos;
    unsigned long context_totals = 0; /* the cummulative count */
    for (int i = nodes[vine].down; i != -1; i = nodes[i].right)
    {
        if (!search_exclusion(nodes[i].symbol, max_index))
        {
            context_totals += nodes[i].count;
            exclusion_list[exclusion_pos++] = nodes[i].symbol;
        }
    }
    return context_totals + 1; /* include teh escape symbol */
}
long ContextTrie::search_by_count(long n, unsigned long count, unsigned long *sym_low, unsigned long *sym_high)
{
    *sym_low = 0;
    long i;
    for (i = nodes[n].down; i != -1; i = nodes[i].right)
    {
        if (!search_exclusion(nodes[i].symbol, max_index))
        {
            if (*sym_low + nodes[i].count > count)
                break;
            *sym_low += nodes[i].count;
        }
    }
    if (i != -1 && search_exclusion(nodes[i].symbol, max_index))
        i = nodes[i].right;
    if (i != -1)
    {
        *sym_high = *sym_low + nodes[i].count;
    }
    else
        *sym_high = *sym_low + 1;
    return i;
}
int ContextTrie::add_by_count(unsigned long count, unsigned long *sym_low, unsigned long *sym_high, unsigned long context_totals)
{
    static long buffer[Settings::ORDER + 1];
    static int buf_pos = 0;
    int symbol;
    if (vine == -1)
    {
        symbol = count;
        *sym_low = count;
        *sym_high = count + 1;
        assert(last != -1);
        nodes[last].vine = 0;
    }
    else
    {
        long n = search_by_count(vine, count, sym_low, sym_high);
        if (n == -1 || *sym_high == context_totals)
        { /* escape symbol? */
            nodes[insert_node].set('?', 1, -1, nodes[vine].down, -1);
            nodes[vine].down = insert_node;
            if (last == -1)
                base = insert_node;
            else
                nodes[last].vine = insert_node;
            last = insert_node;
            vine = nodes[vine].vine;
            buffer[buf_pos++] = insert_node;
            insert_node++;
            return Model::ID_ESCAPE;
        }
        else
        {
            symbol = nodes[n].symbol;
            nodes[n].count++;
            if (last == -1)
                base = n;
            else
                nodes[last].vine = n;
        }
    }
    for (int i = 0; i < buf_pos; i++)
        nodes[buffer[i]].symbol = symbol;
    buf_pos = 0;
    vine = -2;
    return symbol;
}
void ContextTrie::flush()
{
    for (long i = 1; i < insert_node; i++)
    {
        nodes[i].count >>= 1;
        nodes[i].count++;
    }
}

Model::Model()
{
    trie = new ContextTrie;
    buffer = new CircularBuffer;
    if (trie == NULL)
        cout << "Memory Error Model Constructor trie=NULL" << endl;
    if (buffer == NULL)
        cout << "Memory Error Model Constructor buffer=NULL" << endl;
}

Model::~Model()
{
    if (trie)
        delete trie;
    if (buffer)
        delete buffer;
}

void Model::add(ArithmeticEncoder *enc, int s, unsigned long *sym_low, unsigned long *sym_high, unsigned long *context_totals)
{
    if (trie->is_full())
        trie->build(buffer);
    /* While the symbol at a given context is not found, output special escape
	 * symbol. */
    while (!trie->add_by_symbol(s, sym_low, sym_high, context_totals))
    {
        enc->encode_symbol(*sym_low, *sym_high, *context_totals);
    }
    if (s < Settings::SYMBOLS)
        buffer->add((unsigned char)s);
    /* Now sym_low, sym_high and context_totals contain the statistics so that
	 * the encoder can process them. */
}

int Model::add(ArithmeticDecoder *dec)
{
    if (trie->is_full())
        trie->build(buffer);
    int s;
    unsigned long sym_low, sym_high, context_totals;
    do
    {
        context_totals = trie->get_context_totals();
        s = trie->add_by_count(dec->get_count(context_totals), &sym_low, &sym_high, context_totals);
        dec->decode_symbol(sym_low, sym_high, context_totals);
    } while (s == Model::ID_ESCAPE);
    if (s < Settings::SYMBOLS)
        buffer->add((unsigned char)s);
    /* Now sym_low, sym_high and context_totals contain the statistics so that
	 * the encoder can process them. */
    return s;
}

ArithmeticDecoder::ArithmeticDecoder(string input)
{
    model = new Model();
    //br = new BitReader(input);
    br = new SimpleBitInputStream(input);
    if (model == NULL)
        cout << "Out of memory Arithmetic Decoder Constructor" << endl;
    low = 0;
    high = 0x7FFFFFFF; /* maximal value that fits in 31 bits */
}

ArithmeticDecoder::~ArithmeticDecoder()
{
    if (model)
    {
        delete model;
    }
    if (br)
    {
        delete br;
    }
}

unsigned long ArithmeticDecoder::get_count(unsigned long context_totals)
{
    unsigned long range = (high - low + 1) / context_totals;
    return (buffer - low) / range;
}

void ArithmeticDecoder::decode_symbol(unsigned long sym_low, unsigned long sym_high, unsigned long context_totals)
{
    unsigned long range = (high - low + 1) / context_totals;
    unsigned long count = (buffer - low) / range;
    high = low + sym_high * range - 1;
    low = low + sym_low * range;
    while ((low & 0x40000000) == (high & 0x40000000))
    {
        low <<= 1;
        low &= 0x7FFFFFFF;
        high <<= 1;
        high |= 1;
        high &= 0x7FFFFFFF;
        buffer <<= 1;
        buffer |= br->get_bit();
        buffer &= 0x7FFFFFFF;
    }
    while (((low & 0x20000000) != 0) && ((high & 0x20000000) == 0))
    {
        low &= 0x1FFFFFFF;
        low <<= 1;
        high |= 0x20000000;
        high <<= 1;
        high |= 1;
        high &= 0x7FFFFFFF;
        buffer ^= 0x20000000;
        buffer <<= 1;
        buffer |= br->get_bit();
        buffer &= 0x7FFFFFFF;
    }
}

void ArithmeticDecoder::decode()
{
    buffer = 0;
    /* read the beginning of the encoded number */
    for (int i = 0; i < 31; i++)
    {
        buffer = (buffer << 1) | br->get_bit();
    }
    int symbol;
    while ((symbol = model->add(this)) != Model::ID_EOF)
    {
        if (symbol == Model::ID_FLUSH)
            model->flush();
        decoded_string.push_back((char)symbol);
        len--;
        if (len == 0)
        {
            break;
        }
    }
}

string ArithmeticDecoder::ret_decoded_string()
{
    return decoded_string;
}

void ArithmeticDecoder::set_length(unsigned long length)
{
    this->len = length;
}

string ArithmeticEncoder::ret_encoded_string()
{
    auto ret = to_string(this->length_of_input) + " " + bw.get_encoded_string();
    fin_size = ret.size() + 1;
    return ret;
}

unsigned long ArithmeticEncoder::ret_length_of_input()
{
    return this->length_of_input;
}

pair<unsigned long, unsigned long> ArithmeticEncoder::ret_sizes()
{
    return {this->ini_size, this->fin_size};
}
