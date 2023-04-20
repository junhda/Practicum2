#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PATH_MAX 8196

void copy_file(const char *source, const char *destination) {
    FILE *src_file = fopen(source, "rb");
    if (src_file == NULL) {
        perror("Error opening source file");
        return;
    }

    FILE *dst_file = fopen(destination, "wb");
    if (dst_file == NULL) {
        perror("Error opening destination file");
        fclose(src_file);
        return;
    }

    size_t buffer_size = 8192;
    unsigned char buffer[buffer_size];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, buffer_size, src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);
}

void sync_folders(const char *source, const char *destination) {
    DIR *src_dir = opendir(source);
    if (src_dir == NULL) {
        perror("Error opening source directory");
        return;
    }

    struct dirent *entry;
    struct stat src_stat;
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];

    while ((entry = readdir(src_dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", source, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", destination, entry->d_name);

        if (stat(src_path, &src_stat) == -1) {
            perror("Error getting source file information");
            continue;
        }

        if (S_ISREG(src_stat.st_mode)) {
            copy_file(src_path, dst_path);
        } else if (S_ISDIR(src_stat.st_mode)) {
            mkdir(dst_path, src_stat.st_mode);
            sync_folders(src_path, dst_path);
        }
    }

    closedir(src_dir);
}