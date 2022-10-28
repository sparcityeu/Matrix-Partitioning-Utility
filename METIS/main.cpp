#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef> /* NULL */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <metis.h>
#include <iostream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "mmio.h"
#include <map>

// Install metis from:
// http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz

// Build with
// g++ metis.cpp -lmetis -o run -std=c++11
//
// Run with
// ./run -i path/to/MMfile.mtx -k <PART NUMBER> -o <OBJ_TYPE> -t
//      -i         -> input file path
//      -k         -> part number
// OBJ_TYPE:        
//     edge-cut    -> edge-cut ->> edge cut minimization
//     volume      -> volume ->> Total communication volume minimization
//  -t or --time   -> outputing elapsed time information


std::string returnFileName(std::string filePath);

int main(int argc, char *argv[]){
   
    //putting input arguments into a vector to make use easier
    std::vector<std::string> arguments(argv, argv + argc); 

    std::string filePath = "";
    idx_t nParts = 16; 
    int objective = 1;
    FILE *f;
    std::string objective_name = "";
    bool printElapsedTime = false;

    //Reading the input arguments, specifically matrix name, part number and, objective type.
    if(argc < 2){
        fprintf(stderr, "Usage: ./run -i <MatrixMarket File path> -k <Partition Number> -o <Preferred Objective>\nExample: ./run -i matrices/Stanford/Stanford.mtx -k 4 -o edge-cut\n");
        exit(1);
    }else{
     
        for(int j = 0; j < arguments.size(); j++){
            if(arguments[j] == "-i"){
                filePath += arguments[j+1];
                continue;
             }
            if(arguments[j] == "-k"){
                nParts = (idx_t)std::stoi(arguments[j+1]);
                continue;
            }
            if(arguments[j] == "-o"){
                if(arguments[j+1] == "edge-cut"){
                    objective = 1;
                    objective_name += "edge-cut";
                }else if(arguments[j+1] == "volume"){
                    objective = 2;
                    objective_name += "volume";
                }
            }
            if(arguments[j] == "-t" || arguments[j] == "--time"){
                printElapsedTime = true;
            }
        }
    }

    int filePathSize = filePath.size();
    char filePathInChar[filePathSize];
    strcpy(filePathInChar, filePath.c_str());
    
    //Metis and CSR variables
    idx_t nWeights  = 1;
    idx_t *nVertices;
    idx_t objval; 
    idx_t numOfRow = 0, numOfCol = 0, numOfVal = 0;
    
    int ret_code;
    MM_typecode matcode;
    
    int M, N, nz; 
     
    if((f = fopen(filePath.c_str(),"r")) == NULL){
        exit(1);
    }

    nVertices = &M;
    if (mm_read_banner(f, &matcode) != 0)
    {
        printf("Could not process Matrix Market banner.\n");
        exit(1);
    }

    /*  This is how one can screen matrix types if their application */
    /*  only supports a subset of the Matrix Market data types.      */

    if (mm_is_complex(matcode) && mm_is_matrix(matcode) && 
            mm_is_sparse(matcode) )
    {
        printf("Sorry, this application does not support ");
        printf("Market Market type: [%s]\n", mm_typecode_to_str(matcode));
        exit(1);
    }

    /* find out size of sparse matrix .... */
    if ((ret_code = mm_read_mtx_crd_size(f, &M, &N, &nz)) !=0)
        exit(1);


    /* reseve memory for matrices */ 
    double* val = new double[nz]();


    /* NOTE: when reading in doubles, ANSI C requires the use of the "l"  */
    /*   specifier as in "%lg", "%lf", "%le", otherwise errors will occur */
    /*  (ANSI C X3.159-1989, Sec. 4.9.6.2, p. 136 lines 13-15)            */
    
    std::map<std::pair<int, int>, int> mapForValues;
    std::vector<std::pair<int,int>> values;

    std::chrono::steady_clock::time_point beginForInput = std::chrono::steady_clock::now();

    for (int i=0; i<nz; i++)
    {
        int a, b;
        fscanf(f, "%d %d %lg\n", &a, &b, &val[i]);

        //ignore if entry is diagonal
        if(a == b){
            continue;
        }else{
            /*
             -First, we are creating a pair, assigning row value to the first and column value to the second values of the pair
             -Push back that pair to the values vector, which is vector<pair<int,int>>
             -Also, insert that pair to the map in order to keep track of which values are exist and avoid duplicate row-column pairs
            */
            std::pair<int,int> pr;
            pr.first = a, pr.second = b;

            values.push_back(pr);
            mapForValues.insert({pr,1});
        }
    }
    std::chrono::steady_clock::time_point endForInput = std::chrono::steady_clock::now();

    if (f !=stdin) fclose(f);
    

    int numOfValues = values.size();
    
    /*
     * Iterate through the values vector and check if pair<int,int>(b, a), symmetrical of <a, b>, is present or not,
     * if not exist, then push the pair<int, int>(b, a) to the values vector, and ensure the symmetry
     * Also, insert the newly added values to the map in order to avoid possible duplicate values.
     * */
    for(int i = 0; i < numOfValues; i++){

        int a = values[i].first, b = values[i].second;

        if(mapForValues[std::pair<int,int>(b,a)] == 0){
            values.push_back(std::make_pair(b,a));
            mapForValues.insert({{b,a}, 1}); 
        }
    }
    
    
    std::sort(values.begin(), values.end());
    
    numOfValues = values.size();

    /*
     * As soon as we are done with values, we can start to csr operations. 
     * */

    int* I = new int[numOfValues]();
    int* J = new int[numOfValues]();
    
    for(int i = 0; i < values.size(); i++){
        int a = values[i].first, b = values[i].second;

        I[i] = a;
        J[i] = b;
        
        I[i]--;
        J[i]--;
    }

    /*
     * Control for non-square matrices.
     * If row and column numbers are not equal, then the greater one should be selected
     * */
    if(M != N){
        M = std::max(M,N);
    }

    numOfRow=M, numOfCol=N;
    idx_t* csr_col = new idx_t[numOfValues]();
    idx_t* csr_row = new idx_t[M + 1]();
        
    for(idx_t i = 0; i < numOfValues; i++){
        csr_col[i] = J[i];
        csr_row[I[i] + 1]++;
    }
    for(idx_t i = 0; i < numOfRow; i++){
        csr_row[i+1] += csr_row[i]; 
    }
    
    delete [] I;
    delete [] J;
    delete [] val;


    idx_t nnz = numOfValues;
    idx_t* part = new idx_t[numOfRow]();
    
    //Adding options
    idx_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);
    options[METIS_OPTION_SEED] = 1;
    
    
    if(objective == 1){
        options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
    }else if(objective == 2){
        options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_VOL;
    }
    
    std::chrono::steady_clock::time_point beginForPartitioning = std::chrono::steady_clock::now(); 

    int ret = METIS_PartGraphKway(&numOfRow, &nWeights, csr_row, csr_col,
				       NULL, NULL, NULL, &nParts, NULL,
				       NULL, options, &objval, part);
    
    std::chrono::steady_clock::time_point endForPartitioning = std::chrono::steady_clock::now();

    delete [] csr_row;
    delete [] csr_col;

    //Measuring the execution time.
    auto elapsedTimeForPartitioning = 
        std::chrono::duration_cast<std::chrono::milliseconds>(endForPartitioning - beginForPartitioning).count();
    
    //extracting the file name from the given path, and adjusting the output file.
    std::string fileName = returnFileName(filePath);
    std::string partVectorName = fileName +"_metis_"+objective_name+ "_part" + std::to_string(nParts) + ".txt";

    std::fstream partVectorFile;
    partVectorFile.open(partVectorName,std::ios::out);

    //std::cout<<"Elapsed time for partitioning "<< fileName<<" is:"<< elapsedTimeForPartitioning << "ms." << std::endl;
    
    std::string statusFileName = fileName +"_metis_"+objective_name+ "_part" + std::to_string(nParts) + "_info" + ".txt";
    
    std::fstream statusFile;
    statusFile.open(statusFileName, std::ios::out);

    int* freq = new int[nParts]();

    for(idx_t i = 0; i < numOfRow; i++){
        freq[part[i]]++;
    }

    int totalSum = 0, mxWeight = 0;
    for(int i = 0; i < nParts; i++){
        mxWeight = std::max(mxWeight, freq[i]);
        totalSum += freq[i];
    }

    statusFile<<"Resulting imbalance:"<<(double)mxWeight/((double)totalSum/nParts)<<std::endl;
    statusFile<<"cutsize/#vertices:"<<(double)objval/numOfRow<<std::endl;
    statusFile<<"Cutsize:"<<objval<<std::endl;
    statusFile<<"----------------------------"<<std::endl;
    statusFile<<"Part Weights:"<<std::endl;

    for(int i = 0; i < nParts; i++){
        statusFile<<i<<" :"<<freq[i]<<std::endl;   
    }

    delete [] freq;
    statusFile.close();
   
    std::chrono::steady_clock::time_point beginForOutput = std::chrono::steady_clock::now();
    
    //Putting partitioned vector into a .txt file
    for(idx_t part_i = 0; part_i < numOfRow; part_i++){
        partVectorFile<<part[part_i]<<std::endl;
    }
    std::chrono::steady_clock::time_point endForOutput = std::chrono::steady_clock::now();
    
    auto elapsedTimeForInput = std::chrono::duration_cast<std::chrono::milliseconds>(endForInput - beginForInput).count();
    auto elapsedTimeForOutput = std::chrono::duration_cast<std::chrono::milliseconds>(endForOutput - beginForOutput).count();

    std::string fileNameForElapsedTime = +"../status/" + fileName +"_metis_"+objective_name+ "_part" + std::to_string(nParts) + "_status" + ".txt";
    std::fstream elapsedTimeFile;
    elapsedTimeFile.open(fileNameForElapsedTime, std::ios::out);
    
    elapsedTimeFile<<"Elapsed Time For Input Operations:"<<((double)elapsedTimeForInput/1000)<<" seconds"<<std::endl;
    elapsedTimeFile<<"Elapsed Time For Output Operations:"<<((double)elapsedTimeForOutput/1000)<<" seconds"<<std::endl;
    elapsedTimeFile<<"Elapsed Time For Partitioning:"<<((double)elapsedTimeForPartitioning/1000)<<" seconds"<<std::endl;
    
    elapsedTimeFile.close();
    
    delete [] part;
    partVectorFile.close();
    
    if(printElapsedTime){
        std::string outputNameString = fileName + "-Metis-" + objective_name + "-" + std::to_string(nParts) + "Parts:"; 
        std::cout<<outputNameString<<((double)elapsedTimeForPartitioning/1000)<<" seconds"<<std::endl;
    }
    
    return 0;
}

//extracts the file name from path
std::string returnFileName(std::string filePath){
    
    std::string delimiter1 = "/";
    std::string delimiter2 = ".";
    std::string result;
    size_t pos = 0;
    std::string token;
    
    while((pos = filePath.find(delimiter1)) != std::string::npos){
        filePath.erase(0,pos + delimiter1.length());
    }

    pos = filePath.find(delimiter2);
    result = filePath.substr(0,pos);
    return result;
}
