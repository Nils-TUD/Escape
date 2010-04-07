ROOT = ../..
BUILDL = $(BUILD)/dmdo

BIN = $(BUILD)/dmd
OPTABGENBIN = $(BUILD)/dmdoptabgen
IDGENBIN = $(BUILD)/dmdidgen
IMPCNVGENBIN = $(BUILD)/dmdimpcnvgen

C=backend
TK=tk
ROOT=root

CC=g++ -m32

#OPT=-g -g3
#OPT=-O2

#COV=-fprofile-arcs -ftest-coverage

#GFLAGS = -Wno-deprecated -D__near= -D__pascal= -fno-exceptions -g -DDEBUG=1 $(COV)
GFLAGS = -Wno-deprecated -D__near= -D__pascal= -fno-exceptions -O2

CFLAGS = $(GFLAGS) -I$(ROOT) -D__I86__=1 -DMARS=1 -DTARGET_LINUX=1 -D_DH
MFLAGS = $(GFLAGS) -I$C -I$(TK) -D__I86__=1 -DMARS=1 -DTARGET_LINUX=1 -D_DH

CH= $C/cc.h $C/global.h $C/parser.h $C/oper.h $C/code.h $C/type.h \
	$C/dt.h $C/cgcv.h $C/el.h $C/iasm.h
TOTALH=

DMD_OBJS = $(patsubst %.o,$(BUILDL)/%.o, \
	access.o array.o attrib.o bcomplex.o bit.o blockopt.o \
	cast.o code.o cg.o cg87.o cgcod.o cgcs.o cgcv.o cgelem.o cgen.o \
	cgreg.o cgsched.o class.o cod1.o cod2.o cod3.o cod4.o cod5.o \
	constfold.o irstate.o dchar.o cond.o debug.o \
	declaration.o dsymbol.o dt.o dump.o e2ir.o ee.o eh.o el.o \
	dwarf.o enum.o evalu8.o expression.o func.o gdag.o gflow.o \
	glocal.o gloop.o glue.o gnuc.o go.o gother.o html.o iasm.o id.o \
	identifier.o impcnvtab.o import.o inifile.o dmdinit.o inline.o \
	lexer.o link.o lstring.o mangle.o mars.o rmem.o module.o msc.o mtype.o \
	nteh.o cppmangle.o opover.o optimize.o os.o out.o outbuf.o \
	parse.o ph.o ptrntab.o root.o rtlsym.o s2ir.o scope.o statement.o \
	stringtable.o struct.o csymbol.o template.o tk.o tocsym.o todt.o \
	type.o typinf.o dmdutil.o var.o version.o strtold.o utf.o staticassert.o \
	unialpha.o toobj.o toctype.o toelfdebug.o entity.o doc.o macro.o \
	hdrgen.o delegatize.o aa.o ti_achar.o toir.o interpret.o traits.o \
	builtin.o clone.o aliasthis.o \
	man.o arrayop.o port.o response.o async.o json.o speller.o \
	libelf.o elfobj.o)

