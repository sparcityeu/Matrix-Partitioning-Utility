# Matrix-Partitioning-Utility

This program is a matrix market partitioner that utilizes PaToH (Partitioning Tool for Hypergraphs, Ümit Çatalyürek), and Metis (George 
Karypis, Vipin Kumar). It first reads a matrix market file in coordinate (COO) format, then converts the matrix into Compressed Sparse Column 
(CSC), or Compressed Sparse Row (CSR) format and calls either one of the partitioners with the desired user inputs.

# Properties of PaToH and Metis

Metis offers two partitioning objectives, "edge-cut", and "volume", and does not offer a quality option.

PaToH offers two partitioning objectives "conpart" and "cutpart", and three quality objectives "speed", "default", and "quality".

# How to use

Please see the respective readme files of each program to find detailed information on how to use them.
