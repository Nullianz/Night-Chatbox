/* NightOS kernel entry point and command shell. */
#include "kernel.h"

static uint32_t mem_kb;

static void ok(const char *what) {
    con_set_attr(0x08); kprintf("  ["); con_set_attr(0x0A); kprintf(" ok "); con_set_attr(0x08); kprintf("] ");
    con_set_attr(0x07); kprintf("%s\n", what);
}

static void logo(void) {
    con_set_attr(0x0B);
    kprintf("\n");
    kprintf("     *        .   _   _ _       _     _    ___  ____     .      \n");
    kprintf("  .      *       | \\ | (_) __ _| |__ | |_ / _ \\/ ___|        *  \n");
    kprintf("     .           |  \\| | |/ _` | '_ \\| __| | | \\___ \\   .       \n");
    kprintf("  *       .      | |\\  | | (_| | | | | |_| |_| |___) |      .   \n");
    kprintf("     .        *  |_| \\_|_|\\__, |_| |_|\\__|\\___/|____/  *        \n");
    kprintf("                          |___/                                \n");
    con_set_attr(0x07);
}

static void cmd_help(void) {
    kprintf("Commands:\n");
    kprintf("  dir                  list files            type <file>   show a text file\n");
    kprintf("  <program>[.exe] args run a Windows program  info <exe>    look inside an .exe\n");
    kprintf("  del <file>           delete a file          echo <text>   print text\n");
    kprintf("  cls                  clear the screen       mem           memory usage\n");
    kprintf("  time                 date and time          ver           about NightOS\n");
    kprintf("  msg <text>           show a message box     reboot / shutdown\n");
}

static void cmd_dir(void) {
    uint32_t total = 0; int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        kfile *f = fs_at(i);
        if (!f) continue;
        int exe = strlen(f->name) > 4 && !strcasecmp(f->name + strlen(f->name) - 4, ".exe");
        con_set_attr(exe ? 0x0A : 0x07);
        kprintf("  %-20s", f->name);
        con_set_attr(0x08);
        kprintf("%8u bytes  %s\n", f->size, f->readonly ? "built-in" : "");
        con_set_attr(0x07);
        total += f->size; n++;
    }
    kprintf("  %d files, %u bytes\n", n, total);
}

static void cmd_type(const char *name) {
    kfile *f = fs_find(name);
    if (!f) { kprintf("No such file: %s\n", name); return; }
    for (uint32_t i = 0; i < f->size; i++) {
        char c = (char)f->data[i];
        if (c == '\r') continue;
        if (c == '\n' || c == '\t' || (c >= 32 && c < 127)) con_putc(c); else con_putc('.');
    }
    if (f->size && f->data[f->size - 1] != '\n') con_putc('\n');
}

static void shutdown(void) {
    kprintf("Good night!\n");
    time_sleep(300);
    outw(0x604, 0x2000);   /* QEMU (newer) */
    outw(0xB004, 0x2000);  /* QEMU/Bochs (older) */
    outw(0x4004, 0x3400);  /* VirtualBox */
    con_set_attr(0x0E);
    kprintf("It's now safe to turn off your computer.\n");
    for (;;) __asm__ volatile("cli; hlt");
}

static void reboot(void) {
    for (int i = 0; i < 100000 && (inb(0x64) & 2); i++) {}
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("cli; hlt");
}

