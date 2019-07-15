CFLAGS := -std=c99 -O2 -Wall -Wextra -pedantic -Wno-unused-parameter $(CFLAGS)
O = obj/

.PHONY: all clean
all: unimgc

clean:
	rm -rf unimgc $(O)

unimgc: $(O)unimgc.o $(O)image.o $(O)lzo.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(O)%.o: %.c | $(O)
	$(CROSS_COMPILE)$(CC) $(CPPFLAGS) $(CFLAGS) -c $^ -o $@

$(O):
	mkdir -p $@
