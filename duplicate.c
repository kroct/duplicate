#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <xxhash.h>
#include <uthash.h>
#include <sys/stat.h>

#define SMALL_HASH_BYTES 4096

struct filehash {
   size_t size;
   char *path;
   UT_hash_handle hh;
};

struct filehash *filehashes = NULL;

static inline size_t
filesize(const char *path) {
   static struct stat st;
   stat(path, &st);
   return st.st_size;
}

static XXH64_hash_t
checksum(const char *path, int nbytes) {
   size_t size;
   XXH64_hash_t hash;
   char *buffer;
   FILE *file;

   fflush(stdout);
   file = fopen(path, "r");
   if(!file) {
      fprintf(stderr, "%s: No such file or directory\n", path);
      exit(1);
   }
   if(nbytes > 0) {
      size = nbytes;
   } else {
      fseek(file, 0, SEEK_END);
      size = ftell(file);
      fseek(file, 0, SEEK_SET);
   }
   buffer = malloc(size+1);
   if(!buffer) {
      fprintf(stderr, "Out of memory\n");
      exit(1);
   }
   size = fread(buffer, 1, size, file);
   hash = XXH64(buffer, size, 0);
   free(buffer);
   fclose(file);
   return hash;
}

static void
process_dir(const char *parent_path) {
   DIR *dir;
   static struct dirent *d;

   dir = opendir(parent_path);
   if(!dir) {
      fprintf(stderr, "%s: Couldn't open directory\n", parent_path);
      return;
   }
   while((d = readdir(dir))) {
      char path[PATH_MAX];
      if(d->d_name[0] == '.')
         continue;
      snprintf(path, sizeof(path), "%s/%s", parent_path, d->d_name);
      if(d->d_type == DT_DIR) {
         process_dir(path);
      } else {
         struct filehash *f, *suspect;
         size_t size;

         size = filesize(path);
         if(!size)
            continue;

         f = malloc(sizeof(struct filehash));
         if(!f) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
         }
         f->size = size;

         f->path = strdup(path);
         HASH_FIND(hh,filehashes,&f->size,sizeof(size_t),suspect);
         if(!suspect) {
            HASH_ADD(hh,filehashes,size,sizeof(size_t),f);
         } else {
            if(checksum(path, SMALL_HASH_BYTES) == checksum(suspect->path, SMALL_HASH_BYTES)
               && checksum(path, 0) == checksum(suspect->path, 0)) {
                  printf("rm %s # %s\n", path, suspect->path);
                  continue;
            }
         }
      }
   }
   closedir(dir);
}

int
main(int argc, char **argv) {
   if(argc < 2)
      return 1;
   for(int i = 1; i < argc; i++) {
      size_t len;
      len = strlen(argv[i]);
      while(argv[i][--len] == '/')
         argv[i][len] = '\0';
      process_dir(argv[i]);
   }
}