SRC = mars.c enum.c struct.c dsymbol.c import.c idgen.c impcnvgen.c \
	identifier.c mtype.c expression.c optimize.c template.h \
	template.c lexer.c declaration.c cast.c cond.h cond.c link.c \
	aggregate.h parse.c statement.c constfold.c version.h version.c \
	inifile.c iasm.c module.c scope.c dump.c init.h dmdinit.c attrib.h \
	attrib.c opover.c class.c mangle.c bit.c tocsym.c func.c inline.c \
	access.c complex_t.h irstate.h irstate.c glue.c msc.c ph.c tk.c \
	s2ir.c todt.c e2ir.c dmdutil.c identifier.h parse.h objfile.h \
	scope.h enum.h import.h mars.h module.h mtype.h dsymbol.h \
	declaration.h lexer.h expression.h irstate.h statement.h eh.c \
	utf.h utf.c staticassert.h staticassert.c unialpha.c \
	typinf.c toobj.c toctype.c tocvdebug.c toelfdebug.c entity.c \
	doc.h doc.c macro.h macro.c hdrgen.h hdrgen.c arraytypes.h \
	delegatize.c toir.h toir.c interpret.c traits.c cppmangle.c \
	builtin.c clone.c lib.h libomf.c libelf.c libmach.c arrayop.c \
	aliasthis.h aliasthis.c json.h json.c \
	$C/cdef.h $C/cc.h $C/oper.h $C/ty.h $C/optabgen.c \
	$C/global.h $C/parser.h $C/code.h $C/type.h $C/dt.h $C/cgcv.h \
	$C/el.h $C/iasm.h $C/rtlsym.h $C/html.h \
	$C/bcomplex.c $C/blockopt.c $C/cg.c $C/cg87.c \
	$C/cgcod.c $C/cgcs.c $C/cgcv.c $C/cgelem.c $C/cgen.c $C/cgobj.c \
	$C/cgreg.c $C/var.c $C/strtold.c \
	$C/cgsched.c $C/cod1.c $C/cod2.c $C/cod3.c $C/cod4.c $C/cod5.c \
	$C/code.c $C/symbol.c $C/debug.c $C/dt.c $C/ee.c $C/el.c \
	$C/evalu8.c $C/go.c $C/gflow.c $C/gdag.c \
	$C/gother.c $C/glocal.c $C/gloop.c $C/html.c $C/newman.c \
	$C/nteh.c $C/os.c $C/out.c $C/outbuf.c $C/ptrntab.c $C/rtlsym.c \
	$C/type.c $C/melf.h $C/mach.h $C/bcomplex.h \
	$C/cdeflnx.h $C/outbuf.h $C/token.h $C/tassert.h \
	$C/elfobj.c $C/cv4.h $C/dwarf2.h $C/cpp.h $C/exh.h $C/go.h \
	$C/dwarf.c $C/dwarf.h $C/aa.h $C/aa.c $C/tinfo.h $C/ti_achar.c \
	$C/machobj.c \
	$(TK)/filespec.h $(TK)/mem.h $(TK)/list.h $(TK)/vec.h \
	$(TK)/filespec.c $(TK)/mem.c $(TK)/vec.c $(TK)/list.c \
	$(ROOT)/dchar.h $(ROOT)/dchar.c $(ROOT)/lstring.h \
	$(ROOT)/lstring.c $(ROOT)/root.h $(ROOT)/root.c $(ROOT)/array.c \
	$(ROOT)/rmem.h $(ROOT)/rmem.c $(ROOT)/port.h $(ROOT)/port.c \
	$(ROOT)/gnuc.h $(ROOT)/gnuc.c $(ROOT)/man.c \
	$(ROOT)/stringtable.h $(ROOT)/stringtable.c \
	$(ROOT)/response.c $(ROOT)/async.h $(ROOT)/async.c \
	$(ROOT)/speller.h $(ROOT)/speller.c


all: $(BUILDL) $(BIN)

$(BUILDL):
	mkdir -p $(BUILDL)

$(BIN): $(BUILDL)/id.o $(OPTABGENBIN) $(DMD_OBJS)
	gcc -m32 -lstdc++ $(COV) $(DMD_OBJS) -o $(BIN)

clean:
	rm -f $(DMD_OBJS) $(BIN) $(BUILDL)/optab.o $(IMPCNVGENBIN) $(IDGENBIN) id.c id.h \
	impcnvtab.c $(OPTABGENBIN) debtab.c optab.c cdxxx.c elxxx.c fltables.c \
	tytab.c *.cov *.gcda *.gcno

######## optabgen generates some source

$(OPTABGENBIN): $C/optabgen.c $C/cc.h $C/oper.h
	$(CC) $(MFLAGS) $< -o $(OPTABGENBIN)
	$(OPTABGENBIN)

debtab.c optab.c cdxxx.c elxxx.c fltables.c tytab.c : $(OPTABGENBIN)
	$(OPTABGENBIN)

######## idgen generates some source

id.h id.c : $(IDGENBIN)
	$(IDGENBIN)

$(IDGENBIN) : idgen.c
	$(CC) idgen.c -o $(IDGENBIN)

$(BUILDL)/id.o : id.h id.c
	$(CC) -o $@ -c $(CFLAGS) id.c

######### impcnvgen generates some source

impcnvtab.c : $(IMPCNVGENBIN)
	$(IMPCNVGENBIN)

