#ifndef DESCENT_DUMP_H
#define DESCENT_DUMP_H
#include "descent.h"

typedef struct file *open_file(const char *);
typedef bool close_file(struct file *); 
typedef int32_t get_file_size(struct file *);
typedef int32_t read_file(struct file*, void *, int32_t); 
typedef int32_t seek_file(struct file *, int32_t);

struct platform {
    open_file *OpenFile;
    close_file *CloseFile;
    get_file_size *GetFileSize;
    read_file *ReadFile;
    seek_file *SeekFile;
};

#endif
