CC = gcc

MD5 = -lcrypto

PROGRAM = ssu_backup
BAK = backup
REM = remove
REC = recover
LIST = list
HELP = help

all : $(PROGRAM) $(REM) $(BAK) $(REC) $(HELP)

$(PROGRAM) : main.o command.o filepath.o
	$(CC) $^ -o $@

$(BAK) : $(BAK).o command.o filepath.o filedata.o
	$(CC) $^ -o $@ $(MD5)

$(REM) : $(REM).o command.o filepath.o filedata.o
	$(CC) $^ -o $@ $(MD5)

$(REC) : $(REC).o command.o filepath.o filedata.o
	$(CC) $^ -o $@ $(MD5)

# $(LIST) : 

$(HELP) : $(HELP).o command.o
	$(CC) $^ -o $@


$(BAK).o : $(BAK).c command.h filepath.h filedata.h
	$(CC) -c $< -o $@ $(MD5)

$(REM).o : $(REM).c command.h filepath.h filedata.h
	$(CC) -c $< -o $@ $(MD5)

$(REC).o : $(REC).c command.h filepath.h filedata.h
	$(CC) -c $< -o $@ $(MD5)

# $(LIST).o : 

$(HELP).o : $(HELP).c command.h defs.h
	$(CC) -c $< -o $@

main.o : main.c command.h filepath.h
	$(CC) -c $< -o $@

command.o : command.c command.h
	$(CC) -c $< -o $@

filepath.o : filepath.c filepath.h
	$(CC) -c $< -o $@

filedata.o : filedata.c filedata.h
	$(CC) -c $< -o $@ $(MD5)
	

./PHONY: clean
clean:
	rm -f $(PROGRAM) $(BAK) $(REM) $(REC) $(HELP)
	rm -f *.o