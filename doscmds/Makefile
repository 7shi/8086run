TARGET = hcopy.com hello.com quitemu.com

all: $(TARGET)

.SUFFIXES: .asm .com
.asm.com:
	nasm -o $@ $<

clean:
	rm -f $(TARGET)