static void run_line(char *line) {
    while (*line == ' ') line++;
    if (!*line) return;
    char cmd[64]; int i = 0;
    while (line[i] && line[i] != ' ' && i < 63) { cmd[i] = line[i]; i++; }
    cmd[i] = 0;
    char *arg = line + i;
    while (*arg == ' ') arg++;

    if (!strcasecmp(cmd, "help") || !strcmp(cmd, "?")) cmd_help();
    else if (!strcasecmp(cmd, "dir") || !strcasecmp(cmd, "ls")) cmd_dir();
    else if (!strcasecmp(cmd, "type") || !strcasecmp(cmd, "cat")) { if (*arg) cmd_type(arg); else kprintf("usage: type <file>\n"); }
    else if (!strcasecmp(cmd, "del") || !strcasecmp(cmd, "rm")) {
        if (!fs_find(arg)) kprintf("No such file: %s\n", arg);
        else if (!fs_delete(arg)) kprintf("%s is built in and can't be deleted.\n", arg);
    }
    else if (!strcasecmp(cmd, "cls") || !strcasecmp(cmd, "clear")) con_clear();
    else if (!strcasecmp(cmd, "echo")) kprintf("%s\n", arg);
    else if (!strcasecmp(cmd, "info")) pe_info(arg);
    else if (!strcasecmp(cmd, "msg")) messagebox(*arg ? arg : "Hello from NightOS!", "NightOS", 0);
    else if (!strcasecmp(cmd, "mem")) {
        uint32_t t, u, b; heap_stats(&t, &u, &b);
        kprintf("RAM: %u MB   heap: %u KB used of %u KB (%u blocks)\n", mem_kb / 1024, u / 1024, t / 1024, b);
    }
    else if (!strcasecmp(cmd, "time") || !strcasecmp(cmd, "date")) {
        rtc_time t; rtc_read(&t);
        kprintf("%04d-%02d-%02d %02d:%02d:%02d (up %u s)\n", t.year, t.month, t.day, t.hour, t.min, t.sec, time_ms() / 1000);
    }
    else if (!strcasecmp(cmd, "ver") || !strcasecmp(cmd, "about")) {
        kprintf("NightOS %s - a from-scratch hobby OS that runs 32-bit Windows .exe programs\n", NIGHTOS_VERSION);
        kprintf("with its own PE loader and its own Win32 layer. No Wine, no Windows code.\n");
    }
    else if (!strcasecmp(cmd, "shutdown") || !strcasecmp(cmd, "halt") || !strcasecmp(cmd, "poweroff")) shutdown();
    else if (!strcasecmp(cmd, "reboot") || !strcasecmp(cmd, "restart")) reboot();
    else {
        int r = pe_run(cmd, line);
        if (r == -1) kprintf("'%s' is not a command or program. Type 'help' or 'dir'.\n", cmd);
        else if (r != 0 && r > -2) { con_set_attr(0x08); kprintf("(program exited with code %d)\n", r); con_set_attr(0x07); }
    }
}

void kmain(uint32_t magic, uint32_t *mbi) {
    serial_init();
    gdt_init();
    idt_init();
    time_init();
    mem_kb = (magic == 0x2BADB002 && (mbi[0] & 1)) ? mbi[2] + 1024 : 32 * 1024;
    uint32_t mem_end = mem_kb * 1024;
    if (mem_end > 0x40000000u) mem_end = 0x40000000u;
    if (mem_end < 0x02000000u) { con_init(); con_panic_screen("Not enough memory", "NightOS needs at least 32 MB of RAM"); for (;;) __asm__("hlt"); }
    heap_init(0x01000000u, mem_end);
    con_init();
    logo();
    char buf[80];
    ok("CPU tables, exception handlers and FPU/SSE ready");
    snprintf(buf, sizeof buf, "Memory: %u MB", mem_kb / 1024); ok(buf);
    ok("Clock calibrated");
    fs_init();
    ok("RAM disk mounted");
    ok("Windows program loader + Win32 layer (kernel32, user32) ready");
    kprintf("\nType "); con_set_attr(0x0E); kprintf("help"); con_set_attr(0x07);
    kprintf(" for commands, or "); con_set_attr(0x0E); kprintf("dir"); con_set_attr(0x07);
    kprintf(" to see programs you can run.\n\n");

    static char line[256];
    for (;;) {
        con_set_attr(0x0B); kprintf("night"); con_set_attr(0x07); kprintf(":"); con_set_attr(0x0D); kprintf("~"); con_set_attr(0x07); kprintf("> ");
        kreadline(line, sizeof line);
        run_line(line);
    }
}
