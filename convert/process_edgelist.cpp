/**************************************************************************************************
 * Authors: 
 *   Jian He,
 *
 * Routines:
 *   Process graph whose format is edge list.
 *   
 *************************************************************************************************/

#include <iostream>
#include <cassert>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "convert.h"
using namespace convert;
#define LINE_FORMAT		"%u\t%u\n"

char line_buffer[MAX_LINE_LEN];
FILE * in;
int edge_file;
//hejian-debug
unsigned long long line_no=0;
unsigned int src_vert, dst_vert;


/*
 * Regarding the vertex indexing:
 * We will assume the FIRST VERTEX ID SHOULD be 0!
 * Although in real cases, this will not necessarily be true,
 * (in many real graphs, the minimal vertex id may not be 0)
 * the assumption we made will ease the organization of the vertex
 * indexing! 
 * Since with this assumption, the out-edge offset of vertices
 * with vertex_ID can be easily accessed by using the suffix:
 * index_map[vertex_ID]
 */

void process_edgelist( const char* input_file_name,
        const char* edge_file_name,
        const char* out_dir,
        const char* origin_edge_file)
{
    printf( "Start Processing %s.\nWill generate %s in destination folder.\n", 
            input_file_name, edge_file_name );

    in = fopen( input_file_name, "r" );
    if( in == NULL ){
        printf( "Cannot open the input graph file!\n" );
        exit(1);
    }

    edge_file = open( edge_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if( -1 == edge_file ){
        printf( "Cannot create edge list file:%s\nAborted..\n",
                edge_file_name );
        exit( -1 );
    }

    //init the global variable
    line_no = 0;
    num_edges = 0;

    //init the file pointer to  the head of the file
    fseek( in , 0 , SEEK_SET );	
    
    printf( "convert to binary-based edgelist...\n" );       
    while ( read_one_edge() != CUSTOM_EOF ){
        //jump the ##
        if (num_edges == 0)
            continue;
        //jump self cycle
        if (src_vert == dst_vert)
            continue;
        //trace the vertex ids
        if( src_vert < min_vertex_id ) min_vertex_id = src_vert;
        if( dst_vert < min_vertex_id ) min_vertex_id = dst_vert;
        if( src_vert > max_vertex_id ) max_vertex_id = src_vert;
        if( dst_vert > max_vertex_id ) max_vertex_id = dst_vert;

        (*(buf1 + current_buf_size)).src_vert = src_vert;
        (*(buf1 + current_buf_size)).dest_vert = dst_vert;
        current_buf_size++;

        if (current_buf_size == each_buf_size)
        {
            //call function to sort and write back
            std::cout << "copy " << current_buf_size << " edges to append to the output in the form of binary." << std::endl;
            flush_buffer_to_file(edge_file, (char *)buf1, current_buf_size * 2 * sizeof(unsigned int));
            current_buf_size = 0;
        }
    }//while EOF
    if (current_buf_size)
    {
        std::cout << "copy " << current_buf_size << " edges to append to the output in the form of binary." << std::endl;
        flush_buffer_to_file(edge_file, (char *)buf1, current_buf_size * 2 * sizeof(unsigned int));
        current_buf_size = 0;
    }

    //finished processing
    fclose( in );
    close( edge_file );
}

/*
 * this function will flush the content of buffer to file, fd
 * the length of the buffer should be "size".
 * Returns: -1 means failure
 * on success, will return the number of bytes that are actually 
 * written.
 */
int flush_buffer_to_file( int fd, char* buffer, unsigned int size )
{
    unsigned int n, offset, remaining, res;
    n = offset = 0;
    remaining = size;
    while(n<size){
        res = write( fd, buffer+offset, remaining);
        n += res;
        offset += res;
        remaining = size-n;
    }
    return n;
}


/* this function is simple: just read one line,
 * record the increased edge number, retrieve the source and destination vertex
 * leave further processing to its caller.
 * Further processing includes:
 * 1) trace the vertex ids, get the minimal and maximal vertex id.
 * 2) tell if the vertex ids are continuous or not, if not, record the id gap;
 * 3) save the edge buffer to the "type2" binary file, if the buffer is filled.
 * 4) prepare for the index file.
 */
int read_one_edge( void )
{
    char* res;

    if(( res = fgets( line_buffer, MAX_LINE_LEN, in )) == NULL )
        return CUSTOM_EOF;
    line_no++;

    //skip the comments
    if( line_buffer[0] == '#' ) return 0;
    num_edges ++; 
    //	sscanf( line_buffer, "%d\t%d\n", &src_vert, &dst_vert);
    sscanf( line_buffer, LINE_FORMAT, &src_vert, &dst_vert);

    return 0;
}

