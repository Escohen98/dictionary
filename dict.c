#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dict.h"

struct dict_item {
  // Each word is at most 100 bytes.
  char word[100];
  // The length of each word.
  size_t len;
};

struct dict_t {
  // The path to the underlying file
  char *path;
  // The file descriptor of the opened file. Set by dictionary_open_map.
  int fd;
  // How many items the mmaped file should store (this will determine the size).
  // There are ~500k words in /usr/share/dict/words.
  size_t num_items;
  // A pointer to the first item.
  struct dict_item *base;
};

// Construct a new dict_t struct.
// data_file is where to write the data,
// num_items is how many items this data file should store.
struct dict_t *dictionary_new(char *data_file, size_t num_items) {
  struct dict_t *dict = malloc(sizeof(struct dict_t));
  dict->path = data_file;
  dict->num_items = num_items;
}

// Computes the size of the underlying file based on the # of items and the size
// of a dict_item.
size_t dictionary_len(struct dict_t *dict) {
  return dict->num_items * sizeof(struct dict_item);
}

// This is a private helper function. It should:
// Open the underlying path (dict->path), ftruncate it to the appropriate length
// (dictionary_len), then mmap it.
int dictionary_open_map(struct dict_t *dict) {
  // Implemented from
  // https://stackoverflow.com/questions/5328785/c-programming-open-call-path
  if (dict == NULL || dict->path == NULL)
    return 0;
  dict->fd = open(dict->path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (dict->fd == -1) {
    perror("open");
    return 0;
  }
  if (ftruncate(dict->fd, dictionary_len(dict)) == -1) {
    perror("truncate");
    return 0;
  }
  dict->base = mmap(NULL, dictionary_len(dict), PROT_READ | PROT_WRITE,
                    MAP_SHARED, dict->fd, 0);
  if (dict->base == MAP_FAILED) {
    // handle_error("mmap");
    perror("mmap");
    return 0;
  }
  return 1;
}

// The rest of the functions should be whatever is left from the header that
// hasn't been defined yet.
// Good luck!

// Read the file at input. For each line in input, create a new dictionary
// entry.
int dictionary_generate(struct dict_t *dict, char *input) {
  // Implemented from
  // https://stackoverflow.com/questions/9206091/going-through-a-text-file-line-by-line-in-c
  FILE *file = fopen(input, "r");
  char line[100];
  int i = 0;
  int rtrn = dictionary_open_map(dict);
  while (fgets(line, sizeof(line), file)) {
    strcpy(dict->base[i].word, line);
    char *replace = strchr(dict->base[i].word, '\n');
    *replace = '\0';
    dict->base[i].len = strlen(line) - 1;
    i++;
  }
  return rtrn;
}

// load a dictionary that was generated by calling generate
int dictionary_load(struct dict_t *dict) { return dictionary_open_map(dict); }

// Unmaps the given dictionary.
// Free/destroy the underlying dict. Does NOT delete the database file.
void dictionary_close(struct dict_t *dict) {
  // msync(dict->base, dict
  munmap(dict->base, dictionary_len(dict));
  free(dict);
  // destroy(dict);
}

// returns pointer to word if it exists, null otherwise
char *dictionary_exists(struct dict_t *dict, char *word) {
  for (int i = 0; i < dict->num_items; i++) {
    if (strcmp(dict->base[i].word, word) == 0) {
      return dict->base[i].word;
    }
  }
  return NULL;
}

// Count of words with len > n
int dictionary_larger_than(struct dict_t *dict, size_t n) {
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (dict->base[i].len > n) {
      count++;
    }
  }
  return count;
}

// Count of words with len < n
int dictionary_smaller_than(struct dict_t *dict, size_t n) {
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (dict->base[i].len != 0 && dict->base[i].len < n) {
      count++;
    }
  }
  return count;
}

// Count of words with len == n
int dictionary_equal_to(struct dict_t *dict, size_t n) {
  int count = 0;
  for (int i = 0; i < dict->num_items; i++) {
    if (dict->base[i].len == n) {
      count++;
    }
  }
  return count;
}
