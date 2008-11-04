NAME=task1
BUILD=../../build
DISK=$(BUILD)/disk.img
DISKMOUNT=../../disk
BIN=$(BUILD)/user_$(NAME).bin
LDCONF=ld.conf
DUMP=$(BUILD)/user_$(NAME).dump

ASRC=main.asm
AOBJ=$(patsubst %.asm,$(BUILD)/user_$(NAME)_%.o,$(ASRC))

.PHONY: all clean

all:	$(BIN) $(DUMP)

$(BIN):	$(AOBJ) $(LDCONF)
		@echo "===== LINKING =====";
		ld -T $(LDCONF) -o $(BIN) $(AOBJ);
		@echo "===== COPYING BINARY =====";
		sudo mount -o loop $(DISK) $(DISKMOUNT);
		cp $(BIN) $(DISKMOUNT);
		sudo umount $(DISKMOUNT);

$(BUILD)/user_$(NAME)_%.o:	%.asm
		@echo "===== ASSEMBLING $< =====";
		nasm -f elf -o $@ $<;

$(DUMP):	$(BIN)
		$(BUILD)/dump $(BIN) $(DUMP)

clean:
		rm -f $(BIN) $(AOBJ)