$(IMPCNVGENBIN) : mtype.h impcnvgen.c
	$(CC) $(CFLAGS) impcnvgen.c -o $(IMPCNVGENBIN)

#########

$(BUILDL)/aa.o: $C/aa.h $C/tinfo.h $C/aa.c
	$(CC) -o $@ -c $(MFLAGS) -I. $C/aa.c

$(BUILDL)/access.o: access.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/aliasthis.o: aliasthis.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/array.o: $(ROOT)/array.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/arrayop.o: arrayop.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/async.o: $(ROOT)/async.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/attrib.o: attrib.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/bcomplex.o: $C/bcomplex.c
	$(CC) -o $@ -c $(MFLAGS) $C/bcomplex.c

$(BUILDL)/bit.o: expression.h bit.c
	$(CC) -o $@ -c -I$(ROOT) $(MFLAGS) bit.c

$(BUILDL)/blockopt.o: $C/blockopt.c
	$(CC) -o $@ -c $(MFLAGS) $C/blockopt.c

$(BUILDL)/builtin.o: builtin.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/cast.o: cast.c
	$(CC) -o $@ -c $(CFLAGS) $< 

$(BUILDL)/cg.o: fltables.c $C/cg.c
	$(CC) -o $@ -c $(MFLAGS) -I. $C/cg.c

$(BUILDL)/cg87.o: $C/cg87.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/cgcod.o: $C/cgcod.c
	$(CC) -o $@ -c $(MFLAGS) -I. $<

$(BUILDL)/cgcs.o: $C/cgcs.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/cgcv.o: $C/cgcv.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/cgelem.o: $C/rtlsym.h $C/cgelem.c
	$(CC) -o $@ -c $(MFLAGS) -I. $C/cgelem.c

$(BUILDL)/cgen.o: $C/rtlsym.h $C/cgen.c
	$(CC) -o $@ -c $(MFLAGS) $C/cgen.c

$(BUILDL)/cgobj.o: $C/cgobj.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/cgreg.o: $C/cgreg.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/cgsched.o: $C/rtlsym.h $C/cgsched.c
	$(CC) -o $@ -c $(MFLAGS) $C/cgsched.c

$(BUILDL)/class.o: class.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/clone.o: clone.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/cod1.o: $C/rtlsym.h $C/cod1.c
	$(CC) -o $@ -c $(MFLAGS) $C/cod1.c

$(BUILDL)/cod2.o: $C/rtlsym.h $C/cod2.c
	$(CC) -o $@ -c $(MFLAGS) $C/cod2.c

$(BUILDL)/cod3.o: $C/rtlsym.h $C/cod3.c
	$(CC) -o $@ -c $(MFLAGS) $C/cod3.c

$(BUILDL)/cod4.o: $C/cod4.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/cod5.o: $C/cod5.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/code.o: $C/code.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/constfold.o: constfold.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/irstate.o: irstate.h irstate.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) irstate.c

$(BUILDL)/csymbol.o : $C/symbol.c
	$(CC) -o $@ -c $(MFLAGS) $C/symbol.c

$(BUILDL)/dchar.o: $(ROOT)/dchar.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/cond.o: cond.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/cppmangle.o: cppmangle.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/debug.o: $C/debug.c
	$(CC) -o $@ -c $(MFLAGS) -I. $<

$(BUILDL)/declaration.o: declaration.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/delegatize.o: delegatize.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/doc.o: doc.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/dsymbol.o: dsymbol.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/dt.o: $C/dt.h $C/dt.c
	$(CC) -o $@ -c $(MFLAGS) $C/dt.c

$(BUILDL)/dump.o: dump.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/dwarf.o: $C/dwarf.h $C/dwarf.c
	$(CC) -o $@ -c $(MFLAGS) -I. $C/dwarf.c

$(BUILDL)/e2ir.o: $C/rtlsym.h expression.h toir.h e2ir.c
	$(CC) -o $@ -c -I$(ROOT) $(MFLAGS) e2ir.c

$(BUILDL)/ee.o: $C/ee.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/eh.o : $C/cc.h $C/code.h $C/type.h $C/dt.h eh.c
	$(CC) -o $@ -c $(MFLAGS) eh.c

