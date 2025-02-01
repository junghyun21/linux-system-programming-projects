CC = gcc

MD5 = -lcrypto

PROGRAM = ssu_repo
ADD = add
REM = remove
STA = status
COM = commit
REV = revert
LOG = log
HELP = help

all : $(PROGRAM) $(ADD) $(REM) $(STA) $(COM) $(REV) $(LOG) $(HELP)

$(PROGRAM) : main.o command.o
	$(CC) $^ -o $@

$(ADD) : $(ADD).o path.o struct.o
	$(CC) $^ -o $@ $(MD5)

$(REM) : $(REM).o path.o struct.o
	$(CC) $^ -o $@ $(MD5)

$(STA) : $(STA).o path.o struct.o
	$(CC) $^ -o $@ $(MD5)

$(COM) : $(COM).o path.o struct.o
	$(CC) $^ -o $@ $(MD5)

$(REV) : $(REV).o path.o struct.o
	$(CC) $^ -o $@ $(MD5)

$(LOG) : $(LOG).o path.o struct.o
	$(CC) $^ -o $@ $(MD5)

$(HELP) : $(HELP).o
	$(CC) $^ -o $@ $(MD5)

# -------------------------------

main.o : main.c
	$(CC) -c $< -o $@

command.o : command.c
	$(CC) -c $< -o $@

path.o : path.c
	$(CC) -c $< -o $@

struct.o : struct.c
	$(CC) -c $< -o $@

$(ADD).o : $(ADD).c
	$(CC) -c $< -o $@ $(MD5)

$(REM).o : $(REM).c
	$(CC) -c $< -o $@ $(MD5)

$(STA).o : $(STA).c
	$(CC) -c $< -o $@ $(MD5)

$(COM).o : $(COM).c
	$(CC) -c $< -o $@ $(MD5)

$(REV).o : $(REV).c
	$(CC) -c $< -o $@ $(MD5)

$(LOG).o : $(LOG).c
	$(CC) -c $< -o $@ $(MD5)

$(HELP).o : $(HELP).c
	$(CC) -c $< -o $@ $(MD5)
	
# -------------------------------

./PHONY: clean
clean:
	rm -f $(PROGRAM) $(ADD) $(REM) $(STA) $(COM) $(REV) $(LOG) $(HELP)
	rm -f *.o
	rm -rf .repo