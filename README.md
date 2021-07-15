
# Text Compression Tool

- This tool can be used for compressing and decompressing text files.
- It can also be used to benchmark compression and decompression of text files.
- Compression can be done using either BWT + Huffman coding or PPM + Arithmetic coding


The application can be compiled from source by typing "make" in the the terminal.



## Usage

To compress a file: ./CompressionTool c input output

To decompress a file: ./CompressionTool d input output

To benchmark compression methods: ./CompressionTool b test_input test_output_file