$(BUILDL)/el.o: $C/rtlsym.h $C/el.h $C/el.c
	$(CC) -o $@ -c $(MFLAGS) $C/el.c

$(BUILDL)/elfobj.o: $C/elfobj.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/entity.o: entity.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/enum.o: enum.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/evalu8.o: $C/evalu8.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/expression.o: expression.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/func.o: func.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/gdag.o: $C/gdag.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/gflow.o: $C/gflow.c
	$(CC) -o $@ -c $(MFLAGS) $<

#globals.o: globals.c
#	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/glocal.o: $C/rtlsym.h $C/glocal.c
	$(CC) -o $@ -c $(MFLAGS) $C/glocal.c

$(BUILDL)/gloop.o: $C/gloop.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/glue.o: $(CH) $(TOTALH) $C/rtlsym.h mars.h module.h glue.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) glue.c

$(BUILDL)/gnuc.o: $(ROOT)/gnuc.h $(ROOT)/gnuc.c
	$(CC) -o $@ -c $(GFLAGS) $(ROOT)/gnuc.c

$(BUILDL)/go.o: $C/go.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/gother.o: $C/gother.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/hdrgen.o: hdrgen.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/html.o: $(CH) $(TOTALH) $C/html.h $C/html.c
	$(CC) -o $@ -c -I$(ROOT) $(MFLAGS) $C/html.c

$(BUILDL)/iasm.o : $(CH) $(TOTALH) $C/iasm.h iasm.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) iasm.c

$(BUILDL)/identifier.o: identifier.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/impcnvtab.o: mtype.h impcnvtab.c
	$(CC) -o $@ -c $(CFLAGS) -I$(ROOT) impcnvtab.c

$(BUILDL)/import.o: import.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/inifile.o: inifile.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/dmdinit.o: dmdinit.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/inline.o: inline.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/interpret.o: interpret.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/json.o: json.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/lexer.o: lexer.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/libelf.o: libelf.c $C/melf.h
	$(CC) -o $@ -c $(CFLAGS) -I$C $<

$(BUILDL)/libmach.o: libmach.c $C/mach.h
	$(CC) -o $@ -c $(CFLAGS) -I$C $<

$(BUILDL)/link.o: link.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/lstring.o: $(ROOT)/lstring.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/machobj.o: $C/machobj.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/macro.o: macro.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/man.o: $(ROOT)/man.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/mangle.o: mangle.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/mars.o: mars.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/rmem.o: $(ROOT)/rmem.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $(ROOT)/rmem.c
	
$(BUILDL)/module.o: $(TOTALH) $C/html.h module.c
	$(CC) -o $@ -c $(CFLAGS) -I$C module.c

$(BUILDL)/msc.o: $(CH) mars.h msc.c
	$(CC) -o $@ -c $(MFLAGS) msc.c

$(BUILDL)/mtype.o: mtype.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/nteh.o: $C/rtlsym.h $C/nteh.c
	$(CC) -o $@ -c $(MFLAGS) $C/nteh.c

$(BUILDL)/opover.o: opover.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/optimize.o: optimize.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/os.o: $C/os.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/out.o: $C/out.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/outbuf.o : $C/outbuf.h $C/outbuf.c
	$(CC) -o $@ -c $(MFLAGS) $C/outbuf.c

$(BUILDL)/parse.o: parse.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/ph.o: ph.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/port.o: $(ROOT)/port.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/ptrntab.o: $C/iasm.h $C/ptrntab.c
	$(CC) -o $@ -c $(MFLAGS) $C/ptrntab.c

$(BUILDL)/response.o: $(ROOT)/response.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/root.o: $(ROOT)/root.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/rtlsym.o: $C/rtlsym.h $C/rtlsym.c
	$(CC) -o $@ -c $(MFLAGS) $C/rtlsym.c

$(BUILDL)/s2ir.o : $C/rtlsym.h statement.h s2ir.c
	$(CC) -o $@ -c -I$(ROOT) $(MFLAGS) s2ir.c

$(BUILDL)/scope.o: scope.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/speller.o: $(ROOT)/speller.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/statement.o: statement.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/staticassert.o: staticassert.h staticassert.c
	$(CC) -o $@ -c $(CFLAGS) staticassert.c

