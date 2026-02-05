#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <uthash.h>

#define CHECK_BYTES 4096

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

int
is_duplicate(const char *path1, const char *path2, size_t size) {
   static char buf1[CHECK_BYTES];
   static char buf2[CHECK_BYTES];
   char *bigbuf1, *bigbuf2;
   FILE *f1, *f2;
   f1 = fopen(path1, "r");
   f2 = fopen(path2, "r");
   fread(buf1, 1, CHECK_BYTES, f1);
   fread(buf2, 1, CHECK_BYTES, f2);
   if(strcmp(buf1, buf2) != 0)
      return 0;
   bigbuf1 = malloc(size-CHECK_BYTES+1);
   if(!bigbuf1) {
      fprintf(stderr, "Out of memory\n");
      exit(1);
   }
   bigbuf2 = malloc(size-CHECK_BYTES+1);
   if(!bigbuf2) {
      fprintf(stderr, "Out of memory\n");
      exit(1);
   }
   fread(bigbuf1, 1, size-CHECK_BYTES, f1);
   fread(bigbuf2, 1, size-CHECK_BYTES, f2);
   return strcmp(bigbuf1, bigbuf2) == 0 ? 1 : 0;
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
         } else if(is_duplicate(path, suspect->path, size)) {
               printf("rm %s # %s\n", path, suspect->path);
               continue;
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
