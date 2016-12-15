This project contains one programs: "convert"

"convert": The program that converts ASCII-based edgelist file to binary-based edgelist file.

input : every line is srcVID \t dstVID \n

output: every eight bytes space is an edge containing a srcVID and a dstVID
                
---------------------------------------  Prerequisites for building  ------------------------------
Linux (Ubuntu 12.04.4 LTS, 3.5.0-54-generic kernel, x86_64 for our case);
basic build tools (e.g., stdlibc, gcc, etc);
g++ (4.6.3 for our case);
libboost and libboost-dev 1.46.1 (our case) or higher.

---------------------------------------  Explanations to convert  ---------------------------------
1) Where is the source code?
	see {Project root}/convert/

2) How to build it?
	{Project root}$ make convert

3) How to use it?
	There are some parameters for "convert":

    -h Help messages

    -g The original SNAP file (in edgelist). 

    -t The type of the original SNAP file, i.e., two possibilities: edgelist or adjlist

    -d Destination folder. Remember to add a slash at the end of your dest folder:
        e.g., "/home/yourname/data/" or "./data/"


4) Example usage:
    {Project root}$ sudo convert -g ../source-graph/twitter_rv.net -t edgelist -d ../dest-graph/ 

---------------------------------------------------------------------
The end. enjoy!