$(BUILDL)/stringtable.o: $(ROOT)/stringtable.c
	$(CC) -o $@ -c $(GFLAGS) -I$(ROOT) $<

$(BUILDL)/strtold.o: $C/strtold.c
	gcc -m32 -o $@ -c $C/strtold.c

$(BUILDL)/struct.o: struct.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/template.o: template.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/ti_achar.o: $C/tinfo.h $C/ti_achar.c
	$(CC) -o $@ -c $(MFLAGS) -I. $C/ti_achar.c

$(BUILDL)/tk.o: tk.c
	$(CC) -o $@ -c $(MFLAGS) tk.c

$(BUILDL)/tocsym.o: $(CH) $(TOTALH) mars.h module.h tocsym.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) tocsym.c

$(BUILDL)/toctype.o: $(CH) $(TOTALH) $C/rtlsym.h mars.h module.h toctype.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) toctype.c

$(BUILDL)/todt.o : mtype.h expression.h $C/dt.h todt.c
	$(CC) -o $@ -c -I$(ROOT) $(MFLAGS) todt.c

$(BUILDL)/toelfdebug.o: $(CH) $(TOTALH) mars.h toelfdebug.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) toelfdebug.c

$(BUILDL)/toir.o: $C/rtlsym.h expression.h toir.h toir.c
	$(CC) -o $@ -c -I$(ROOT) $(MFLAGS) toir.c

$(BUILDL)/toobj.o: $(CH) $(TOTALH) mars.h module.h toobj.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) toobj.c

$(BUILDL)/traits.o: $(TOTALH) traits.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/type.o: $C/type.c
	$(CC) -o $@ -c $(MFLAGS) $C/type.c

$(BUILDL)/typinf.o: $(CH) $(TOTALH) mars.h module.h mtype.h typinf.c
	$(CC) -o $@ -c $(MFLAGS) -I$(ROOT) typinf.c

$(BUILDL)/dmdutil.o: dmdutil.c
	$(CC) -o $@ -c $(MFLAGS) $<

$(BUILDL)/utf.o: utf.h utf.c
	$(CC) -o $@ -c $(CFLAGS) utf.c

$(BUILDL)/unialpha.o: unialpha.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(BUILDL)/var.o: $C/var.c optab.c
	$(CC) -o $@ -c $(MFLAGS) -I. $C/var.c

$(BUILDL)/version.o: version.c
	$(CC) -o $@ -c $(CFLAGS) $<

######################################################

gcov:
	gcov access.c
	gcov aliasthis.c
	gcov arrayop.c
	gcov attrib.c
	gcov bit.c
	gcov builtin.c
	gcov cast.c
	gcov class.c
	gcov clone.c
	gcov cond.c
	gcov constfold.c
	gcov declaration.c
	gcov delegatize.c
	gcov doc.c
	gcov dsymbol.c
	gcov dump.c
	gcov e2ir.c
	gcov eh.c
	gcov entity.c
	gcov enum.c
	gcov expression.c
	gcov func.c
	gcov glue.c
	gcov iasm.c
	gcov identifier.c
	gcov import.c
	gcov inifile.c
	gcov dmdinit.c
	gcov inline.c
	gcov interpret.c
	gcov irstate.c
	gcov json.c
	gcov lexer.c
	gcov libelf.c
	gcov link.c
	gcov macro.c
	gcov mangle.c
	gcov mars.c
	gcov module.c
	gcov msc.c
	gcov mtype.c
	gcov opover.c
	gcov optimize.c
	gcov parse.c
	gcov ph.c
	gcov scope.c
	gcov statement.c
	gcov staticassert.c
	gcov s2ir.c
	gcov struct.c
	gcov template.c
	gcov tk.c
	gcov tocsym.c
	gcov todt.c
	gcov toobj.c
	gcov toctype.c
	gcov toelfdebug.c
	gcov typinf.c
	gcov unialpha.c
	gcov utf.c
	gcov dmdutil.c
	gcov version.c

#	gcov hdrgen.c
#	gcov tocvdebug.c

######################################################

zip:
	-rm -f dmdsrc.zip
	zip dmdsrc $(SRC)
