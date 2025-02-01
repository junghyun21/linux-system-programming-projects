CC = gcc

MD5 = -lcrypto

PROGRAM = ssu_sync
ADD = add
REM = remove
LIST = list
HELP = help

all : $(PROGRAM) $(ADD) $(REM) $(LIST) $(HELP)

$(PROGRAM) : main.o command.o
	$(CC) $^ -o $@

$(ADD) : $(ADD).o monitoring.o path.o
	$(CC) $^ -o $@ $(MD5)

$(REM) : $(REM).o monitoring.o path.o
	$(CC) $^ -o $@ $(MD5)

$(LIST) : $(LIST).o monitoring.o path.o
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

monitoring.o : monitoring.c
	$(CC) -c $< -o $@

$(ADD).o : $(ADD).c
	$(CC) -c $< -o $@ $(MD5)

$(REM).o : $(REM).c
	$(CC) -c $< -o $@ $(MD5)

$(LIST).o : $(LIST).c
	$(CC) -c $< -o $@ $(MD5)

$(HELP).o : $(HELP).c
	$(CC) -c $< -o $@ $(MD5)
	
# -------------------------------

./PHONY: clean
clean:
	rm -f $(PROGRAM) $(ADD) $(REM) $(STA) $(COM) $(REV) $(LOG) $(HELP)
	rm -f *.o
	rm -rf .repo