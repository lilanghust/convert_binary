/**************************************************************************************************
 * Authors: 
 *   Jian He,
 *
 * Routines:
 *   process in-edge
 *   
 *************************************************************************************************/

#include <iostream>
#include <sstream>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "convert.h"
using namespace convert;

typedef unsigned long long u64_t;
typedef unsigned int u32_t;
struct in_edge in_edge_buffer[EDGE_BUFFER_LEN];
struct vert_index in_vert_buffer[VERT_BUFFER_LEN];

FILE *in_edge_fd;
struct tmp_in_edge * buf1, *buf2;
u64_t each_buf_len;
u64_t each_buf_size; //How many edges can be stored in this buf
u32_t num_parts; //init to 0, add by bufs
u64_t *file_len;
struct tmp_in_edge * edge_buf_for_sort;
char * buf;
char * tmp_out_dir;
char * origin_edge_file;
u32_t num_tmp_files;
const char * prev_name_tmp_file;
const char * in_name_file;

u64_t current_buf_size;
u64_t total_buf_size;
u64_t total_buf_len;
u32_t current_file_id;

enum
{
    READ_FILE = 0,
    WRITE_FILE
};

void *map_anon_memory( u64_t size,
        bool mlocked,
        bool zero)
{
    void *space = mmap(NULL, size > 0 ? size:4096,
            PROT_READ|PROT_WRITE,
            MAP_ANONYMOUS|MAP_SHARED, -1, 0);
    printf( "Engine::map_anon_memory had allocated 0x%llx bytes at %llx\n", size, (u64_t)space);
    if(space == MAP_FAILED) {
        std::cerr << "mmap_anon_mem -- allocation " << "Error!\n";
        exit(-1);
    }
    if(mlocked) {
        if(mlock(space, size) < 0) {
            std::cerr << "mmap_anon_mem -- mlock " << "Error!\n";
        }
    }
    if(zero) {
        memset(space, 0, size);
    }
    return space;
}

void do_io_work(const char *file_name_in, u32_t operation, char* buf, u64_t offset_in, u64_t size_in)
{
    int fd;
    switch(operation)
    {
        case READ_FILE:
            {
                int read_finished = 0, remain = size_in, res;
                fd = open(file_name_in, O_RDWR, S_IRUSR | S_IRGRP | S_IROTH); 
                if (fd < 0)
                {
                    printf( "Cannot open attribute file for writing!\n");
                    exit(-1);
                }
                if (lseek(fd, offset_in, SEEK_SET) < 0)
                {
                    printf( "Cannot seek the attribute file!\n");
                    exit(-1);
                }
                while (read_finished < (int)size_in)
                {
                    if( (res = read(fd, buf, remain)) < 0 )
                    {
                        printf( "Cannot seek the attribute file!\n");
                        exit(-1);
                    }
                    read_finished += res;
                    remain -= res;
                }
                close(fd);
                break;
            }
        case WRITE_FILE:
            {
                int written = 0, remain = size_in, res;
                fd = open(file_name_in, O_RDWR, S_IRUSR | S_IRGRP | S_IROTH); 
                if (fd < 0)
                {
                    printf( "Cannot open attribute file for writing!\n");
                    exit(-1);
                }
                if (lseek(fd, offset_in, SEEK_SET) < 0)
                {
                    printf( "Cannot seek the attribute file!\n");
                    exit(-1);
                }
                while (written < (int)size_in)
                {
                    if( (res = write(fd, buf, remain)) < 0 )
                    {
                        printf( "Cannot seek the attribute file!\n");
                        exit(-1);
                    }
                    written += res;
                    remain -= res;
                }
                close(fd);
                break;
            }
    }
}

char* process_in_edge(u64_t mem_size,
        const char * edge_file_name,
        const char * out_dir)
{
    /*struct stat st;
      u64_t edge_file_size;
    //open the edge file
    in_edge_fd = fopen(edge_file_name, "r");
    if (in_edge_fd < 0)
    {
    printf("Cannot open edge_file : %s\n", edge_file_name);
    exit(-1);
    }
    //fstat(in_edge_fd, &st);
    stat(edge_file_name, &st);
    edge_file_size = (u64_t)st.st_size;
    fclose(in_edge_fd);
    printf( "edge file size:%lld(MBytes)\n", edge_file_size/(1024*1024) );
    printf( "edge file size:%lld\n", edge_file_size );
    exit(-1);*/
    tmp_out_dir = new char[strlen(out_dir)+1];
    strcpy(tmp_out_dir, out_dir);

    origin_edge_file = new char[strlen(edge_file_name)+1];
    strcpy(origin_edge_file, edge_file_name);
    //determine how many files to sort
    /*u64_t per_file_size;
      if (mem_size >= (2*edge_file_size))
      {
      num_parts = 1;
      per_file_size = edge_file_size;
      }
      else
      {
      num_parts = ((edge_file_size)%(mem_size)) == 0 ? 
      (u32_t)(edge_file_size/mem_size)
      :(u32_t)(edge_file_size/mem_size + 1);
      per_file_size = mem_size/2;
      }*/

    num_parts = 0;
    each_buf_len = mem_size/2;
    each_buf_size = (u64_t)mem_size/(sizeof(struct tmp_in_edge)*2);
    current_buf_size = 0;
    current_file_id = 0;
    //std::cout << "each_buf_len = " << total_buf_len << std::endl;
    //std::cout << "each_buf_size = " << total_buf_size << std::endl;
    //std::cout << "current_buf_size = " << current_buf_size << std::endl;

    /*for (u32_t i = 0; i < num_parts; i++)
      {
      if (i == num_parts - 1 && (edge_file_size%(mem_size) != 0))
      file_len[i] = edge_file_size%mem_size;
      else
      file_len[i] = per_file_size;

      std::cout << "Init for each file:" << file_len[i] << std::endl;
      }*/

    buf = (char *)map_anon_memory(mem_size, true, true );
    edge_buf_for_sort = (struct tmp_in_edge *)buf;
    buf1 = (struct tmp_in_edge *)buf;
    buf2 = (struct tmp_in_edge *)(buf + each_buf_len);
    //printf("the address of edge_buf_for_sort is %llx\n", (u64_t)edge_buf_for_sort);
    //printf("the address of buf1 is %llx\n", (u64_t)buf1);
    //printf("the address of buf2 is %llx\n", (u64_t)buf2);
    return buf;
}

void hook_for_merge()
{
    for (u32_t i = 0; i < num_tmp_files; i++)
    {
        std::stringstream current_file_id;
        current_file_id << i;
        std::string current_file_name = std::string(prev_name_tmp_file) + current_file_id.str();
        //std::cout << current_file_name << std::endl;
    }
}

