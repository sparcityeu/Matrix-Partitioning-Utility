# Requirements

- METIS library needs to be installed. It can be downloaded [from here.](http://glaros.dtc.umn.edu/gkhome/metis/metis/download)
- Supported C++ compiler (such as clang++, g++).

# Compiling
```
g++ -g mmio.c main.cpp -o run -std=c++11 -lmetis
```
Here, `run` is the executable of the program.

# Running the Program

Run with
```
./run -i <path/to/MMfile.mtx> -k <PART NUMBER> -o <OBJ_TYPE> -t
```
- -i         -> input file path
- -k         -> part number
- -o -> OBJ_TYPE:        
     - edge-cut    -> edge-cut   => Edge-cut minimization
     - volume      -> volume     => Total communication volume minimization
- [OPTIONAL] -t or --time   => outputing elapsed time information

## Example run:

To partition the 1138_bus matrix into 64 parts with edge-cut objective and printing out timing information.
```
./run -i 1138_bus.mtx -k 64 -o edge-cut -t
```

