# NightOS build. Needs: gcc (with -m32), nasm, ld, clang + lld-link + llvm-dlltool, grub-mkrescue + xorriso (for the ISO).
CC      := gcc
CFLAGS  := -m32 -std=gnu11 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
           -mno-sse -mno-mmx -O2 -Wall -Wextra -Wno-unused-parameter
B       := build
APPS    := hello guess notes msgbox sysinfo crash threads
KOBJS   := $(B)/boot.o $(B)/cpu_asm.o $(B)/apps_blob.o $(patsubst kernel/%.c,$(B)/%.o,$(wildcard kernel/*.c))
APPCF   := --target=i686-pc-windows-msvc -O2 -ffreestanding -fno-builtin -fno-stack-protector -mno-stack-arg-probe -Wall

all: $(B)/nightos.elf
iso: nightos.iso

$(B):
	mkdir -p $(B)/apps

$(B)/%.o: kernel/%.c kernel/kernel.h | $(B)
	$(CC) $(CFLAGS) -c $< -o $@
$(B)/boot.o: boot/boot.asm | $(B)
	nasm -f elf32 $< -o $@
$(B)/cpu_asm.o: boot/cpu.asm | $(B)
	nasm -f elf32 $< -o $@

# Demo programs: real 32-bit Windows .exe files (they also run on actual Windows).
$(B)/apps/%.lib: apps/%.def | $(B)
	llvm-dlltool -m i386 -k -d $< -l $@
$(B)/apps/%.obj: apps/%.c apps/night.h | $(B)
	clang $(APPCF) -c $< -o $@
$(B)/apps/%.exe: $(B)/apps/%.obj $(B)/apps/crt.obj $(B)/apps/kernel32.lib $(B)/apps/user32.lib
	lld-link /nologo /nodefaultlib /entry:main /subsystem:console /out:$@ $^

$(B)/apps_blob.asm: $(foreach a,$(APPS),$(B)/apps/$(a).exe) | $(B)
	@{ echo 'section .rodata'; echo 'global app_table'; echo 'align 4'; echo 'app_table:'; \
	  for a in $(APPS); do echo "  dd n_$$a, d_$$a, e_$$a - d_$$a"; done; echo '  dd 0, 0, 0'; \
	  for a in $(APPS); do echo "n_$$a: db \"$$a.exe\", 0"; echo 'align 4'; \
	    echo "d_$$a: incbin \"$(B)/apps/$$a.exe\""; echo "e_$$a:"; done; } > $@
$(B)/apps_blob.o: $(B)/apps_blob.asm
	nasm -f elf32 $< -o $@

$(B)/nightos.elf: $(KOBJS) kernel/linker.ld
	ld -m elf_i386 -z noexecstack -T kernel/linker.ld -o $@ $(KOBJS)

nightos.iso: $(B)/nightos.elf iso/boot/grub/grub.cfg
	rm -rf $(B)/iso && cp -r iso $(B)/iso && cp $(B)/nightos.elf $(B)/iso/boot/
	grub-mkrescue -o $@ $(B)/iso 2>/dev/null

run: $(B)/nightos.elf
	qemu-system-i386 -m 128 -kernel $< -serial stdio
run-iso: nightos.iso
	qemu-system-i386 -m 128 -cdrom $< -serial stdio
test: $(B)/nightos.elf
	./tools/test.sh

clean:
	rm -rf $(B) nightos.iso
.PHONY: all iso run run-iso test clean
.SECONDARY:
