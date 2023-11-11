#LINUX
CC=g++
OPTS=-O2 -DLINUX -Wall  -I$(HOME)/usrlocal/mmablib/include
LIBS=$(HOME)/usrlocal/lib/libombf_4.a

#--------------------------------------
all : harmonics annuals first_pass postquarter processquarter

harmonics : harmonics.C
	$(CC) $(OPTS)  $< -o $(@) $(LIBS)

annuals : annuals.C
	$(CC) $(OPTS)  $< -o $(@) $(LIBS)

first_pass : first_pass.C
	$(CC) $(OPTS) $< -o $(@) $(LIBS)

postquarter : postquarter.C
	$(CC) $(OPTS) $< -o $(@) $(LIBS)

processquarter : processquarter.C 
	$(CC) $(OPTS) $< -o $(@) $(LIBS)
