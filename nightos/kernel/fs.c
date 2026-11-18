/* RAM filesystem. Built-in programs live in the kernel image; new files live on the heap. */
#include "kernel.h"

typedef struct { const char *name; const uint8_t *data; uint32_t size; } builtin_file;
extern const builtin_file app_table[];

static kfile files[FS_MAX_FILES];

static const char readme[] =
    "Welcome to NightOS!\n"
    "\n"
    "NightOS is a tiny hobby operating system written from scratch.\n"
    "It can load Windows-style .exe programs (32-bit PE files) directly,\n"
    "using its own loader and its own versions of common Win32 functions.\n"
    "\n"
    "Try these:\n"
    "  hello        - say hello\n"
    "  guess        - number guessing game\n"
    "  notes        - write a note to a file\n"
    "  msgbox       - pop up a message box\n"
    "  sysinfo      - show system information\n"
    "  crash        - see how NightOS survives a crashing program\n"
    "  info <exe>   - look inside an .exe file\n";

kfile *fs_at(int i) { return (i >= 0 && i < FS_MAX_FILES && files[i].used) ? &files[i] : 0; }

kfile *fs_find(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (files[i].used && !strcasecmp(files[i].name, name)) return &files[i];
    return 0;
}

kfile *fs_create(const char *name) {
    kfile *f = fs_find(name);
    if (f) return f;
    if (!*name || strlen(name) >= FS_NAME_MAX) return 0;
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (!files[i].used) {
            f = &files[i];
            memset(f, 0, sizeof *f);
            f->used = 1;
            strncpy0(f->name, name, FS_NAME_MAX);
            return f;
        }
    return 0;
}

int fs_delete(const char *name) {
    kfile *f = fs_find(name);
    if (!f || f->readonly) return 0;
    kfree(f->data);
    memset(f, 0, sizeof *f);
    return 1;
}

int fs_write(kfile *f, uint32_t pos, const void *src, uint32_t n) {
    if (f->readonly) return 0;
    if (pos + n > f->cap) {
        uint32_t nc = f->cap ? f->cap : 256;
        while (nc < pos + n) nc *= 2;
        uint8_t *d = krealloc(f->data, nc);
        if (!d) return 0;
        f->data = d; f->cap = nc;
    }
    if (pos > f->size) memset(f->data + f->size, 0, pos - f->size);
    memcpy(f->data + pos, src, n);
    if (pos + n > f->size) f->size = pos + n;
    return 1;
}

void fs_truncate(kfile *f) { if (!f->readonly) f->size = 0; }

void fs_init(void) {
    for (const builtin_file *b = app_table; b->name; b++) {
        kfile *f = fs_create(b->name);
        f->data = (uint8_t *)b->data; f->size = b->size; f->readonly = 1;
    }
    kfile *r = fs_create("readme.txt");
    fs_write(r, 0, readme, sizeof readme - 1);
}
