# Matrix-Market-Partitioner-With-PaToH

This program is a matrix market partitioner that utilizes PaToH (Partitioning Tool for Hypergraphs, Ümit Çatalyürek). 
It first reads a matrix market file in coordinate (COO) format, then converts the matrix into a Compressed Sparse Column 
(CSC) structure and calls PaToH with the desired user inputs.

# How to use

This program can be compiled and used on its own and does not require any specific packages or software to be installed. 

After placing all the files in this repository under the same directory, either use "gcc main.c libpatoh_linux.a -lm" or 
"gcc main.c libpatoh_macOS.a" to compile the program. This command will give you a single executable file named "a.out".

This executable takes command line arguments as input. The general structure of the command to run the program is like the 
following: "./a.out [martix-market-pathname] [partition-type] [desired amount of partitions] [partition-quality] [seed]"

Where:

[martix-market-pathname]: Path to the matrix market file that the user is willing to partition.

[partition-type]: Either "conpart" or "cutpart". These are the partitioning objectives that PaToH provides.

[desired amount of partitions]: The number of partitions that the user wants.

[partition-quality]: Is either "speed", "default", or, "quality". These are the partitioning quality options that PaToH provides. 

[seed]: Is the seed for the random number generator that is used in the partitioning process. The partitioning algorithm that PaToH 
uses relies on randomness. A specific seed can be useful if the user requires consistency among many partitions (for example, if the 
user is benchmarking different systems with the same partitioning process). 

# Outputs

The program will create 3 different text files with the following naming scheme:

"PaToH_[matrix-file-name]_[partitioning-type]_[partition-counts]_[parttion-quality]_[seed]_[file-type].txt"

[file-type] is the type of the output file, which are "partvec", "partinfo", and "timeinfo".

partvec is the file that contains the partitioning vector as a list of integers. Integer in any given index shows which partitioning
group the corresponding row of the matrix belongs to.

partinfo is the file that contains various information about the partitioning process, such as part weights, cut size, and maximum imbalance.

timeinfo is the file that contains information about how long the partitioning process took with and without I/O operations.

# Notes

1- "quality" option runs substantially longer than "default" or "speed" options for matrices that have a significantly higher number of rows than columns.

2- Scripts to run these programs on large matrix datasets are provided in the "scripts" sub-directory of this repository. Feel free to use them if needed.



